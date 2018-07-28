#include "pch.h"
#include "util.h"

namespace Util {

std::string StringFromFormat(const char* fmt, ...)
{
  std::va_list ap;
  va_start(ap, fmt);
  std::string ret = StringFromFormatV(fmt, ap);
  va_end(ap);
  return ret;
}

std::string StringFromFormatV(const char* fmt, std::va_list ap)
{
  int size = std::vsnprintf(nullptr, 0, fmt, ap);
  std::string ret;
  ret.resize(size);
  std::vsnprintf(&ret[0], size + 1, fmt, ap);
  return ret;
}

std::vector<std::string> SplitString(const char* str, char sep, bool include_empty_tokens)
{
  std::vector<std::string> ret;
  std::string current;
  size_t len = std::strlen(str);
  for (size_t i = 0; i < len; i++)
  {
    if (str[i] != sep)
    {
      current.push_back(str[i]);
      continue;
    }

    if (include_empty_tokens || !current.empty())
    {
      ret.push_back(std::move(current));
      current = std::string();
    }
  }

  if (!current.empty())
    ret.push_back(std::move(current));

  return ret;
}

std::string RemoveFilenameExtensions(const std::string& filename, bool only_one /*= false*/)
{
  std::string ret = filename;
  for (;;)
  {
    std::size_t pos = ret.rfind('.');
    if (pos == std::string::npos)
      break;

    std::size_t lbound = ret.rfind('/');
    if (lbound != std::string::npos && pos < lbound)
      break;

    ret.erase(pos);
    if (only_one)
      return ret;
  }

  return ret;
}

std::string SanitizeFilename(const std::string& filename, bool allow_spaces /*= false*/)
{
  std::string ret = filename;
  for (size_t i = 0; i < ret.length(); i++)
  {
    const char ch = ret[i];
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
        (ch == '_' || ch == '-' || ch == '.') || (ch == ' ' && allow_spaces))
    {
      continue;
    }

    ret[i] = '_';
  }

  return ret;
}

std::string CanonicalizePath(const char* path, bool ospath /*= true*/)
{
  // get length
  size_t path_length = std::strlen(path);
  std::string destination = path;
  if (path_length == 0)
    return destination;

  // iterate path
  size_t destination_length = 0;
  for (size_t i = 0; i < path_length;)
  {
    char previous_char = (i > 0) ? path[i - 1] : '\0';
    char current_char = path[i];
    char next_char = (i < path_length) ? path[i + 1] : '\0';

    if (current_char == '.')
    {
      if (previous_char == '\\' || previous_char == '/' || previous_char == '\0')
      {
        // handle '.'
        if (next_char == '\\' || next_char == '/' || next_char == '\0')
        {
          // skip '.\'
          i++;

          // remove the previous \, if we have one trailing the dot it'll append it anyway
          if (destination_length > 0)
            --destination_length;

          continue;
        }
        // handle '..'
        else if (next_char == '.')
        {
          char afterNext = ((i + 1) < path_length) ? path[i + 2] : '\0';
          if (afterNext == '\\' || afterNext == '/' || afterNext == '\0')
          {
            // remove one directory of the path, including the /.
            if (destination_length > 1)
            {
              size_t j;
              for (j = destination_length - 2; j > 0; j--)
              {
                if (destination[j] == '\\' || destination[j] == '/')
                  break;
              }

              destination_length = j;
            }

            // skip the dot segment
            i += 2;
            continue;
          }
        }
      }
    }

    // fix ospath
    if (ospath && (current_char == '\\' || current_char == '/'))
    {
#ifdef _WIN32
      current_char = '\\';
#else
      current_char = '/';
#endif
    }

    // copy character
    destination[destination_length++] = current_char;

    // increment position by one
    i++;
  }

  // ensure null termination
  destination[destination_length] = '\0';
  destination.resize(destination_length);
  return destination;
}

std::unique_ptr<std::FILE, void (*)(FILE*)> FOpenUniquePtr(const char* filename, const char* mode)
{
  FILE* fp = std::fopen(filename, mode);
  return std::unique_ptr<std::FILE, void (*)(FILE*)>(fp, [](FILE* fp) {
    if (fp)
    {
      std::fclose(fp);
    }
  });
}

} // namespace Util