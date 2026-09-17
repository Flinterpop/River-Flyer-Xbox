// Glyph atlas from the in-code pixel font. One 128 x 48 texture holds the 95
// glyphs in 8 x 8 cells (16 per row); each character is a textured quad.
#include "rf/Text.h"

#include <array>
#include <cassert>
#include <cstring>

#include "PixelFont.h"
#include "rf/Draw.h"
#include "rf/Graphics.h"
#include "rf/Log.h"

namespace rf::text {

namespace {

constexpr int kCell    = 8;                               // glyph plus a one-pixel gap each way
constexpr int kColumns = 16;
constexpr int kAtlasW  = kColumns * kCell;                // 128
constexpr int kAtlasH  = ((font::kGlyphCount + kColumns - 1) / kColumns) * kCell;   // 48
constexpr int kAdvance = font::kGlyphW + 1;               // 6 native pixels per character
constexpr int kMaxChars = 1024;                           // per Draw / Measure call

// raylib's default face, which laid out every panel in the game, advances
// about 0.4 em per character and stands about 0.7 em tall. Scaling the 5 x 7
// glyphs by size/15 across and size/10 down reproduces those proportions, so
// text fits where the game expects it to.
constexpr float kEmW = 15.0f;
constexpr float kEmH = 10.0f;

TextureId g_atlas;

bool Lit(int glyph, int gx, int gy)
{
    assert(glyph >= 0 && glyph < font::kGlyphCount);
    assert(gx >= 0 && gx < font::kGlyphW && gy >= 0 && gy < font::kGlyphH);
    return font::kGlyphs[glyph].rows[gy][gx] == '#';
}

int GlyphIndex(char c)
{
    const int i = static_cast<int>(static_cast<unsigned char>(c)) - font::kFirstChar;
    return (i >= 0 && i < font::kGlyphCount) ? i : ('?' - font::kFirstChar);
}

int Length(const char* s)
{
    int n = 0;
    while (n < kMaxChars && s[n] != '\0') { ++n; }
    return n;
}

} // namespace

bool Init()
{
    static std::array<Color, kAtlasW * kAtlasH> pixels {};   // static: 24 KB, built once, never on the stack
    for (int g = 0; g < font::kGlyphCount; ++g) {
        const int cx = (g % kColumns) * kCell, cy = (g / kColumns) * kCell;
        for (int y = 0; y < font::kGlyphH; ++y) {
            for (int x = 0; x < font::kGlyphW; ++x) {
                const unsigned char v = Lit(g, x, y) ? 255 : 0;
                pixels[static_cast<size_t>((cy + y) * kAtlasW + cx + x)] = Color {255, 255, 255, v};
            }
        }
    }
    g_atlas = gfx::CreateTexture(kAtlasW, kAtlasH, pixels.data(), false);
    if (!gfx::TextureValid(g_atlas)) { log::Write(log::Level::Error, "TEXT: atlas texture failed"); return false; }
    gfx::SetTextureFilter(g_atlas, gfx::Filter::Point);   // crisp pixels at integer scales
    return true;
}

void Shutdown()
{
    gfx::DestroyTexture(g_atlas);
    g_atlas = TextureId {};
}

int Measure(const char* s, int size)
{
    assert(s != nullptr && size > 0);
    const int n = Length(s);
    if (n == 0) { return 0; }
    const float sx = static_cast<float>(size) / kEmW;
    const int   w  = static_cast<int>(static_cast<float>(n * kAdvance - 1) * sx);
    assert(w >= 0);
    return w;
}

void Draw(const char* s, int x, int y, int size, Color c)
{
    assert(s != nullptr && size > 0);
    if (!gfx::TextureValid(g_atlas)) { return; }
    const float sx = static_cast<float>(size) / kEmW, sy = static_cast<float>(size) / kEmH;
    const float gw = font::kGlyphW * sx, gh = font::kGlyphH * sy;
    const float top = static_cast<float>(y) + (static_cast<float>(size) - gh) * 0.5f;   // centred in the line
    float px = static_cast<float>(x);
    const int n = Length(s);
    for (int i = 0; i < n; ++i) {
        const int g = GlyphIndex(s[i]);
        if (s[i] != ' ') {
            const Rectangle src {static_cast<float>((g % kColumns) * kCell), static_cast<float>((g / kColumns) * kCell),
                                 static_cast<float>(font::kGlyphW), static_cast<float>(font::kGlyphH)};
            draw::Quad(g_atlas, src, Rectangle {px, top, gw, gh}, Vector2 {0.0f, 0.0f}, 0.0f, c);
        }
        px += kAdvance * sx;
    }
}

void Blit(const char* s, int x, int y, int size, Color c, Color* pixels, int width, int height)
{
    assert(s != nullptr && size > 0 && pixels != nullptr && width > 0 && height > 0);
    // Integer scales only: this is for sprite labels, which stay pixel-exact.
    const int scaleX = (size >= static_cast<int>(kEmW)) ? size / static_cast<int>(kEmW) : 1;
    const int scaleY = (size >= static_cast<int>(kEmH)) ? size / static_cast<int>(kEmH) : 1;
    const int n = Length(s);
    for (int i = 0; i < n; ++i) {
        const int g  = GlyphIndex(s[i]);
        const int ox = x + i * kAdvance * scaleX;
        for (int gy = 0; gy < font::kGlyphH; ++gy) {
            for (int gx = 0; gx < font::kGlyphW; ++gx) {
                if (!Lit(g, gx, gy)) { continue; }
                for (int sy = 0; sy < scaleY; ++sy) {
                    for (int sx = 0; sx < scaleX; ++sx) {
                        const int px = ox + gx * scaleX + sx, py = y + gy * scaleY + sy;
                        if (px >= 0 && py >= 0 && px < width && py < height) {
                            pixels[static_cast<size_t>(py) * static_cast<size_t>(width) + static_cast<size_t>(px)] = c;
                        }
                    }
                }
            }
        }
    }
}

} // namespace rf::text
