/**
 * @file src/gfx/Resources.h
 * @brief Declarations for Resources.
 */

#pragma once

#include <string>

#include <raylib.h>

namespace gfx {

/**
 * @brief RAII wrapper for raylib `Texture2D`.
 */
class TextureResource {
 public:
  TextureResource() = default;
  explicit TextureResource(const std::string& path) { Load(path); }
  ~TextureResource() { Reset(); }

  TextureResource(const TextureResource&) = delete;
  TextureResource& operator=(const TextureResource&) = delete;

  TextureResource(TextureResource&& other) noexcept : texture_(other.texture_), loaded_(other.loaded_) {
    other.texture_ = Texture2D{};
    other.loaded_ = false;
  }

  TextureResource& operator=(TextureResource&& other) noexcept {
    if (this != &other) {
      Reset();
      texture_ = other.texture_;
      loaded_ = other.loaded_;
      other.texture_ = Texture2D{};
      other.loaded_ = false;
    }
    return *this;
  }

  /**
   * @brief Loads texture from file path.
   * @param path Texture file path.
   */
  void Load(const std::string& path) {
    Reset();
    texture_ = LoadTexture(path.c_str());
    loaded_ = texture_.id > 0;
  }

  /**
   * @brief Uploads texture from in-memory image.
   * @param image Source image.
   */
  void LoadFromImage(const Image& image) {
    Reset();
    texture_ = LoadTextureFromImage(image);
    loaded_ = texture_.id > 0;
  }

  /**
   * @brief Releases owned texture if loaded.
   */
  void Reset() {
    if (loaded_) {
      UnloadTexture(texture_);
      loaded_ = false;
      texture_ = Texture2D{};
    }
  }

  /**
   * @brief Returns underlying raylib texture.
   * @return Texture handle.
   */
  const Texture2D& Get() const { return texture_; }
  /**
   * @brief Checks whether texture is currently loaded.
   * @return True if valid.
   */
  bool IsValid() const { return loaded_; }

 private:
  Texture2D texture_{};
  bool loaded_ = false;
};

/**
 * @brief RAII wrapper for raylib `Sound`.
 */
class SoundResource {
 public:
  SoundResource() = default;
  explicit SoundResource(const std::string& path) { Load(path); }
  ~SoundResource() { Reset(); }

  SoundResource(const SoundResource&) = delete;
  SoundResource& operator=(const SoundResource&) = delete;

  SoundResource(SoundResource&& other) noexcept : sound_(other.sound_), loaded_(other.loaded_) {
    other.sound_ = Sound{};
    other.loaded_ = false;
  }

  SoundResource& operator=(SoundResource&& other) noexcept {
    if (this != &other) {
      Reset();
      sound_ = other.sound_;
      loaded_ = other.loaded_;
      other.sound_ = Sound{};
      other.loaded_ = false;
    }
    return *this;
  }

  /**
   * @brief Loads sound from file path.
   * @param path Sound file path.
   */
  void Load(const std::string& path) {
    Reset();
    sound_ = LoadSound(path.c_str());
    loaded_ = sound_.frameCount > 0;
  }

  /**
   * @brief Releases owned sound if loaded.
   */
  void Reset() {
    if (loaded_) {
      UnloadSound(sound_);
      loaded_ = false;
      sound_ = Sound{};
    }
  }

  /**
   * @brief Returns underlying sound handle.
   * @return Sound object.
   */
  const Sound& Get() const { return sound_; }
  /**
   * @brief Checks whether sound is currently loaded.
   * @return True if valid.
   */
  bool IsValid() const { return loaded_; }

 private:
  Sound sound_{};
  bool loaded_ = false;
};

/**
 * @brief RAII wrapper for raylib `RenderTexture2D`.
 */
class RenderTextureResource {
 public:
  RenderTextureResource() = default;
  RenderTextureResource(int width, int height) { Load(width, height); }
  ~RenderTextureResource() { Reset(); }

  RenderTextureResource(const RenderTextureResource&) = delete;
  RenderTextureResource& operator=(const RenderTextureResource&) = delete;

  RenderTextureResource(RenderTextureResource&& other) noexcept : texture_(other.texture_), loaded_(other.loaded_) {
    other.texture_ = RenderTexture2D{};
    other.loaded_ = false;
  }

  RenderTextureResource& operator=(RenderTextureResource&& other) noexcept {
    if (this != &other) {
      Reset();
      texture_ = other.texture_;
      loaded_ = other.loaded_;
      other.texture_ = RenderTexture2D{};
      other.loaded_ = false;
    }
    return *this;
  }

  /**
   * @brief Allocates render texture with given dimensions.
   * @param width Texture width.
   * @param height Texture height.
   */
  void Load(int width, int height) {
    Reset();
    texture_ = LoadRenderTexture(width, height);
    loaded_ = texture_.id > 0;
  }

  /**
   * @brief Releases owned render texture if loaded.
   */
  void Reset() {
    if (loaded_) {
      UnloadRenderTexture(texture_);
      loaded_ = false;
      texture_ = RenderTexture2D{};
    }
  }

  /**
   * @brief Returns underlying render texture.
   * @return Render texture handle.
   */
  const RenderTexture2D& Get() const { return texture_; }
  /**
   * @brief Checks whether render texture is loaded.
   * @return True if valid.
   */
  bool IsValid() const { return loaded_; }

 private:
  RenderTexture2D texture_{};
  bool loaded_ = false;
};

/**
 * @brief RAII wrapper for raylib `Shader`.
 */
class ShaderResource {
 public:
  ShaderResource() = default;
  ~ShaderResource() { Reset(); }

  ShaderResource(const ShaderResource&) = delete;
  ShaderResource& operator=(const ShaderResource&) = delete;

  ShaderResource(ShaderResource&& other) noexcept : shader_(other.shader_), loaded_(other.loaded_) {
    other.shader_ = Shader{};
    other.loaded_ = false;
  }

  ShaderResource& operator=(ShaderResource&& other) noexcept {
    if (this != &other) {
      Reset();
      shader_ = other.shader_;
      loaded_ = other.loaded_;
      other.shader_ = Shader{};
      other.loaded_ = false;
    }
    return *this;
  }

  /**
   * @brief Compiles shader from source strings.
   * @param vertexCode Vertex shader source, nullable for default.
   * @param fragmentCode Fragment shader source.
   */
  void LoadFromMemory(const char* vertexCode, const char* fragmentCode) {
    Reset();
    shader_ = LoadShaderFromMemory(vertexCode, fragmentCode);
    loaded_ = shader_.id > 0;
  }

  /**
   * @brief Releases owned shader if loaded.
   */
  void Reset() {
    if (loaded_) {
      UnloadShader(shader_);
      loaded_ = false;
      shader_ = Shader{};
    }
  }

  /**
   * @brief Returns underlying shader object.
   * @return Shader handle.
   */
  const Shader& Get() const { return shader_; }
  /**
   * @brief Checks whether shader is compiled and loaded.
   * @return True if valid.
   */
  bool IsValid() const { return loaded_; }

 private:
  Shader shader_{};
  bool loaded_ = false;
};

/**
 * @brief RAII wrapper for raylib `Image`.
 */
class ImageResource {
 public:
  ImageResource() = default;
  explicit ImageResource(const std::string& path) { Load(path); }
  ~ImageResource() { Reset(); }

  ImageResource(const ImageResource&) = delete;
  ImageResource& operator=(const ImageResource&) = delete;

  ImageResource(ImageResource&& other) noexcept : image_(other.image_), loaded_(other.loaded_) {
    other.image_ = Image{};
    other.loaded_ = false;
  }

  ImageResource& operator=(ImageResource&& other) noexcept {
    if (this != &other) {
      Reset();
      image_ = other.image_;
      loaded_ = other.loaded_;
      other.image_ = Image{};
      other.loaded_ = false;
    }
    return *this;
  }

  /**
   * @brief Loads image from file path.
   * @param path Image file path.
   */
  void Load(const std::string& path) {
    Reset();
    image_ = LoadImage(path.c_str());
    loaded_ = image_.data != nullptr;
  }

  /**
   * @brief Releases owned image if loaded.
   */
  void Reset() {
    if (loaded_) {
      UnloadImage(image_);
      loaded_ = false;
      image_ = Image{};
    }
  }

  /**
   * @brief Returns underlying image object.
   * @return Image handle.
   */
  const Image& Get() const { return image_; }
  /**
   * @brief Checks whether image is currently loaded.
   * @return True if valid.
   */
  bool IsValid() const { return loaded_; }

 private:
  Image image_{};
  bool loaded_ = false;
};

}  // namespace gfx
