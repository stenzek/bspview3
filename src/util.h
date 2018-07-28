#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

namespace Util {

// Creates a C++ string from a C format string.
std::string StringFromFormat(const char* fmt, ...);
std::string StringFromFormatV(const char* fmt, std::va_list ap);

// Splits a string into components.
std::vector<std::string> SplitString(const char* str, char sep, bool include_empty_tokens = false);

// Removes extensions from a filename.
std::string RemoveFilenameExtensions(const std::string& filename, bool only_one = false);

// Removes any invalid characters from a filename.
std::string SanitizeFilename(const std::string& filename, bool allow_spaces = false);

// Canonicalizes a path, i.e. replaces .. and . with their absolute locations.
std::string CanonicalizePath(const char* path, bool ospath = true);

// Opens a file, returning a unique_ptr which automatically closes the handle.
std::unique_ptr<std::FILE, void (*)(FILE*)> FOpenUniquePtr(const char* filename, const char* mode);

} // namespace Util