```mermaid
classDiagram
    direction LR
    
    class IModbusConnection {
        +connect() int
        +disconnect() void
        +isConnected() bool
        +read() int
        +write() int
        -reconnect() int
        #m_ctx* 
        #m_connected bool
        #m_conn modbus_conn
    }
    
    class ModbusConnectionRTU {
        +connect() int
        +read() int
        +write() int
        -m_mutex std::mutex
    }
    
    class ModbusConnectionTCP {
        +connect() int
    }
    
    class ModbusConnectionFactory {
        +create() unique_ptr~IModbusConnection~
    }
    
    class ModbusSchedulerClient {
        +connect() bool
        +disconnect() void
        +addRead() int
        +optimizeReadGroups() int
        +write() void
        +setDataCallback() void
        +startScheduler() void
        +stopScheduler() void
        -m_connection shared_ptr~IModbusConnection~
        -m_param_manager shared_ptr~ParameterManager~
        -m_data_handler shared_ptr~DataHandler~
        -m_read_scheduler shared_ptr~ReadScheduler~
        -m_write_scheduler shared_ptr~WriteScheduler~
    }
    
    class ParameterManager {
        +addReadParameter() void
        +removeReadParameter() void
        +optimizeReadGroups() int
        +readGroups() vector~modbus_group_request~
        +queueWriteRequest() void
        +getNextWriteRequest() bool
        -m_read_groups vector~modbus_group_request~
        -m_read_parameters set~modbus_parameter_request~
        -m_write_queue queue~modbus_parameter_request~
    }
    
    class ReadScheduler {
        +start() void
        +stop() void
        +setReadInterval() void
        -readLoop() void
        -m_connection shared_ptr~IModbusConnection~
        -m_param_manager shared_ptr~ParameterManager~
        -m_data_handler shared_ptr~DataHandler~
    }
    
    class WriteScheduler {
        +start() void
        +stop() void
        -writeLoop() void
        -m_connection shared_ptr~IModbusConnection~
        -m_param_manager shared_ptr~ParameterManager~
        -m_data_handler shared_ptr~DataHandler~
    }
    
    class DataHandler {
        +setResponseCallback() void
        +setErrorCallback() void
        +processResponse() void
        +handleError() void
        -m_response_callback ResponseCallback
        -m_error_callback ErrorCallback
    }
    
    class modbus_parameter_request {
        +mallocData() uint8_t*
        +freeData() void
        +getParent() modbus_request&
    }
    
    class modbus_group_request {
        +addParameter() int
        +spliteToParameters() vector~modbus_parameter_request~
        +mallocData() uint8_t*
    }
    
    IModbusConnection <|-- ModbusConnectionRTU
    IModbusConnection <|-- ModbusConnectionTCP
    ModbusConnectionFactory ..> IModbusConnection : 创建
    
    ModbusSchedulerClient --> IModbusConnection : 使用
    ModbusSchedulerClient --> ParameterManager : 管理
    ModbusSchedulerClient --> DataHandler : 处理数据
    ModbusSchedulerClient --> ReadScheduler : 控制
    ModbusSchedulerClient --> WriteScheduler : 控制
    
    ReadScheduler --> IModbusConnection : 读写
    ReadScheduler --> ParameterManager : 获取请求
    ReadScheduler --> DataHandler : 传递响应
    
    WriteScheduler --> IModbusConnection : 写操作
    WriteScheduler --> ParameterManager : 获取队列
    WriteScheduler --> DataHandler : 错误处理
    
    ParameterManager --> modbus_group_request : 包含
    ParameterManager --> modbus_parameter_request : 包含
