#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <type_traits>
#include <stdexcept>
#include <utility>
#include <limits>

namespace wf {

    template<typename T>
    struct Span {
        const T* data = nullptr;
        size_t   size = 0;

        const T& operator[](size_t i) const { return data[i]; }
        bool empty() const noexcept { return size == 0; }
    };

    class Variant {
    public:
        enum class Kind : uint8_t {
            Null,

            Bool,
            Int32,
            Int64,
            UInt32,
            UInt64,
            Float,
            Double,

            String, 

            BoolArray,
            Int16Array,
            Int32Array,
            Int64Array,
            UInt16Array,
            UInt32Array,
            UInt64Array,
            DoubleArray,
            StringArray,
        };

    public:
        Variant() noexcept;
        ~Variant();

        Variant(const Variant&);
        Variant(Variant&&) noexcept;
        Variant& operator=(Variant);
        Variant& operator=(Variant&&) noexcept;

        // scalar
        Variant(bool);
        Variant(int32_t);
        Variant(int64_t);
        Variant(uint32_t);
        Variant(uint64_t);
        Variant(float);
        Variant(double);

        // aggregate
        Variant(const std::string&);
        Variant(std::string&&);
        Variant(const std::vector<int32_t>&);
        Variant(const std::vector<double>&);
        Variant(const std::vector<std::string>&);

        Kind kind() const noexcept { return m_kind; }
        bool is_null() const noexcept { return m_kind == Kind::Null; }
        bool is_numeric() const noexcept;

        // strict access
        bool     as_bool()   const;
        int16_t  as_i16()    const;
        int32_t  as_i32()    const;
        int64_t  as_i64()    const;
        uint32_t as_u32()    const;
        uint64_t as_u64()    const;
        float    as_float()  const;
        double   as_double() const;

        const std::string& as_string() const;

        const std::vector<int32_t>& as_i32_array() const;
        const std::vector<double>& as_double_array() const;
        const std::vector<std::string>& as_string_array() const;

        // numeric promotion (non-mutating)
        bool to_bool(bool& out) const noexcept;
        bool to_int16(int16_t& out) const noexcept;
        bool to_uint16(uint16_t& out) const noexcept;
        bool to_int32(int32_t& out) const noexcept;
        bool to_uint32(uint32_t& out) const noexcept;
        bool to_int64(int64_t& out) const noexcept;
        bool to_uint64(uint64_t& out) const noexcept;
        bool to_float(float& out) const noexcept;
        bool to_double(double& out) const noexcept;


        // Array numeric promotion (non-mutating)
        bool to_bool_array(std::vector<bool>& out) const noexcept;
        bool to_int16_array(std::vector<int16_t>& out) const noexcept;
        bool to_uint16_array(std::vector<uint16_t>& out) const noexcept;
        bool to_int32_array(std::vector<int32_t>& out) const noexcept;
        bool to_uint32_array(std::vector<uint32_t>& out) const noexcept;
        bool to_int64_array(std::vector<int64_t>& out) const noexcept;
        bool to_uint64_array(std::vector<uint64_t>& out) const noexcept;
        bool to_double_array(std::vector<double>& out) const noexcept;

        // zero-copy numeric array view
        bool view_double_array(Span<double>& out) const noexcept;
        bool view_i32_array(Span<int32_t>& out) const noexcept;

    private:
        static constexpr size_t SBO_SIZE = 32;
        static constexpr size_t SBO_ALIGN = alignof(std::max_align_t);
        using SBO = std::aligned_storage_t<SBO_SIZE, SBO_ALIGN>;

        union Storage {
            bool     b;
            int32_t  i32;
            int64_t  i64;
            uint32_t u32;
            uint64_t u64;
            float    f;
            double   d;
            SBO      sbo;
            void* heap;
        };

        struct Ops {
            void (*destroy)(Variant&);
            void (*copy)(Variant&, const Variant&);
        };

        // ---------- internal helpers ----------
        template<typename T>
        T* sbo_ptr() { return reinterpret_cast<T*>(&m_storage.sbo); }

        template<typename T>
        const T* sbo_ptr() const { return reinterpret_cast<const T*>(&m_storage.sbo); }

        void destroy();
        void copy_from(const Variant&);

        static const Ops& ops_for(Kind);

        // destroy
        static void destroy_null(Variant&);
        static void destroy_string(Variant&);
        static void destroy_i32_array(Variant&);
        static void destroy_double_array(Variant&);
        static void destroy_string_array(Variant&);

        // copy
        static void copy_pod(Variant&, const Variant&);
        static void copy_string(Variant&, const Variant&);
        static void copy_i32_array(Variant&, const Variant&);
        static void copy_double_array(Variant&, const Variant&);
        static void copy_string_array(Variant&, const Variant&);

    private:
        Kind    m_kind;
        bool    m_is_heap;
        Storage m_storage;
    };

} // namespace wf
