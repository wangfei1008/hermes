#include "service.h"
#include "message/message_bus.h"
#include "log/log.h"
#include "database/sqlite_repository.h"
#include "device/device_builder.h"
#include "data_models/data_hub.h"

void Service::init()
{
	//1、启动消息总线
	MessageBus::instance().start();

	//2、启动数据总线
	DataHub::instance().create();

	//3、启动设备代理
	if (!setup_devices_proxy()){
		release();
	}
}

void Service::release()
{
	//1、卸载设备代理
	teardown_devices_proxy();

	//2、卸载数据总线

	//3、停止消息总线
	MessageBus::instance().stop();
}

bool Service::setup_devices_proxy()
{
	if (!SQLiteRepository::open("db/netsys_daq_hub.db")){
		LOGERROR("Failed to open database: %s", SQLiteRepository::err_message().c_str());
		return false;
	}

	// 1. 从数据库加载设备配置
	SQLiteRepository::init();
	auto devices = SQLiteRepository::query_all_device();
	LOGINFO("Loaded %zu devices from database", devices.size());

	// 2. 构建所有设备代理
	DeviceBuilder builder;
	for (auto& dev : devices) {
		auto proxy = builder.build(dev);
		if (proxy) {
			m_proxies.push_back(proxy);
			MessageBus::instance().subscribe(MESSAGE_PUBLICE_WRITE, proxy.get());//只有 Proxy 订阅总线
			LOGINFO("Device [%s] built successfully", dev.name.c_str());
		}
		else {
			LOGERROR("Failed to build device [%s]", dev.name.c_str());
		}
	}

	// 3. 启动所有设备
	for (auto& proxy : m_proxies) {
		proxy->start();
	}
	LOGINFO("All devices started");
	return true;
}

void Service::teardown_devices_proxy()
{
	for (auto& proxy : m_proxies)
	{
		// 1. 停止代理
		proxy->stop();

		// 2. 取消订阅总线
		MessageBus::instance().unsubscribe(MESSAGE_PUBLICE_WRITE, proxy.get());
	}
	m_proxies.clear();
}
