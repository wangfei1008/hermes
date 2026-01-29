#ifndef DATA_CONTEXT_H
#define DATA_CONTEXT_H
#include "data/variant_ex.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>


// 定义数据域，增强算法承载的逻辑清晰度
enum class DataDomain 
{
    TIME_SERIES,    // 时域序列（采样数据）
    FREQUENCY,      // 频域（分析结果）
    FEATURE,        // 特征值（最大、最小、均值）
    ALARM           // 逻辑判定（报警位）
};

class DataContext
{
public:
    using Ptr = std::shared_ptr<DataContext>;

    // 1. 帧头信息
    struct Header 
    {
        std::string source_device;
        int64_t timestamp_ms;
        uint64_t frame_index; // 帧序号，算法对齐必用
    } header;

    // 2. 核心：非对称变量桶
    // 算法逻辑定制的关键：不再预定义字段，全部动态化
    struct Item 
    {
        wf::Variant data;      // 承载数据（单值或数组）
        DataDomain domain;     // 逻辑域
        std::string lineage;   // 溯源：记录是哪个算法产出的（如 "LowPassFilter_0.5Hz"）
        int64_t proc_cost_us;  // 算法耗时统计，用于性能调优
    };

    // 使用 unordered_map 提高算法检索效率
    std::unordered_map<std::string, Item> items;

    // 3. 逻辑定制支撑：计算黑匣子 (Opaque Params)
    // 允许算法在流中传递私有对象（如：滤波器的状态机指针、当前增益倍数）
    std::unordered_map<std::string, wf::Variant> algo_params;

    // --- 算法操作接口 ---

    // 动态增加算法结果
    void push_result(const std::string& name, wf::Variant&& val, DataDomain dom, const std::string& algo_name)
    {
        items[name] = { std::move(val), dom, algo_name, 0 };
    }

    // 逻辑判定：算法可以根据包内已有的 Lineage 决定是否执行
    bool was_processed_by(const std::string& algo_name) const 
    {
        for (auto& [k, v] : items) 
            if (v.lineage == algo_name) return true;
        return false;
    }
};

#endif
