#include "service.h"
#include "message/message_bus.h"
#include "log/log.h"
#include "database/sqlite_repository.h"
#include "device/device_builder.h"

void Service::init()
{
	//1、启动消息总线
	MessageBus::instance().start();

	//2、启动数据总线
	
	//3、启动设备代理
	setup_devices_proxy();
}

void Service::release()
{
	//1、卸载设备代理
	teardown_devices_proxy();

	//2、卸载数据总线

	//3、停止消息总线
	MessageBus::instance().stop();
}

void Service::setup_devices_proxy()
{
	if (!SQLiteRepository::open("db/netsys_daq_hub.db"))
	{
		LOGERROR("Failed to open database: %s", SQLiteRepository::err_message().c_str());
		return;
	}
	SQLiteRepository::init_tables();

	auto devices = SQLiteRepository::query_all_device();
	DeviceBuilder builder;
	for (auto& dev : devices)
	{
		// 1. 创建代理
		auto proxy = builder.build(dev);

		// 2. 只有 Proxy 订阅总线
		MessageBus::instance().subscribe(MESSAGE_PUBLICE_WRITE, proxy.get());

		// 3. 启动设备
		proxy->start();

		m_proxies.push_back(proxy);
	}
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
