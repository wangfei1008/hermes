// 将 DataContext 序列化为 JSON 字符串，供 MQTT 发送组件使用。
// 依赖 common/json/json_write 封装的 rapidjson。
#ifndef DATA_CONTEXT_JSON_H
#define DATA_CONTEXT_JSON_H

#include "core/data_context.h"
#include "common/data/variant.h"

#include <string>
#include <vector>

// DataDomain → 字符串（声明，定义在 data_context_json.cpp）
std::string domain_to_string(DataDomain domain);

// DataContext 的 JSON 序列化辅助类（实现位于 data_context_json.cpp）
class DataContextJsonConverter
{
public:
    struct ConvertOptions
    {
        bool include_header   = true;
        bool include_items    = true;
        bool include_params   = false;  // 算法参数通常不发
        bool compress_arrays  = true;   // 对大数组做统计压缩
        int  double_precision = 6;      // 浮点打印精度
        std::vector<std::string> item_filter; // 只包含指定 item；空表示全部
    };

    // 单帧序列化
    static std::string to_json(const DataContext& ctx,
                               const ConvertOptions& opt = {});

    // 多帧（历史数据）序列化
    static std::string to_json_array(const std::vector<DataContext::Ptr>& list,
                                     const ConvertOptions& opt = {});

private:
    // 时间工具
    static std::string ms_to_iso_string(int64_t ms);
    static std::string current_iso_time();

    // Variant 序列化（实现内部使用 json_write）
    static void write_variant(struct json_write& w,
                              const wf::Variant& v,
                              const ConvertOptions& opt);

    static void write_compressed_array(struct json_write& w,
                                       const wf::Variant& v,
                                       const ConvertOptions& opt);
};

#endif // DATA_CONTEXT_JSON_H