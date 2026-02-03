#pragma once
#include "device/device_proxy.h"

class Service
{
public:
	void init();
	void release();
private:
	bool setup_devices_proxy();
	void teardown_devices_proxy(); 
private:
	std::vector<std::shared_ptr<DeviceProxy>> m_proxies;
};
