#pragma once

#include <hex.hpp>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#ifdef __MINGW32__
#include <winsock.h>

#else
#include <arpa/inet.h>
#endif

#include "lang/token.hpp"

#ifdef __APPLE__
    #define off64_t off_t
    #define fopen64 fopen
    #define fseeko64 fseek
    #define ftello64 ftell
#endif

#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION <= 12000
#if __has_include(<concepts>)
// Make sure we break when derived_from is implemented in libc++. Then we can fix a compatibility version above
#include <concepts>
#endif
// libcxx 12 still doesn't have derived_from implemented, as a result we need to define it ourself using clang built-ins.
// [concept.derived] (patch from https://reviews.llvm.org/D74292)
namespace hex {
template<class _Dp, class _Bp>
    concept derived_from =
      __is_base_of(_Bp, _Dp) && __is_convertible_to(const volatile _Dp*, const volatile _Bp*);
}
#else
// Assume supported
#include <concepts>
namespace hex {
    using std::derived_from;
}
#endif

namespace hex {

    template<typename ... Args>
    inline std::string format(const char *format, Args ... args) {
        ssize_t size = snprintf( nullptr, 0, format, args ... );

        if (size <= 0)
            return "";

        std::vector<char> buffer(size + 1, 0x00);
        snprintf(buffer.data(), size + 1, format, args ...);

        return std::string(buffer.data(), buffer.data() + size);
    }

    [[nodiscard]] constexpr inline u64 extract(u8 from, u8 to, const u64 &value) {
        u64 mask = (std::numeric_limits<u64>::max() >> (63 - (from - to))) << to;
        return (value & mask) >> to;
    }

    [[nodiscard]] constexpr inline u64 signExtend(u64 value, u8 currWidth, u8 targetWidth) {
        u64 mask = 1LLU << (currWidth - 1);
        return (((value ^ mask) - mask) << (64 - targetWidth)) >> (64 - targetWidth);
    }

    [[nodiscard]] constexpr inline bool isUnsigned(const lang::Token::TypeToken::Type type) {
        return (static_cast<u32>(type) & 0x0F) == 0x00;
    }

    [[nodiscard]] constexpr inline bool isSigned(const lang::Token::TypeToken::Type type) {
        return (static_cast<u32>(type) & 0x0F) == 0x01;
    }

    [[nodiscard]] constexpr inline bool isFloatingPoint(const lang::Token::TypeToken::Type type) {
        return (static_cast<u32>(type) & 0x0F) == 0x02;
    }

    [[nodiscard]] constexpr inline u32 getTypeSize(const lang::Token::TypeToken::Type type) {
        return static_cast<u32>(type) >> 4;
    }

    std::string toByteString(u64 bytes);
    std::string makePrintable(char c);

    template<typename T>
    struct always_false : std::false_type {};

    template<typename T>
    constexpr T changeEndianess(T value, std::endian endian) {
        if (endian == std::endian::native)
            return value;

        if constexpr (sizeof(T) == 1)
            return value;
        else if constexpr (sizeof(T) == 2)
            return __builtin_bswap16(value);
        else if constexpr (sizeof(T) == 4)
            return __builtin_bswap32(value);
        else if constexpr (sizeof(T) == 8)
            return __builtin_bswap64(value);
        else
            static_assert(always_false<T>::value, "Invalid type provided!");
    }

    template<typename T>
    constexpr T changeEndianess(T value, size_t size, std::endian endian) {
        if (endian == std::endian::native)
            return value;

        if (size == 1)
            return value;
        else if (size == 2)
            return __builtin_bswap16(value);
        else if (size == 4)
            return __builtin_bswap32(value);
        else if (size == 8)
            return __builtin_bswap64(value);
        else
            throw std::invalid_argument("Invalid value size!");
    }

    template< class T >
    constexpr T bit_width(T x) noexcept {
        return std::numeric_limits<T>::digits - std::countl_zero(x);
    }

    template<typename T>
    constexpr T bit_ceil(T x) noexcept {
        if (x <= 1u)
            return T(1);

        return T(1) << bit_width(T(x - 1));
    }

    std::vector<u8> readFile(std::string_view path);

    class ScopeExit {
    public:
        ScopeExit(std::function<void()> func) : m_func(func) {}
        ~ScopeExit() { if (this->m_func != nullptr) this->m_func(); }

        void release() {
            this->m_func = nullptr;
        }

    private:
        std::function<void()> m_func;
    };

    struct Region {
        u64 address;
        size_t size;
    };

    struct Bookmark {
        Region region;

        std::vector<char> name;
        std::vector<char> comment;
    };
}