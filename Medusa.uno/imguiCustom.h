#pragma once

#include "imgui/imgui.h"

#include "Config.h"

struct Color4;
struct ColorToggle;
struct ColorToggleRounding;
struct ColorToggleThickness;
struct ColorToggleThicknessRounding;

namespace ImGuiCustom
{
    void colorPicker(const char* name, float color[3], float* alpha = nullptr, bool* rainbow = nullptr, float* rainbowSpeed = nullptr, bool* enable = nullptr, float* thickness = nullptr, float* rounding = nullptr, bool* outline = nullptr) noexcept;
    void checkbox(const char* label, bool* v);
    void colorPicker(const char* name, ColorToggle3& colorConfig) noexcept;
    void colorPicker(const char* name, Color3& colorConfig) noexcept;
    void colorPicker(const char* name, Color4& colorConfig, bool* enable = nullptr, float* thickness = nullptr) noexcept;
    void colorPicker(const char* name, ColorToggle& colorConfig) noexcept;
    void colorPicker(const char* name, ColorToggleOutline& colorConfig, bool* enable = nullptr, float* thickness = nullptr) noexcept;
    void colorPicker(const char* name, ColorToggleRounding& colorConfig) noexcept;
    void colorPicker(const char* name, ColorToggleThickness& colorConfig) noexcept;
    void colorPicker(const char* name, ColorToggleThicknessRounding& colorConfig) noexcept;
    void arrowButtonDisabled(const char* id, ImGuiDir dir) noexcept;
}

class KeyBind;

namespace ImGui
{
    bool SliderScalar2(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags);
    void progressBarFullWidth(float fraction, float height) noexcept;
    void textUnformattedCentered(const char* text) noexcept;
    bool SelectableWithBullet(const char* label, ImU32 bulletColor, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0, 0));
    void hotkey(const char* label, KeyBind& key, float samelineOffset = 0.0f, const ImVec2& size = { 100.0f, 0.0f }) noexcept;
    void hotkey2(const char* label, KeyBind& key, float samelineOffset = 0.0f, const ImVec2& size = { 100.0f, 0.0f }, bool border = false) noexcept;
    void MultiCombo(const char* label, bool combos[], const char* items[], int items_count);
    bool smallButtonFullWidth(const char* label, bool disabled) noexcept;

    bool beginTable(const char* str_id, int columns_count, ImGuiTableFlags flags = 0, const ImVec2& outer_size = ImVec2(0, 0), float inner_width = 0.0f) noexcept;
    bool beginTableEx(const char* name, ImGuiID id, int columns_count, ImGuiTableFlags flags = 0, const ImVec2& outer_size = ImVec2(0, 0), float inner_width = 0.0f) noexcept;

    void textEllipsisInTableCell(const char* text) noexcept;
}