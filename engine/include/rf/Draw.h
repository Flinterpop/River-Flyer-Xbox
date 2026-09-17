#pragma once

#include "rf/Types.h"

// Batched 2D drawing into whatever gfx target is bound. Coordinates are
// pixels with y down, exactly as the game already thinks. Everything is
// triangles with a texture and a per-vertex colour; solid shapes sample the
// 1x1 white texture. Vertices accumulate in a fixed buffer and are flushed
// when it fills, when the texture changes, and at the end of the frame.
namespace rf::draw {

// Camera: screen = world * zoom + offset. Reset to identity each frame.
void SetView(Vector2 offset, float zoom);
void Flush();

// A textured quad. 'src' is in texels; a negative src width or height flips
// that axis. 'dst' places the 'origin' point, and rotation is about it.
void Quad(TextureId tex, Rectangle src, Rectangle dst, Vector2 origin, float rotationDeg, Color tint);

void Rect(Rectangle r, Color c);
void RectGradientH(Rectangle r, Color left, Color right);
void RectLines(Rectangle r, float thick, Color c);
void Triangle(Vector2 a, Vector2 b, Vector2 c, Color col);       // any winding
void Line(Vector2 a, Vector2 b, float thick, Color c);
void Circle(Vector2 centre, float radius, Color c);
void CircleLines(Vector2 centre, float radius, Color c);
void CircleSector(Vector2 centre, float radius, float startDeg, float endDeg, int segments, Color c);
void Ellipse(Vector2 centre, float rx, float ry, Color c);

} // namespace rf::draw
