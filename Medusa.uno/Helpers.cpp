#include <cmath>
#include <cwctype>
#include <fstream>
#include <tuple>

#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

#include "Config.h"
#include "ConfigStructs.h"
#include "GameData.h"
#include "Helpers.h"
#include "Memory.h"
#include "Interfaces.h"
#include "SDK/UserCmd.h"
#include "SDK/GlobalVars.h"
#include "SDK/Engine.h"

static auto rainbowColor(float time, float speed, float alpha) noexcept
{
    constexpr float pi = std::numbers::pi_v<float>;
    return std::array{ std::sin(speed * time) * 0.5f + 0.5f,
                       std::sin(speed * time + 2 * pi / 3) * 0.5f + 0.5f,
                       std::sin(speed * time + 4 * pi / 3) * 0.5f + 0.5f,
                       alpha };
}

#define M_RADPI 57.295779513082f
Vector Helpers::calculate_angle(const Vector& src, const Vector& dst) {
    Vector angles;

    Vector delta = src - dst;
    float hyp = delta.length2D();

    angles.y = std::atanf(delta.y / delta.x) * M_RADPI;
    angles.x = std::atanf(-delta.z / hyp) * -M_RADPI;
    angles.z = 0.0f;

    if (delta.x >= 0.0f)
        angles.y += 180.0f;

    return angles;
}


void Helpers::logConsole(std::string_view msg, const std::array<std::uint8_t, 4> color) noexcept
{
    static constexpr int LS_MESSAGE = 0;

    static const auto channelId = memory->findLoggingChannel("Console");

    memory->logDirect(channelId, LS_MESSAGE, color, msg.data());
}

float Helpers::simpleSpline(float value) noexcept
{
    float valueSquared = value * value;

    return (3 * valueSquared - 2 * valueSquared * value);
}

float Helpers::simpleSplineRemapVal(float val, float A, float B, float C, float D) noexcept
{
    if (A == B)
        return val >= B ? D : C;
    float cVal = (val - A) / (B - A);
    return C + (D - C) * simpleSpline(cVal);
}

float Helpers::simpleSplineRemapValClamped(float val, float A, float B, float C, float D) noexcept
{
    if (A == B)
        return val >= B ? D : C;
    float cVal = (val - A) / (B - A);
    cVal = std::clamp(cVal, 0.0f, 1.0f);
    return C + (D - C) * simpleSpline(cVal);
}

Vector Helpers::lerp(float percent, Vector a, Vector b) noexcept
{
    return a + (b - a) * percent;
}

float Helpers::lerp(float percent, float a, float b) noexcept
{
    return a + (b - a) * percent;
}

float Helpers::bias(float x, float biasAmt) noexcept
{
    static float lastAmt = -1;
    static float lastExponent = 0;
    if (lastAmt != biasAmt)
    {
        lastExponent = log(biasAmt) * -1.4427f;
    }
    return pow(x, lastExponent);
}

bool Helpers::worldToWindow(const Matrix4x4& matrix, const Vector& worldPosition, ImVec2& screenPosition) noexcept
{
    auto window = ImGui::GetCurrentWindow();
    if (!window)
        return false;
    const auto w = matrix._41 * worldPosition.x + matrix._42 * worldPosition.y + matrix._43 * worldPosition.z + matrix._44;
    if (w < 0.001f)
        return false;
    screenPosition = window->Size / 2.0f;

    screenPosition.x *= 1.0f + (matrix._11 * worldPosition.x + matrix._12 * worldPosition.y + matrix._13 * worldPosition.z + matrix._14) / w;
    screenPosition.y *= 1.0f - (matrix._21 * worldPosition.x + matrix._22 * worldPosition.y + matrix._23 * worldPosition.z + matrix._24) / w;
    screenPosition += window->Pos;
    return true;
}

float Helpers::smoothStepBounds(float edge0, float edge1, float x) noexcept
{
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return x * x * (3 - 2 * x);
}

float Helpers::clampCycle(float clycle) noexcept
{
    clycle -= (float)(int)clycle;

    if (clycle < 0.0f)
    {
        clycle += 1.0f;
    }
    else if (clycle > 1.0f)
    {
        clycle -= 1.0f;
    }

    return clycle;
}

static bool transformWorldPositionToScreenPosition(const Matrix4x4& matrix, const Vector& worldPosition, ImVec2& screenPosition) noexcept
{
    const auto w = matrix._41 * worldPosition.x + matrix._42 * worldPosition.y + matrix._43 * worldPosition.z + matrix._44;
    if (w < 0.001f)
        return false;

    screenPosition = ImGui::GetIO().DisplaySize / 2.0f;
    screenPosition.x *= 1.0f + (matrix._11 * worldPosition.x + matrix._12 * worldPosition.y + matrix._13 * worldPosition.z + matrix._14) / w;
    screenPosition.y *= 1.0f - (matrix._21 * worldPosition.x + matrix._22 * worldPosition.y + matrix._23 * worldPosition.z + matrix._24) / w;
    return true;
}

bool Helpers::worldToScreenPixelAligned(const Vector& worldPosition, ImVec2& screenPosition, RenderTarget target) noexcept
{
    const bool onScreen = transformWorldPositionToScreenPosition(GameData::toScreenMatrix(), worldPosition, screenPosition);
    screenPosition = ImFloor(screenPosition);
    return onScreen;
}

void Helpers::Draw3DFilledCircle(ImDrawList* drawList, const Vector& origin, float radius, ImU32 color) noexcept
{
    static const auto circumference = [radius]
    {
        std::array<Vector, 72> points;
        for (std::size_t i = 0; i < points.size(); ++i)
        {

            points[i] = Vector{ radius * std::cos(Helpers::deg2rad(i * (360.0f / points.size()))),
                radius * std::sin(Helpers::deg2rad(i * (360.0f / points.size()))),
                0.0f };
        }
        return points;
    }();

    std::array<ImVec2, circumference.size()> groundPoints;
    std::size_t count = 0;

    for (const auto& point : circumference)
    {
        if (worldToScreen(origin + point, groundPoints[count]))
            ++count;
    }

    if (count == 72)
    {
        std::swap(groundPoints[0], *std::min_element(groundPoints.begin(), groundPoints.begin() + count, [](const auto& a, const auto& b) { return a.y < b.y || (a.y == b.y && a.x < b.x); }));

        constexpr auto orientation = [](const ImVec2& a, const ImVec2& b, const ImVec2& c)
        {
            return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
        };

        std::sort(groundPoints.begin() + 1, groundPoints.begin() + count, [&](const auto& a, const auto& b) { return orientation(groundPoints[0], a, b) > 0.0f; });

        //drawList->AddShadowConvexPoly(groundPoints.data(), groundPoints.size(), IM_COL32_BLACK, 20.f, { 0.f,0.f }, ImDrawFlags_ShadowCutOutShapeBackground);
        drawList->AddConvexPolyFilled(groundPoints.data(), groundPoints.size(), color);

        ImVec4 colorv4 = ImGui::ColorConvertU32ToFloat4(color);
        colorv4.w = 1.f;
        drawList->AddPolyline(groundPoints.data(), groundPoints.size(), ImGui::GetColorU32(colorv4), ImDrawFlags_Closed, 1.f);
    }
}

float Helpers::approach(float target, float value, float speed) noexcept
{
    float delta = target - value;

    if (delta > speed)
        value += speed;
    else if (delta < -speed)
        value -= speed;
    else
        value = target;

    return value;
}

float Helpers::approachValueSmooth(float target, float value, float fraction) noexcept
{
    float delta = target - value;
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    delta *= fraction;
    return value + delta;
}

float Helpers::angleDiff(float destAngle, float srcAngle) noexcept
{
    float delta = std::fmodf(destAngle - srcAngle, 360.0f);

    if (destAngle > srcAngle) 
    {
        if (delta >= 180)
            delta -= 360;
    }
    else 
    {
        if (delta <= -180)
            delta += 360;
    }
    return delta;
}

Vector Helpers::approach(Vector target, Vector value, float speed) noexcept
{
    Vector diff = (target - value);

    float delta = diff.length();
    if (delta > speed)
        value += diff.normalized() * speed;
    else if (delta < -speed)
        value -= diff.normalized() * speed;
    else
        value = target;

    return value;
}

float Helpers::angleNormalize(float angle) noexcept
{
    angle = fmodf(angle, 360.0f);

    if (angle > 180.f)
        angle -= 360.f;

    if (angle < -180.f)
        angle += 360.f;

    return angle;
}

float Helpers::VectorNormalize(Vector v) noexcept
{
    float l = v.length();

    if (l != 0.0f)
    {
        v /= l;
    }
    else
    {
        v.x = v.y = 0.0f; v.z = 1.0f;
    }

    return l;
}

void Helpers::AngleVectors(Vector angles, Vector* forward, Vector* right, Vector* up) {
    float angle;
    static float sr, sp, sy, cr, cp, cy, cpi = (M_PI * 2 / 360);

    angle = angles.x * cpi;
    sy = sin(angle);
    cy = cos(angle);
    angle = angles.y * cpi;
    sp = sin(angle);
    cp = cos(angle);
    angle = angles.z * cpi;
    sr = sin(angle);
    cr = cos(angle);

    if (forward) {
        forward->y = (cp * cy);
        forward->x = cp * sy;
        forward->z = -sp;
    }

    if (right) {
        right->y = (-1 * sr * sp * cy + -1 * cr * -sy);
        right->x = (-1 * sr * sp * sy + -1 * cr * cy);
        right->z = -1 * sr * cp;
    }

    if (up) {
        up->y = (cr * sp * cy + -sr * -sy);
        up->x = (cr * sp * sy + -sr * cy);
        up->z = cr * cp;
    }
}
float Helpers::approachAngle(float target, float value, float speed) noexcept
{
    auto anglemod = [](float a)
    {
        a = (360.f / 65536) * ((int)(a * (65536.f / 360.0f)) & 65535);
        return a;
    };
    target = anglemod(target);
    value = anglemod(value);

    float delta = target - value;

    if (speed < 0)
        speed = -speed;

    if (delta < -180)
        delta += 360;
    else if (delta > 180)
        delta -= 360;

    if (delta > speed)
        value += speed;
    else if (delta < -speed)
        value -= speed;
    else
        value = target;

    return value;
}

float Helpers::remapValClamped(float val, float A, float B, float C, float D) noexcept
{
    if (A == B)
        return val >= B ? D : C;
    float cVal = (val - A) / (B - A);
    cVal = std::clamp(cVal, 0.0f, 1.0f);

    return C + (D - C) * cVal;
}

float Helpers::normalizeYaw(float yaw) noexcept
{
    if (!std::isfinite(yaw))
        return 0.0f;

    if (yaw >= -180.f && yaw <= 180.f)
        return yaw;

    const float rot = std::round(std::abs(yaw / 360.f));

    yaw = (yaw < 0.f) ? yaw + (360.f * rot) : yaw - (360.f * rot);
    return yaw;
}

bool Helpers::worldToScreen(const Vector& in, ImVec2& out, bool floor) noexcept
{
    const auto& matrix = GameData::toScreenMatrix();

    const auto w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;
    if (w < 0.001f)
        return false;

    out = ImGui::GetIO().DisplaySize / 2.0f;
    out.x *= 1.0f + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w;
    out.y *= 1.0f - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w;
    if (floor)
        out = ImFloor(out);
    return true;
}

static float alphaFactor = 1.0f;

unsigned int Helpers::calculateColor(Color4 color) noexcept
{
    color.color[3] *= alphaFactor;
    return ImGui::ColorConvertFloat4ToU32(color.rainbow ? rainbowColor(memory->globalVars->realtime, color.rainbowSpeed, color.color[3]) : color.color);
}

unsigned int Helpers::calculateColor(Color4 color, float alpha) noexcept
{
    color.color[3] *= alphaFactor;
    return ImGui::ColorConvertFloat4ToU32(color.rainbow ? rainbowColor(memory->globalVars->realtime, color.rainbowSpeed, alpha) : ImVec4{ color.color[0], color.color[1], color.color[2], alpha });
}

unsigned int Helpers::calculateColor(Color3 color) noexcept
{
    return ImGui::ColorConvertFloat4ToU32(color.rainbow ? rainbowColor(memory->globalVars->realtime, color.rainbowSpeed, 1.0f) : ImVec4{ color.color[0], color.color[1], color.color[2], 1.0f});
}

unsigned int Helpers::calculateColor(int r, int g, int b, int a) noexcept
{
    a -= static_cast<int>(a * GameData::local().flashDuration / 255.0f);
    return IM_COL32(r, g, b, a * alphaFactor);
}
void Helpers::setAlphaFactor(float newAlphaFactor) noexcept
{
    alphaFactor = newAlphaFactor;
}

float Helpers::getAlphaFactor() noexcept
{
    return alphaFactor;
}

void Helpers::convertHSVtoRGB(float h, float s, float v, float& outR, float& outG, float& outB) noexcept
{
    ImGui::ColorConvertHSVtoRGB(h, s, v, outR, outG, outB);
}

void Helpers::healthColor(float fraction, float& outR, float& outG, float& outB) noexcept
{
    constexpr auto greenHue = 1.0f / 3.0f;
    constexpr auto redHue = 0.0f;
    convertHSVtoRGB(std::lerp(redHue, greenHue, fraction), 1.0f, 1.0f, outR, outG, outB);
}

unsigned int Helpers::healthColor(float fraction) noexcept
{
    float r, g, b;
    healthColor(fraction, r, g, b);
    return calculateColor(static_cast<int>(r * 255.0f), static_cast<int>(g * 255.0f), static_cast<int>(b * 255.0f), 255);
}

void Helpers::rotate_triangle(std::array<Vector2D, 3>& points, float rotation)
{
    const auto pointsCenter = (points.at(0) + points.at(1) + points.at(2)) / 3;
    for (auto& point : points)
    {
        point -= pointsCenter;

        const auto tempX = point.x;
        const auto tempY = point.y;

        const auto theta = deg2rad(rotation);
        const auto c = cos(theta);
        const auto s = sin(theta);

        point.x = tempX * c - tempY * s;
        point.y = tempX * s + tempY * c;

        point += pointsCenter;
    }
}

ImWchar* Helpers::getFontGlyphRanges() noexcept
{
    static ImVector<ImWchar> ranges;
    if (ranges.empty()) {
        ImFontGlyphRangesBuilder builder;
        constexpr ImWchar baseRanges[]{
            0x0100, 0x024F, // Latin Extended-A + Latin Extended-B
            0x0300, 0x03FF, // Combining Diacritical Marks + Greek/Coptic
            0x0600, 0x06FF, // Arabic
            0x0E00, 0x0E7F, // Thai
            0
        };
        builder.AddRanges(baseRanges);
        builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
        builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
        builder.AddText("\u9F8D\u738B\u2122");
        builder.BuildRanges(&ranges);
    }
    return ranges.Data;
}

std::wstring Helpers::toWideString(const std::string& str) noexcept
{
    std::wstring upperCase(str.length(), L'\0');
    if (const auto newLen = std::mbstowcs(upperCase.data(), str.c_str(), upperCase.length()); newLen != static_cast<std::size_t>(-1))
        upperCase.resize(newLen);
    return upperCase;
}

std::wstring Helpers::toUpper(std::wstring str) noexcept
{
    std::transform(str.begin(), str.end(), str.begin(), [](wchar_t w) -> wchar_t {
        if (w >= 0 && w <= 127) {
            if (w >= 'a' && w <= 'z')
                return w - ('a' - 'A');
            return w;
        }

        return std::towupper(w);
    });
    return str;
}

bool Helpers::decodeVFONT(std::vector<char>& buffer) noexcept
{
    constexpr std::string_view tag = "VFONT1";
    unsigned char magic = 0xA7;

    if (buffer.size() <= tag.length())
        return false;

    const auto tagIndex = buffer.size() - tag.length();
    if (std::memcmp(tag.data(), &buffer[tagIndex], tag.length()))
        return false;

    unsigned char saltBytes = buffer[tagIndex - 1];
    const auto saltIndex = tagIndex - saltBytes;
    --saltBytes;

    for (std::size_t i = 0; i < saltBytes; ++i)
        magic ^= (buffer[saltIndex + i] + 0xA7) % 0x100;

    for (std::size_t i = 0; i < saltIndex; ++i) {
        unsigned char xored = buffer[i] ^ magic;
        magic = (buffer[i] + 0xA7) % 0x100;
        buffer[i] = xored;
    }

    buffer.resize(saltIndex);
    return true;
}

std::vector<char> Helpers::loadBinaryFile(const std::string& path) noexcept
{
    std::vector<char> result;
    std::ifstream in{ path, std::ios::binary };
    if (!in)
        return result;
    in.seekg(0, std::ios_base::end);
    result.resize(static_cast<std::size_t>(in.tellg()));
    in.seekg(0, std::ios_base::beg);
    in.read(result.data(), result.size());
    return result;
}
