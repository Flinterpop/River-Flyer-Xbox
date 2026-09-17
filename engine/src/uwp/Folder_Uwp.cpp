#include "../storage/StorageFolder.h"

#include <cassert>
#include <cstdio>
#include <string>

#include <winrt/base.h>
#include <winrt/Windows.Storage.h>

namespace rf::storage::detail {

// The app's private writable folder. The install folder is read-only on a console.
const char* PlatformFolder()
{
    static char folder[512] = {0};
    if (folder[0] != '\0') { return folder; }
    const std::string path = winrt::to_string(winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path());
    const int len = std::snprintf(folder, sizeof(folder), "%s\\", path.c_str());
    assert(len > 0 && len < static_cast<int>(sizeof(folder)));
    (void)len;
    return folder;
}

} // namespace rf::storage::detail
