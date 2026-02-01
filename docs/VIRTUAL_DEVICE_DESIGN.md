# 虚拟设备设计：转发与存储

## 设计原则

**复用现有架构**：转发/存储通过"虚拟设备"实现，与采集设备共用 DeviceProxy → ExecutionStream → Pipeline 模型。

## 架构图

```
┌─────────────────────────────────────────────────────────────────┐
│  采集设备（物理设备）                                             │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐        │
│  │ 采集组件      │ → │ 处理组件      │ → │ 结果组件      │        │
│  │ publish(sid) │   │ process()    │   │ publish("0") │        │
│  └──────────────┘   └──────────────┘   └──────────────┘        │
└─────────────────────────────────────────────────────────────────┘
                                               │
                                               ↓ topic = "0"
                                          ┌─────────┐
                                          │ DataHub │
                                          └─────────┘
                                               │
                                               ↓ subscribe("0")
┌─────────────────────────────────────────────────────────────────┐
│  虚拟设备（逻辑设备）                                             │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐        │
│  │ FilterComp   │ → │ StorageComp  │ → │ ForwardComp  │        │
│  │ (过滤)       │   │ (存储)        │   │ (MQTT/HTTP) │        │
│  └──────────────┘   └──────────────┘   └──────────────┘        │
└─────────────────────────────────────────────────────────────────┘
```

## 关键改动

### 1. ExecutionStream 支持配置订阅 topic

```cpp
// execution_stream.h
ExecutionStream(int stream_id, const std::string& name, const std::string& subscribe_topic = "");

// 默认订阅 stream_id，虚拟设备可配置为 "0" 订阅结果数据
```

### 2. StreamDTO 增加 subscribe_topic 字段

```cpp
// db_models.h
struct StreamDTO
{
    int id;
    int device_id;
    int is_active;
    std::string stream_name;
    std::string subscribe_topic;  // 空 → 默认 stream_id；"0" → 订阅结果数据
};
```

### 3. 数据库表 device_streams 增加字段

```sql
ALTER TABLE device_streams ADD COLUMN subscribe_topic TEXT DEFAULT '';
```

## 配置示例

### 采集设备配置

| 表 | 字段 | 值 |
|----|------|-----|
| device | id=1, name="传感器A", protocol="modbus" |
| device_streams | id=1, device_id=1, stream_name="采集流", subscribe_topic="" |
| stream_components | stream_id=1, order_index=0, lib_name="modbus_collector" |
| stream_components | stream_id=1, order_index=1, lib_name="data_processor" |
| stream_components | stream_id=1, order_index=2, lib_name="result_publisher" |

### 虚拟设备配置

| 表 | 字段 | 值 |
|----|------|-----|
| device | id=100, name="数据转发器", protocol="internal" |
| device_streams | id=100, device_id=100, stream_name="转发流", subscribe_topic="0" |
| stream_components | stream_id=100, order_index=0, lib_name="filter_component" |
| stream_components | stream_id=100, order_index=1, lib_name="storage_component" |
| stream_components | stream_id=100, order_index=2, lib_name="mqtt_forward_component" |
| stream_components | stream_id=100, order_index=3, lib_name="http_forward_component" |

## 组件接口

所有组件实现 `IComponent` 接口：

```cpp
class IComponent 
{
public:
    virtual bool init(const DeviceContext& ctx, IDataHub* hub, const std::string& config) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool process(DataContext::Ptr& pkg) = 0;
    // ...
};
```

### 转发/存储组件示例

#### FilterComponent

```cpp
bool FilterComponent::process(DataContext::Ptr& pkg) {
    // 按设备/域/字段过滤
    if (should_filter(pkg)) {
        return false;  // 中断 Pipeline
    }
    return true;  // 继续
}
```

#### StorageComponent

```cpp
bool StorageComponent::process(DataContext::Ptr& pkg) {
    // 存储到文件/数据库
    m_storage->save(pkg);
    return true;  // 继续
}
```

#### MqttForwardComponent

```cpp
bool MqttForwardComponent::process(DataContext::Ptr& pkg) {
    // 序列化并发送到 MQTT broker
    std::string json = serialize(pkg);
    m_mqtt_client->publish(m_topic, json);
    return true;  // 继续
}
```

#### HttpForwardComponent

```cpp
bool HttpForwardComponent::process(DataContext::Ptr& pkg) {
    // 序列化并 POST 到 HTTP endpoint
    std::string json = serialize(pkg);
    m_http_client->post(m_url, json);
    return true;  // 继续
}
```

## 优势

1. **架构一致**：采集设备和虚拟设备用同一套模型，运维统一
2. **可配置**：组件链在数据库配置，不需要改代码
3. **可扩展**：想加聚合/压缩/加密...加对应组件即可
4. **复用能力**：生命周期、日志、异常处理...全部复用

## 数据流总结

1. 采集组件 → `publish(stream_id)` → DataHub → 本流 Pipeline
2. 结果组件 → `publish("0")` → DataHub
3. 虚拟设备流 → `subscribe("0")` → 收到结果数据 → 过滤/存储/转发

## topic 约定

| topic | 含义 |
|-------|------|
| `"1"`, `"2"`, ... | 流 ID，流内数据，只有对应流订阅 |
| `"0"` | 结果数据，供虚拟设备（转发/存储）订阅 |
