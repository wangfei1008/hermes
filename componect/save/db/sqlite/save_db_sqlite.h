#ifndef SAVE_DB_SQLITE_H
#define SAVE_DB_SQLITE_H

#include "i_component.h"
#include "i_data_hub.h"
#include "data_context.h"
#include "data/variant.h"

#include "ConCurrentQueue/concurrentqueue.h"
#include <sqlite/sqlite3.h>

#include <atomic>
#include <thread>
#include <unordered_map>
#include <vector>
#include <string>

struct SaveDbSqliteConfig
{
    std::string db_path = "db/netsys_daq_hub.db";
    int batch_size = 500;
    int flush_interval_ms = 1000;
    size_t queue_max = 50000;
    // 第一阶段：只入库时域序列
    bool accept_time_series_only = true;
    int quality_default = 0;
};

class SaveDbSqlite : public IComponent
{
public:
    SaveDbSqlite();
    ~SaveDbSqlite();

    bool init(const DeviceContext& ctx, IDataHub* hub, const std::string& config) override;
    void start() override;
    void pause() override;
    void resume() override;
    void stop() override;

    void on_message(int type, const std::string& msg) override;
    bool process(DataContext::Ptr& pkg) override;

private:
    void worker_loop();
    void ensure_schema();
    void preload_point_mapping();
    bool variant_to_double(const wf::Variant& v, double& out) const;
    void enqueue_rows_from_context(const DataContext::Ptr& ctx, std::vector<std::tuple<int, double, int64_t, int>>& rows);

private:
    DeviceContext m_ctx{};
    IDataHub* m_hub = nullptr;
    SaveDbSqliteConfig m_cfg{};

    std::atomic<bool> m_running{false};
    uint64_t m_sub_id = 0;
    std::thread m_worker;

    moodycamel::ConcurrentQueue<DataContext::Ptr> m_queue;

    sqlite3* m_db = nullptr;

    struct PointInfo
    {
        int point_id = 0;
        double scale = 1.0;
    };

    // device_id -> (description/key -> PointInfo)
    std::unordered_map<int, std::unordered_map<std::string, PointInfo>> m_points_by_device;
};

#endif // SAVE_DB_SQLITE_H

