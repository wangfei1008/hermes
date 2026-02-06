#include "variant.h"
#include <new>

namespace wf {

    // ---------- lifecycle ----------
    Variant::Variant() noexcept : m_kind(Kind::Null), m_is_heap(false) {}
    Variant::~Variant() { destroy(); }

    Variant::Variant(const Variant& o) : m_kind(o.m_kind), m_is_heap(false) {
        copy_from(o);
    }

    Variant::Variant(Variant&& o) noexcept : m_kind(o.m_kind), m_is_heap(o.m_is_heap), m_storage(o.m_storage) {
        // 移动后置空源
        o.m_kind = Kind::Null;
        o.m_is_heap = false;
    }

    Variant& Variant::operator=(Variant o) {
        std::swap(m_kind, o.m_kind);
        std::swap(m_is_heap, o.m_is_heap);
        std::swap(m_storage, o.m_storage);
        return *this;
    }

    Variant& Variant::operator=(Variant&& o) noexcept {
        destroy();
        m_kind = o.m_kind;
        m_is_heap = o.m_is_heap;
        m_storage = o.m_storage;
        o.m_kind = Kind::Null;
        o.m_is_heap = false;
        return *this;
    }

    void Variant::destroy() {
        ops_for(m_kind).destroy(*this);
        m_kind = Kind::Null;
        m_is_heap = false;
    }

    void Variant::copy_from(const Variant& o) {
        ops_for(o.m_kind).copy(*this, o);
    }

    // ---------- destroy ----------
    void Variant::destroy_null(Variant&) {}

    void Variant::destroy_string(Variant& v) {
        if (v.m_is_heap) delete static_cast<std::string*>(v.m_storage.heap);
        else v.sbo_ptr<std::string>()->~basic_string();
    }

    void Variant::destroy_i32_array(Variant& v) {
        if (v.m_is_heap) delete static_cast<std::vector<int32_t>*>(v.m_storage.heap);
        else v.sbo_ptr<std::vector<int32_t>>()->~vector();
    }

    void Variant::destroy_double_array(Variant& v) {
        if (v.m_is_heap) delete static_cast<std::vector<double>*>(v.m_storage.heap);
        else v.sbo_ptr<std::vector<double>>()->~vector();
    }

    void Variant::destroy_string_array(Variant& v) {
        if (v.m_is_heap) delete static_cast<std::vector<std::string>*>(v.m_storage.heap);
        else v.sbo_ptr<std::vector<std::string>>()->~vector();
    }

    // ---------- copy ----------
    void Variant::copy_pod(Variant& d, const Variant& s) {
        d.m_storage = s.m_storage;
        d.m_is_heap = false;
    }

    void Variant::copy_string(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::string(*static_cast<std::string*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::string(*s.sbo_ptr<std::string>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_i32_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<int32_t>(*static_cast<std::vector<int32_t>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<int32_t>(*s.sbo_ptr<std::vector<int32_t>>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_double_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<double>(*static_cast<std::vector<double>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<double>(*s.sbo_ptr<std::vector<double>>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_string_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<std::string>(*static_cast<std::vector<std::string>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<std::string>(*s.sbo_ptr<std::vector<std::string>>());
            d.m_is_heap = false;
        }
    }

    // ---------- ops ----------
    const Variant::Ops& Variant::ops_for(Kind k) {
        // 顺序必须与 Kind 枚举保持一致
        static const Ops table[] = {
            { destroy_null,         copy_pod        }, // Null
            { destroy_null,         copy_pod        }, // Bool
            { destroy_null,         copy_pod        }, // Int32
            { destroy_null,         copy_pod        }, // Int64
            { destroy_null,         copy_pod        }, // UInt32
            { destroy_null,         copy_pod        }, // UInt64
            { destroy_null,         copy_pod        }, // Float
            { destroy_null,         copy_pod        }, // Double
            { destroy_string,       copy_string     }, // String
            { destroy_null,         copy_pod        }, // BoolArray (当前未专门使用)
            { destroy_null,         copy_pod        }, // Int16Array (当前未专门使用)
            { destroy_i32_array,    copy_i32_array  }, // Int32Array
            { destroy_null,         copy_pod        }, // Int64Array (当前未专门使用)
            { destroy_null,         copy_pod        }, // UInt16Array (当前未专门使用)
            { destroy_null,         copy_pod        }, // UInt32Array (当前未专门使用)
            { destroy_null,         copy_pod        }, // UInt64Array (当前未专门使用)
            { destroy_double_array, copy_double_array }, // DoubleArray
            { destroy_string_array, copy_string_array }, // StringArray
        };
        return table[static_cast<size_t>(k)];
    }

    // ---------- ctors ----------
    Variant::Variant(bool v) : m_kind(Kind::Bool), m_is_heap(false) { m_storage.b = v; }
    Variant::Variant(int32_t v) : m_kind(Kind::Int32), m_is_heap(false) { m_storage.i32 = v; }
    Variant::Variant(int64_t v) : m_kind(Kind::Int64), m_is_heap(false) { m_storage.i64 = v; }
    Variant::Variant(uint32_t v) : m_kind(Kind::UInt32), m_is_heap(false) { m_storage.u32 = v; }
    Variant::Variant(uint64_t v) : m_kind(Kind::UInt64), m_is_heap(false) { m_storage.u64 = v; }
    Variant::Variant(float v) : m_kind(Kind::Float), m_is_heap(false) { m_storage.f = v; }
    Variant::Variant(double v) : m_kind(Kind::Double), m_is_heap(false) { m_storage.d = v; }

    Variant::Variant(const std::string& s) : m_kind(Kind::String) {
        if (s.size() + 1 > SBO_SIZE) {
            m_storage.heap = new std::string(s);
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::string(s);
            m_is_heap = false;
        }
    }

    Variant::Variant(std::string&& s) : m_kind(Kind::String) {
        if (s.size() + 1 > SBO_SIZE) {
            m_storage.heap = new std::string(std::move(s));
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::string(std::move(s));
            m_is_heap = false;
        }
    }

    Variant::Variant(const std::vector<int32_t>& v) : m_kind(Kind::Int32Array) {
        new (&m_storage.sbo) std::vector<int32_t>(v);
        m_is_heap = false;
    }

    Variant::Variant(const std::vector<double>& v) : m_kind(Kind::DoubleArray) {
        new (&m_storage.sbo) std::vector<double>(v);
        m_is_heap = false;
    }

    Variant::Variant(const std::vector<std::string>& v) : m_kind(Kind::StringArray) {
        new (&m_storage.sbo) std::vector<std::string>(v);
        m_is_heap = false;
    }

    // ---------- strict access ----------
#define WF_REQ(k) if (m_kind != (k)) throw std::logic_error("bad Variant access")

    bool     Variant::as_bool()   const { WF_REQ(Kind::Bool);  return m_storage.b; }

    int16_t  Variant::as_i16()    const {
        int16_t out;
        if (!to_int16(out)) throw std::logic_error("bad Variant access");
        return out;
    }

    int32_t  Variant::as_i32()    const { WF_REQ(Kind::Int32);  return m_storage.i32; }
    int64_t  Variant::as_i64()    const { WF_REQ(Kind::Int64);  return m_storage.i64; }
    uint32_t Variant::as_u32()    const { WF_REQ(Kind::UInt32); return m_storage.u32; }
    uint64_t Variant::as_u64()    const { WF_REQ(Kind::UInt64); return m_storage.u64; }
    float    Variant::as_float()  const { WF_REQ(Kind::Float);  return m_storage.f; }
    double   Variant::as_double() const { WF_REQ(Kind::Double); return m_storage.d; }

    const std::string& Variant::as_string() const {
        WF_REQ(Kind::String);
        if (m_is_heap) return *static_cast<const std::string*>(m_storage.heap);
        return *sbo_ptr<std::string>();
    }

    const std::vector<int32_t>& Variant::as_i32_array() const {
        WF_REQ(Kind::Int32Array);
        if (m_is_heap) return *static_cast<const std::vector<int32_t>*>(m_storage.heap);
        return *sbo_ptr<std::vector<int32_t>>();
    }

    const std::vector<double>& Variant::as_double_array() const {
        WF_REQ(Kind::DoubleArray);
        if (m_is_heap) return *static_cast<const std::vector<double>*>(m_storage.heap);
        return *sbo_ptr<std::vector<double>>();
    }

    const std::vector<std::string>& Variant::as_string_array() const {
        WF_REQ(Kind::StringArray);
        if (m_is_heap) return *static_cast<const std::vector<std::string>*>(m_storage.heap);
        return *sbo_ptr<std::vector<std::string>>();
    }

#undef WF_REQ

    // ---------- numeric ----------
    bool Variant::is_numeric() const noexcept {
        return m_kind >= Kind::Bool && m_kind <= Kind::Double;
    }

    bool Variant::to_bool(bool& out) const noexcept {
        switch (m_kind) {
        case Kind::Bool:   out = m_storage.b; return true;
        case Kind::Int32:  out = (m_storage.i32 != 0); return true;
        case Kind::Int64:  out = (m_storage.i64 != 0); return true;
        case Kind::UInt32: out = (m_storage.u32 != 0); return true;
        case Kind::UInt64: out = (m_storage.u64 != 0); return true;
        case Kind::Float:  out = (m_storage.f != 0.0f); return true;
        case Kind::Double: out = (m_storage.d != 0.0);  return true;
        default: return false;
        }
    }

    bool Variant::to_int16(int16_t& out) const noexcept {
        int64_t tmp;
        if (!to_int64(tmp)) return false;
        if (tmp < std::numeric_limits<int16_t>::min() ||
            tmp > std::numeric_limits<int16_t>::max()) return false;
        out = static_cast<int16_t>(tmp);
        return true;
    }

    bool Variant::to_uint16(uint16_t& out) const noexcept {
        uint64_t tmp;
        if (!to_uint64(tmp)) return false;
        if (tmp > std::numeric_limits<uint16_t>::max()) return false;
        out = static_cast<uint16_t>(tmp);
        return true;
    }

    bool Variant::to_int32(int32_t& out) const noexcept {
        if (m_kind == Kind::Int32) { out = m_storage.i32; return true; }
        int64_t tmp;
        if (!to_int64(tmp)) return false;
        if (tmp < std::numeric_limits<int32_t>::min() ||
            tmp > std::numeric_limits<int32_t>::max()) return false;
        out = static_cast<int32_t>(tmp);
        return true;
    }

    bool Variant::to_uint32(uint32_t& out) const noexcept {
        if (m_kind == Kind::UInt32) { out = m_storage.u32; return true; }
        uint64_t tmp;
        if (!to_uint64(tmp)) return false;
        if (tmp > std::numeric_limits<uint32_t>::max()) return false;
        out = static_cast<uint32_t>(tmp);
        return true;
    }

    bool Variant::to_int64(int64_t& o) const noexcept {
        switch (m_kind) {
        case Kind::Bool:   o = m_storage.b;  return true;
        case Kind::Int32:  o = m_storage.i32; return true;
        case Kind::Int64:  o = m_storage.i64; return true;
        case Kind::UInt32: o = m_storage.u32; return true;
        case Kind::UInt64:
            if (m_storage.u64 > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
                return false;
            o = static_cast<int64_t>(m_storage.u64); return true;
        case Kind::Float:  o = static_cast<int64_t>(m_storage.f); return true;
        case Kind::Double: o = static_cast<int64_t>(m_storage.d); return true;
        default: return false;
        }
    }

    bool Variant::to_uint64(uint64_t& o) const noexcept {
        switch (m_kind) {
        case Kind::Bool:   o = m_storage.b ? 1u : 0u; return true;
        case Kind::Int32:
            if (m_storage.i32 < 0) return false;
            o = static_cast<uint64_t>(m_storage.i32); return true;
        case Kind::Int64:
            if (m_storage.i64 < 0) return false;
            o = static_cast<uint64_t>(m_storage.i64); return true;
        case Kind::UInt32: o = m_storage.u32; return true;
        case Kind::UInt64: o = m_storage.u64; return true;
        case Kind::Float:
            if (m_storage.f < 0.0f) return false;
            o = static_cast<uint64_t>(m_storage.f); return true;
        case Kind::Double:
            if (m_storage.d < 0.0) return false;
            o = static_cast<uint64_t>(m_storage.d); return true;
        default: return false;
        }
    }

    bool Variant::to_float(float& out) const noexcept {
        double d;
        if (!to_double(d)) return false;
        out = static_cast<float>(d);
        return true;
    }

    bool Variant::to_double(double& o) const noexcept {
        switch (m_kind) {
        case Kind::Bool:   o = m_storage.b ? 1.0 : 0.0; return true;
        case Kind::Int32:  o = static_cast<double>(m_storage.i32); return true;
        case Kind::Int64:  o = static_cast<double>(m_storage.i64); return true;
        case Kind::UInt32: o = static_cast<double>(m_storage.u32); return true;
        case Kind::UInt64: o = static_cast<double>(m_storage.u64); return true;
        case Kind::Float:  o = static_cast<double>(m_storage.f);  return true;
        case Kind::Double: o = m_storage.d; return true;
        default: return false;
        }
    }

    // ---------- array numeric promotion ----------

    bool Variant::to_bool_array(std::vector<bool>& out) const noexcept {
        out.clear();
        if (m_kind == Kind::Int32Array) {
            const auto& in = as_i32_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(v != 0);
            return true;
        }
        if (m_kind == Kind::DoubleArray) {
            const auto& in = as_double_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(v != 0.0);
            return true;
        }
        return false;
    }

    bool Variant::to_int16_array(std::vector<int16_t>& out) const noexcept {
        out.clear();
        if (m_kind == Kind::Int32Array) {
            const auto& in = as_i32_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(static_cast<int16_t>(v));
            return true;
        }
        if (m_kind == Kind::DoubleArray) {
            const auto& in = as_double_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(static_cast<int16_t>(v));
            return true;
        }
        return false;
    }

    bool Variant::to_uint16_array(std::vector<uint16_t>& out) const noexcept {
        out.clear();
        if (m_kind == Kind::Int32Array) {
            const auto& in = as_i32_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(static_cast<uint16_t>(v));
            return true;
        }
        if (m_kind == Kind::DoubleArray) {
            const auto& in = as_double_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(static_cast<uint16_t>(v));
            return true;
        }
        return false;
    }

    bool Variant::to_int32_array(std::vector<int32_t>& out) const noexcept {
        out.clear();
        if (m_kind == Kind::Int32Array) {
            out = as_i32_array();
            return true;
        }
        if (m_kind == Kind::DoubleArray) {
            const auto& in = as_double_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(static_cast<int32_t>(v));
            return true;
        }
        return false;
    }

    bool Variant::to_uint32_array(std::vector<uint32_t>& out) const noexcept {
        out.clear();
        if (m_kind == Kind::Int32Array) {
            const auto& in = as_i32_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(static_cast<uint32_t>(v));
            return true;
        }
        if (m_kind == Kind::DoubleArray) {
            const auto& in = as_double_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(static_cast<uint32_t>(v));
            return true;
        }
        return false;
    }

    bool Variant::to_int64_array(std::vector<int64_t>& out) const noexcept {
        out.clear();
        if (m_kind == Kind::Int32Array) {
            const auto& in = as_i32_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(static_cast<int64_t>(v));
            return true;
        }
        if (m_kind == Kind::DoubleArray) {
            const auto& in = as_double_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(static_cast<int64_t>(v));
            return true;
        }
        return false;
    }

    bool Variant::to_uint64_array(std::vector<uint64_t>& out) const noexcept {
        out.clear();
        if (m_kind == Kind::Int32Array) {
            const auto& in = as_i32_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(static_cast<uint64_t>(v));
            return true;
        }
        if (m_kind == Kind::DoubleArray) {
            const auto& in = as_double_array();
            out.reserve(in.size());
            for (auto v : in) out.push_back(static_cast<uint64_t>(v));
            return true;
        }
        return false;
    }

    bool Variant::to_double_array(std::vector<double>& out) const noexcept {
        out.clear();
        if (m_kind == Kind::DoubleArray) {
            out = as_double_array();
            return true;
        }
        if (m_kind == Kind::Int32Array) {
            const auto& in = as_i32_array();
            out.reserve(in.size());
            for (int32_t v : in) out.push_back(static_cast<double>(v));
            return true;
        }
        return false;
    }

    // ---------- zero-copy views ----------
    bool Variant::view_double_array(Span<double>& out) const noexcept {
        if (m_kind != Kind::DoubleArray) return false;
        const auto& v = as_double_array();
        out.data = v.data();
        out.size = v.size();
        return true;
    }

    bool Variant::view_i32_array(Span<int32_t>& out) const noexcept {
        if (m_kind != Kind::Int32Array) return false;
        const auto& v = as_i32_array();
        out.data = v.data();
        out.size = v.size();
        return true;
    }

} // namespace wf
