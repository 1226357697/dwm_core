#pragma once
#include "ntdll.h"
#include <string_view>

#define TRACE(fmt) TRACE_INFO(fmt)
#define TRACE_INFO(fmt) dbg_printf_tag("[dwm_core] ", "function:%s " ##fmt ##"\n", __FUNCTION__)
#define TRACE_ERR(fmt) dbg_printf_tag("[dwm_core] ", "file:%s, function:%s line: %d, "##fmt ##"\n", __FUNCTION__,  __FILE__, __LINE__)

#define TRACE_V(fmt, ...) TRACE_INFO_V(fmt, __VA_ARGS__)
#define TRACE_INFO_V(fmt, ...) dbg_printf_tag("[dwm_core] ", "function:%s" ##fmt ##"\n", __FUNCTION__, __VA_ARGS__)
#define TRACE_ERR_V(fmt, ...) dbg_printf_tag("[dwm_core] ", "file: %s,  line: %d, function: %s"##fmt ##"\n",  __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)



template<typename ...ARGT>
int dbg_printf(const std::string_view& fmt, ARGT... args)
{
  char bufer[0x1000] = {'\0'};
  std::vsnprintf(bufer, sizeof(bufer), fmt.data(), args...);
  OutputDebugStringA(bufer);
  return 0;
}

template<typename ...ARGT>
int dbg_printf_tag(const std::string_view& tag, const std::string_view& fmt, ARGT... args)
{
  char buffer[0x1000] = { '\0' };
  size_t copy_count = MIN(sizeof(buffer), tag.size());
  strncpy(buffer, tag.data(), copy_count);
  if constexpr(sizeof...(args) == 0)
  {
    strcat(buffer, fmt.data());
  }
  else
  {
    std::snprintf(buffer + tag.size(), sizeof(buffer) - copy_count, fmt.data(), args...);
  }
  OutputDebugStringA(buffer);
  return 0;
}
