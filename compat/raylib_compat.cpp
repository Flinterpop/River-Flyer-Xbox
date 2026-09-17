// raylib.h implemented on the rf engine. Kept in raylib's grouping so a
// function is easy to find from the declaration.
#include "raylib.h"

#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rf/Audio.h"
#include "rf/Draw.h"
#include "rf/Graphics.h"
#include "rf/Input.h"
#include "rf/Log.h"
#include "rf/Platform.h"
#include "rf/Random.h"
#include "rf/Storage.h"
#include "rf/Text.h"

namespace {

constexpr int kTextBuffers   = 4;
constexpr int kTextBufferLen = 1024;
constexpr int kMaxFileBytes  = 16 * 1024 * 1024;

struct State {
    unsigned int flags {0};
    int          exitKey {KEY_NULL};
    bool         ready {false};
    bool         warnedRotation {false};
    bool         warnedImageText {false};
};

State g;

rf::TextureId TexId(const Texture2D& t)           { return rf::TextureId {static_cast<int>(t.id) - 1}; }
rf::TargetId  TargetId(const RenderTexture2D& r)  { return rf::TargetId {static_cast<int>(r.id) - 1}; }
rf::SoundId   SndId(const Sound& s)               { return rf::SoundId {s.stream.id - 1}; }

Texture2D MakeTexture(rf::TextureId id)
{
    Texture2D t {};
    if (!rf::gfx::TextureValid(id)) { return t; }
    t.id      = static_cast<unsigned int>(id.index + 1);
    t.width   = rf::gfx::TextureWidth(id);
    t.height  = rf::gfx::TextureHeight(id);
    t.mipmaps = 1;
    t.format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    return t;
}

Rectangle RectI(int x, int y, int w, int h)
{
    return Rectangle {static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)};
}

// ---- image helpers -----------------------------------------------------------

bool ImageOk(const Image* img)
{
    return img != nullptr && img->data != nullptr && img->width > 0 && img->height > 0;
}

Color* Pixels(Image* img)
{
    assert(ImageOk(img));
    return static_cast<Color*>(img->data);
}

Color Blend(Color dst, Color src)   // source-over, as raylib's ImageDrawPixel
{
    if (src.a == 255) { return src; }
    if (src.a == 0)   { return dst; }
    const float a  = src.a / 255.0f;
    const float ia = 1.0f - a;
    const float outA = a + dst.a / 255.0f * ia;
    if (outA <= 0.0f) { return BLANK; }
    auto mix = [&](unsigned char s, unsigned char d) {
        return static_cast<unsigned char>((s * a + d * (dst.a / 255.0f) * ia) / outA + 0.5f);
    };
    return Color {mix(src.r, dst.r), mix(src.g, dst.g), mix(src.b, dst.b), static_cast<unsigned char>(outA * 255.0f + 0.5f)};
}

void Plot(Image* dst, int x, int y, Color c)
{
    if (x < 0 || y < 0 || x >= dst->width || y >= dst->height) { return; }
    Color& p = Pixels(dst)[static_cast<size_t>(y) * static_cast<size_t>(dst->width) + static_cast<size_t>(x)];
    p = Blend(p, c);
}

// Barycentric fill; 'gradient' interpolates the three colours, else c1 throughout.
void FillTriangle(Image* dst, Vector2 v1, Vector2 v2, Vector2 v3, Color c1, Color c2, Color c3, bool gradient)
{
    if (!ImageOk(dst)) { return; }
    const float area = (v2.x - v1.x) * (v3.y - v1.y) - (v3.x - v1.x) * (v2.y - v1.y);
    if (std::fabs(area) < 1e-6f) { return; }
    int minX = static_cast<int>(std::floor(std::fmin(v1.x, std::fmin(v2.x, v3.x))));
    int maxX = static_cast<int>(std::ceil(std::fmax(v1.x, std::fmax(v2.x, v3.x))));
    int minY = static_cast<int>(std::floor(std::fmin(v1.y, std::fmin(v2.y, v3.y))));
    int maxY = static_cast<int>(std::ceil(std::fmax(v1.y, std::fmax(v2.y, v3.y))));
    if (minX < 0) { minX = 0; }
    if (minY < 0) { minY = 0; }
    if (maxX > dst->width - 1)  { maxX = dst->width - 1; }
    if (maxY > dst->height - 1) { maxY = dst->height - 1; }

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const float px = static_cast<float>(x) + 0.5f, py = static_cast<float>(y) + 0.5f;
            const float w1 = ((v2.x - px) * (v3.y - py) - (v3.x - px) * (v2.y - py)) / area;
            const float w2 = ((v3.x - px) * (v1.y - py) - (v1.x - px) * (v3.y - py)) / area;
            const float w3 = 1.0f - w1 - w2;
            if (w1 < 0.0f || w2 < 0.0f || w3 < 0.0f) { continue; }
            Color c = c1;
            if (gradient) {
                c.r = static_cast<unsigned char>(c1.r * w1 + c2.r * w2 + c3.r * w3);
                c.g = static_cast<unsigned char>(c1.g * w1 + c2.g * w2 + c3.g * w3);
                c.b = static_cast<unsigned char>(c1.b * w1 + c2.b * w2 + c3.b * w3);
                c.a = static_cast<unsigned char>(c1.a * w1 + c2.a * w2 + c3.a * w3);
            }
            Plot(dst, x, y, c);
        }
    }
}

// ---- Perlin noise --------------------------------------------------------------
// Classic gradient noise with a permutation table shuffled once from a fixed
// seed, summed over six octaves like raylib's stb_perlin fbm call.

unsigned char g_perm[512];
bool          g_permReady = false;

void InitPerm()
{
    unsigned char p[256];
    for (int i = 0; i < 256; ++i) { p[i] = static_cast<unsigned char>(i); }
    unsigned int s = 1337u;
    for (int i = 255; i > 0; --i) {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        const int j = static_cast<int>(s % static_cast<unsigned int>(i + 1));
        const unsigned char t = p[i]; p[i] = p[j]; p[j] = t;
    }
    for (int i = 0; i < 512; ++i) { g_perm[i] = p[i & 255]; }
    g_permReady = true;
}

float FadeCurve(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
float Lerp(float a, float b, float t) { return a + (b - a) * t; }

float Grad(int hash, float x, float y)
{
    const int   h = hash & 7;
    const float u = (h < 4) ? x : y;
    const float v = (h < 4) ? y : x;
    return (((h & 1) != 0) ? -u : u) + (((h & 2) != 0) ? -2.0f * v : 2.0f * v);
}

float Noise2(float x, float y)
{
    const int   xi = static_cast<int>(std::floor(x)), yi = static_cast<int>(std::floor(y));
    const float xf = x - static_cast<float>(xi), yf = y - static_cast<float>(yi);
    const int   X = xi & 255, Y = yi & 255;
    const float u = FadeCurve(xf), v = FadeCurve(yf);
    const int aa = g_perm[g_perm[X] + Y],     ab = g_perm[g_perm[X] + Y + 1];
    const int ba = g_perm[g_perm[X + 1] + Y], bb = g_perm[g_perm[X + 1] + Y + 1];
    const float x1 = Lerp(Grad(aa, xf, yf),        Grad(ba, xf - 1.0f, yf),        u);
    const float x2 = Lerp(Grad(ab, xf, yf - 1.0f), Grad(bb, xf - 1.0f, yf - 1.0f), u);
    return Lerp(x1, x2, v) * 0.5f;   // roughly [-1, 1]
}

float Fbm(float x, float y)
{
    float sum = 0.0f, amp = 1.0f, freq = 1.0f, norm = 0.0f;
    for (int o = 0; o < 6; ++o) {
        sum  += amp * Noise2(x * freq, y * freq);
        norm += amp;
        amp  *= 0.5f;
        freq *= 2.0f;
    }
    return sum / norm;
}

// ---- file helpers ------------------------------------------------------------

std::FILE* Open(const char* path, const char* mode)
{
    std::FILE* f = nullptr;
    if (fopen_s(&f, path, mode) != 0) { return nullptr; }
    return f;
}

} // namespace

// ============================================================================
// window and timing
// ============================================================================

void InitWindow(int width, int height, const char* title)
{
    assert(width > 0 && height > 0 && title != nullptr);
    const rf::platform::Config cfg {width, height, title, (g.flags & FLAG_WINDOW_RESIZABLE) != 0};
    g.ready = rf::platform::Init(cfg) && rf::gfx::Init() && rf::text::Init();
    rf::random::Seed(static_cast<unsigned int>(rf::platform::Seconds() * 1000.0) ^ 0xA5A5A5A5u);
    if (!g.ready) { rf::log::Write(rf::log::Level::Error, "COMPAT: InitWindow failed"); }
}

void CloseWindow()
{
    rf::text::Shutdown();
    rf::gfx::Shutdown();
    rf::platform::Shutdown();
    g.ready = false;
}

bool WindowShouldClose()
{
    if (g.exitKey != KEY_NULL && IsKeyPressed(g.exitKey)) { return true; }
    return rf::platform::CloseRequested();
}

bool   IsWindowReady()                   { return g.ready; }
void   SetConfigFlags(unsigned int flags){ g.flags |= flags; }
void   SetExitKey(int key)               { g.exitKey = key; }
void   SetTargetFPS(int fps)             { assert(fps >= 0); (void)fps; }
int    GetScreenWidth()                  { return rf::platform::Width(); }
int    GetScreenHeight()                 { return rf::platform::Height(); }
float  GetFrameTime()                    { return rf::platform::FrameSeconds(); }
double GetTime()                         { return rf::platform::Seconds(); }

// ============================================================================
// drawing
// ============================================================================

void BeginDrawing()          { rf::gfx::BeginFrame(); }
void EndDrawing()            { rf::gfx::EndFrame(); rf::platform::PumpEvents(); }
void ClearBackground(Color c){ rf::gfx::Clear(c); }

void BeginMode2D(Camera2D cam)
{
    const float zoom = (cam.zoom > 0.0f) ? cam.zoom : 1.0f;
    if (cam.rotation != 0.0f && !g.warnedRotation) {
        g.warnedRotation = true;
        rf::log::Write(rf::log::Level::Warning, "COMPAT: Camera2D rotation is not supported");
    }
    rf::draw::SetView(Vector2 {cam.offset.x - cam.target.x * zoom, cam.offset.y - cam.target.y * zoom}, zoom);
}

void EndMode2D()                              { rf::draw::SetView(Vector2 {0.0f, 0.0f}, 1.0f); }
void BeginTextureMode(RenderTexture2D target) { rf::gfx::BeginTarget(TargetId(target)); }
void EndTextureMode()                         { rf::gfx::EndTarget(); }

void DrawRectangle(int x, int y, int w, int h, Color c)                  { rf::draw::Rect(RectI(x, y, w, h), c); }
void DrawRectangleLines(int x, int y, int w, int h, Color c)             { rf::draw::RectLines(RectI(x, y, w, h), 1.0f, c); }
void DrawRectangleGradientH(int x, int y, int w, int h, Color l, Color r){ rf::draw::RectGradientH(RectI(x, y, w, h), l, r); }
void DrawCircleV(Vector2 centre, float radius, Color c)                  { rf::draw::Circle(centre, radius, c); }
void DrawCircleLines(int cx, int cy, float radius, Color c)              { rf::draw::CircleLines(Vector2 {static_cast<float>(cx), static_cast<float>(cy)}, radius, c); }
void DrawCircleSector(Vector2 centre, float radius, float a0, float a1, int segments, Color c) { rf::draw::CircleSector(centre, radius, a0, a1, segments, c); }
void DrawEllipse(int cx, int cy, float rh, float rv, Color c)            { rf::draw::Ellipse(Vector2 {static_cast<float>(cx), static_cast<float>(cy)}, rh, rv, c); }
void DrawLineV(Vector2 a, Vector2 b, Color c)                            { rf::draw::Line(a, b, 1.0f, c); }
void DrawLineEx(Vector2 a, Vector2 b, float thick, Color c)              { rf::draw::Line(a, b, thick, c); }
void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color c)           { rf::draw::Triangle(v1, v2, v3, c); }
void DrawText(const char* text, int x, int y, int size, Color c)         { rf::text::Draw(text, x, y, size, c); }
int  MeasureText(const char* text, int size)                             { return rf::text::Measure(text, size); }

void DrawTexturePro(Texture2D texture, Rectangle src, Rectangle dst, Vector2 origin, float rotation, Color tint)
{
    const rf::TextureId id = TexId(texture);
    // raylib's GL render targets are stored upside down and the game flips them
    // with a negative source height. Direct3D targets are not, so undo the flip.
    if (rf::gfx::TextureIsTarget(id)) { src.height = -src.height; }
    rf::draw::Quad(id, src, dst, origin, rotation, tint);
}

// ============================================================================
// textures and images
// ============================================================================

Texture2D LoadTexture(const char* fileName)
{
    rf::log::Write(rf::log::Level::Warning, "COMPAT: LoadTexture(\"%s\"): no image loader in this engine", fileName);
    return Texture2D {};
}

Texture2D LoadTextureFromImage(Image image)
{
    assert(ImageOk(&image));
    return MakeTexture(rf::gfx::CreateTexture(image.width, image.height, static_cast<const Color*>(image.data), true));
}

void UnloadTexture(Texture2D texture) { rf::gfx::DestroyTexture(TexId(texture)); }

void GenTextureMipmaps(Texture2D* texture)
{
    assert(texture != nullptr);
    int levels = 1;
    for (int s = (texture->width > texture->height) ? texture->width : texture->height; s > 1; s /= 2) { ++levels; }
    texture->mipmaps = levels;   // the chain was built at load
}

void SetTextureFilter(Texture2D texture, int filter)
{
    const rf::gfx::Filter f = (filter == TEXTURE_FILTER_POINT) ? rf::gfx::Filter::Point
                            : (filter == TEXTURE_FILTER_BILINEAR) ? rf::gfx::Filter::Linear : rf::gfx::Filter::Trilinear;
    rf::gfx::SetTextureFilter(TexId(texture), f);
}

void SetTextureWrap(Texture2D texture, int wrap)
{
    rf::gfx::SetTextureWrap(TexId(texture), (wrap == TEXTURE_WRAP_REPEAT) ? rf::gfx::Wrap::Repeat : rf::gfx::Wrap::Clamp);
}

RenderTexture2D LoadRenderTexture(int width, int height)
{
    RenderTexture2D rt {};
    const rf::TargetId id = rf::gfx::CreateTarget(width, height);
    if (!rf::gfx::TargetValid(id)) { return rt; }
    rt.id      = static_cast<unsigned int>(id.index + 1);
    rt.texture = MakeTexture(rf::gfx::TargetTexture(id));
    return rt;
}

void UnloadRenderTexture(RenderTexture2D target)  { rf::gfx::DestroyTarget(TargetId(target)); }
bool IsRenderTextureValid(RenderTexture2D target) { return rf::gfx::TargetValid(TargetId(target)); }

Image GenImageColor(int width, int height, Color color)
{
    assert(width > 0 && height > 0);
    Image img {};
    img.data    = MemAlloc(static_cast<unsigned int>(width * height) * sizeof(Color));
    img.width   = width;
    img.height  = height;
    img.mipmaps = 1;
    img.format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    if (img.data != nullptr) {
        Color* px = Pixels(&img);
        for (int i = 0; i < width * height; ++i) { px[i] = color; }
    }
    return img;
}

Image GenImagePerlinNoise(int width, int height, int offsetX, int offsetY, float scale)
{
    if (!g_permReady) { InitPerm(); }
    Image img = GenImageColor(width, height, BLACK);
    if (img.data == nullptr) { return img; }
    Color* px = Pixels(&img);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float nx = static_cast<float>(x + offsetX) * scale / static_cast<float>(width);
            const float ny = static_cast<float>(y + offsetY) * scale / static_cast<float>(height);
            float p = Fbm(nx, ny);
            if (p < -1.0f) { p = -1.0f; }
            if (p > 1.0f)  { p = 1.0f; }
            const unsigned char v = static_cast<unsigned char>((p + 1.0f) * 0.5f * 255.0f);
            px[y * width + x] = Color {v, v, v, 255};
        }
    }
    return img;
}

void UnloadImage(Image image) { MemFree(image.data); }

Color* LoadImageColors(Image image)
{
    assert(ImageOk(&image));
    const size_t bytes = static_cast<size_t>(image.width) * static_cast<size_t>(image.height) * sizeof(Color);
    Color* out = static_cast<Color*>(MemAlloc(static_cast<unsigned int>(bytes)));
    if (out != nullptr) { std::memcpy(out, image.data, bytes); }
    return out;
}

void UnloadImageColors(Color* colors) { MemFree(colors); }

void ImageDrawPixel(Image* dst, int x, int y, Color c)
{
    if (!ImageOk(dst)) { return; }
    Plot(dst, x, y, c);
}

void ImageDrawRectangle(Image* dst, int x, int y, int w, int h, Color c)
{
    if (!ImageOk(dst)) { return; }
    int x0 = (x < 0) ? 0 : x, y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > dst->width) ? dst->width : x + w;
    int y1 = (y + h > dst->height) ? dst->height : y + h;
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) { Plot(dst, px, py, c); }
    }
}

void ImageDrawRectangleLines(Image* dst, Rectangle rec, int thick, Color c)
{
    const int x = static_cast<int>(rec.x), y = static_cast<int>(rec.y);
    const int w = static_cast<int>(rec.width), h = static_cast<int>(rec.height);
    ImageDrawRectangle(dst, x, y, w, thick, c);
    ImageDrawRectangle(dst, x, y + h - thick, w, thick, c);
    ImageDrawRectangle(dst, x, y + thick, thick, h - 2 * thick, c);
    ImageDrawRectangle(dst, x + w - thick, y + thick, thick, h - 2 * thick, c);
}

void ImageDrawCircle(Image* dst, int cx, int cy, int radius, Color c)
{
    if (!ImageOk(dst) || radius < 0) { return; }
    const int r2 = radius * radius;
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= r2) { Plot(dst, cx + x, cy + y, c); }
        }
    }
}

void ImageDrawCircleV(Image* dst, Vector2 centre, int radius, Color c)
{
    ImageDrawCircle(dst, static_cast<int>(centre.x), static_cast<int>(centre.y), radius, c);
}

void ImageDrawLineEx(Image* dst, Vector2 start, Vector2 end, int thick, Color c)
{
    if (!ImageOk(dst)) { return; }
    const float dx = end.x - start.x, dy = end.y - start.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    int steps = static_cast<int>(len) + 1;
    if (steps > dst->width + dst->height) { steps = dst->width + dst->height; }   // bounded even for wild inputs
    const int r = (thick > 1) ? thick / 2 : 0;
    for (int i = 0; i <= steps; ++i) {
        const float t = (steps > 0) ? static_cast<float>(i) / static_cast<float>(steps) : 0.0f;
        const int x = static_cast<int>(start.x + dx * t), y = static_cast<int>(start.y + dy * t);
        if (r == 0) { Plot(dst, x, y, c); } else { ImageDrawCircle(dst, x, y, r, c); }
    }
}

void ImageDrawTriangle(Image* dst, Vector2 v1, Vector2 v2, Vector2 v3, Color c)
{
    FillTriangle(dst, v1, v2, v3, c, c, c, false);
}

void ImageDrawTriangleEx(Image* dst, Vector2 v1, Vector2 v2, Vector2 v3, Color c1, Color c2, Color c3)
{
    FillTriangle(dst, v1, v2, v3, c1, c2, c3, true);
}

void ImageDrawText(Image* dst, const char* text, int x, int y, int fontSize, Color c)
{
    if (!ImageOk(dst) || text == nullptr) { return; }
    rf::text::Blit(text, x, y, fontSize, c, Pixels(dst), dst->width, dst->height);
}

// ============================================================================
// audio
// ============================================================================

void InitAudioDevice()     { (void)rf::audio::Init(); }
void CloseAudioDevice()    { rf::audio::Shutdown(); }
bool IsAudioDeviceReady()  { return rf::audio::Ready(); }

Sound LoadSoundFromWave(Wave wave)
{
    Sound s {};
    if (wave.sampleSize != 16 || wave.channels != 1 || wave.data == nullptr || wave.frameCount == 0) {
        rf::log::Write(rf::log::Level::Warning, "COMPAT: LoadSoundFromWave: only 16-bit mono is supported");
        return s;
    }
    const rf::SoundId id = rf::audio::Create(static_cast<const short*>(wave.data), static_cast<int>(wave.frameCount),
                                             static_cast<int>(wave.sampleRate));
    if (!id.Valid()) { return s; }
    s.stream.id  = id.index + 1;
    s.frameCount = wave.frameCount;
    return s;
}

void UnloadSound(Sound sound)                   { rf::audio::Destroy(SndId(sound)); }
void UnloadWave(Wave wave)                      { MemFree(wave.data); }
void PlaySound(Sound sound)                     { rf::audio::Play(SndId(sound)); }
void StopSound(Sound sound)                     { rf::audio::Stop(SndId(sound)); }
bool IsSoundPlaying(Sound sound)                { return rf::audio::Playing(SndId(sound)); }
void SetSoundVolume(Sound sound, float volume)  { rf::audio::SetVolume(SndId(sound), volume); }

// ============================================================================
// input
// ============================================================================

bool  IsKeyDown(int key)                          { return rf::input::KeyDown(static_cast<rf::input::Key>(key)); }
bool  IsKeyPressed(int key)                       { return rf::input::KeyPressed(static_cast<rf::input::Key>(key)); }
bool  IsKeyPressedRepeat(int key)                 { return rf::input::KeyPressedRepeat(static_cast<rf::input::Key>(key)); }
int   GetKeyPressed()                             { return rf::input::NextKey(); }
int   GetCharPressed()                            { return rf::input::NextChar(); }
bool  IsGamepadAvailable(int pad)                 { return rf::input::PadConnected(pad); }
bool  IsGamepadButtonDown(int pad, int button)    { return rf::input::ButtonDown(pad, static_cast<rf::input::Button>(button)); }
bool  IsGamepadButtonPressed(int pad, int button) { return rf::input::ButtonPressed(pad, static_cast<rf::input::Button>(button)); }
float GetGamepadAxisMovement(int pad, int axis)   { return rf::input::AxisValue(pad, static_cast<rf::input::Axis>(axis)); }

// ============================================================================
// files and memory
// ============================================================================

const char* GetApplicationDirectory() { return rf::storage::Folder(); }

bool FileExists(const char* fileName)
{
    assert(fileName != nullptr);
    std::FILE* f = Open(fileName, "rb");
    if (f == nullptr) { return false; }
    (void)std::fclose(f);
    return true;
}

unsigned char* LoadFileData(const char* fileName, int* dataSize)
{
    assert(fileName != nullptr && dataSize != nullptr);
    *dataSize = 0;
    std::FILE* f = Open(fileName, "rb");
    if (f == nullptr) { return nullptr; }
    (void)std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    (void)std::fseek(f, 0, SEEK_SET);
    if (size < 0 || size > kMaxFileBytes) { (void)std::fclose(f); return nullptr; }
    unsigned char* data = static_cast<unsigned char*>(MemAlloc(static_cast<unsigned int>(size) + 1u));
    if (data != nullptr) {
        *dataSize = static_cast<int>(std::fread(data, 1, static_cast<size_t>(size), f));
    }
    (void)std::fclose(f);
    return data;
}

void UnloadFileData(unsigned char* data) { MemFree(data); }

bool SaveFileText(const char* fileName, char* text)
{
    assert(fileName != nullptr && text != nullptr);
    std::FILE* f = Open(fileName, "wb");
    if (f == nullptr) { rf::log::Write(rf::log::Level::Warning, "COMPAT: cannot write %s", fileName); return false; }
    const size_t len = std::strlen(text);
    const bool ok = std::fwrite(text, 1, len, f) == len;
    (void)std::fclose(f);
    return ok;
}

void* MemAlloc(unsigned int size) { return std::calloc(size, 1); }
void  MemFree(void* ptr)          { std::free(ptr); }

// ============================================================================
// misc
// ============================================================================

Color Fade(Color color, float alpha)
{
    if (alpha < 0.0f) { alpha = 0.0f; }
    if (alpha > 1.0f) { alpha = 1.0f; }
    color.a = static_cast<unsigned char>(255.0f * alpha);
    return color;
}

bool CheckCollisionRecs(Rectangle a, Rectangle b)
{
    return a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y;
}

bool CheckCollisionCircleRec(Vector2 c, float radius, Rectangle r)
{
    const float cx = (c.x < r.x) ? r.x : ((c.x > r.x + r.width) ? r.x + r.width : c.x);    // closest point on the box
    const float cy = (c.y < r.y) ? r.y : ((c.y > r.y + r.height) ? r.y + r.height : c.y);
    const float dx = c.x - cx, dy = c.y - cy;
    return dx * dx + dy * dy <= radius * radius;
}

int GetRandomValue(int min, int max) { return rf::random::Range(min, max); }

const char* TextFormat(const char* text, ...)
{
    static char buffers[kTextBuffers][kTextBufferLen];
    static int  next = 0;
    char* buf = buffers[next];
    next = (next + 1) % kTextBuffers;
    va_list args;
    va_start(args, text);
    (void)std::vsnprintf(buf, kTextBufferLen, text, args);
    va_end(args);
    buf[kTextBufferLen - 1] = '\0';
    return buf;
}

void TraceLog(int logLevel, const char* text, ...)
{
    char line[512] = {0};
    va_list args;
    va_start(args, text);
    (void)std::vsnprintf(line, sizeof(line), text, args);
    va_end(args);
    const rf::log::Level level = (logLevel >= LOG_ERROR) ? rf::log::Level::Error
                               : (logLevel == LOG_WARNING) ? rf::log::Level::Warning : rf::log::Level::Info;
    rf::log::Write(level, "%s", line);
}
