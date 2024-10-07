#pragma once
#include "ntdll.h"
#include <stdint.h>
#include <assert.h>


typedef uint64_t hash_t;

#ifdef __cplusplus

namespace importer {
  namespace detail {
    const uint64_t fnv_prime = 1099511628211ULL;
    const uint64_t fnv_offset_basis = 14695981039346656037ULL;

    constexpr char to_lower(char ch)
    {
      return (ch >= 'A' && ch <= 'Z' ? (ch | (1 << 5)) : ch);
    }
  }

  struct hash_obj {
    using value_type = hash_t;
    constexpr static value_type         offset = importer::detail::fnv_offset_basis;
    constexpr static value_type         prime = importer::detail::fnv_prime;

    template<class CharT>
    inline constexpr static value_type single(value_type value, CharT c) noexcept
    {
      return static_cast<hash_obj::value_type>(
        (value ^ importer::detail::to_lower((char)c)) *
        static_cast<value_type>(prime));
    }
  };

  template<class CharT = char>
  inline constexpr hash_obj::value_type
    khash(const CharT* str, hash_obj::value_type value = hash_obj::offset) noexcept
  {
    return (*str ? khash(str + 1, hash_obj::single(value, *str)) : value);
  }

  template<class CharT = char>
  inline hash_obj::value_type hash(const CharT* str) noexcept
  {
    hash_obj::value_type value = hash_obj::offset;

    for (;;) {
      char c = *str++;
      if (!c)
        return value;
      value = hash_obj::single(value, c);
    }
  }

  inline hash_obj::value_type hash(
    const UNICODE_STRING& str) noexcept
  {
    auto       first = str.Buffer;
    const auto last = first + (str.Length / sizeof(wchar_t));
    auto       value = hash_obj::offset;
    for (; first != last; ++first)
      value = hash_obj::single(value, static_cast<char>(*first));

    return value;
  }
}


#endif // __cplusplus


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

hash_t string_hash(const char* string);
hash_t wstring_hash(const wchar_t* string);
hash_t unicode_string_hash(const UNICODE_STRING* string);

void* ldr_find_module(hash_t hash);
void* ldr_find_function(void* dllbase, hash_t hash);


#ifdef __cplusplus
}
#endif // __cplusplus


#ifdef __cplusplus
#define quick_import_function(module_name, function) \
using function##_t = decltype(function); \
constexpr  hash_t module_hash_##module_name_##function = importer::khash(module_name);\
constexpr  hash_t function_hash_##module_name_##function = importer::khash(#function);\
function##_t* function = (function##_t*)ldr_find_function( ldr_find_module(module_hash_##module_name_##function), function_hash_##module_name_##function); \
assert(function != NULL);
#else
#define quick_import_function(module_hash, function_hash) \
using function##_t = decltype(function); \
function##_t* function = (function##_t*)ldr_find_function(ldr_find_module(module_hash), function_hash); \
assert(function != NULL);
#endif
