#include "component_pipeline.h"
#include "library/plugin_loader.h"
#include "log/log.h"

ComponentPipeline::ComponentPipeline(const std::string& device_name, int pipeline_id, const std::string& name)
    : m_device_name(device_name)
    , m_id(pipeline_id)
    , m_name(name)
    , m_started(false)
{
    LOGINFO("[ComponentPipeline][%s][%d] stream = %s created", m_device_name.c_str(), m_id, m_name.c_str());
}

ComponentPipeline::~ComponentPipeline()
{
    if (m_started) {
        stop();
    }
    cleanup_components();
    LOGINFO("[ComponentPipeline][%s][%d] stream = %s destroyed", m_device_name.c_str(), m_id, m_name.c_str());
}

void ComponentPipeline::add_component(IComponent* comp, const std::string& lib_name)
{
    if (m_started) {
        LOGERROR("[ComponentPipeline][%s][%d] Cannot add component after start()", m_device_name.c_str(), m_id);
        return;
    }

    if (!comp) {
        LOGERROR("[ComponentPipeline][%s][%d] Null component pointer", m_device_name.c_str(), m_id);
        return;
    }

    m_components.push_back({comp, lib_name});
    LOGINFO("[ComponentPipeline][%s][%d] Component from %s added (total: %d)", m_device_name.c_str(), m_id, lib_name.c_str(), m_components.size());
}

bool ComponentPipeline::start()
{
    if (m_started) {
        LOGWARN("[ComponentPipeline][%s][%d] Already started", m_device_name.c_str(), m_id);
        return false;
    }

    for (auto& info : m_components) {
        try {
            info.component->start();
            LOGINFO("[ComponentPipeline][%s][%d] Component %s started", m_device_name.c_str(), m_id, info.lib_name.c_str());
        } catch (const std::exception& e) {
            LOGERROR("[ComponentPipeline][%s][%d] Failed to start component %s: %s", m_device_name.c_str(), m_id, info.lib_name.c_str(), e.what());
            return false;
        } catch (...) {
            LOGERROR("[ComponentPipeline][%s][%d] Failed to start component %s (unknown error)", m_device_name.c_str(), m_id, info.lib_name.c_str());
            return false;
        }
    }

    m_started = true;
    LOGINFO("[ComponentPipeline][%s][%d] stream = %s started with %zu components", m_device_name.c_str(), m_id, m_name.c_str(), m_components.size());
    return true;
}

void ComponentPipeline::pause()
{
    if (!m_started) return;

    for (auto& info : m_components) {
        try {
            info.component->pause();
        } catch (const std::exception& e) {
            LOGERROR("[ComponentPipeline][%s][%d] Failed to pause component %s: %s", m_device_name.c_str(), m_id, info.lib_name.c_str(), e.what());
        }
    }
    LOGINFO("[ComponentPipeline][%s][%d] paused", m_device_name.c_str(), m_id);
}

void ComponentPipeline::resume()
{
    if (!m_started) return;

    for (auto& info : m_components) {
        try {
            info.component->resume();
        } catch (const std::exception& e) {
            LOGERROR("[ComponentPipeline][%s][%d] Failed to resume component %s: %s", m_device_name.c_str(), m_id, info.lib_name.c_str(), e.what());
        }
    }
    LOGINFO("[ComponentPipeline][%s][%d] resumed", m_device_name.c_str(), m_id);
}

void ComponentPipeline::stop()
{
    if (!m_started) return;

    for (auto& info : m_components) {
        try {
            info.component->stop();
            LOGINFO("[ComponentPipeline][%s][%d] ComponentPipeline[%d]: Component %s stopped", m_device_name.c_str(), m_id, info.lib_name.c_str());
        } catch (const std::exception& e) {
            LOGERROR("[ComponentPipeline][%s][%d] Failed to stop component %s: %s", m_device_name.c_str(),  m_id, info.lib_name.c_str(), e.what());
        }
    }

    m_started = false;
    LOGINFO("[ComponentPipeline][%s][%d] stopped", m_device_name.c_str(), m_id);
}

bool ComponentPipeline::execute(DataContext::Ptr& data)
{
    if (!m_started || !data) {
        return false;
    }

    // Pipeline 模式：串行处理
    for (auto& info : m_components)
    {
        try {
            bool continue_pipeline = info.component->process(data);
            
            if (!continue_pipeline) {
                LOGDEBUG("[ComponentPipeline][%s][%d] Component %s stopped pipeline", m_device_name.c_str(), m_id, info.lib_name.c_str());
                return false;
            }
        } catch (const std::exception& e) {
            LOGERROR("[ComponentPipeline][%s][%d] Component %s process failed: %s", m_device_name.c_str(), m_id, info.lib_name.c_str(), e.what());
            return false;
        } catch (...) {
            LOGERROR("[ComponentPipeline][%s][%d] Component %s process failed (unknown error)", m_device_name.c_str(), m_id, info.lib_name.c_str());
            return false;
        }
    }

    return true; // 所有组件处理成功
}

void ComponentPipeline::dispatch_message(int type, const std::string& msg)
{
    for (auto& info : m_components)
    {
        try {
            info.component->on_message(type, msg);
        } catch (const std::exception& e) {
            LOGERROR("[ComponentPipeline][%s][%d] Component %s on_message failed: %s", m_device_name.c_str(), m_id, info.lib_name.c_str(), e.what());
        } catch (...) {
            LOGERROR("[ComponentPipeline][%s][%d] Component %s on_message failed (unknown)", m_device_name.c_str(), m_id, info.lib_name.c_str());
        }
    }
}

size_t ComponentPipeline::component_count() const
{
    return m_components.size();
}

void ComponentPipeline::cleanup_components()
{
    for (auto& info : m_components)
    {
        if (info.component)
        {
            try {
                PluginLoader::instance().release(info.lib_name, info.component);
                LOGINFO("[ComponentPipeline][%s][%d] Component %s released", m_device_name.c_str(),  m_id, info.lib_name.c_str());
            } catch (const std::exception& e) {
                LOGERROR("[ComponentPipeline][%s][%d] Failed to release component %s: %s", m_device_name.c_str(), m_id, info.lib_name.c_str(), e.what());
            }
            info.component = nullptr;
        }
    }
    m_components.clear();
}
