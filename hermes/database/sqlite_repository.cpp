#include "sqlite_repository.h"
#include <iostream>
#include "log/log.h"
#include "db_defines.h"

// 构造函数：初始化指针

SQLiteRepository::SQLiteRepository() 
{
}

// 析构函数：确保关闭连接
SQLiteRepository::~SQLiteRepository() 
{
    close();
}

std::string SQLiteRepository::err_message()
{
	if (!m_db) return "Database not opened.";

    return sqlite3_errmsg(m_db);
}

void SQLiteRepository::close() 
{
    MutexLockGuard lock(m_mutex);
    if (m_db) 
    {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool SQLiteRepository::init()
{
    if (!init_tables()) {
		LOGERROR("Failed to initialize database tables: %s", err_message().c_str());
        return false;
    }

    if (!init_default_data()) {
		LOGERROR("Failed to initialzie database data: %s", err_message().c_str());
        return false;
    }
    return true;
}

bool SQLiteRepository::init_tables()
{
    std::ostringstream oss;

    // 1. 创建 Device 表
    oss << SQL_CREATE_IF_NOT_EXISTS << " " << TABLE_DEVICE << "("
        << FIELD_DEVICE_ID << " " << SQL_PRIMARY_KEY_AUTOINCREMENT << ","
        << FIELD_DEVICE_NAME << " TEXT NOT NULL UNIQUE,"
        << FIELD_DEVICE_PROTOCOL << " TEXT NOT NULL,"
        << FIELD_DEVICE_CREATED_AT << " TEXT DEFAULT " << SQL_NOW_LOCALTIME << ","
        << FIELD_DEVICE_UPDATED_AT << " TEXT DEFAULT " << SQL_NOW_LOCALTIME
        << ");";

    // 2. 创建 Protocol Config 表
    oss << SQL_CREATE_IF_NOT_EXISTS << " " << TABLE_PROTOCOL_CONFIG << "("
        << FIELD_CONFIG_ID << " " << SQL_PRIMARY_KEY_AUTOINCREMENT << ","
        << FIELD_CONFIG_DEVICE_ID << " INTEGER NOT NULL,"
        << FIELD_CONFIG_HOST << " TEXT,"
        << FIELD_CONFIG_PORT << " INTEGER,"
        << FIELD_CONFIG_STATION << " INTEGER,"
        << FIELD_CONFIG_BAUD << " INTEGER,"
        << FIELD_CONFIG_DATA << " INTEGER,"
        << FIELD_CONFIG_STOP << " REAL,"
        << FIELD_CONFIG_PARITY << " TEXT,"
        << FIELD_CONFIG_TIMEOUT_MS << " INTEGER DEFAULT 1000,"
        << FIELD_CONFIG_RETRY << " INTEGER DEFAULT 3,"
        << FIELD_CONFIG_CREATED_AT << " TEXT DEFAULT " << SQL_NOW_LOCALTIME << ","
        << "FOREIGN KEY(" << FIELD_CONFIG_DEVICE_ID << ") REFERENCES " << TABLE_DEVICE << "(" << FIELD_DEVICE_ID << ") ON DELETE CASCADE"
        << ");";

    // 3. 创建 Data Point 表
    oss << SQL_CREATE_IF_NOT_EXISTS << " " << TABLE_DATA_POINT << "("
        << FIELD_POINT_ID << " " << SQL_PRIMARY_KEY_AUTOINCREMENT << ","
        << FIELD_POINT_DEVICE_ID << " INTEGER NOT NULL,"
        << FIELD_POINT_TYPE << " TEXT NOT NULL,"
        << FIELD_POINT_ADDRESS << " INTEGER NOT NULL,"
        << FIELD_POINT_VALUE_TYPE << " TEXT NOT NULL,"
        << FIELD_POINT_SCALE << " REAL DEFAULT 1.0,"
        << FIELD_POINT_EXPRESSION << " TEXT DEFAULT '',"
        << FIELD_POINT_DESCRIPTION << " TEXT DEFAULT '',"
        << FIELD_POINT_CONTROL << " INTEGER DEFAULT 0,"
        << FIELD_POINT_CREATED_AT << " TEXT DEFAULT " << SQL_NOW_LOCALTIME << ","
        << FIELD_POINT_UPDATED_AT << " TEXT DEFAULT " << SQL_NOW_LOCALTIME << ","
        << "FOREIGN KEY(" << FIELD_POINT_DEVICE_ID << ") REFERENCES " << TABLE_DEVICE << "(" << FIELD_DEVICE_ID << ") ON DELETE CASCADE"
        << ");";

    // 4. 创建 Data Point Record 表
    oss << SQL_CREATE_IF_NOT_EXISTS << " " << TABLE_DATA_POINT_RECORD << "("
        << FIELD_RECORD_ID << " " << SQL_PRIMARY_KEY_AUTOINCREMENT << ","
        << FIELD_RECORD_POINT_ID << " INTEGER NOT NULL,"
        << FIELD_RECORD_VALUE << " TEXT NOT NULL,"
        << FIELD_RECORD_TIMESTAMP << " TEXT DEFAULT " << SQL_NOW_LOCALTIME << ","
        << "FOREIGN KEY(" << FIELD_RECORD_POINT_ID << ") REFERENCES " << TABLE_DATA_POINT << "(" << FIELD_POINT_ID << ") ON DELETE CASCADE"
        << ");";

    // 5. 创建 device_streams 表 (组件流定义)
    oss << SQL_CREATE_IF_NOT_EXISTS << " " << TABLE_DEVICE_STREAMS << "("
        << FIELD_STREAM_ID << " " << SQL_PRIMARY_KEY_AUTOINCREMENT << ","
        << FIELD_STREAM_DEVICE_ID << " INTEGER,"
        << FIELD_STREAM_NAME << " TEXT,"
        << FIELD_STREAM_IS_ACTIVE << " INTEGER DEFAULT 1,"
        << FIELD_STREAM_SUBSCRIBE_TOPIC << " TEXT DEFAULT '',"  // 订阅 topic，空则默认 stream_id
        << "FOREIGN KEY(" << FIELD_STREAM_DEVICE_ID << ") REFERENCES " << TABLE_DEVICE << "(" << FIELD_DEVICE_ID << ") ON DELETE CASCADE,"
        // 联合唯一约束
        << "UNIQUE(" << FIELD_STREAM_DEVICE_ID << ", " << FIELD_STREAM_NAME << ")"
        << ");";

    // 6. 创建 stream_components 表 (流中的组件节点)
    // 假设常量定义为: TABLE_STREAM_COMPONENTS, FIELD_COMP_ID, FIELD_COMP_STREAM_ID, FIELD_COMP_ORDER_INDEX, 
    // FIELD_COMP_LIB_NAME, FIELD_COMP_CONFIG, FIELD_COMP_OUTPUT_NEXT
    oss << SQL_CREATE_IF_NOT_EXISTS << " " << TABLE_STREAM_COMPONENTS << "("
        << FIELD_COMP_ID << " " << SQL_PRIMARY_KEY_AUTOINCREMENT << ","
        << FIELD_COMP_STREAM_ID << " INTEGER,"
        << FIELD_COMP_ORDER_INDEX << " INTEGER,"
        << FIELD_COMP_LIB_NAME << " TEXT,"
        << FIELD_COMP_CONFIG << " TEXT,"
        << FIELD_COMP_OUTPUT_NEXT << " INTEGER,"
        << "FOREIGN KEY(" << FIELD_COMP_STREAM_ID << ") REFERENCES " << TABLE_DEVICE_STREAMS << "(" << FIELD_STREAM_ID << ") ON DELETE CASCADE,"
        // 添加以下这一行：联合唯一约束
        << "UNIQUE(" << FIELD_COMP_STREAM_ID << ", " << FIELD_COMP_ORDER_INDEX << ")"
        << ");";
    if (!execute_query(oss.str())) return false;

    // 5. 创建索引
    std::ostringstream ioss;
    ioss << SQL_CREATE_INDEX_IF_NOT_EXISTS << " " << INDEX_DEVICE_PROTOCOL
        << " ON " << TABLE_DEVICE << "(" << FIELD_DEVICE_PROTOCOL << ", " << FIELD_DEVICE_NAME << ");"
        << SQL_CREATE_INDEX_IF_NOT_EXISTS << " " << INDEX_POINT_DEVICE_ADDR
        << " ON " << TABLE_DATA_POINT << "(" << FIELD_POINT_DEVICE_ID << ");"
        << SQL_CREATE_INDEX_IF_NOT_EXISTS << " " << INDEX_RECORD_POINT_TIME
        << " ON " << TABLE_DATA_POINT_RECORD << "(" << FIELD_RECORD_POINT_ID << ", " << FIELD_RECORD_TIMESTAMP << ");"
        << SQL_CREATE_INDEX_IF_NOT_EXISTS << " "<< INDEX_STREAM_DEVICE 
        << " ON " << TABLE_DEVICE_STREAMS << "(" << FIELD_STREAM_DEVICE_ID << "); "
        << SQL_CREATE_INDEX_IF_NOT_EXISTS << " "<< INDEX_COMP_STREAM_ORDER 
        << " ON " << TABLE_STREAM_COMPONENTS << "(" << FIELD_COMP_STREAM_ID << ", " << FIELD_COMP_ORDER_INDEX << "); ";
    return execute_query(ioss.str());
}

// 初始化数据库
bool SQLiteRepository::open(const std::string& db_path) 
{
    if (m_db) return true;
    MutexLockGuard lock(m_mutex);
    m_db_path = db_path;

    // 1. 打开数据库
    int rc = sqlite3_open(m_db_path.c_str(), &m_db);
    if (rc != SQLITE_OK)
    {
		LOGERROR("Can't open database: %s", sqlite3_errmsg(m_db));
        return false;
    }
 
    sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    return true;
}

bool SQLiteRepository::init_default_data()
{
    int device_id = 0;
    // 默认在设备表创建一个虚拟设备，用于总线流程，强制设备id为0
    if (!insert_device("Virtual_Bus_Device", "VIRTUAL", device_id)) {
        return false;
    }
    else {
        if (device_id == 0)
            device_id = query_device("Virtual_Bus_Device");
    }

	// 创建一个默认的数据总线流
	int stream_id = query_device_stream(device_id, "Data_Hub");
    if (stream_id == 0 && !insert_device_stream(device_id, "Data_Hub", 1, "data_hub_topic", stream_id)) {
        return false;
    }

	// 创建一个默认的转发 MQTT 组件
    int component_id = query_stream_component(stream_id, 1);
    if (component_id == 0 &&!insert_stream_component(stream_id, 1, "forward_send_mqtt", "{}", 0, component_id)) {
        return false;
    }
    return  true;
}

// 辅助函数：安全读取文本列
std::string SQLiteRepository::column_text(sqlite3_stmt* stmt, int col_index)
{
    const unsigned char* text = sqlite3_column_text(stmt, col_index);
    return text ? std::string(reinterpret_cast<const char*>(text)) : std::string();
}


// 辅助函数：执行简单的 EXEC
bool SQLiteRepository::execute_query(const std::string& sql) 
{
    char* z_err_msg = 0;
    int rc = sqlite3_exec(m_db, sql.c_str(), 0, 0, &z_err_msg);
    if (rc != SQLITE_OK) {
		LOGERROR("SQL error: %s", z_err_msg);
        sqlite3_free(z_err_msg);
        return false;
    }
    return true;
}

//获取所有设备
std::vector<DeviceDTO> SQLiteRepository::query_all_device() 
{
    sqlite3_stmt* stmt;
    std::vector<DeviceDTO> list;

    MutexLockGuard lock(m_mutex);

    std::ostringstream oss;
    oss << "SELECT d." << FIELD_DEVICE_ID << ", d." << FIELD_DEVICE_NAME << ", d." << FIELD_DEVICE_PROTOCOL
        << ", d." << FIELD_DEVICE_CREATED_AT << ", d." << FIELD_DEVICE_UPDATED_AT << ", "
        << "pc." << FIELD_CONFIG_HOST << ", pc." << FIELD_CONFIG_PORT << ", pc." << FIELD_CONFIG_STATION
        << ", pc." << FIELD_CONFIG_BAUD << ", pc." << FIELD_CONFIG_DATA << ", "
        << "pc." << FIELD_CONFIG_STOP << ", pc." << FIELD_CONFIG_PARITY << ", pc." << FIELD_CONFIG_TIMEOUT_MS
        << ", pc." << FIELD_CONFIG_RETRY << ", pc." << FIELD_CONFIG_CREATED_AT << " "
        << "FROM " << TABLE_DEVICE << " d "
        << "LEFT JOIN " << TABLE_PROTOCOL_CONFIG << " pc ON d." << FIELD_DEVICE_ID << " = pc." << FIELD_CONFIG_DEVICE_ID << " "
        << "ORDER BY d." << FIELD_DEVICE_ID << ";";
    std::string sql = oss.str();

    // 预编译 SQL
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) 
    {
		LOGERROR("Failed to prepare statement: %s", sqlite3_errmsg(m_db));
        return list;
    }

    // 遍历结果集
    while (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        DeviceDTO device;
        // device 表字段
        device.id = sqlite3_column_int(stmt, 0);
        device.name = column_text(stmt, 1);
        device.protocol = column_text(stmt, 2);
        device.created_at = column_text(stmt, 3);
		device.updated_at = column_text(stmt, 4);
		
        // 检查 protocol_config 的字段是否为 NULL
		auto& protocol_cfg = device.protocol_config;
        protocol_cfg.host = sqlite3_column_type(stmt, 5) != SQLITE_NULL ? column_text(stmt, 5) : "";
		protocol_cfg.port = sqlite3_column_type(stmt, 6) != SQLITE_NULL ? sqlite3_column_int(stmt, 6) : 0;
		protocol_cfg.station = sqlite3_column_type(stmt, 7) != SQLITE_NULL ? sqlite3_column_int(stmt, 7) : 0;
        protocol_cfg.baud = sqlite3_column_type(stmt, 8) != SQLITE_NULL ? sqlite3_column_int(stmt,8) : 0;
        protocol_cfg.data_bits = sqlite3_column_type(stmt, 9) != SQLITE_NULL ? sqlite3_column_int(stmt, 9) : 0;
        protocol_cfg.stop_bits = sqlite3_column_type(stmt, 10) != SQLITE_NULL ? sqlite3_column_double(stmt, 10) : 0;
        protocol_cfg.parity = sqlite3_column_type(stmt, 11) != SQLITE_NULL ? column_text(stmt, 11) : "";
        protocol_cfg.timeout_ms = sqlite3_column_type(stmt, 12) != SQLITE_NULL ? sqlite3_column_int(stmt, 12) : 0;
        protocol_cfg.retry = sqlite3_column_type(stmt, 13) != SQLITE_NULL ? sqlite3_column_int(stmt, 13) : 0;
        protocol_cfg.created_at = sqlite3_column_type(stmt, 14) != SQLITE_NULL ? column_text(stmt, 14) : "";

        list.push_back(device);
    }

    // 释放 Statement
    sqlite3_finalize(stmt);
    return list;
}

bool SQLiteRepository::insert_device(const std::string& name, const std::string& protocol, int& device_id)
{
    if (!m_db) return false;
    MutexLockGuard lock(m_mutex);

    std::ostringstream oss;
    oss << "INSERT OR IGNORE INTO " << TABLE_DEVICE << " ("
        << FIELD_DEVICE_NAME << ", "
        << FIELD_DEVICE_PROTOCOL << ") "
        << "VALUES ("
		<< "'" << name << "', "
        << "'" << protocol << "');";

    if (!execute_query(oss.str())) return false;
    device_id = (int)sqlite3_last_insert_rowid(m_db);
    return true;
}

DeviceDTO SQLiteRepository::query_device(int device_id)
{
    MutexLockGuard lock(m_mutex);
    DeviceDTO device;
    device.id = -1; // 初始化为无效 ID，用于表示未找到

    std::ostringstream oss;
    oss << "SELECT d." << FIELD_DEVICE_ID << ", d." << FIELD_DEVICE_NAME << ", d." << FIELD_DEVICE_PROTOCOL
        << ", d." << FIELD_DEVICE_CREATED_AT << ", d." << FIELD_DEVICE_UPDATED_AT << ", "
        << "pc." << FIELD_CONFIG_HOST << ", pc." << FIELD_CONFIG_PORT << ", pc." << FIELD_CONFIG_STATION
        << ", pc." << FIELD_CONFIG_BAUD << ", pc." << FIELD_CONFIG_DATA << ", "
        << "pc." << FIELD_CONFIG_STOP << ", pc." << FIELD_CONFIG_PARITY << ", pc." << FIELD_CONFIG_TIMEOUT_MS
        << ", pc." << FIELD_CONFIG_RETRY << ", pc." << FIELD_CONFIG_CREATED_AT << " "
        << "FROM " << TABLE_DEVICE << " d "
        << "LEFT JOIN " << TABLE_PROTOCOL_CONFIG << " pc ON d." << FIELD_DEVICE_ID << " = pc." << FIELD_CONFIG_DEVICE_ID << " "
        << "WHERE d." << FIELD_DEVICE_ID << " = " << device_id << ";";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, oss.str().c_str(), -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            device.id = sqlite3_column_int(stmt, 0);
            device.name = column_text(stmt, 1);
            device.protocol = column_text(stmt, 2);
            device.created_at = column_text(stmt, 3);
            device.updated_at = column_text(stmt, 4);

            auto& pc = device.protocol_config;
            pc.host = (sqlite3_column_type(stmt, 5) != SQLITE_NULL) ? column_text(stmt, 5) : "";
            pc.port = sqlite3_column_int(stmt, 6);
            pc.station = sqlite3_column_int(stmt, 7);
            pc.baud = sqlite3_column_int(stmt, 8);
            pc.data_bits = sqlite3_column_int(stmt, 9);
            pc.stop_bits = sqlite3_column_double(stmt, 10);
            pc.parity = column_text(stmt, 11);
            pc.timeout_ms = sqlite3_column_int(stmt, 12);
            pc.retry = sqlite3_column_int(stmt, 13);
            pc.created_at = column_text(stmt, 14);
        }
        sqlite3_finalize(stmt);
    }
    else {
        LOGERROR("Query device failed: %s", sqlite3_errmsg(m_db));
    }
    return device;
}

int SQLiteRepository::query_device(const std::string& name)
{
    std::vector<DataPointDTO> list;
    MutexLockGuard lock(m_mutex);

    std::ostringstream oss;
    oss << "SELECT "
        << FIELD_DEVICE_ID << " "
        << "FROM " << TABLE_DEVICE << " "
        << "WHERE " << FIELD_DEVICE_NAME << " = " << "'" << name << "'" << ";";
    std::string sql = oss.str();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK)
    {
        LOGERROR("Failed to prepare statement: %s", sqlite3_errmsg(m_db));
        return 0;
    }

    int device_id = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        device_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return device_id;
}

bool SQLiteRepository::insert_device_stream(int device_id, const std::string& stream_name, int is_active, const std::string& subscribe_topic, int& stream_id)
{
    if (!m_db) return false;
    MutexLockGuard lock(m_mutex);

    std::ostringstream oss;
    oss << "INSERT INTO " << TABLE_DEVICE_STREAMS << " ("
        << FIELD_STREAM_DEVICE_ID << ", " << FIELD_STREAM_NAME << ", "
        << FIELD_STREAM_IS_ACTIVE << ", " << FIELD_STREAM_SUBSCRIBE_TOPIC << ") VALUES ("
        << device_id << ", '" << stream_name << "', " << is_active << ", '" << subscribe_topic << "');";

    if (!execute_query(oss.str())) return false;

    stream_id = (int)sqlite3_last_insert_rowid(m_db);
    return true;
}

StreamDTO SQLiteRepository::query_device_stream(int stream_id)
{
    MutexLockGuard lock(m_mutex);
    StreamDTO stream;
    stream.id = -1;

    std::ostringstream oss;
    oss << "SELECT " << FIELD_STREAM_ID << ", " << FIELD_STREAM_DEVICE_ID << ", "
        << FIELD_STREAM_NAME << ", " << FIELD_STREAM_IS_ACTIVE << ", "
        << FIELD_STREAM_SUBSCRIBE_TOPIC << " FROM " << TABLE_DEVICE_STREAMS
        << " WHERE " << FIELD_STREAM_ID << " = " << stream_id << ";";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, oss.str().c_str(), -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stream.id = sqlite3_column_int(stmt, 0);
            stream.device_id = sqlite3_column_int(stmt, 1);
            stream.stream_name = column_text(stmt, 2);
            stream.is_active = sqlite3_column_int(stmt, 3);
            stream.subscribe_topic = column_text(stmt, 4);
        }
        sqlite3_finalize(stmt);
    }
    return stream;
}

int SQLiteRepository::query_device_stream(int device_id, const std::string& stream_name)
{
    std::vector<DataPointDTO> list;
    MutexLockGuard lock(m_mutex);

    std::ostringstream oss;
    oss << "SELECT "
        << FIELD_STREAM_ID << " "
        << "FROM " << TABLE_DEVICE_STREAMS << " "
        << "WHERE " << FIELD_STREAM_DEVICE_ID << " = " << device_id << " AND "
        << FIELD_STREAM_NAME <<" = '" << stream_name << "'" << ";";
    std::string sql = oss.str();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK)
    {
        LOGERROR("Failed to prepare statement: %s", sqlite3_errmsg(m_db));
        return 0;
    }

    int id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return id;
}

bool SQLiteRepository::insert_stream_component(int stream_id, int order_index, const std::string& lib_name, const std::string& comp_config, int output_to_next, int& component_id)
{
    if (!m_db) return false;
    MutexLockGuard lock(m_mutex);

    std::ostringstream oss;
    oss << "INSERT INTO " << TABLE_STREAM_COMPONENTS << " ("
        << FIELD_COMP_STREAM_ID << ", " << FIELD_COMP_ORDER_INDEX << ", "
        << FIELD_COMP_LIB_NAME << ", " << FIELD_COMP_CONFIG << ", "
        << FIELD_COMP_OUTPUT_NEXT << ") VALUES ("
        << stream_id << ", " << order_index << ", '" << lib_name << "', '"
        << comp_config << "', " << output_to_next << ");";

    if (!execute_query(oss.str())) return false;

    component_id = (int)sqlite3_last_insert_rowid(m_db);
    return true;
}

ComponectDTO SQLiteRepository::query_stream_component(int component_id)
{
    MutexLockGuard lock(m_mutex);
    ComponectDTO comp;
    comp.id = -1;

    std::ostringstream oss;
    oss << "SELECT " << FIELD_COMP_ID << ", " << FIELD_COMP_STREAM_ID << ", "
        << FIELD_COMP_ORDER_INDEX << ", " << FIELD_COMP_LIB_NAME << ", "
        << FIELD_COMP_CONFIG << ", " << FIELD_COMP_OUTPUT_NEXT
        << " FROM " << TABLE_STREAM_COMPONENTS << " WHERE " << FIELD_COMP_ID << " = " << component_id << ";";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, oss.str().c_str(), -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            comp.id = sqlite3_column_int(stmt, 0);
            comp.stream_id = sqlite3_column_int(stmt, 1);
            comp.order_index = sqlite3_column_int(stmt, 2);
            comp.lib_name = column_text(stmt, 3);
            comp.comp_config = column_text(stmt, 4);
            comp.output_to_next = sqlite3_column_int(stmt, 5);
        }
        sqlite3_finalize(stmt);
    }
    return comp;
}

int SQLiteRepository::query_stream_component(int stream_id, int order_index)
{
    std::vector<DataPointDTO> list;
    MutexLockGuard lock(m_mutex);

    std::ostringstream oss;
    oss << "SELECT "
        << FIELD_COMP_ID << " "
        << "FROM " << TABLE_STREAM_COMPONENTS << " "
        << "WHERE " << FIELD_COMP_STREAM_ID << " = " << stream_id << " AND "
        << FIELD_COMP_ORDER_INDEX << " = " << order_index << ";";
    std::string sql = oss.str();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK)
    {
        LOGERROR("Failed to prepare statement: %s", sqlite3_errmsg(m_db));
        return 0;
    }

    int id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return id;
}

std::vector<StreamDTO> SQLiteRepository::query_streams_by_device(int device_id)
{
    std::vector<StreamDTO> list;
    MutexLockGuard lock(m_mutex);

    std::ostringstream oss;
    oss << "SELECT "
        << FIELD_STREAM_ID << ", "
        << FIELD_STREAM_DEVICE_ID << ", "
        << FIELD_STREAM_NAME << ", "
        << FIELD_STREAM_IS_ACTIVE << ", "
        << FIELD_STREAM_SUBSCRIBE_TOPIC << " "
        << "FROM " << TABLE_DEVICE_STREAMS << " "
        << "WHERE " << FIELD_STREAM_DEVICE_ID << " = " << device_id << " "
        << "AND " << FIELD_STREAM_IS_ACTIVE << " = 1;";
    std::string sql = oss.str();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK)
    {
        LOGERROR("Failed to prepare statement: %s", sqlite3_errmsg(m_db));
        return list;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        StreamDTO stream;
        stream.id = sqlite3_column_int(stmt, 0);
        stream.device_id = sqlite3_column_int(stmt, 1);
        stream.stream_name = column_text(stmt, 2);
        stream.is_active = sqlite3_column_int(stmt, 3);
        stream.subscribe_topic = column_text(stmt, 4);
        list.push_back(stream);
    }

    sqlite3_finalize(stmt);
    return list;
}


// 获取数据点
std::vector<DataPointDTO> SQLiteRepository::query_points_by_device(int device_id)
{
    std::vector<DataPointDTO> list;
    MutexLockGuard lock(m_mutex);

    std::ostringstream oss;
    oss << "SELECT "
        << FIELD_POINT_ID << ", "
        << FIELD_POINT_TYPE << ", "
        << FIELD_POINT_ADDRESS << ", "
        << FIELD_POINT_VALUE_TYPE << ", "
        << FIELD_POINT_SCALE << ", "
        << FIELD_POINT_EXPRESSION << ", "
        << FIELD_POINT_DESCRIPTION << ", "
        << FIELD_POINT_CONTROL << ", "
        << FIELD_POINT_CREATED_AT << ", "
        << FIELD_POINT_UPDATED_AT << " "
        << "FROM " << TABLE_DATA_POINT << " "
        << "WHERE " << FIELD_POINT_DEVICE_ID << " = " << device_id << ";";
    std::string sql = oss.str();    

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK)
    {
		LOGERROR("Failed to prepare statement: %s", sqlite3_errmsg(m_db));
        return list;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        DataPointDTO point;
        point.point_id = sqlite3_column_int(stmt, 0);
        point.type = column_text(stmt, 1);
        point.address = sqlite3_column_int(stmt, 2);
        point.value_type = column_text(stmt, 3);
        point.scale = sqlite3_column_double(stmt, 4);
        point.expression = column_text(stmt, 5);
        point.description = column_text(stmt, 6);
        point.control = sqlite3_column_int(stmt, 7);
		point.created_at = column_text(stmt, 8);
		point.updated_at = column_text(stmt, 9);

		point.device_id = device_id;
        list.push_back(point);
    }

    sqlite3_finalize(stmt);
    return list;
}

std::vector<ComponectDTO> SQLiteRepository::query_components_by_stream(int stream_id)
{
    std::vector<ComponectDTO> list;
    MutexLockGuard lock(m_mutex);

    std::ostringstream oss;
    oss << "SELECT "
        << FIELD_COMP_ID << ", "
        << FIELD_COMP_STREAM_ID << ", "
        << FIELD_COMP_ORDER_INDEX << ", "
        << FIELD_COMP_LIB_NAME << ", "
        << FIELD_COMP_CONFIG << ", "
        << FIELD_COMP_OUTPUT_NEXT << " "
        << "FROM " << TABLE_STREAM_COMPONENTS << " "
        << "WHERE " << FIELD_COMP_STREAM_ID << " = " << stream_id << " "
        << "ORDER BY " << FIELD_COMP_ORDER_INDEX << " ASC;";
    std::string sql = oss.str();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK)
    {
        LOGERROR("Failed to prepare statement: %s", sqlite3_errmsg(m_db));
        return list;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ComponectDTO comp;
        comp.id = sqlite3_column_int(stmt, 0);
        comp.stream_id = sqlite3_column_int(stmt, 1);
        comp.order_index = sqlite3_column_int(stmt, 2);
        comp.lib_name = column_text(stmt, 3);
        comp.comp_config = column_text(stmt, 4);
        comp.output_to_next = sqlite3_column_int(stmt, 5);
        list.push_back(comp);
    }

    sqlite3_finalize(stmt);
    return list;
}
