#include "pch.h"
#include "resource_manager.h"
#include "texture.h"
#include "util.h"

static ResourceManager s_resource_manager;
ResourceManager* g_resource_manager = &s_resource_manager;

ResourceManager::ResourceManager() = default;

ResourceManager::~ResourceManager() = default;

bool ResourceManager::Initialize()
{
  m_default_texture = Texture::CreateCheckerboardTexture();
  if (!m_default_texture)
  {
    std::fprintf(stderr, "Failed to create default texture.\n");
    return false;
  }

  return true;
}

const Texture* ResourceManager::GetTexture(const char* name)
{
  auto iter = m_textures.find(name);
  if (iter != m_textures.end())
    return iter->second.get() ? iter->second.get() : m_default_texture.get();

  std::string filename = GetTextureFilename(name);
  std::unique_ptr<Texture> texture;
  if (!filename.empty())
  {
    std::string safe_filename = Util::CanonicalizePath(filename.c_str());
    texture = Texture::LoadFromFile(safe_filename.c_str(), true, true, true, true);
  }
  else
  {
    std::fprintf(stderr, "Failed to find file for texture '%s'.\n", name);
  }

  auto ip = m_textures.emplace(name, std::move(texture));
  return ip.first->second.get() ? ip.first->second.get() : m_default_texture.get();
}

const Texture* ResourceManager::GetDefaultTexture()
{
  return m_default_texture.get();
}

void ResourceManager::UnloadAllResources()
{
  m_textures.clear();
  m_default_texture.reset();
}

std::string ResourceManager::GetTextureFilename(const std::string& texture_name)
{
  static const char* extensions_to_try[] = {".tga", ".jpg"};
  for (size_t i = 0; i < sizeof(extensions_to_try) / sizeof(extensions_to_try[0]); i++)
  {
    std::string filename = texture_name;
    filename += extensions_to_try[i];
    struct stat sb;
    if (stat(filename.c_str(), &sb) == 0)
      return filename;
  }

  return std::string();
}
