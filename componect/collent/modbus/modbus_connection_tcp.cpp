
#include "modbus_connection_tcp.h"
#include "log/log.h"

ModbusConnectionTCP::ModbusConnectionTCP(const std::string& ip, int port, int slave_id)
    : IModbusConnection()
    , m_read_ctx(nullptr)
	, m_write_ctx(nullptr)
{
    m_conn.type = modbus_conntype::IP_TCP;
    m_conn.slave = slave_id;
    memcpy(m_conn.info.tcp.ip, ip.c_str(), ip.length());
    m_conn.info.tcp.port = port;
}


bool ModbusConnectionTCP::create_contexts()
{
    // 创建主连接上下文
    modbus_t* main_ctx = modbus_new_tcp(m_conn.info.tcp.ip, m_conn.info.tcp.port);
    if (!main_ctx) {
        LOGERROR("Failed to create main Modbus context");
        return false;
    }

    // 设置SO_REUSEADDR选项
    set_reuse_addr(main_ctx);

    // 设置连接超时
    modbus_set_response_timeout(main_ctx, 5, 0);

    if (modbus_connect(main_ctx) == -1) {
        LOGERROR("Main connection failed: %s", modbus_strerror(errno));
        modbus_free(main_ctx);
        return false;
    }

    // 获取主连接的套接字
    int socket_fd = modbus_get_socket(main_ctx);

    // 创建读上下文
    m_read_ctx = modbus_new_tcp(m_conn.info.tcp.ip, m_conn.info.tcp.port);
    if (!m_read_ctx) {
        LOGERROR("Failed to create read context, err = %s", modbus_strerror(errno));
        goto cleanup;
    }
    modbus_set_socket(m_read_ctx, socket_fd);

    // 创建写上下文
    m_write_ctx = modbus_new_tcp(m_conn.info.tcp.ip, m_conn.info.tcp.port);
    if (!m_write_ctx) {
        modbus_set_socket(m_read_ctx, -1);  // 分离socket
        LOGERROR("Failed to create write context, err = %s", modbus_strerror(errno));
        goto cleanup;
    }
    modbus_set_socket(m_write_ctx, socket_fd);

    // 主上下文已完成使命，可以释放
    modbus_close(main_ctx);
    modbus_free(main_ctx);
    main_ctx = nullptr;

    // 设置keepalive
    setup_keepalive(m_read_ctx);

    m_connected = true;
    LOGINFO("Modbus TCP connected successfully");
    return true;

cleanup:
    if (main_ctx) {
        modbus_close(main_ctx);
        modbus_free(main_ctx);
        main_ctx = nullptr;
    }
    if (m_read_ctx) {
        modbus_free(m_read_ctx);
        m_read_ctx = nullptr;
    }
    if (m_write_ctx) {
        modbus_free(m_write_ctx);
        m_write_ctx = nullptr;
    }
    close_sokcet(socket_fd);
    return false;
}

modbus_t* ModbusConnectionTCP::get_read_context() const
{
    return m_read_ctx;
}

modbus_t* ModbusConnectionTCP::get_write_context() const
{
    return m_write_ctx;
}

void ModbusConnectionTCP::disconnect()
{
    int socket_fd = 0;
    if (m_read_ctx)
    {
        // 先分离socket，防止被关闭两次
        socket_fd = modbus_get_socket(m_read_ctx);
        modbus_set_socket(m_read_ctx, -1);
        modbus_close(m_read_ctx);
        modbus_free(m_read_ctx);
        m_read_ctx = nullptr;

        // 如果写上下文使用同一个socket，也分离
        if (m_write_ctx && modbus_get_socket(m_write_ctx) == socket_fd) {
            modbus_set_socket(m_write_ctx, -1);
        }
    }

    if (m_write_ctx && m_write_ctx != m_read_ctx)
    {
        modbus_close(m_write_ctx);
        modbus_free(m_write_ctx);
        m_write_ctx = nullptr;
    }
    close_sokcet(socket_fd);

    m_connected = false;
}

void ModbusConnectionTCP::setup_keepalive(modbus_t* ctx)
{
    int socket_fd = modbus_get_socket(ctx);
    if (socket_fd != -1)
    {
        int enable_keepalive = 1;
        setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable_keepalive, sizeof(enable_keepalive));
#ifdef __linux__
        // Linux下设置更激进的keepalive参数
        int keep_idle = 5;    // 5秒无数据后开始发送keepalive
        int keep_interval = 5; // 每5秒发送一次
        int keep_count = 3;   // 发送3次后无响应判定断开

        setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle));
        setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval));
        setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPCNT, &keep_count, sizeof(keep_count));
#endif
    }
}

void ModbusConnectionTCP::set_reuse_addr(modbus_t *ctx)
{

    int socket_fd = modbus_get_socket(ctx);
    if (socket_fd != -1)
    {
        // 设置SO_REUSEADDR选项
        int enable = 1;
        setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable));
#ifdef __linux__

#endif
    }
}

void ModbusConnectionTCP::close_sokcet(int socket_fd)
{
    if(socket_fd!=0)
    {
#ifdef WIN32
        shutdown(socket_fd, SD_BOTH);
        closesocket(socket_fd);
#endif

#ifdef linux
        val = close(socket_fd);
#endif
    }
}

bool ModbusConnectionTCP::check_connection_active(modbus_t* ctx)
{
    // 发送一个小的测试请求
    uint8_t test_data;
    uint32_t to_sec =0,  to_usec = 0;
    modbus_get_response_timeout(ctx, &to_sec, &to_usec);

    // 设置较短的超时时间用于连接测试
    modbus_set_response_timeout(ctx, 2, 0);

    int rc = modbus_read_input_bits(ctx, 0, 1, &test_data);

    // 恢复原始超时设置
    modbus_set_response_timeout(ctx, to_sec, to_usec);

    if (rc == -1) {
#ifdef _WIN32
        int error = WSAGetLastError();
        return !is_windows_connection_error(error);
#else
        return !is_unix_connection_error(errno);
#endif
    }
    LOGERROR("modbus tcp test read success");
    return true;
}

#ifdef _WIN32
bool ModbusConnectionTCP::is_windows_connection_error(int error_code)
{
    LOGERROR("modbus tcp test error code  = %d", error_code);
    switch (error_code) {
    case WSAECONNRESET:    // 连接被对端重置
    case WSAETIMEDOUT:     // 连接超时
    case WSAECONNREFUSED:  // 连接被拒绝
    case WSAENETUNREACH:   // 网络不可达
    case WSAEHOSTUNREACH:  // 主机不可达
    case WSAENOTCONN:      // Socket未连接
    case WSAENETDOWN:      // 网络断开
    case WSAESHUTDOWN:     // Socket已关闭
    case ERROR_BAD_NETPATH:// 网络路径错误
    case WSAECONNABORTED:  //
    case ERROR_NETWORK_UNREACHABLE:
    {
        LOGERROR("modbus tcp test fail, return true");
    }
        return true;
    default:
    {
        LOGERROR("modbus tcp test fail, return false");
    }
        return false;
    }
}
#else
bool ModbusConnectionTCP::is_unix_connection_error(int error_code)
{
    return (error_code == ECONNRESET ||
            error_code == ETIMEDOUT ||
            error_code == ECONNREFUSED ||
            error_code == EHOSTUNREACH ||
            error_code == ENETUNREACH ||
            error_code == EPIPE ||
            error_code == EIO ||
            error_code == ENETDOWN);
}
#endif
