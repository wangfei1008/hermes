#include "variant.h"
#include <cstring>
#include <limits>

namespace wf {

    // ---------- lifecycle ----------

    Variant::Variant() noexcept : m_kind(Kind::Null), m_is_heap(false) {
        m_storage.heap = nullptr;
    }

    Variant::~Variant() {
        destroy();
    }

    Variant::Variant(const Variant& o) : m_kind(o.m_kind), m_is_heap(false) {
        copy_from(o);
    }

    Variant::Variant(Variant&& o) noexcept
        : m_kind(o.m_kind), m_is_heap(o.m_is_heap), m_storage(o.m_storage) {
        o.m_kind = Kind::Null;
        o.m_is_heap = false;
        o.m_storage.heap = nullptr;
    }

    Variant& Variant::operator=(Variant o) {
        std::swap(m_kind, o.m_kind);
        std::swap(m_is_heap, o.m_is_heap);
        std::swap(m_storage, o.m_storage);
        return *this;
    }

    //Variant& Variant::operator=(Variant&& o) noexcept {
    //    if (this != &o) {
    //        destroy();
    //        m_kind = o.m_kind;
    //        m_is_heap = o.m_is_heap;
    //        m_storage = o.m_storage;
    //        o.m_kind = Kind::Null;
    //        o.m_is_heap = false;
    //        o.m_storage.heap = nullptr;
    //    }
    //    return *this;
    //}

    void Variant::destroy() {
        ops_for(m_kind).destroy(*this);
        m_kind = Kind::Null;
        m_is_heap = false;
    }

    void Variant::copy_from(const Variant& o) {
        ops_for(o.m_kind).copy(*this, o);
    }

    // ---------- destroy operations ----------

    void Variant::destroy_null(Variant&) {}

    void Variant::destroy_string(Variant& v) {
        if (v.m_is_heap) {
            delete static_cast<std::string*>(v.m_storage.heap);
        }
        else {
            v.sbo_ptr<std::string>()->~basic_string();
        }
    }

    void Variant::destroy_bool_array(Variant& v) {
        if (v.m_is_heap) {
            delete static_cast<std::vector<bool>*>(v.m_storage.heap);
        }
        else {
            v.sbo_ptr<std::vector<bool>>()->~vector();
        }
    }

    void Variant::destroy_i16_array(Variant& v) {
        if (v.m_is_heap) {
            delete static_cast<std::vector<int16_t>*>(v.m_storage.heap);
        }
        else {
            v.sbo_ptr<std::vector<int16_t>>()->~vector();
        }
    }

    void Variant::destroy_i32_array(Variant& v) {
        if (v.m_is_heap) {
            delete static_cast<std::vector<int32_t>*>(v.m_storage.heap);
        }
        else {
            v.sbo_ptr<std::vector<int32_t>>()->~vector();
        }
    }

    void Variant::destroy_i64_array(Variant& v) {
        if (v.m_is_heap) {
            delete static_cast<std::vector<int64_t>*>(v.m_storage.heap);
        }
        else {
            v.sbo_ptr<std::vector<int64_t>>()->~vector();
        }
    }

    void Variant::destroy_u16_array(Variant& v) {
        if (v.m_is_heap) {
            delete static_cast<std::vector<uint16_t>*>(v.m_storage.heap);
        }
        else {
            v.sbo_ptr<std::vector<uint16_t>>()->~vector();
        }
    }

    void Variant::destroy_u32_array(Variant& v) {
        if (v.m_is_heap) {
            delete static_cast<std::vector<uint32_t>*>(v.m_storage.heap);
        }
        else {
            v.sbo_ptr<std::vector<uint32_t>>()->~vector();
        }
    }

    void Variant::destroy_u64_array(Variant& v) {
        if (v.m_is_heap) {
            delete static_cast<std::vector<uint64_t>*>(v.m_storage.heap);
        }
        else {
            v.sbo_ptr<std::vector<uint64_t>>()->~vector();
        }
    }

    void Variant::destroy_double_array(Variant& v) {
        if (v.m_is_heap) {
            delete static_cast<std::vector<double>*>(v.m_storage.heap);
        }
        else {
            v.sbo_ptr<std::vector<double>>()->~vector();
        }
    }

    void Variant::destroy_string_array(Variant& v) {
        if (v.m_is_heap) {
            delete static_cast<std::vector<std::string>*>(v.m_storage.heap);
        }
        else {
            v.sbo_ptr<std::vector<std::string>>()->~vector();
        }
    }

    // ---------- copy operations ----------

    void Variant::copy_pod(Variant& d, const Variant& s) {
        d.m_storage = s.m_storage;
        d.m_is_heap = false;
    }

    void Variant::copy_string(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::string(*static_cast<const std::string*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::string(*s.sbo_ptr<std::string>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_bool_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<bool>(*static_cast<const std::vector<bool>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<bool>(*s.sbo_ptr<std::vector<bool>>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_i16_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<int16_t>(*static_cast<const std::vector<int16_t>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<int16_t>(*s.sbo_ptr<std::vector<int16_t>>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_i32_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<int32_t>(*static_cast<const std::vector<int32_t>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<int32_t>(*s.sbo_ptr<std::vector<int32_t>>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_i64_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<int64_t>(*static_cast<const std::vector<int64_t>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<int64_t>(*s.sbo_ptr<std::vector<int64_t>>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_u16_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<uint16_t>(*static_cast<const std::vector<uint16_t>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<uint16_t>(*s.sbo_ptr<std::vector<uint16_t>>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_u32_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<uint32_t>(*static_cast<const std::vector<uint32_t>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<uint32_t>(*s.sbo_ptr<std::vector<uint32_t>>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_u64_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<uint64_t>(*static_cast<const std::vector<uint64_t>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<uint64_t>(*s.sbo_ptr<std::vector<uint64_t>>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_double_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<double>(*static_cast<const std::vector<double>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<double>(*s.sbo_ptr<std::vector<double>>());
            d.m_is_heap = false;
        }
    }

    void Variant::copy_string_array(Variant& d, const Variant& s) {
        if (s.m_is_heap) {
            d.m_storage.heap = new std::vector<std::string>(*static_cast<const std::vector<std::string>*>(s.m_storage.heap));
            d.m_is_heap = true;
        }
        else {
            new (&d.m_storage.sbo) std::vector<std::string>(*s.sbo_ptr<std::vector<std::string>>());
            d.m_is_heap = false;
        }
    }

    // ---------- ops table ----------

    const Variant::Ops& Variant::ops_for(Kind k) {
        static const Ops table[] = {
            { destroy_null,         copy_pod },           // Null
            { destroy_null,         copy_pod },           // Bool
            { destroy_null,         copy_pod },           // Int32
            { destroy_null,         copy_pod },           // Int64
            { destroy_null,         copy_pod },           // UInt32
            { destroy_null,         copy_pod },           // UInt64
            { destroy_null,         copy_pod },           // Float
            { destroy_null,         copy_pod },           // Double
            { destroy_string,       copy_string },        // String
            { destroy_bool_array,   copy_bool_array },    // BoolArray
            { destroy_i16_array,    copy_i16_array },     // Int16Array
            { destroy_i32_array,    copy_i32_array },     // Int32Array
            { destroy_i64_array,    copy_i64_array },     // Int64Array
            { destroy_u16_array,    copy_u16_array },     // UInt16Array
            { destroy_u32_array,    copy_u32_array },     // UInt32Array
            { destroy_u64_array,    copy_u64_array },     // UInt64Array
            { destroy_double_array, copy_double_array },  // DoubleArray
            { destroy_string_array, copy_string_array },  // StringArray
        };
        return table[static_cast<size_t>(k)];
    }

    // ---------- scalar constructors ----------

    Variant::Variant(bool v) : m_kind(Kind::Bool), m_is_heap(false) {
        m_storage.b = v;
    }

    Variant::Variant(int32_t v) : m_kind(Kind::Int32), m_is_heap(false) {
        m_storage.i32 = v;
    }

    Variant::Variant(int64_t v) : m_kind(Kind::Int64), m_is_heap(false) {
        m_storage.i64 = v;
    }

    Variant::Variant(uint32_t v) : m_kind(Kind::UInt32), m_is_heap(false) {
        m_storage.u32 = v;
    }

    Variant::Variant(uint64_t v) : m_kind(Kind::UInt64), m_is_heap(false) {
        m_storage.u64 = v;
    }

    Variant::Variant(float v) : m_kind(Kind::Float), m_is_heap(false) {
        m_storage.f = v;
    }

    Variant::Variant(double v) : m_kind(Kind::Double), m_is_heap(false) {
        m_storage.d = v;
    }

    // ---------- string constructors ----------

    Variant::Variant(const std::string& s) : m_kind(Kind::String) {
        if (sizeof(std::string) > SBO_SIZE || !fits_in_sbo<std::string>()) {
            m_storage.heap = new std::string(s);
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::string(s);
            m_is_heap = false;
        }
    }

    Variant::Variant(std::string&& s) : m_kind(Kind::String) {
        if (sizeof(std::string) > SBO_SIZE || !fits_in_sbo<std::string>()) {
            m_storage.heap = new std::string(std::move(s));
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::string(std::move(s));
            m_is_heap = false;
        }
    }

    Variant::Variant(const char* s) : Variant(std::string(s)) {}

    // ---------- array constructors ----------

    Variant::Variant(const std::vector<bool>& v) : m_kind(Kind::BoolArray) {
        if (sizeof(std::vector<bool>) > SBO_SIZE || !fits_in_sbo<std::vector<bool>>()) {
            m_storage.heap = new std::vector<bool>(v);
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::vector<bool>(v);
            m_is_heap = false;
        }
    }

    Variant::Variant(const std::vector<int16_t>& v) : m_kind(Kind::Int16Array) {
        if (sizeof(std::vector<int16_t>) > SBO_SIZE || !fits_in_sbo<std::vector<int16_t>>()) {
            m_storage.heap = new std::vector<int16_t>(v);
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::vector<int16_t>(v);
            m_is_heap = false;
        }
    }

    Variant::Variant(const std::vector<int32_t>& v) : m_kind(Kind::Int32Array) {
        if (sizeof(std::vector<int32_t>) > SBO_SIZE || !fits_in_sbo<std::vector<int32_t>>()) {
            m_storage.heap = new std::vector<int32_t>(v);
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::vector<int32_t>(v);
            m_is_heap = false;
        }
    }

    Variant::Variant(const std::vector<int64_t>& v) : m_kind(Kind::Int64Array) {
        if (sizeof(std::vector<int64_t>) > SBO_SIZE || !fits_in_sbo<std::vector<int64_t>>()) {
            m_storage.heap = new std::vector<int64_t>(v);
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::vector<int64_t>(v);
            m_is_heap = false;
        }
    }

    Variant::Variant(const std::vector<uint16_t>& v) : m_kind(Kind::UInt16Array) {
        if (sizeof(std::vector<uint16_t>) > SBO_SIZE || !fits_in_sbo<std::vector<uint16_t>>()) {
            m_storage.heap = new std::vector<uint16_t>(v);
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::vector<uint16_t>(v);
            m_is_heap = false;
        }
    }

    Variant::Variant(const std::vector<uint32_t>& v) : m_kind(Kind::UInt32Array) {
        if (sizeof(std::vector<uint32_t>) > SBO_SIZE || !fits_in_sbo<std::vector<uint32_t>>()) {
            m_storage.heap = new std::vector<uint32_t>(v);
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::vector<uint32_t>(v);
            m_is_heap = false;
        }
    }

    Variant::Variant(const std::vector<uint64_t>& v) : m_kind(Kind::UInt64Array) {
        if (sizeof(std::vector<uint64_t>) > SBO_SIZE || !fits_in_sbo<std::vector<uint64_t>>()) {
            m_storage.heap = new std::vector<uint64_t>(v);
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::vector<uint64_t>(v);
            m_is_heap = false;
        }
    }

    Variant::Variant(const std::vector<double>& v) : m_kind(Kind::DoubleArray) {
        if (sizeof(std::vector<double>) > SBO_SIZE || !fits_in_sbo<std::vector<double>>()) {
            m_storage.heap = new std::vector<double>(v);
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::vector<double>(v);
            m_is_heap = false;
        }
    }

    Variant::Variant(const std::vector<std::string>& v) : m_kind(Kind::StringArray) {
        if (sizeof(std::vector<std::string>) > SBO_SIZE || !fits_in_sbo<std::vector<std::string>>()) {
            m_storage.heap = new std::vector<std::string>(v);
            m_is_heap = true;
        }
        else {
            new (&m_storage.sbo) std::vector<std::string>(v);
            m_is_heap = false;
        }
    }

    // ---------- type checks ----------

    bool Variant::is_numeric() const noexcept {
        return m_kind >= Kind::Bool && m_kind <= Kind::Double;
    }

    bool Variant::is_array() const noexcept {
        return m_kind >= Kind::BoolArray && m_kind <= Kind::StringArray;
    }

    // ---------- strict access ----------

#define WF_REQ(k) \
    if (m_kind != (k)) throw std::logic_error("bad Variant access")

#define WF_IMPL_SCALAR_ACCESS(method, kind_enum, type, member) \
    type Variant::method() const { \
        WF_REQ(Kind::kind_enum); \
        return m_storage.member; \
    }

#define WF_IMPL_ARRAY_ACCESS(method, kind_enum, elem_type) \
    const std::vector<elem_type>& Variant::method() const { \
        WF_REQ(Kind::kind_enum); \
        if (m_is_heap) \
            return *static_cast<const std::vector<elem_type>*>(m_storage.heap); \
        return *sbo_ptr<std::vector<elem_type>>(); \
    }

    WF_IMPL_SCALAR_ACCESS(as_bool, Bool, bool, b)
        WF_IMPL_SCALAR_ACCESS(as_i32, Int32, int32_t, i32)
        WF_IMPL_SCALAR_ACCESS(as_i64, Int64, int64_t, i64)
        WF_IMPL_SCALAR_ACCESS(as_u32, UInt32, uint32_t, u32)
        WF_IMPL_SCALAR_ACCESS(as_u64, UInt64, uint64_t, u64)
        WF_IMPL_SCALAR_ACCESS(as_float, Float, float, f)
        WF_IMPL_SCALAR_ACCESS(as_double, Double, double, d)

        int16_t Variant::as_i16() const {
        int16_t out;
        if (!to_int16(out)) throw std::logic_error("bad Variant access");
        return out;
    }

    const std::string& Variant::as_string() const {
        WF_REQ(Kind::String);
        if (m_is_heap)
            return *static_cast<const std::string*>(m_storage.heap);
        return *sbo_ptr<std::string>();
    }

    WF_IMPL_ARRAY_ACCESS(as_bool_array, BoolArray, bool)
        WF_IMPL_ARRAY_ACCESS(as_i16_array, Int16Array, int16_t)
        WF_IMPL_ARRAY_ACCESS(as_i32_array, Int32Array, int32_t)
        WF_IMPL_ARRAY_ACCESS(as_i64_array, Int64Array, int64_t)
        WF_IMPL_ARRAY_ACCESS(as_u16_array, UInt16Array, uint16_t)
        WF_IMPL_ARRAY_ACCESS(as_u32_array, UInt32Array, uint32_t)
        WF_IMPL_ARRAY_ACCESS(as_u64_array, UInt64Array, uint64_t)
        WF_IMPL_ARRAY_ACCESS(as_double_array, DoubleArray, double)
        WF_IMPL_ARRAY_ACCESS(as_string_array, StringArray, std::string)

#undef WF_REQ
#undef WF_IMPL_SCALAR_ACCESS
#undef WF_IMPL_ARRAY_ACCESS

        // ---------- numeric promotion (scalar) ----------

        bool Variant::to_bool(bool& out) const noexcept {
        switch (m_kind) {
        case Kind::Bool:    out = m_storage.b; return true;
        case Kind::Int32:   out = (m_storage.i32 != 0); return true;
        case Kind::Int64:   out = (m_storage.i64 != 0); return true;
        case Kind::UInt32:  out = (m_storage.u32 != 0); return true;
        case Kind::UInt64:  out = (m_storage.u64 != 0); return true;
        case Kind::Float:   out = (m_storage.f != 0.0f); return true;
        case Kind::Double:  out = (m_storage.d != 0.0); return true;
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
        if (m_kind == Kind::Int32) {
            out = m_storage.i32;
            return true;
        }
        int64_t tmp;
        if (!to_int64(tmp)) return false;
        if (tmp < std::numeric_limits<int32_t>::min() ||
            tmp > std::numeric_limits<int32_t>::max()) return false;
        out = static_cast<int32_t>(tmp);
        return true;
    }

    bool Variant::to_uint32(uint32_t& out) const noexcept {
        if (m_kind == Kind::UInt32) {
            out = m_storage.u32;
            return true;
        }
        uint64_t tmp;
        if (!to_uint64(tmp)) return false;
        if (tmp > std::numeric_limits<uint32_t>::max()) return false;
        out = static_cast<uint32_t>(tmp);
        return true;
    }

    bool Variant::to_int64(int64_t& o) const noexcept {
        switch (m_kind) {
        case Kind::Bool:    o = m_storage.b; return true;
        case Kind::Int32:   o = m_storage.i32; return true;
        case Kind::Int64:   o = m_storage.i64; return true;
        case Kind::UInt32:  o = m_storage.u32; return true;
        case Kind::UInt64:
            if (m_storage.u64 > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
                return false;
            o = static_cast<int64_t>(m_storage.u64);
            return true;
        case Kind::Float:   o = static_cast<int64_t>(m_storage.f); return true;
        case Kind::Double:  o = static_cast<int64_t>(m_storage.d); return true;
        default: return false;
        }
    }

    bool Variant::to_uint64(uint64_t& o) const noexcept {
        switch (m_kind) {
        case Kind::Bool:    o = m_storage.b ? 1u : 0u; return true;
        case Kind::Int32:
            if (m_storage.i32 < 0) return false;
            o = static_cast<uint64_t>(m_storage.i32);
            return true;
        case Kind::Int64:
            if (m_storage.i64 < 0) return false;
            o = static_cast<uint64_t>(m_storage.i64);
            return true;
        case Kind::UInt32:  o = m_storage.u32; return true;
        case Kind::UInt64:  o = m_storage.u64; return true;
        case Kind::Float:
            if (m_storage.f < 0.0f) return false;
            o = static_cast<uint64_t>(m_storage.f);
            return true;
        case Kind::Double:
            if (m_storage.d < 0.0) return false;
            o = static_cast<uint64_t>(m_storage.d);
            return true;
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
        case Kind::Bool:    o = m_storage.b ? 1.0 : 0.0; return true;
        case Kind::Int32:   o = static_cast<double>(m_storage.i32); return true;
        case Kind::Int64:   o = static_cast<double>(m_storage.i64); return true;
        case Kind::UInt32:  o = static_cast<double>(m_storage.u32); return true;
        case Kind::UInt64:  o = static_cast<double>(m_storage.u64); return true;
        case Kind::Float:   o = static_cast<double>(m_storage.f); return true;
        case Kind::Double:  o = m_storage.d; return true;
        default: return false;
        }
    }

    // ---------- array numeric promotion ----------

    template <typename OutType, typename InType>
    static void convert_array(std::vector<OutType>& out, const std::vector<InType>& in) {
        out.clear();
        out.reserve(in.size());
        for (const auto& v : in) {
            out.push_back(static_cast<OutType>(v));
        }
    }

#define WF_ARRAY_CONV_IMPL(method, out_type) \
    bool Variant::method(std::vector<out_type>& out) const noexcept { \
        try { \
            switch (m_kind) { \
                case Kind::BoolArray:    convert_array(out, as_bool_array()); return true; \
                case Kind::Int16Array:   convert_array(out, as_i16_array()); return true; \
                case Kind::Int32Array:   convert_array(out, as_i32_array()); return true; \
                case Kind::Int64Array:   convert_array(out, as_i64_array()); return true; \
                case Kind::UInt16Array:  convert_array(out, as_u16_array()); return true; \
                case Kind::UInt32Array:  convert_array(out, as_u32_array()); return true; \
                case Kind::UInt64Array:  convert_array(out, as_u64_array()); return true; \
                case Kind::DoubleArray:  convert_array(out, as_double_array()); return true; \
                default: return false; \
            } \
        } catch (...) { return false; } \
    }

    WF_ARRAY_CONV_IMPL(to_bool_array, bool)
        WF_ARRAY_CONV_IMPL(to_int16_array, int16_t)
        WF_ARRAY_CONV_IMPL(to_uint16_array, uint16_t)
        WF_ARRAY_CONV_IMPL(to_int32_array, int32_t)
        WF_ARRAY_CONV_IMPL(to_uint32_array, uint32_t)
        WF_ARRAY_CONV_IMPL(to_int64_array, int64_t)
        WF_ARRAY_CONV_IMPL(to_uint64_array, uint64_t)
        WF_ARRAY_CONV_IMPL(to_double_array, double)

#undef WF_ARRAY_CONV_IMPL

        // ---------- zero-copy array views ----------

        template <typename T>
    static bool view_array_impl(const Variant& v, Variant::Kind expected, Span<T>& out) {
        if (v.kind() != expected) return false;
        try {
            const auto* vec_ptr = v.is_heap()
                ? static_cast<const std::vector<T>*>(static_cast<const void*>(
                    &(*reinterpret_cast<const std::vector<T>*>(
                        static_cast<const void*>(
                            static_cast<const char*>(nullptr) +
                            reinterpret_cast<uintptr_t>(&v) -
                            reinterpret_cast<uintptr_t>(&v))))))
                : nullptr;

            // 简化版本:直接访问
            if (v.kind() == Variant::Kind::DoubleArray) {
                const auto& vec = v.as_double_array();
                out.data = vec.data();
                out.size = vec.size();
                return true;
            }
            if (v.kind() == Variant::Kind::Int32Array) {
                const auto& vec = v.as_i32_array();
                out.data = vec.data();
                out.size = vec.size();
                return true;
            }
            return false;
        }
        catch (...) {
            return false;
        }
    }

    bool Variant::view_double_array(Span<double>& out) const noexcept {
        if (m_kind != Kind::DoubleArray) return false;
        try {
            const auto& v = as_double_array();
            out.data = v.data();
            out.size = v.size();
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool Variant::view_i32_array(Span<int32_t>& out) const noexcept {
        if (m_kind != Kind::Int32Array) return false;
        try {
            const auto& v = as_i32_array();
            out.data = v.data();
            out.size = v.size();
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool Variant::view_i64_array(Span<int64_t>& out) const noexcept {
        if (m_kind != Kind::Int64Array) return false;
        try {
            const auto& v = as_i64_array();
            out.data = v.data();
            out.size = v.size();
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool Variant::view_u32_array(Span<uint32_t>& out) const noexcept {
        if (m_kind != Kind::UInt32Array) return false;
        try {
            const auto& v = as_u32_array();
            out.data = v.data();
            out.size = v.size();
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool Variant::view_u64_array(Span<uint64_t>& out) const noexcept {
        if (m_kind != Kind::UInt64Array) return false;
        try {
            const auto& v = as_u64_array();
            out.data = v.data();
            out.size = v.size();
            return true;
        }
        catch (...) {
            return false;
        }
    }

} // namespace wf