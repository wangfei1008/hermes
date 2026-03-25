#include "save_db_sqlite.h"

#include "log/log.h"
#include "component_export.h"
#include "json/json_read.h"
#include "error_code.h"

#include <chrono>
#include <sstream>

extern "C"
{
    COM_EXPORT bool create_lib(IComponent** new_component);
    COM_EXPORT bool release_lib(IComponent** new_component);
}
bool create_lib(IComponent** new_component)
{
    LOGINFO("[save_db_sqlite] create component interface");
    *new_component = (IComponent*)new SaveDBSqlite();
    return true;
}

bool release_lib(IComponent** new_component)
{
    LOGINFO("[save_db_sqlite] release component interface");
    if (!new_component || !*new_component) return true;
    auto* comp = (SaveDBSqlite*)*new_component;
    delete comp;
    *new_component = nullptr;
    return true;
}


SaveDBSqlite::SaveDBSqlite() {}

SaveDBSqlite::~SaveDBSqlite()
{
    stop();
}

bool SaveDBSqlite::init(const DeviceContext& ctx, IDataHub* hub, const std::string& config)
{
    m_ctx = ctx;
    m_hub = hub;

    if (!m_hub) {
        LOGERROR("[save_db_sqlite] init failed: hub is null");
        return false;
    }

    // 解析配置（第一阶段：提供最小可用参数）
    if (!config.empty())
    {
        json_read reader;
        if (reader.init(config) == RES_SUCCESS)
        {
            m_cfg.db_path = reader.get_value<std::string>("db_path");
            // 未配置字段会返回默认值（reader.get_value<T> 不存在时返回 T()）
            const int bs = reader.get_value<int>("batch_size");
            if (bs > 0) m_cfg.batch_size = bs;

            const int fi = reader.get_value<int>("flush_interval_ms");
            if (fi > 0) m_cfg.flush_interval_ms = fi;

            const size_t qm = static_cast<size_t>(reader.get_value<int>("queue_max"));
            if (qm > 0) m_cfg.queue_max = qm;

            const int tsOnly = reader.get_value<int>("accept_time_series_only");
            m_cfg.accept_time_series_only = (tsOnly != 0);

            const int qd = reader.get_value<int>("quality_default");
            if (qd >= 0) m_cfg.quality_default = qd;
        }
    }

    return true;
}

void SaveDBSqlite::start()
{
    if (m_running) return;
    m_running = true;

    // 1) 打开数据库并 ensure schema
    const int rc = sqlite3_open(m_cfg.db_path.c_str(), &m_db);
    if (rc != SQLITE_OK || !m_db) {
        LOGERROR("[save_db_sqlite] sqlite3_open failed: %s", sqlite3_errmsg(m_db));
        m_running = false;
        return;
    }

    ensure_schema();
    preload_point_mapping();

    // 2) 订阅数据总线总转发 topic="0"
    m_sub_id = m_hub->subscribe(DATA_HUB_TOPIC_FORWARD, [this](DataContext::Ptr pkg) {
        if (!pkg) return;

        // 队列保护：尽量丢新数据，保证监控“最新”
        if (m_queue.size_approx() >= m_cfg.queue_max) {
            return;
        }
        m_queue.enqueue(pkg);
    });

    // 3) 启动落库工作线程
    m_worker = std::thread(&SaveDBSqlite::worker_loop, this);
}

void SaveDBSqlite::pause() {}

void SaveDBSqlite::resume() {}

void SaveDBSqlite::stop()
{
    if (!m_running) return;
    m_running = false;

    if (m_hub && m_sub_id > 0) {
        m_hub->unsubscribe(m_sub_id);
        m_sub_id = 0;
    }

    if (m_worker.joinable()) {
        m_worker.join();
    }

    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

void SaveDBSqlite::on_message(int type, const std::string& msg)
{
    (void)type;
    (void)msg;
}

bool SaveDBSqlite::process(DataContext::Ptr& pkg)
{
    // 第一阶段：实际入库发生在订阅回调的 worker_loop 中；
    // process 仅作为组件在 pipeline 中的占位，避免 pipeline 被打断。
    (void)pkg;
    return true;
}

void SaveDBSqlite::ensure_schema()
{
    // data_point_record 在 hermes 初始 schema 下只有 value + timestamp；
    // 但 system_link 第一阶段监控要求 sample_ts_ms / quality，因此这里补齐。
    sqlite3_exec(m_db, "ALTER TABLE data_point_record ADD COLUMN quality INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "ALTER TABLE data_point_record ADD COLUMN sample_ts_ms INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);

    sqlite3_exec(m_db,
                 "CREATE INDEX IF NOT EXISTS idx_record_point_sample_ms "
                 "ON data_point_record(point_id, sample_ts_ms DESC);",
                 nullptr, nullptr, nullptr);
}

void SaveDBSqlite::preload_point_mapping()
{
    // 预加载 device_id 和点位映射，避免每条数据都查库
    // key 采用 device_id + "|" + item_key(description)
    std::string sql =
        "SELECT d.id, d.name, dp.id, dp.description, dp.scale "
        "FROM device d "
        "JOIN data_point dp ON dp.device_id=d.id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        LOGERROR("[save_db_sqlite] preload_point_mapping prepare failed: %s", sqlite3_errmsg(m_db));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const int device_id = sqlite3_column_int(stmt, 0);
        const std::string device_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const int point_id = sqlite3_column_int(stmt, 2);
        const std::string desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const double scale = sqlite3_column_double(stmt, 4);

        (void)device_name; // 目前从 ctx.header.source_device 映射需要另一层；第二阶段可增强
        m_points_by_device[device_id][desc] = PointInfo{point_id, scale};
    }

    sqlite3_finalize(stmt);
}

bool SaveDBSqlite::variant_to_double(const wf::Variant& v, double& out) const
{
    return v.to_double(out);
}

void SaveDBSqlite::enqueue_rows_from_context(const DataContext::Ptr& ctx,
                                             std::vector<std::tuple<int, double, int64_t, int>>& rows)
{
    if (!ctx) return;

    const int64_t sample_ts_ms = static_cast<int64_t>(ctx->header.timestamp_ms);
    const std::string& source_device_name = ctx->header.source_device;

    // device_name -> device_id 当前用两级映射：device表里只有 name 存在；
    // 第一阶段简化：如果无法推导 device_id，则放弃写入。
    // （这里设计上也建议后续把 device_name -> device_id 也预加载）
    // 为了不引入设备名解析逻辑，此处退化为：当无法在 m_points_by_device 中找到对应点位时跳过。
    // 注意：m_points_by_device 的 key 是 device_id，因此这里需要通过 device_name 反查 device_id。

    // 方案：临时查询 device_id（第一阶段容忍，但会影响性能；建议后续改为预加载映射）
    int device_id = -1;
    {
        std::string q = "SELECT id FROM device WHERE name=?1;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, q.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, source_device_name.c_str(), static_cast<int>(source_device_name.size()), SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                device_id = sqlite3_column_int(stmt, 0);
            }
        }
        if (stmt) sqlite3_finalize(stmt);
    }

    if (device_id <= 0) return;

    auto dev_it = m_points_by_device.find(device_id);
    if (dev_it == m_points_by_device.end()) return;

    for (const auto& kv : ctx->items) {
        const std::string& item_key = kv.first;
        const auto& item = kv.second;

        if (m_cfg.accept_time_series_only && item.domain != DataDomain::TIME_SERIES) {
            continue;
        }

        auto pit = dev_it->second.find(item_key);
        if (pit == dev_it->second.end()) {
            continue;
        }

        double raw = 0.0;
        if (!variant_to_double(item.data, raw)) {
            continue;
        }

        const double stored = raw * pit->second.scale;
        rows.emplace_back(pit->second.point_id, stored, sample_ts_ms, m_cfg.quality_default);
    }
}

void SaveDBSqlite::worker_loop()
{
    if (!m_db) return;

    // 预编译插入语句
    sqlite3_stmt* insert_stmt = nullptr;
    const std::string insert_sql =
        "INSERT INTO data_point_record(point_id, value, sample_ts_ms, quality) "
        "VALUES(?1, ?2, ?3, ?4);";

    if (sqlite3_prepare_v2(m_db, insert_sql.c_str(), -1, &insert_stmt, nullptr) != SQLITE_OK) {
        LOGERROR("[save_db_sqlite] prepare insert_stmt failed: %s", sqlite3_errmsg(m_db));
        return;
    }

    std::vector<std::tuple<int, double, int64_t, int>> batch_rows;
    batch_rows.reserve(static_cast<size_t>(m_cfg.batch_size));

    auto last_flush = std::chrono::steady_clock::now();

    // 事务控制
    sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    while (m_running) {
        DataContext::Ptr ctx;
        if (!m_queue.try_dequeue(ctx)) {
            // 若队列暂空，按 flush_interval 定时刷一次已积累的数据
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_flush).count();
            if (!batch_rows.empty() && elapsed_ms >= m_cfg.flush_interval_ms) {
                // flush
                for (const auto& r : batch_rows) {
                    const int point_id = std::get<0>(r);
                    const double value = std::get<1>(r);
                    const int64_t ts_ms = std::get<2>(r);
                    const int quality = std::get<3>(r);

                    sqlite3_bind_int(insert_stmt, 1, point_id);
                    sqlite3_bind_double(insert_stmt, 2, value);
                    sqlite3_bind_int64(insert_stmt, 3, ts_ms);
                    sqlite3_bind_int(insert_stmt, 4, quality);

                    if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
                        LOGERROR("[save_db_sqlite] insert failed: %s", sqlite3_errmsg(m_db));
                    }
                    sqlite3_reset(insert_stmt);
                }

                sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
                sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

                batch_rows.clear();
                last_flush = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // 拆行：一个 DataContext 可能包含多个点位 item
        enqueue_rows_from_context(ctx, batch_rows);

        // flush 条件
        if (static_cast<int>(batch_rows.size()) >= m_cfg.batch_size) {
            for (const auto& r : batch_rows) {
                const int point_id = std::get<0>(r);
                const double value = std::get<1>(r);
                const int64_t ts_ms = std::get<2>(r);
                const int quality = std::get<3>(r);

                sqlite3_bind_int(insert_stmt, 1, point_id);
                sqlite3_bind_double(insert_stmt, 2, value);
                sqlite3_bind_int64(insert_stmt, 3, ts_ms);
                sqlite3_bind_int(insert_stmt, 4, quality);

                if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
                    LOGERROR("[save_db_sqlite] insert failed: %s", sqlite3_errmsg(m_db));
                }
                sqlite3_reset(insert_stmt);
            }

            sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
            sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

            batch_rows.clear();
            last_flush = std::chrono::steady_clock::now();
        }
    }

    // 退出前 flush
    if (!batch_rows.empty()) {
        for (const auto& r : batch_rows) {
            const int point_id = std::get<0>(r);
            const double value = std::get<1>(r);
            const int64_t ts_ms = std::get<2>(r);
            const int quality = std::get<3>(r);

            sqlite3_bind_int(insert_stmt, 1, point_id);
            sqlite3_bind_double(insert_stmt, 2, value);
            sqlite3_bind_int64(insert_stmt, 3, ts_ms);
            sqlite3_bind_int(insert_stmt, 4, quality);

            if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
                LOGERROR("[save_db_sqlite] insert failed: %s", sqlite3_errmsg(m_db));
            }
            sqlite3_reset(insert_stmt);
        }
        sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
    }

    if (insert_stmt) {
        sqlite3_finalize(insert_stmt);
    }
}


