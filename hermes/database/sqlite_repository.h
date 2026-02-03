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

    static bool init();

    
    //获取所有设备
    static std::vector<DeviceDTO> query_all_device();
    
    //设备表
    static bool insert_device(const std::string& name, const std::string& protocol, int& device_id);
    static DeviceDTO query_device(int device_id);
    static int query_device(const std::string& name);

    //设备流表
	static bool insert_device_stream(int device_id, const std::string& stream_name, int is_active, const std::string& subscribe_topic, int& stream_id);
	static StreamDTO query_device_stream(int stream_id);
	static int query_device_stream(int device_id, const std::string& stream_name);

    // 根据设备ID获取所有流
    static std::vector<StreamDTO> query_streams_by_device(int device_id);
    
    //流组件表
	static bool insert_stream_component(int stream_id, int order_index, const std::string& lib_name, const std::string& comp_config, int output_to_next, int& component_id);
	static ComponectDTO query_stream_component(int component_id);
	static int query_stream_component(int stream_id, int order_index);
    static std::vector<ComponectDTO> query_components_by_stream(int stream_id);

    // 根据协议ID获取数据点
    static std::vector<DataPointDTO> query_points_by_device(int device_id);


private:
    static bool init_tables();

    //初始化部分表数据
	static bool init_default_data();
    
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