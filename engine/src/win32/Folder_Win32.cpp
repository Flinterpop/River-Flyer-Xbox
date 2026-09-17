#include "../storage/StorageFolder.h"

#include <cassert>
#include <cstdio>

#include <windows.h>

#include "rf/Log.h"

namespace rf::storage::detail {

// "%LOCALAPPDATA%\RiverFlyer\", created on first use.
const char* PlatformFolder()
{
    static char folder[512] = {0};
    if (folder[0] != '\0') { return folder; }

    char base[512] = {0};
    const DWORD n = GetEnvironmentVariableA("LOCALAPPDATA", base, sizeof(base));
    if (n == 0 || n >= sizeof(base)) {
        log::Write(log::Level::Warning, "STORAGE: LOCALAPPDATA not set; using the working directory");
        (void)std::snprintf(folder, sizeof(folder), ".\\");
        return folder;
    }
    const int len = std::snprintf(folder, sizeof(folder), "%s\\RiverFlyer\\", base);
    assert(len > 0 && len < static_cast<int>(sizeof(folder)));
    (void)len;
    if (!CreateDirectoryA(folder, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
        log::Write(log::Level::Warning, "STORAGE: could not create %s", folder);
    }
    return folder;
}

} // namespace rf::storage::detail
