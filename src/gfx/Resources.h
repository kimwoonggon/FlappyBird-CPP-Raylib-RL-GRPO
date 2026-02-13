#pragma once

#include <string>

#include <raylib.h>

namespace gfx {

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

  void Load(const std::string& path) {
    Reset();
    texture_ = LoadTexture(path.c_str());
    loaded_ = texture_.id > 0;
  }

  void LoadFromImage(const Image& image) {
    Reset();
    texture_ = LoadTextureFromImage(image);
    loaded_ = texture_.id > 0;
  }

  void Reset() {
    if (loaded_) {
      UnloadTexture(texture_);
      loaded_ = false;
      texture_ = Texture2D{};
    }
  }

  const Texture2D& Get() const { return texture_; }
  bool IsValid() const { return loaded_; }

 private:
  Texture2D texture_{};
  bool loaded_ = false;
};

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

  void Load(const std::string& path) {
    Reset();
    sound_ = LoadSound(path.c_str());
    loaded_ = sound_.frameCount > 0;
  }

  void Reset() {
    if (loaded_) {
      UnloadSound(sound_);
      loaded_ = false;
      sound_ = Sound{};
    }
  }

  const Sound& Get() const { return sound_; }
  bool IsValid() const { return loaded_; }

 private:
  Sound sound_{};
  bool loaded_ = false;
};

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

  void Load(int width, int height) {
    Reset();
    texture_ = LoadRenderTexture(width, height);
    loaded_ = texture_.id > 0;
  }

  void Reset() {
    if (loaded_) {
      UnloadRenderTexture(texture_);
      loaded_ = false;
      texture_ = RenderTexture2D{};
    }
  }

  const RenderTexture2D& Get() const { return texture_; }
  bool IsValid() const { return loaded_; }

 private:
  RenderTexture2D texture_{};
  bool loaded_ = false;
};

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

  void Load(const std::string& path) {
    Reset();
    image_ = LoadImage(path.c_str());
    loaded_ = image_.data != nullptr;
  }

  void Reset() {
    if (loaded_) {
      UnloadImage(image_);
      loaded_ = false;
      image_ = Image{};
    }
  }

  const Image& Get() const { return image_; }
  bool IsValid() const { return loaded_; }

 private:
  Image image_{};
  bool loaded_ = false;
};

}  // namespace gfx
