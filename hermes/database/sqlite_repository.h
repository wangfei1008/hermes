#ifndef SQLITE_REPOSITORY_H
#define SQLITE_REPOSITORY_H

#include "sqlite/sqlite3.h"
#include "thread/MutexLock.h"
#include "db_models.h"

class SQLiteRepository 
{
public:
    SQLiteRepository();
    ~SQLiteRepository();

    static std::string err_message();

    // 初始化数据库表结构
    static bool open(const std::string& db_path);

    // 关闭数据库
    static void close();

    static bool init_tables();

    //获取所有设备
    static std::vector<DeviceDTO> query_all_device();

    static DeviceDTO query_device(int device_id);

    static std::vector<StreamDTO> query_streams_by_device(int device_id);

    // 根据协议ID获取数据点
    static std::vector<DataPointDTO> query_points_by_device(int device_id);


    static std::vector<ComponectDTO> query_components_by_stream(int stream_id);

private:
    // 辅助函数：安全地将 SQLite 的 unsigned char* 转为 std::string
    static std::string column_text(sqlite3_stmt* stmt, int col_index);
    // 辅助函数：执行简单的 EXEC
    static bool execute_query(const std::string& sql);
private:
    static inline sqlite3* m_db = nullptr;
    static inline std::string m_db_path;
    static inline MutexLock m_mutex = MutexLock();
};

#endif // SQLITE_REPOSITORY_H