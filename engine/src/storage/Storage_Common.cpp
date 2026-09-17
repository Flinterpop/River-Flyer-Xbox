#include "rf/Storage.h"

#include <cassert>
#include <cstdio>
#include <cstring>

#include "StorageFolder.h"
#include "rf/Log.h"

namespace rf::storage {

namespace {

constexpr int kPathMax = 512;

// Bounded "<folder><key>"; keys are plain ASCII file names.
void PathFor(const char* key, char* out, int cap)
{
    assert(key != nullptr && out != nullptr && cap > 0);
    const int len = std::snprintf(out, static_cast<size_t>(cap), "%s%s", detail::PlatformFolder(), key);
    assert(len > 0 && len < cap);
    (void)len;
}

std::FILE* Open(const char* path, const char* mode)
{
    std::FILE* f = nullptr;
    if (fopen_s(&f, path, mode) != 0) { return nullptr; }
    return f;
}

} // namespace

const char* Folder()
{
    return detail::PlatformFolder();
}

bool Exists(const char* key)
{
    assert(key != nullptr && key[0] != '\0');
    char path[kPathMax] = {0};
    PathFor(key, path, kPathMax);
    std::FILE* f = Open(path, "rb");
    if (f == nullptr) { return false; }
    (void)std::fclose(f);
    return true;
}

int Read(const char* key, char* buf, int cap)
{
    assert(key != nullptr && key[0] != '\0');
    assert(buf != nullptr && cap > 1);
    buf[0] = '\0';

    char path[kPathMax] = {0};
    PathFor(key, path, kPathMax);
    std::FILE* f = Open(path, "rb");
    if (f == nullptr) { return 0; }

    const size_t got = std::fread(buf, 1, static_cast<size_t>(cap - 1), f);   // anything longer is truncated
    (void)std::fclose(f);
    const int n = static_cast<int>(got);
    buf[n] = '\0';
    assert(n >= 0 && n < cap);
    return n;
}

bool Write(const char* key, const char* text)
{
    assert(key != nullptr && key[0] != '\0');
    assert(text != nullptr);

    char path[kPathMax] = {0};
    PathFor(key, path, kPathMax);
    std::FILE* f = Open(path, "wb");
    if (f == nullptr) {
        log::Write(log::Level::Warning, "STORAGE: could not open %s for writing", path);
        return false;
    }
    const size_t len = std::strlen(text);
    const bool   ok  = std::fwrite(text, 1, len, f) == len;
    (void)std::fclose(f);
    if (!ok) { log::Write(log::Level::Warning, "STORAGE: short write to %s", path); }
    return ok;
}

} // namespace rf::storage
