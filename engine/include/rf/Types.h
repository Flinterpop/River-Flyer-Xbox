#pragma once

// Value types shared by the engine and the game. These three are laid out
// exactly as raylib's so River Flyer's code, and compat/raylib.h, can use them
// unchanged. They live in the global namespace for the same reason.
struct Vector2 {
    float x;
    float y;
};

struct Rectangle {
    float x;
    float y;
    float width;
    float height;
};

struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

namespace rf {

// Handles into the engine's fixed tables. Index -1 is "no such thing"; the
// tables never grow, so a handle is never dangling in the heap sense, only stale.
struct TextureId {
    int  index {-1};
    bool Valid() const { return index >= 0; }
};

struct TargetId {
    int  index {-1};
    bool Valid() const { return index >= 0; }
};

struct SoundId {
    int  index {-1};
    bool Valid() const { return index >= 0; }
};

constexpr Color Rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
{
    return Color {r, g, b, a};
}

} // namespace rf
