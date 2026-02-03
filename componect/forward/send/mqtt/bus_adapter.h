#ifndef BUS_ADAPTER_H
#define BUS_ADAPTER_H
#include <string>
#include "i_component.h"

class BusAdapter {
public:
    BusAdapter() = default;
    ~BusAdapter() = default;
    
    bool initialize(IDataHub* hub, int stream_id);
    void subscribe(const std::string& topic, IComponent* component);
    void publish(const std::string& topic, const std::string& message);
    void publish_to_stream(DataContext::Ptr data);
    
    // 将 DataContext 转换为总线消息格式
    static std::string serialize_data(DataContext::Ptr data);
    static DataContext::Ptr deserialize_data(const std::string& msg);
    
private:
    IDataHub* m_hub{nullptr};
    int m_stream_id{0};
};
#endif // BUS_ADAPTER_H