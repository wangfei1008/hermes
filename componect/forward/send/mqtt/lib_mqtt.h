#pragma once
#include "i_component.h"

class LibMqtt: public IComponent
{
public:
    bool init(const DeviceContext& ctx, IDataHub* hub, const std::string& config) override;

    void start() override;
    void pause() override;
    void resume() override;
    void stop() override;

    void on_message(int type, const std::string& msg) override;

    bool process(DataContext::Ptr& pkg) override;
};
