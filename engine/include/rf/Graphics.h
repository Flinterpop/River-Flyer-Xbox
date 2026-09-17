#pragma once

#include "rf/Types.h"

// The graphics device, the frame, textures and render targets.
//
// Backend: engine/src/d3d11/Graphics_D3D11.cpp. Direct3D 11 is the whole
// story on Xbox: there is no OpenGL or Vulkan on the console. Textures and
// targets live in fixed tables, so nothing is allocated after Init().
namespace rf::gfx {

constexpr int kMaxTextures = 64;
constexpr int kMaxTargets  = 4;

enum class Filter { Point, Linear, Trilinear };
enum class Wrap   { Clamp, Repeat };

bool Init();                       // after platform::Init: device, swap chain on the native window, pipeline state
void Shutdown();
void Trim();                       // console suspend: release what the driver can rebuild (IDXGIDevice3::Trim)

// A frame draws into the back buffer unless a target is bound. BeginFrame
// resizes the swap chain if the window changed; EndFrame flushes and presents
// with vsync, so the frame rate is the display's.
void BeginFrame();
void EndFrame();
void Clear(Color c);               // clears whatever is currently bound
int  BackBufferWidth();
int  BackBufferHeight();

// Textures are RGBA8, rows top to bottom. With mipmaps=true the whole chain is
// generated on upload, so Filter::Trilinear has something to sample.
TextureId CreateTexture(int width, int height, const Color* pixels, bool mipmaps);
void      DestroyTexture(TextureId id);
bool      TextureValid(TextureId id);
int       TextureWidth(TextureId id);
int       TextureHeight(TextureId id);
bool      TextureIsTarget(TextureId id);   // the colour buffer of a render target
void      SetTextureFilter(TextureId id, Filter f);
void      SetTextureWrap(TextureId id, Wrap w);
TextureId WhiteTexture();                  // 1x1 opaque white: every untextured shape samples it

// Render targets: draw into a texture, then draw that texture (the letterbox
// path). Unlike OpenGL, Direct3D targets are not stored upside down.
TargetId  CreateTarget(int width, int height);
void      DestroyTarget(TargetId id);
bool      TargetValid(TargetId id);
TextureId TargetTexture(TargetId id);
void      BeginTarget(TargetId id);        // flushes, binds the target; drawing is now in its pixels
void      EndTarget();                     // flushes, rebinds the back buffer

} // namespace rf::gfx
