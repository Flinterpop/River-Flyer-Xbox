#pragma once

// raylib compatibility header: the 86 raylib calls River Flyer makes, with
// raylib's exact signatures, implemented on rf::* in raylib_compat.cpp. The
// game compiles against this file unchanged; the real raylib is not involved.
//
// Deliberate differences from raylib are listed next to each declaration.
// Everything not listed here is not implemented: add it when the game needs it.

#include <cstddef>

#include "rf/Types.h"   // Vector2, Rectangle, Color: same layout as raylib's

// ---- types ------------------------------------------------------------------

struct Texture {
    unsigned int id;        // rf texture index + 1; 0 is invalid
    int width;
    int height;
    int mipmaps;
    int format;
};
typedef Texture Texture2D;

struct RenderTexture {
    unsigned int id;        // rf target index + 1; 0 is invalid
    Texture texture;
    Texture depth;          // always empty
};
typedef RenderTexture RenderTexture2D;

struct Image {
    void* data;             // RGBA8, rows top to bottom, owned via MemAlloc/MemFree
    int width;
    int height;
    int mipmaps;
    int format;             // always PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
};

struct Wave {
    unsigned int frameCount;
    unsigned int sampleRate;
    unsigned int sampleSize;   // only 16 is accepted
    unsigned int channels;     // only 1 is accepted
    void* data;
};

struct AudioStream {
    int id;                 // rf sound index + 1; 0 is invalid (raylib keeps a buffer pointer here)
};

struct Sound {
    AudioStream stream;
    unsigned int frameCount;
};

struct Camera2D {
    Vector2 offset;
    Vector2 target;
    float rotation;         // ignored (logged once if non-zero)
    float zoom;
};

// ---- constants -------------------------------------------------------------

inline constexpr Color LIGHTGRAY  {200, 200, 200, 255};
inline constexpr Color GRAY       {130, 130, 130, 255};
inline constexpr Color DARKGRAY   {80, 80, 80, 255};
inline constexpr Color YELLOW     {253, 249, 0, 255};
inline constexpr Color GOLD       {255, 203, 0, 255};
inline constexpr Color ORANGE     {255, 161, 0, 255};
inline constexpr Color PINK       {255, 109, 194, 255};
inline constexpr Color RED        {230, 41, 55, 255};
inline constexpr Color MAROON     {190, 33, 55, 255};
inline constexpr Color GREEN      {0, 228, 48, 255};
inline constexpr Color LIME       {0, 158, 47, 255};
inline constexpr Color DARKGREEN  {0, 117, 44, 255};
inline constexpr Color SKYBLUE    {102, 191, 255, 255};
inline constexpr Color BLUE       {0, 121, 241, 255};
inline constexpr Color DARKBLUE   {0, 82, 172, 255};
inline constexpr Color PURPLE     {200, 122, 255, 255};
inline constexpr Color VIOLET     {135, 60, 190, 255};
inline constexpr Color DARKPURPLE {112, 31, 126, 255};
inline constexpr Color BEIGE      {211, 176, 131, 255};
inline constexpr Color BROWN      {127, 106, 79, 255};
inline constexpr Color DARKBROWN  {76, 63, 47, 255};
inline constexpr Color WHITE      {255, 255, 255, 255};
inline constexpr Color BLACK      {0, 0, 0, 255};
inline constexpr Color BLANK      {0, 0, 0, 0};
inline constexpr Color MAGENTA    {255, 0, 255, 255};
inline constexpr Color RAYWHITE   {245, 245, 245, 255};

enum ConfigFlags {
    FLAG_WINDOW_RESIZABLE = 0x00000004,
};

enum TraceLogLevel { LOG_ALL = 0, LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL, LOG_NONE };

// GLFW numbering, shared with rf::input::Key.
enum KeyboardKey {
    KEY_NULL = 0,
    KEY_APOSTROPHE = 39, KEY_COMMA = 44, KEY_MINUS = 45, KEY_PERIOD = 46, KEY_SLASH = 47,
    KEY_ZERO = 48, KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE,
    KEY_SEMICOLON = 59, KEY_EQUAL = 61,
    KEY_A = 65, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M,
    KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_SPACE = 32, KEY_ESCAPE = 256, KEY_ENTER = 257, KEY_TAB = 258, KEY_BACKSPACE = 259,
    KEY_RIGHT = 262, KEY_LEFT = 263, KEY_DOWN = 264, KEY_UP = 265,
    KEY_F1 = 290, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_LEFT_SHIFT = 340, KEY_LEFT_CONTROL = 341, KEY_LEFT_ALT = 342,
    KEY_RIGHT_SHIFT = 344, KEY_RIGHT_CONTROL = 345, KEY_RIGHT_ALT = 346,
};

enum GamepadButton {
    GAMEPAD_BUTTON_UNKNOWN = 0,
    GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_RIGHT, GAMEPAD_BUTTON_LEFT_FACE_DOWN, GAMEPAD_BUTTON_LEFT_FACE_LEFT,
    GAMEPAD_BUTTON_RIGHT_FACE_UP, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
    GAMEPAD_BUTTON_LEFT_TRIGGER_1, GAMEPAD_BUTTON_LEFT_TRIGGER_2, GAMEPAD_BUTTON_RIGHT_TRIGGER_1, GAMEPAD_BUTTON_RIGHT_TRIGGER_2,
    GAMEPAD_BUTTON_MIDDLE_LEFT, GAMEPAD_BUTTON_MIDDLE, GAMEPAD_BUTTON_MIDDLE_RIGHT,
    GAMEPAD_BUTTON_LEFT_THUMB, GAMEPAD_BUTTON_RIGHT_THUMB,
};

enum GamepadAxis {
    GAMEPAD_AXIS_LEFT_X = 0, GAMEPAD_AXIS_LEFT_Y, GAMEPAD_AXIS_RIGHT_X, GAMEPAD_AXIS_RIGHT_Y,
    GAMEPAD_AXIS_LEFT_TRIGGER, GAMEPAD_AXIS_RIGHT_TRIGGER,
};

enum TextureFilter {
    TEXTURE_FILTER_POINT = 0, TEXTURE_FILTER_BILINEAR, TEXTURE_FILTER_TRILINEAR,
    TEXTURE_FILTER_ANISOTROPIC_4X, TEXTURE_FILTER_ANISOTROPIC_8X, TEXTURE_FILTER_ANISOTROPIC_16X,
};

enum TextureWrap { TEXTURE_WRAP_REPEAT = 0, TEXTURE_WRAP_CLAMP, TEXTURE_WRAP_MIRROR_REPEAT, TEXTURE_WRAP_MIRROR_CLAMP };

enum PixelFormat { PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 = 7 };

// ---- window and timing -----------------------------------------------------

void   InitWindow(int width, int height, const char* title);
void   CloseWindow();
bool   WindowShouldClose();
bool   IsWindowReady();
void   SetConfigFlags(unsigned int flags);
void   SetExitKey(int key);
void   SetTargetFPS(int fps);          // no-op: presentation is vsync-paced
int    GetScreenWidth();
int    GetScreenHeight();
float  GetFrameTime();
double GetTime();

// ---- drawing ---------------------------------------------------------------

void BeginDrawing();
void EndDrawing();                     // presents, then polls input (raylib order)
void ClearBackground(Color color);
void BeginMode2D(Camera2D camera);
void EndMode2D();
void BeginTextureMode(RenderTexture2D target);
void EndTextureMode();

void DrawRectangle(int posX, int posY, int width, int height, Color color);
void DrawRectangleLines(int posX, int posY, int width, int height, Color color);
void DrawRectangleGradientH(int posX, int posY, int width, int height, Color left, Color right);
void DrawCircleV(Vector2 center, float radius, Color color);
void DrawCircleLines(int centerX, int centerY, float radius, Color color);
void DrawCircleSector(Vector2 center, float radius, float startAngle, float endAngle, int segments, Color color);
void DrawEllipse(int centerX, int centerY, float radiusH, float radiusV, Color color);
void DrawLineV(Vector2 startPos, Vector2 endPos, Color color);
void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);
void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);   // any winding (raylib culls clockwise)
void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
void DrawText(const char* text, int posX, int posY, int fontSize, Color color);
int  MeasureText(const char* text, int fontSize);

// ---- textures and images ---------------------------------------------------

Texture2D       LoadTexture(const char* fileName);      // no image loader: logs and returns an invalid texture
Texture2D       LoadTextureFromImage(Image image);      // always builds mipmaps, so GenTextureMipmaps is bookkeeping
void            UnloadTexture(Texture2D texture);
void            GenTextureMipmaps(Texture2D* texture);
void            SetTextureFilter(Texture2D texture, int filter);
void            SetTextureWrap(Texture2D texture, int wrap);
RenderTexture2D LoadRenderTexture(int width, int height);
void            UnloadRenderTexture(RenderTexture2D target);
bool            IsRenderTextureValid(RenderTexture2D target);

Image  GenImageColor(int width, int height, Color color);
Image  GenImagePerlinNoise(int width, int height, int offsetX, int offsetY, float scale);   // own Perlin, not stb_perlin: different but similar
void   UnloadImage(Image image);
Color* LoadImageColors(Image image);
void   UnloadImageColors(Color* colors);
void   ImageDrawPixel(Image* dst, int posX, int posY, Color color);
void   ImageDrawRectangle(Image* dst, int posX, int posY, int width, int height, Color color);
void   ImageDrawRectangleLines(Image* dst, Rectangle rec, int thick, Color color);
void   ImageDrawCircle(Image* dst, int centerX, int centerY, int radius, Color color);
void   ImageDrawCircleV(Image* dst, Vector2 center, int radius, Color color);
void   ImageDrawLineEx(Image* dst, Vector2 start, Vector2 end, int thick, Color color);
void   ImageDrawTriangle(Image* dst, Vector2 v1, Vector2 v2, Vector2 v3, Color color);
void   ImageDrawTriangleEx(Image* dst, Vector2 v1, Vector2 v2, Vector2 v3, Color c1, Color c2, Color c3);
void   ImageDrawText(Image* dst, const char* text, int posX, int posY, int fontSize, Color color);   // not implemented: logs once

// ---- audio -----------------------------------------------------------------

void  InitAudioDevice();
void  CloseAudioDevice();
bool  IsAudioDeviceReady();
Sound LoadSoundFromWave(Wave wave);    // 16-bit mono only
void  UnloadSound(Sound sound);
void  UnloadWave(Wave wave);
void  PlaySound(Sound sound);
void  StopSound(Sound sound);
bool  IsSoundPlaying(Sound sound);
void  SetSoundVolume(Sound sound, float volume);

// ---- input -----------------------------------------------------------------

bool  IsKeyDown(int key);
bool  IsKeyPressed(int key);
bool  IsKeyPressedRepeat(int key);
int   GetKeyPressed();
int   GetCharPressed();
bool  IsGamepadAvailable(int gamepad);
bool  IsGamepadButtonDown(int gamepad, int button);
bool  IsGamepadButtonPressed(int gamepad, int button);
float GetGamepadAxisMovement(int gamepad, int axis);

// ---- files and memory ------------------------------------------------------

const char*    GetApplicationDirectory();   // the WRITABLE folder (rf::storage::Folder), not the exe's: consoles cannot write beside the exe
bool           FileExists(const char* fileName);
unsigned char* LoadFileData(const char* fileName, int* dataSize);
void           UnloadFileData(unsigned char* data);
bool           SaveFileText(const char* fileName, char* text);
void*          MemAlloc(unsigned int size);
void           MemFree(void* ptr);

// ---- misc ------------------------------------------------------------------

Color       Fade(Color color, float alpha);
bool        CheckCollisionRecs(Rectangle rec1, Rectangle rec2);
bool        CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec);
int         GetRandomValue(int min, int max);
const char* TextFormat(const char* text, ...);   // four rotating 1 KB buffers
void        TraceLog(int logLevel, const char* text, ...);
