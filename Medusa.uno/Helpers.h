#pragma once

#include <array>
#include <numbers>
#include <string>
#include <vector>

#include "imgui/imgui.h"
#include "Config.h"
#include "Vector2D.hpp"

struct Color3;
struct Color4;
struct Vector;
struct Matrix4x4;
enum class RenderTarget
{
    screen,
    window1,
    window2
};
namespace Helpers
{
    Vector calculate_angle(const Vector& src, const Vector& dst);
    bool worldToScreenPixelAligned(const Vector& worldPosition, ImVec2& screenPosition, RenderTarget target = RenderTarget::screen) noexcept;
    void Draw3DFilledCircle(ImDrawList* drawList, const Vector& origin, float radius, ImU32 color) noexcept;
    void AngleVectors(Vector angles, Vector* forward, Vector* right, Vector* up);
    void logConsole(std::string_view msg, const std::array<std::uint8_t, 4> color = { 255, 255, 255, 255 }) noexcept;
    float simpleSpline(float value) noexcept;
    float simpleSplineRemapVal(float val, float A, float B, float C, float D) noexcept;
    float simpleSplineRemapValClamped(float val, float A, float B, float C, float D) noexcept;
    Vector lerp(float percent, Vector a, Vector b) noexcept;
    float lerp(float percent, float a, float b) noexcept;
    float bias(float x, float biasAmt) noexcept;
    bool worldToWindow(const Matrix4x4& matrix, const Vector& worldPosition, ImVec2& screenPosition) noexcept;
    float smoothStepBounds(float edge0, float edge1, float x) noexcept;
    float clampCycle(float clycle) noexcept;
    float approach(float target, float value, float speed) noexcept;
    float approachValueSmooth(float target, float value, float fraction) noexcept;
    float angleDiff(float destAngle, float srcAngle) noexcept;
    Vector approach(Vector target, Vector value, float speed) noexcept;
    float angleNormalize(float angle) noexcept;
    float approachAngle(float target, float value, float speed) noexcept;
    float remapValClamped(float val, float A, float B, float C, float D) noexcept;
    float normalizeYaw(float yaw) noexcept;

    bool worldToScreen(const Vector& in, ImVec2& out, bool floor = false) noexcept;

    unsigned int calculateColor(Color4 color) noexcept;
    unsigned int calculateColor(Color4 color, float alpha) noexcept;
    unsigned int calculateColor(Color3 color) noexcept;
    unsigned int calculateColor(int r, int g, int b, int a) noexcept;
    void setAlphaFactor(float newAlphaFactor) noexcept;
    float getAlphaFactor() noexcept;
    void convertHSVtoRGB(float h, float s, float v, float& outR, float& outG, float& outB) noexcept;
    void healthColor(float fraction, float& outR, float& outG, float& outB) noexcept;
    unsigned int healthColor(float fraction) noexcept;
    void rotate_triangle(std::array<Vector2D, 3>& points, float rotation);

    constexpr auto units2meters(float units) noexcept
    {
        return units * 0.0254f;
    }

    ImWchar* getFontGlyphRanges() noexcept;

    constexpr int utf8SeqLen(char firstByte) noexcept
    {
        return (firstByte & 0x80) == 0x00 ? 1 :
               (firstByte & 0xE0) == 0xC0 ? 2 :
               (firstByte & 0xF0) == 0xE0 ? 3 :
               (firstByte & 0xF8) == 0xF0 ? 4 :
               -1;
    }

    constexpr auto utf8Substr(char* start, char* end, int n) noexcept
    {
        while (start < end && --n)
            start += utf8SeqLen(*start);
        return start;
    }
    float VectorNormalize(Vector v) noexcept;
    std::wstring toWideString(const std::string& str) noexcept;
    std::wstring toUpper(std::wstring str) noexcept;

    bool decodeVFONT(std::vector<char>& buffer) noexcept;
    std::vector<char> loadBinaryFile(const std::string& path) noexcept;

    constexpr auto deg2rad(float degrees) noexcept { return degrees * (std::numbers::pi_v<float> / 180.0f); }
    constexpr auto rad2deg(float radians) noexcept { return radians * (180.0f / std::numbers::pi_v<float>); }
}
