#pragma once

// Diagnostic output. Goes to the debugger (OutputDebugString) and to stderr,
// so it shows in Visual Studio's Output pane on the PC, the Device Portal's
// debug output on an Xbox, and the console of a Debug build.
namespace rf::log {

enum class Level { Info, Warning, Error };

// printf-style; each line is bounded at 512 characters.
void Write(Level level, const char* fmt, ...);

} // namespace rf::log
