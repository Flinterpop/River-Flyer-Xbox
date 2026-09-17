#pragma once

// Small text records that survive between runs (the high-score table, the
// pilot names). Each backend maps a key to a file in the app's writable
// folder: %LOCALAPPDATA%\RiverFlyer\ on the PC, ApplicationData.LocalFolder
// under UWP, XGameSave under the GDK. The exe's own folder is read-only on a
// console, which is why this exists.
namespace rf::storage {

int         Read(const char* key, char* buf, int cap);   // bytes stored (NUL-terminated, truncated at cap-1); 0 if absent
bool        Write(const char* key, const char* text);
bool        Exists(const char* key);
const char* Folder();                                    // the writable folder, with a trailing separator

} // namespace rf::storage
