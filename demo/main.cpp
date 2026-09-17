// Phase 2 of the plan: a window, a cleared screen, a controller, and a plane
// that follows the stick. Everything River Flyer needs from an engine is
// exercised here in miniature: the frame loop, a render target scaled to the
// window (the TV letterbox), textured and solid drawing, keyboard and pad.
#include <cassert>
#include <cmath>

#include "rf/Draw.h"
#include "rf/Graphics.h"
#include "rf/Input.h"
#include "rf/Log.h"
#include "rf/Platform.h"
#include "rf/Text.h"

namespace {

constexpr int   kCanvasW  = 960;     // the game's fixed portrait canvas
constexpr int   kCanvasH  = 1000;
constexpr float kSpeed    = 420.0f;  // px per second
constexpr float kDeadZone = 0.25f;

// Returns the stick or WASD direction, each axis in [-1, 1].
Vector2 ReadMove()
{
    using rf::input::Axis;
    using rf::input::Key;
    Vector2 d {rf::input::AxisValue(0, Axis::LeftX), rf::input::AxisValue(0, Axis::LeftY)};
    if (std::fabs(d.x) < kDeadZone) { d.x = 0.0f; }
    if (std::fabs(d.y) < kDeadZone) { d.y = 0.0f; }
    if (rf::input::KeyDown(Key::A) || rf::input::KeyDown(Key::Left))  { d.x -= 1.0f; }
    if (rf::input::KeyDown(Key::D) || rf::input::KeyDown(Key::Right)) { d.x += 1.0f; }
    if (rf::input::KeyDown(Key::W) || rf::input::KeyDown(Key::Up))    { d.y -= 1.0f; }
    if (rf::input::KeyDown(Key::S) || rf::input::KeyDown(Key::Down))  { d.y += 1.0f; }
    d.x = (d.x < -1.0f) ? -1.0f : ((d.x > 1.0f) ? 1.0f : d.x);
    d.y = (d.y < -1.0f) ? -1.0f : ((d.y > 1.0f) ? 1.0f : d.y);
    return d;
}

void DrawScene(Vector2 plane, bool padOn, bool firing)
{
    rf::gfx::Clear(rf::Rgba(58, 130, 220));                                            // river
    rf::draw::Rect(Rectangle {0.0f, 0.0f, 220.0f, kCanvasH}, rf::Rgba(70, 150, 60));   // banks
    rf::draw::Rect(Rectangle {kCanvasW - 220.0f, 0.0f, 220.0f, kCanvasH}, rf::Rgba(70, 150, 60));
    rf::draw::RectGradientH(Rectangle {40.0f, 40.0f, 300.0f, 24.0f}, rf::Rgba(0, 200, 0), rf::Rgba(200, 0, 0));   // a fuel bar
    rf::draw::Circle(Vector2 {480.0f, 300.0f}, 40.0f, rf::Rgba(90, 90, 90));           // a rock
    rf::draw::CircleLines(Vector2 {480.0f, 300.0f}, 48.0f, rf::Rgba(255, 255, 255, 120));
    rf::draw::Ellipse(Vector2 {700.0f, 600.0f}, 60.0f, 30.0f, rf::Rgba(255, 220, 0));   // a duck-ish blob

    // The plane: a triangle fuselage, a rect wing, a firing line.
    const Color body = firing ? rf::Rgba(255, 240, 120) : rf::Rgba(240, 200, 40);
    rf::draw::Triangle(Vector2 {plane.x, plane.y - 40.0f}, Vector2 {plane.x - 22.0f, plane.y + 30.0f}, Vector2 {plane.x + 22.0f, plane.y + 30.0f}, body);
    rf::draw::Rect(Rectangle {plane.x - 50.0f, plane.y - 4.0f, 100.0f, 10.0f}, body);
    if (firing) { rf::draw::Line(Vector2 {plane.x, plane.y - 44.0f}, Vector2 {plane.x, plane.y - 140.0f}, 3.0f, rf::Rgba(255, 255, 255)); }

    // Pad indicator (text comes in phase 3, so a square will do).
    rf::draw::Rect(Rectangle {kCanvasW - 60.0f, 40.0f, 24.0f, 24.0f}, padOn ? rf::Rgba(0, 220, 0) : rf::Rgba(120, 120, 120));
    rf::draw::RectLines(Rectangle {0.0f, 0.0f, kCanvasW, kCanvasH}, 4.0f, rf::Rgba(255, 203, 0));
}

// Present the canvas scaled to fit the window, centred, like the TV build.
void PresentCanvas(rf::TargetId canvas)
{
    const float sw = static_cast<float>(rf::gfx::BackBufferWidth()), sh = static_cast<float>(rf::gfx::BackBufferHeight());
    const float sx = sw / kCanvasW, sy = sh / kCanvasH;
    const float scale = (sx < sy) ? sx : sy;
    const float w = kCanvasW * scale, h = kCanvasH * scale;
    rf::draw::Quad(rf::gfx::TargetTexture(canvas), Rectangle {0.0f, 0.0f, kCanvasW, kCanvasH},
                   Rectangle {(sw - w) * 0.5f, (sh - h) * 0.5f, w, h}, Vector2 {0.0f, 0.0f}, 0.0f, rf::Rgba(255, 255, 255));
}

} // namespace

int main()
{
    const rf::platform::Config cfg {1280, 720, "River Flyer engine demo", true};
    if (!rf::platform::Init(cfg) || !rf::gfx::Init() || !rf::text::Init()) { return 1; }

    rf::TargetId canvas = rf::gfx::CreateTarget(kCanvasW, kCanvasH);
    assert(rf::gfx::TargetValid(canvas));
    rf::gfx::SetTextureFilter(rf::gfx::TargetTexture(canvas), rf::gfx::Filter::Linear);

    Vector2 plane {kCanvasW * 0.5f, kCanvasH - 140.0f};
    while (!rf::platform::CloseRequested()) {
        rf::platform::PumpEvents();
        if (rf::input::KeyPressed(rf::input::Key::Escape)) { rf::platform::RequestClose(); }

        const float   dt = rf::platform::FrameSeconds();
        const Vector2 d  = ReadMove();
        plane.x += d.x * kSpeed * dt;
        plane.y += d.y * kSpeed * dt;
        plane.x = (plane.x < 240.0f) ? 240.0f : ((plane.x > kCanvasW - 240.0f) ? kCanvasW - 240.0f : plane.x);
        plane.y = (plane.y < 60.0f) ? 60.0f : ((plane.y > kCanvasH - 60.0f) ? kCanvasH - 60.0f : plane.y);
        const bool firing = rf::input::KeyDown(rf::input::Key::Space) || rf::input::ButtonDown(0, rf::input::Button::RightFaceDown);

        rf::gfx::BeginFrame();
        rf::gfx::Clear(rf::Rgba(0, 0, 0));
        rf::gfx::BeginTarget(canvas);
        DrawScene(plane, rf::input::PadConnected(0), firing);
        rf::gfx::EndTarget();
        PresentCanvas(canvas);
        rf::gfx::EndFrame();
    }

    rf::gfx::DestroyTarget(canvas);
    rf::text::Shutdown();
    rf::gfx::Shutdown();
    rf::platform::Shutdown();
    return 0;
}
