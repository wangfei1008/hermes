// 实现 DataContext → JSON 的序列化，依赖 common/json/json_write。

#include "data_context_json.h"
#include "json/json_write.h"

#include <algorithm>
#include <numeric>
#include <chrono>

using wf::Variant;

// ----------- 辅助函数 -----------

std::string domain_to_string(DataDomain domain)
{
    switch (domain)
    {
    case DataDomain::TIME_SERIES: return "time_series";
    case DataDomain::FREQUENCY:   return "frequency";
    case DataDomain::FEATURE:     return "feature";
    case DataDomain::ALARM:       return "alarm";
    default:                      return "unknown";
    }
}

std::string DataContextJsonConverter::to_json(const DataContext& ctx, const ConvertOptions& opt)
{
    json_write w;

    // 根对象
    w.front_object();

    // 1. header
    if (opt.include_header)
    {
        w.front_object("header");

        w.set_value_string("source_device", ctx.header.source_device);

        // json_write 只支持 int/double，这里用 double 承载 64 位数值
        w.set_value("stream_id",       static_cast<int>(ctx.header.stream_id));
        w.set_value("timestamp_ms",    static_cast<double>(ctx.header.timestamp_ms));
        w.set_value("frame_index",     static_cast<double>(ctx.header.frame_index));
        w.set_value_string("timestamp_iso", ms_to_iso_string(ctx.header.timestamp_ms));

        w.end_object();
    }

    // 2. items
    if (opt.include_items && !ctx.items.empty())
    {
        w.front_object("items");

        for (const auto& kv : ctx.items)
        {
            const auto& key  = kv.first;
            const auto& item = kv.second;

            // 过滤
            if (!opt.item_filter.empty() &&
                std::find(opt.item_filter.begin(), opt.item_filter.end(), key) == opt.item_filter.end())
            {
                continue;
            }

            w.front_object(key); // item 对象

            w.set_value_string("domain",  domain_to_string(item.domain));
            w.set_value_string("lineage", item.lineage);
            w.set_value("proc_cost_us",   static_cast<double>(item.proc_cost_us));

            // data 字段
            if (opt.compress_arrays &&
                (item.domain == DataDomain::TIME_SERIES ||
                 item.domain == DataDomain::FREQUENCY))
            {
                w.front_object("data");
                write_compressed_array(w, item.data, opt);
                w.end_object();
            }
            else
            {
                w.front_object("data");
                write_variant(w, item.data, opt);
                w.end_object();
            }

            w.end_object(); // end item
        }

        w.end_object(); // end items
    }

    // 3. algo_params（调试用）
    if (opt.include_params && !ctx.algo_params.empty())
    {
        w.front_object("algo_params");

        for (const auto& kv : ctx.algo_params)
        {
            w.front_object(kv.first);
            write_variant(w, kv.second, opt);
            w.end_object();
        }

        w.end_object();
    }

    // 4. metadata
    w.front_object("metadata");
    w.set_value_string("version", "1.0");
    w.set_value("item_count", static_cast<int>(ctx.items.size()));
    w.set_value_string("generated_at", current_iso_time());
    w.end_object();

    // 结束根对象
    w.end_object();

    return std::string(w.data(), w.data_length());
}

std::string DataContextJsonConverter::to_json_array(const std::vector<DataContext::Ptr>& list, const ConvertOptions& opt)
{
    json_write w;

    w.front_array();

    for (const auto& ctx : list)
    {
        if (!ctx) continue;

        // 每个元素是一个对象
        // 这里复用 to_json 的逻辑不方便直接嵌套 json_write，
        // 简化为：对每个 ctx 单独生成字符串再作为字符串值发送不合适，
        // 因为 json_write 没有“原样插入 JSON 文本”的接口。
        //
        // 因此这里直接展开：与 to_json 基本一致，只是去掉 metadata 段。

        w.front_object();

        // header
        if (opt.include_header)
        {
            w.front_object("header");
            w.set_value_string("source_device", ctx->header.source_device);
            w.set_value("stream_id",    static_cast<int>(ctx->header.stream_id));
            w.set_value("timestamp_ms", static_cast<double>(ctx->header.timestamp_ms));
            w.set_value("frame_index",  static_cast<double>(ctx->header.frame_index));
            w.set_value_string("timestamp_iso",
                               ms_to_iso_string(ctx->header.timestamp_ms));
            w.end_object();
        }

        // items
        if (opt.include_items && !ctx->items.empty())
        {
            w.front_object("items");

            for (const auto& kv : ctx->items)
            {
                const auto& key  = kv.first;
                const auto& item = kv.second;

                if (!opt.item_filter.empty() &&
                    std::find(opt.item_filter.begin(), opt.item_filter.end(), key) == opt.item_filter.end())
                {
                    continue;
                }

                w.front_object(key);
                w.set_value_string("domain",  domain_to_string(item.domain));
                w.set_value_string("lineage", item.lineage);
                w.set_value("proc_cost_us",   static_cast<double>(item.proc_cost_us));

                if (opt.compress_arrays &&
                    (item.domain == DataDomain::TIME_SERIES ||
                     item.domain == DataDomain::FREQUENCY))
                {
                    w.front_object("data");
                    write_compressed_array(w, item.data, opt);
                    w.end_object();
                }
                else
                {
                    w.front_object("data");
                    write_variant(w, item.data, opt);
                    w.end_object();
                }

                w.end_object(); // end item
            }

            w.end_object(); // end items
        }

        // algo_params
        if (opt.include_params && !ctx->algo_params.empty())
        {
            w.front_object("algo_params");
            for (const auto& kv : ctx->algo_params)
            {
                w.front_object(kv.first);
                write_variant(w, kv.second, opt);
                w.end_object();
            }
            w.end_object();
        }

        w.end_object(); // end element object
    }

    w.end_array();

    return std::string(w.data(), w.data_length());
}

// ---------- 私有静态工具 ----------

std::string DataContextJsonConverter::ms_to_iso_string(int64_t ms)
{
    using namespace std::chrono;
    system_clock::time_point tp{milliseconds(ms)};
    std::time_t t = system_clock::to_time_t(tp);

    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);

    auto fractional = static_cast<long long>(ms % 1000);
    char ms_buf[8];
    std::snprintf(ms_buf, sizeof(ms_buf), "%03lld", fractional);

    std::string out(buffer);
    out.push_back('.');
    out += ms_buf;
    out.push_back('Z');
    return out;
}

std::string DataContextJsonConverter::current_iso_time()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms  = duration_cast<milliseconds>(now.time_since_epoch()).count();
    return ms_to_iso_string(ms);
}

void DataContextJsonConverter::write_variant(json_write& w, const wf::Variant& v, const ConvertOptions& opt)
{
    using Kind = wf::Variant::Kind;

    switch (v.kind())
    {
    case Kind::Null:
        // rapidjson 中 null 需要独立 API，这里用字符串 "null" 表达
        w.set_value_string("value", "null");
        break;
    case Kind::Bool:
        w.set_value("value", v.as_bool());
        break;
    case Kind::Int32:
        w.set_value("value", static_cast<int>(v.as_i32()));
        break;
    case Kind::Int64:
        w.set_value("value", static_cast<double>(v.as_i64()));
        break;
    case Kind::UInt32:
        w.set_value("value", static_cast<int>(v.as_u32()));
        break;
    case Kind::UInt64:
        w.set_value("value", static_cast<double>(v.as_u64()));
        break;
    case Kind::Float:
        w.set_value("value", static_cast<double>(v.as_float()));
        break;
    case Kind::Double:
        w.set_value("value", v.as_double());
        break;
    case Kind::String:
        w.set_value_string("value", v.as_string());
        break;
    case Kind::Int32Array:
    {
        w.front_array("value");
        const auto& arr = v.as_i32_array();
        for (auto n : arr)
            w.set_value(static_cast<int>(n));
        w.end_array();
        break;
    }
    case Kind::DoubleArray:
    {
        w.front_array("value");
        const auto& arr = v.as_double_array();
        for (auto d : arr)
            w.set_value(static_cast<double>(d));
        w.end_array();
        break;
    }
    case Kind::StringArray:
    {
        w.front_array("value");
        const auto& arr = v.as_string_array();
        for (const auto& s : arr)
            w.set_value_string(s);
        w.end_array();
        break;
    }
    default:
        w.set_value_string("value", "unsupported");
        break;
    }

    (void)opt; // 当前未使用精度；如需，可扩展到 double 序列化
}

void DataContextJsonConverter::write_compressed_array(json_write& w, const wf::Variant& v, const ConvertOptions& opt)
{
    using Kind = wf::Variant::Kind;

    if (v.kind() != Kind::DoubleArray)
    {
        // 非 double 数组走普通路径
        write_variant(w, v, opt);
        return;
    }

    const auto& arr = v.as_double_array();
    if (arr.size() <= 1000)
    {
        // 小数组直接输出
        w.front_array("value");
        for (auto d : arr)
            w.set_value(static_cast<double>(d));
        w.end_array();
        return;
    }

    // 大数组输出统计信息
    auto minmax = std::minmax_element(arr.begin(), arr.end());
    double sum  = std::accumulate(arr.begin(), arr.end(), 0.0);
    double mean = sum / static_cast<double>(arr.size());

    w.set_value_string("type", "compressed_array");
    w.set_value("size", static_cast<int>(arr.size()));
    w.set_value("min",  *minmax.first);
    w.set_value("max",  *minmax.second);
    w.set_value("mean", mean);

    (void)opt;
}

