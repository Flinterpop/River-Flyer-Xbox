#pragma once

// The one platform-specific piece of rf::storage: where the writable folder
// is. Storage_Common.cpp does the file I/O against it.
namespace rf::storage::detail {

// Absolute path with a trailing separator, created if needed. Must be stable
// for the life of the process (a static buffer).
const char* PlatformFolder();

} // namespace rf::storage::detail
