#pragma once

#include "rf/Types.h"

// Text drawing from a glyph atlas. The font is a 5 x 7 pixel face drawn in
// code (engine/src/text/PixelFont.h); Init() rasterises it into one texture
// and Draw() emits a quad per character. 'size' is the line height in pixels;
// the native height is 8, so size 16 is a crisp 2x and other sizes are
// nearest-neighbour scaled. Blit() draws into a CPU image instead, for
// sprites that carry a label.
namespace rf::text {

bool Init();                                                    // after gfx::Init: builds the atlas
void Shutdown();
void Draw(const char* s, int x, int y, int size, Color c);
int  Measure(const char* s, int size);                          // width in pixels
void Blit(const char* s, int x, int y, int size, Color c, Color* pixels, int width, int height);

} // namespace rf::text
