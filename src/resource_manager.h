#pragma once
#include "common.h"
#include <string>
#include <unordered_map>

class Texture;

class ResourceManager
{
public:
  ResourceManager();
  ~ResourceManager();

  bool Initialize();

  const Texture* GetTexture(const char* name);
  const Texture* GetDefaultTexture();

  void UnloadAllResources();

private:
  static std::string GetTextureFilename(const std::string& texture_name);

  std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;

  std::unique_ptr<Texture> m_default_texture;
};

extern ResourceManager* g_resource_manager;