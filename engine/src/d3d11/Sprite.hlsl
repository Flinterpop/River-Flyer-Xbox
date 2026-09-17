// The one shader pair the 2D renderer needs. Compiled at build time by fxc
// (see CMakeLists.txt) into byte arrays the engine embeds, so nothing is
// compiled on the player's machine and the same bytes run on the console.
//
// Vertices arrive in pixels; 'proj' folds the camera (zoom + offset) and the
// pixel-to-clip mapping into one matrix. row_major matches the float[16] the
// C++ side writes without a transpose.

cbuffer Frame : register(b0)
{
    row_major float4x4 proj;
};

Texture2D    tex : register(t0);
SamplerState smp : register(s0);

struct VSIn  { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 col : COLOR0; };
struct PSIn  { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 col : COLOR0; };

PSIn VSMain(VSIn i)
{
    PSIn o;
    o.pos = mul(proj, float4(i.pos, 0.0f, 1.0f));
    o.uv  = i.uv;
    o.col = i.col;
    return o;
}

float4 PSMain(PSIn i) : SV_TARGET
{
    return tex.Sample(smp, i.uv) * i.col;
}
