#include "rf/Log.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>

#include <windows.h>

namespace rf::log {

void Write(Level level, const char* fmt, ...)
{
    assert(fmt != nullptr);
    const char* tag = (level == Level::Error) ? "ERROR" : (level == Level::Warning) ? "WARNING" : "INFO";

    char line[512] = {0};
    const int head = std::snprintf(line, sizeof(line), "%s: ", tag);
    assert(head > 0 && head < static_cast<int>(sizeof(line)));

    va_list args;
    va_start(args, fmt);
    (void)std::vsnprintf(line + head, sizeof(line) - static_cast<size_t>(head) - 1, fmt, args);   // truncates silently
    va_end(args);
    line[sizeof(line) - 1] = '\0';

    std::fputs(line, stderr);
    std::fputc('\n', stderr);
    OutputDebugStringA(line);
    OutputDebugStringA("\n");
}

} // namespace rf::log
