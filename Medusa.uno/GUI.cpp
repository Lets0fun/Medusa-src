#include <array>
#include <cwctype>
#include <fstream>
#include <functional>
#include <string>
#include <ShlObj.h>
#include <Windows.h>
#include <string>
#include <iostream>
#include "xor.h"
#include <cstring>
#include <fstream>
#include <map>
#include <UrlMon.h>
#include <shellapi.h>
#include "postprocessing.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_win32.h"
#include "font.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_stdlib.h"
#include "imguiCustom.h"
#include "snowflakes.hpp"
#include "Hacks/AntiAim.h"
#include "Hacks/Backtrack.h"
#include "Hacks/Glow.h"
#include "Hacks/Misc.h"
#include "SkinChanger.h"
#include "Hacks/Visuals.h"
#include "Texture.h"
#include "GUI.h"
#include "Config.h"
#include "Helpers.h"
#include "Hooks.h"
#include "Interfaces.h"
#include "../Medusa.uno/SDK/Surface.h"
#include "Hacks\Resolver.h"
#include <ctype.h> 
#include <stdint.h> 
#include <cmath>
#include <iostream>
#include <string>
#include "SDK/InputSystem.h"
#include <algorithm>
inline constexpr float animationLength() { return 0.45f; }
float toggleAnimationEnd = 0.0f;
float getTransparency() noexcept 
{ 
    return std::clamp(gui->isOpen() ? toggleAnimationEnd : 0.f, 0.0f, 1.0f);
}
constexpr auto windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
static int activeTab = 1;

static ImFont* addFontFromVFONT(const std::string& path, float size, const ImWchar* glyphRanges, bool merge) noexcept
{
    auto file = Helpers::loadBinaryFile(path);
    if (!Helpers::decodeVFONT(file))
        return nullptr;

    ImFontConfig cfg;
    cfg.FontData = file.data();
    cfg.FontDataSize = file.size();
    cfg.FontDataOwnedByAtlas = false;
    cfg.MergeMode = merge;
    cfg.GlyphRanges = glyphRanges;
    cfg.SizePixels = size;

    return ImGui::GetIO().Fonts->AddFont(&cfg);
}

GUI::GUI() noexcept
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScrollbarSize = 11.5f;

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange | ImFontAtlasFlags_NoBakedLines;

    ImFontConfig font_config;
    font_config.PixelSnapH = false;
    font_config.OversampleH = 5;
    font_config.OversampleV = 5;
    font_config.RasterizerMultiply = 1.2f;
    ImFontConfig font_config2;
    font_config.PixelSnapH = false;
    font_config.OversampleH = 5;
    font_config.OversampleV = 5;
    font_config.RasterizerMultiply = 1.2f;
    font_config2.SizePixels = 12.0f;
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0xE000, 0xE226, // icons
        0,
    };

    font_config.GlyphRanges = ranges;
    ImFontConfig cfg;
    cfg.SizePixels = 15.0f;
    ImFontConfig cfg1;
    cfg1.SizePixels = 14.0f;
    ImFontConfig cfg2;
    cfg2.SizePixels = 12.0f;
    if (PWSTR pathToFonts; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &pathToFonts))) {
        const std::filesystem::path path{ pathToFonts };
        CoTaskMemFree(pathToFonts);

        fonts.normal15px = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 15.0f, &cfg, Helpers::getFontGlyphRanges());
        if (!fonts.normal15px)
            io.Fonts->AddFontDefault(&cfg);

        fonts.tahoma34 = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 34.0f, &cfg, Helpers::getFontGlyphRanges());
        if (!fonts.tahoma34)
            io.Fonts->AddFontDefault(&cfg);

        fonts.tahoma24 = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 22.0f, &cfg, Helpers::getFontGlyphRanges());
        if (!fonts.tahoma24)
            io.Fonts->AddFontDefault(&cfg);
        fonts.tahoma16 = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 16.0f, &cfg, Helpers::getFontGlyphRanges());
        if (!fonts.tahoma16)
            io.Fonts->AddFontDefault(&cfg);
        fonts.tahoma9 = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 9.0f, &cfg, Helpers::getFontGlyphRanges());
        if (!fonts.tahoma9)
            io.Fonts->AddFontDefault(&cfg);
        fonts.tab_ico = io.Fonts->AddFontFromMemoryTTF((void*)main_icon, sizeof(main_icon), 20.0f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
        if (!fonts.tab_ico)
            io.Fonts->AddFontDefault(&cfg);
        fonts.tahoma28 = io.Fonts->AddFontFromFileTTF((path / "tahomabd.ttf").string().c_str(), 28.0f, &cfg, Helpers::getFontGlyphRanges());
        if (!fonts.tahoma28)
            io.Fonts->AddFontDefault(&cfg);
        //fonts.smallfonts = io.Fonts->AddFontFromFileTTF((path / "smalle.fon").string().c_str(), 9.0f, &cfg1, Helpers::getFontGlyphRanges());
        fonts.smallfonts = io.Fonts->AddFontFromMemoryTTF((void*)smallestcock, sizeof(smallestcock), 12.0f, &cfg1, io.Fonts->GetGlyphRangesCyrillic());
        fonts.weaponIcons = io.Fonts->AddFontFromMemoryTTF((void*)weaponIcon, sizeof(weaponIcon), 15.0f, &cfg1, io.Fonts->GetGlyphRangesCyrillic());
        fonts.fIcons = io.Fonts->AddFontFromMemoryTTF(iconsbyztechnology, sizeof(iconsbyztechnology), 20.0f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
        fonts.logo = io.Fonts->AddFontFromMemoryTTF(main_logo, sizeof(main_logo), 20.0f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
        fonts.logoBig = io.Fonts->AddFontFromMemoryTTF(main_logo, sizeof(main_logo), 50.0f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
        fonts.nazi = io.Fonts->AddFontFromMemoryTTF(nazilol, sizeof(nazilol), 30.0f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
        fonts.nades = io.Fonts->AddFontFromMemoryTTF(grenadesFont, sizeof(grenadesFont), 20.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
        fonts.espFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", 12.0f, &font_config);
        
        io.Fonts->AddFontDefault(&cfg);
        cfg.MergeMode = true;
        static constexpr ImWchar symbol[]{
            0x2605, 0x2605, // ★
            0
        };
        fonts.verdana = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", 13.0f, &font_config, ranges);
        fonts.verdana11 = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", 12.0f, &font_config);
        cfg.MergeMode = false;
    }

    if (!fonts.normal15px)
        io.Fonts->AddFontDefault(&cfg);
    if (!fonts.verdana)
        io.Fonts->AddFontDefault(&cfg);
    if (!fonts.tahoma34)
        io.Fonts->AddFontDefault(&cfg);
    if (!fonts.tahoma28)
        io.Fonts->AddFontDefault(&cfg);
    addFontFromVFONT("csgo/panorama/fonts/notosanskr-regular.vfont", 15.0f, io.Fonts->GetGlyphRangesKorean(), true);
    addFontFromVFONT("csgo/panorama/fonts/notosanssc-regular.vfont", 15.0f, io.Fonts->GetGlyphRangesChineseFull(), true);
    constexpr auto unicodeFontSize = 16.0f;
    fonts.unicodeFont = addFontFromVFONT("csgo/panorama/fonts/notosans-bold.vfont", unicodeFontSize, Helpers::getFontGlyphRanges(), false);
}

ImFont* GUI::weaponIcons() const noexcept
{
    return fonts.weaponIcons;
}

ImFont* GUI::getTahoma28Font() const noexcept
{
    return fonts.tahoma28;
}

ImFont* GUI::indicators() const noexcept
{
    return fonts.indicators;
}

ImFont* GUI::ver11() const noexcept
{
    return fonts.verdana11;
}

ImFont* GUI::grenades() const noexcept
{
    return fonts.nades;
}

ImFont* GUI::getConsolas10Font() const noexcept
{
    return fonts.smallfonts;
}

ImFont* GUI::espFont() const noexcept
{
    return fonts.espFont;
}

ImFont* GUI::getTabIcoFont() const noexcept
{
    return fonts.tab_ico;
}

ImFont* GUI::getVerdanaFont() const noexcept
{
    return fonts.nazi;
}

ImFont* GUI::getFIconsFont() const noexcept
{
    return fonts.verdana;
}

ImFont* GUI::WiconsFonts() const noexcept
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig cfg;
    cfg.SizePixels = 12.0f;
    return io.Fonts->AddFontFromMemoryTTF((void*)weaponicons, sizeof(weaponicons), 14.0f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
}

void AddRadialGradient(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col_in, ImU32 col_out)
{
    if (((col_in | col_out) & IM_COL32_A_MASK) == 0 || radius < 0.5f)
        return;

    // Use arc with automatic segment count
    draw_list->_PathArcToFastEx(center, radius, 0, IM_DRAWLIST_ARCFAST_SAMPLE_MAX, 0);
    const int count = draw_list->_Path.Size - 1;

    unsigned int vtx_base = draw_list->_VtxCurrentIdx;
    draw_list->PrimReserve(count * 3, count + 1);

    // Submit vertices
    const ImVec2 uv = draw_list->_Data->TexUvWhitePixel;
    draw_list->PrimWriteVtx(center, uv, col_in);
    for (int n = 0; n < count; n++)
        draw_list->PrimWriteVtx(draw_list->_Path[n], uv, col_out);

    // Submit a fan of triangles
    for (int n = 0; n < count; n++)
    {
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base));
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + n));
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((n + 1) % count)));
    }
    draw_list->_Path.Size = 0;
}

void GUI::render() noexcept
{
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, getTransparency());
    if (open && toggleAnimationEnd < 1.0f)
        ImGui::SetWindowFocus();

    if (open)
        toggleAnimationEnd += ImGui::GetIO().DeltaTime / animationLength();

    renderGuiStyle();

    //AddRadialGradient(ImGui::GetBackgroundDrawList(), ImGui::GetIO().MousePos, 30.f, Helpers::calculateColor(config->menu.accentColor, 0.75f), Helpers::calculateColor(config->menu.accentColor, 0.0f));
    ImGui::PopStyleVar();
}

#include "InputUtil.h"
#include "fontawesome.h"

static void hotkey3(const char* label, KeyBind& key, float samelineOffset = 0.0f, const ImVec2& size = { 100.0f, 0.0f }) noexcept
{
    const auto id = ImGui::GetID(label);
    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(samelineOffset);

    if (ImGui::GetActiveID() == id) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
        ImGui::Button(skCrypt("..."), size);
        ImGui::PopStyleColor();

        ImGui::GetCurrentContext()->ActiveIdAllowOverlap = true;
        if ((!ImGui::IsItemHovered() && ImGui::GetIO().MouseClicked[0]) || key.setToPressedKey())
        {
            ImGui::ClearActiveID();
            hooks->input_listen = true;
        }
    }
    else if (ImGui::Button(key.toString(), size)) {
        hooks->input_listen = true;
        ImGui::SetActiveID(id, ImGui::GetCurrentWindow());
    }

    ImGui::PopID();
}

ImFont* GUI::getUnicodeFont() const noexcept
{
    return fonts.unicodeFont;
}

void GUI::handleToggle() noexcept
{
    if (config->misc.menuKey.isPressed()) {
        open = !open;
        if (!open)
            interfaces->inputSystem->resetInputState();

        if (toggleAnimationEnd > 0.0f && toggleAnimationEnd < 1.0f)
            toggleAnimationEnd = 1.0f - toggleAnimationEnd;
        else
            toggleAnimationEnd = 0.0f;
        static auto filtererrors = interfaces->cvar->findVar(skCrypt("con_filter_enable"));
        filtererrors->setValue(1);
        static auto filtererrors1 = interfaces->cvar->findVar(skCrypt("con_filter_text_out"));
        filtererrors1->setValue(skCrypt("MATERIAL"));
        static auto filtererrors2 = interfaces->cvar->findVar(skCrypt("con_filter_text_out"));
        filtererrors2->setValue(skCrypt("MATERIAL"));
        static auto fpsMax = interfaces->cvar->findVar(skCrypt("fps_max"));
        fpsMax->setValue(0);
    }
}

static void menuBarItem(const char* name, bool& enabled) noexcept
{
    if (ImGui::MenuItem(name)) {
        enabled = true;
        ImGui::SetWindowFocus(name);
        ImGui::SetWindowPos(name, { 100.0f, 100.0f });
    }
}
void renderLegitBotWindow() noexcept
{
    static const char* hitboxes[]{ "Head","Chest","Stomach","Arms","Legs" };
    static bool hitbox[ARRAYSIZE(hitboxes)] = { false, false, false, false, false };
    static std::string previewvalue = "";
    bool once = false;

    static int currentWeapon{ 0 };
    /* {
        ImGui::Columns(2);
        ImGui::SetCursorPosY(5);
        ImGui::BeginChildFrame(0, { 308, 460 });
        ImGui::EndChildFrame();
        ImGui::SetColumnOffset(1, 317.5);
        ImGui::NextColumn();
        ImGui::SetCursorPosY(5);
        ImGui::BeginChildFrame(1, { 308, 460 });
        ImGui::EndChildFrame();
    }*/
    ImGui::PushID(skCrypt("Key"));
    ImGui::Checkbox(skCrypt("Enable Legitbot"), &config->lgb.enabled);
    ImGui::SameLine();
    ImGui::hotkey2(skCrypt("On Key"), config->legitbotKey, 0.0f, {100.f, 0.f});
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::Columns(2, nullptr, false);
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(100.0f);
    ImGui::PushID(0);
    ImGui::SetCursorPosX(8);
    ImGui::Combo("", &currentCategory, skCrypt("Global\0Pistols\0Heavy\0SMG\0Rifles\0"), -1 , false);
    ImGui::PopID();
    ImGui::SameLine();

    ImGui::PushID(1);

    switch (currentCategory) {
    case 0:
        currentWeapon = 0;
        ImGui::NewLine();
        break;
    case 1: {
        static int currentPistol{ 0 };
        static constexpr const char* pistols[]{ "All", "Glock-18", "P2000", "USP-S", "Dual Berettas", "P250", "Tec-9", "Five-Seven", "CZ-75", "Desert Eagle", "Revolver" };

        ImGui::Combo("", &currentPistol, [](void* data, int idx, const char** out_text) {
            if (config->legitbob[idx ? idx : 35].override) {
                static std::string name;
                name = pistols[idx];
                *out_text = name.append(" *").c_str();
            }
            else {
                *out_text = pistols[idx];
            }
        return true;
            }, nullptr, IM_ARRAYSIZE(pistols), -1, false);

        currentWeapon = currentPistol ? currentPistol : 35;
        break;
    }
    case 2: {
        static int currentHeavy{ 0 };
        static constexpr const char* heavies[]{ "All", "Nova", "XM1014", "Sawed-off", "MAG-7", "M249", "Negev" };

        ImGui::Combo("", &currentHeavy, [](void* data, int idx, const char** out_text) {
            if (config->legitbob[idx ? idx + 10 : 36].override) {
                static std::string name;
                name = heavies[idx];
                *out_text = name.append(" *").c_str();
            }
            else {
                *out_text = heavies[idx];
            }
        return true;
            }, nullptr, IM_ARRAYSIZE(heavies), -1, false);

        currentWeapon = currentHeavy ? currentHeavy + 10 : 36;
        break;
    }
    case 3: {
        static int currentSmg{ 0 };
        static constexpr const char* smgs[]{ "All", "Mac-10", "MP9", "MP7", "MP5-SD", "UMP-45", "P90", "PP-Bizon" };

        ImGui::Combo("", &currentSmg, [](void* data, int idx, const char** out_text) {
            if (config->legitbob[idx ? idx + 16 : 37].override) {
                static std::string name;
                name = smgs[idx];
                *out_text = name.append(" *").c_str();
            }
            else {
                *out_text = smgs[idx];
            }
        return true;
            }, nullptr, IM_ARRAYSIZE(smgs), -1, false);

        currentWeapon = currentSmg ? currentSmg + 16 : 37;
        break;
    }
    case 4: {
        static int currentRifle{ 0 };
        static constexpr const char* rifles[]{ "All", "Galil AR", "Famas", "AK-47", "M4A4", "M4A1-S", "SSG-08", "SG-553", "AUG", "AWP", "G3SG1", "SCAR-20" };

        ImGui::Combo("", &currentRifle, [](void* data, int idx, const char** out_text) {
            if (config->legitbob[idx ? idx + 23 : 38].override) {
                static std::string name;
                name = rifles[idx];
                *out_text = name.append(" *").c_str();
            }
            else {
                *out_text = rifles[idx];
            }
        return true;
            }, nullptr, IM_ARRAYSIZE(rifles), -1, false);

        currentWeapon = currentRifle ? currentRifle + 23 : 38;
        break;
    }
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(220);
    ImGui::Checkbox(skCrypt("Aimlock"), &config->lgb.aimlock);
    ImGui::Checkbox(skCrypt("Friendly fire"), &config->lgb.friendlyFire);
    ImGui::Checkbox(skCrypt("Visible only"), &config->lgb.visibleOnly);
    ImGui::Checkbox(skCrypt("Scoped only"), &config->lgb.scopedOnly);
    ImGui::Checkbox(skCrypt("While flashed"), &config->lgb.ignoreFlash);
    ImGui::Checkbox(skCrypt("Through smoke"), &config->lgb.ignoreSmoke);
    ImGuiCustom::colorPicker(c_xor("Draw FOV"), config->lgb.legitbotFov);
    ImGui::Checkbox(skCrypt("Kill shot"), &config->lgb.killshot);
    ImGui::Checkbox(skCrypt("Triggerbot"), &config->lgb.enableTriggerbot);
    if (config->lgb.enableTriggerbot)
    {
        ImGui::SameLine();
        ImGui::PushID("smthsafaf");
        ImGui::hotkey2("", config->triggerbotKey);
        ImGui::PopID();
    }
    ImGui::Checkbox(skCrypt("Backtrack"), &config->backtrack.enabled);
    if (config->backtrack.enabled)
    {
        ImGui::SliderInt(skCrypt("Time limit"), &config->backtrack.timeLimit, 1, 200, "%d ms");
    }
    ImGui::PopItemWidth();
    ImGui::NextColumn();
    ImGui::SetColumnOffset(1, 230);
    ImGui::PushItemWidth(220);
    ImGui::Checkbox(skCrypt("Override config"), &config->legitbob[currentWeapon].override);
    ImGui::SliderFloat(skCrypt("Fov"), &config->legitbob[currentWeapon].fov, 0.f, 25.f, "%.2f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat(skCrypt("Smoothnes"), &config->legitbob[currentWeapon].smooth, 1.f, 20.f, "%.2f");
    ImGui::SliderInt(skCrypt("Reaction time"), &config->legitbob[currentWeapon].reactionTime, 0, 300, "%d ms");
    ImGui::SliderInt(skCrypt("Triggerbot Hitchance"), &config->legitbob[currentWeapon].hitchanceT, 0, 100, "%d");
    ImGui::SliderInt(skCrypt("Minimum damage"), &config->legitbob[currentWeapon].minDamage, 0, 101, "%d");
    //hitboxes
    for (size_t i = 0; i < ARRAYSIZE(hitbox); i++)
    {
        hitbox[i] = (config->legitbob[currentWeapon].hitboxes & 1 << i) == 1 << i;
    }
    if (ImGui::BeginCombo(skCrypt("Hitbox"), previewvalue.c_str()))
    {
        previewvalue = "";
        for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
        {
            ImGui::Selectable(hitboxes[i], &hitbox[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
        }
        ImGui::EndCombo();
    }
    for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
    {
        if (!once)
        {
            previewvalue = "";
            once = true;
        }
        if (hitbox[i])
        {
            previewvalue += previewvalue.size() ? std::string(", ") + hitboxes[i] : hitboxes[i];
            config->legitbob[currentWeapon].hitboxes |= 1 << i;
        }
        else
        {
            config->legitbob[currentWeapon].hitboxes &= ~(1 << i);
        }
    }
    ImGui::Checkbox(skCrypt("Aim between shots"), &config->legitbob[currentWeapon].betweenShots);
    ImGui::Checkbox(skCrypt("Recoil control system"), &config->recoilControlSystem[currentWeapon].enabled);
    if (config->recoilControlSystem[currentWeapon].enabled)
    {
        ImGui::SameLine();
        ImGui::Checkbox(skCrypt("Silent"), &config->recoilControlSystem[currentWeapon].silent);
        ImGui::SliderInt(skCrypt("RCS Ignore Shots"), &config->recoilControlSystem[currentWeapon].shotsFired, 0, 150, "%d");
        ImGui::SliderFloat(skCrypt("RCS Horizontal"), &config->recoilControlSystem[currentWeapon].horizontal, 0.0f, 1.0f, "%.5f");
        ImGui::SliderFloat(skCrypt("RCS Vertical"), &config->recoilControlSystem[currentWeapon].vertical, 0.0f, 1.0f, "%.5f");
    }
    ImGui::PopItemWidth();
}
void ActiveTab1() {
    ImGui::PushStyleColor(ImGuiCol_Text, Helpers::calculateColor(config->menu.accentColor));
    ImGui::PushStyleColor(ImGuiCol_Button, { Helpers::calculateColor(config->menu.accentColor, 0.35f) });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { Helpers::calculateColor(config->menu.accentColor, 0.35f) });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { Helpers::calculateColor(config->menu.accentColor, 0.35f) });
}

void InactiveTab1() {
    ImGui::PushStyleColor(ImGuiCol_Text, { 255.f, 255.f, 255.f, 255.f });
    ImGui::PushStyleColor(ImGuiCol_Button, { 0.06f, 0.06f, 0.06f, 1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.06f, 0.06f, 0.06f, 1.f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.06f, 0.06f, 0.06f, 1.f });
}
static int RcurrentCategory{ 0 };
void renderRageBotWindow(ImDrawList* drawList) noexcept
{
    if (activeTab != 2)
        return;

    static const char* hitboxes[]{ "Head", "Upper Chest", "Chest", "Lower Chest", "Stomach", "Pelvis", "Arms", "Legs", "Feet" };
    static bool hitbox[ARRAYSIZE(hitboxes)] = { false, false, false, false, false, false, false, false };
    static std::string previewvalue = "";
    bool once = false;

    ImGui::Checkbox(skCrypt("Enable ragebot"), &config->ragebot.enabled);
    ImGui::SameLine();
    ImGui::PushID(skCrypt("smth Key"));
    ImGui::hotkey2(skCrypt("On Key"), config->ragebotKey);
    ImGui::PopID();
    ImGui::Separator();
    ImGui::PushItemWidth(110.0f);
    ImGui::BeginChild(skCrypt("Ragebot weapons"), ImVec2{600, 30}, false);
    {
        if (activeTab == 2)
        {
            ImGui::SetCursorPosX(10);
            if (RcurrentCategory == 0) ActiveTab1(); else InactiveTab1();
            if (ImGui::Button1(skCrypt("All"), ImVec2{ 40,30 })) RcurrentCategory = 0;
            ImGui::PopStyleColor(4);
            ImGui::SameLine();
            ImGui::PushFont(gui->weaponIcons());
            if (RcurrentCategory == 1) ActiveTab1(); else InactiveTab1();
            if (ImGui::Button1(skCrypt("D"), ImVec2{ 50,30 })) RcurrentCategory = 1;
            ImGui::PopStyleColor(4);
            ImGui::SameLine();
            if (RcurrentCategory == 2) ActiveTab1(); else InactiveTab1();
            if (ImGui::Button1(skCrypt("A"), ImVec2{ 50,30 })) RcurrentCategory = 2;
            ImGui::PopStyleColor(4);
            ImGui::SameLine();
            if (RcurrentCategory == 3) ActiveTab1(); else InactiveTab1();
            if (ImGui::Button1(skCrypt("J"), ImVec2{ 50,30 })) RcurrentCategory = 3;
            ImGui::PopStyleColor(4);
            ImGui::SameLine();
            if (RcurrentCategory == 4) ActiveTab1(); else InactiveTab1();
            if (ImGui::Button1(skCrypt("L"), ImVec2{ 50,30 })) RcurrentCategory = 4;
            ImGui::PopStyleColor(4);
            ImGui::SameLine();
            if (RcurrentCategory == 5) ActiveTab1(); else InactiveTab1();
            if (ImGui::Button1(skCrypt("W"), ImVec2{ 50,30 })) RcurrentCategory = 5;
            ImGui::PopStyleColor(4);
            ImGui::SameLine();
            if (RcurrentCategory == 6) ActiveTab1(); else InactiveTab1();
            if (ImGui::Button1(skCrypt("a"), ImVec2{ 58,30 })) RcurrentCategory = 6;
            ImGui::PopStyleColor(4);
            ImGui::SameLine();
            if (RcurrentCategory == 7) ActiveTab1(); else InactiveTab1();
            if (ImGui::Button1(skCrypt("Z"), ImVec2{ 60,30 })) RcurrentCategory = 7;
            ImGui::PopStyleColor(4);
            ImGui::SameLine();
            if (RcurrentCategory == 8) ActiveTab1(); else InactiveTab1();
            if (ImGui::Button1(skCrypt("Y"), ImVec2{ 50,30 })) RcurrentCategory = 8;
            ImGui::PopStyleColor(4);
            ImGui::SameLine(); 
            if (RcurrentCategory == 9) ActiveTab1(); else InactiveTab1();
            if (ImGui::Button1(skCrypt("h"), ImVec2{ 50,30 })) RcurrentCategory = 9;
            ImGui::PopStyleColor(4);
            ImGui::PopFont();
        }
    }
    ImGui::EndChild();
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 277.0f);
    ImGui::PushItemWidth(190.0f);
    ImGui::Checkbox(skCrypt("Silent"), &config->ragebot.silent);
    ImGui::Checkbox(skCrypt("Friendly fire"), &config->ragebot.friendlyFire);
    ImGui::Checkbox(skCrypt("Auto shot"), &config->ragebot.autoShot);
    ImGui::Checkbox(skCrypt("Auto scope"), &config->ragebot.autoScope);
    ImGui::Checkbox(skCrypt("Auto stop"), &config->rageBot[RcurrentCategory].autoStop);
    static bool multi[4] = { false, false, false, false };
    const char* multicombo_items[] = { "Force accuracy", "Between shots", "Full stop", "Duck" };
    static std::string previewvalue1 = "";
    bool once1 = false;
    for (size_t i = 0; i < ARRAYSIZE(multi); i++)
    {
        multi[i] = (config->rageBot[RcurrentCategory].autoStopMod & 1 << i) == 1 << i;
    }
    if (ImGui::BeginCombo(c_xor("Auto stop modifiers"), previewvalue1.c_str()))
    {
        previewvalue1 = "";
        for (size_t i = 0; i < ARRAYSIZE(multicombo_items); i++)
        {
            ImGui::Selectable(multicombo_items[i], &multi[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
        }
        ImGui::EndCombo();
    }
    for (size_t i = 0; i < ARRAYSIZE(multicombo_items); i++)
    {
        if (!once1)
        {
            previewvalue1 = "";
            once1 = true;
        }
        if (multi[i])
        {
            previewvalue1 += previewvalue1.size() ? std::string(", ") + multicombo_items[i] : multicombo_items[i];
            config->rageBot[RcurrentCategory].autoStopMod |= 1 << i;
        }
        else
        {
            config->rageBot[RcurrentCategory].autoStopMod &= ~(1 << i);
        }
    }
    ImGui::Checkbox(skCrypt("Telepeek"), &config->tickbase.teleport);
    ImGui::Checkbox(skCrypt("Knifebot"), &config->ragebot.knifebot);
    ImGui::Combo(skCrypt("Priority"), &config->ragebot.priority, skCrypt("Health\0Distance\0Fov\0"));

    for (size_t i = 0; i < ARRAYSIZE(hitbox); i++)
    {
        hitbox[i] = (config->rageBot[RcurrentCategory].hitboxes & 1 << i) == 1 << i;
    }
    if (ImGui::BeginCombo(c_xor("Hitboxes"), previewvalue.c_str()))
    {
        previewvalue = "";
        for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
        {
            ImGui::Selectable(hitboxes[i], &hitbox[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
        }
        ImGui::EndCombo();
    }
    for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
    {
        if (!once)
        {
            previewvalue = "";
            once = true;
        }
        if (hitbox[i])
        {
            previewvalue += previewvalue.size() ? std::string(", ") + hitboxes[i] : hitboxes[i];
            config->rageBot[RcurrentCategory].hitboxes |= 1 << i;
        }
        else
        {
            config->rageBot[RcurrentCategory].hitboxes &= ~(1 << i);
        }
    }
    ImGui::hotkey2(skCrypt("Force baim"), config->forceBaim);
    ImGui::Text(skCrypt("Exploits"));
    ImGui::PushID(skCrypt("Doubletap"));
    ImGui::hotkey2(skCrypt("Doubletap"), config->tickbase.doubletap);
    ImGui::PopID();
    ImGui::PushID("Hideshots");
    ImGui::hotkey2(skCrypt("Hideshots"), config->tickbase.hideshots);
    ImGui::PopID();
    ImGui::NextColumn();
    ImGui::Checkbox(skCrypt("Override config"), &config->rageBot[RcurrentCategory].enabled);
    ImGui::PushItemWidth(200.0f);
    ImGui::SliderFloat(skCrypt("Fov"), &config->ragebot.fov, 0.0f, 255.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderInt(skCrypt("Hitchance"), &config->rageBot[RcurrentCategory].hitChance, 0, 100, "%d");
    ImGui::SliderInt(skCrypt("Multipoint head"), &config->rageBot[RcurrentCategory].multiPointHead, 0, 100, "%d");
    ImGui::SliderInt(skCrypt("Multipoint body"), &config->rageBot[RcurrentCategory].multiPointBody, 0, 100, "%d");
    ImGui::SliderInt(skCrypt("Min damage"), &config->rageBot[RcurrentCategory].minDamage, 0, 110, "%d");
    config->rageBot[RcurrentCategory].minDamage = std::clamp(config->rageBot[RcurrentCategory].minDamage, 0, 250);
    ImGui::PushID("Damage override Key");
    ImGui::Checkbox(c_xor("Damage override"), &config->rageBot[RcurrentCategory].dmgov);
    ImGui::SameLine();
    ImGui::hotkey2(skCrypt("Key"), config->minDamageOverrideKey);
    ImGui::PopID();
    if (config->rageBot[RcurrentCategory].dmgov)
    {
        ImGui::SliderInt(skCrypt("Damage override"), &config->rageBot[RcurrentCategory].minDamageOverride, 0, 110, "%d");
    }
    config->rageBot[RcurrentCategory].minDamageOverride = std::clamp(config->rageBot[RcurrentCategory].minDamageOverride, 0, 250);
    ImGui::PushID("adms");
    ImGui::Checkbox(c_xor("Hitchance override"), &config->rageBot[RcurrentCategory].hcov);
    ImGui::SameLine();
    ImGui::hotkey2(skCrypt("Key"), config->hitchanceOverride);
    ImGui::PopID();
    if (config->rageBot[RcurrentCategory].hcov)
    {
        ImGui::SliderInt(skCrypt("Hitchance override"), &config->rageBot[RcurrentCategory].OvrHitChance, 0, 100, "%d");
    }
    ImGui::Checkbox(skCrypt("Backtrack"), &config->backtrack.enabled);
    if (config->backtrack.enabled)
    {
        ImGui::SliderInt(skCrypt("Time limit"), &config->backtrack.timeLimit, 1, 200, "%d ms");
    }
    ImGui::PopItemWidth();
    ImGui::Columns(1);
}

void GUI::renderRageAntiAimWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 300.f);
    ImGui::PushItemWidth(190.0f);
    ImGui::PushID("nothinglmao");
    ImGui::Checkbox(skCrypt("Enable Anti-Aim"), &config->condAA.global);
    static int current_category{};
    ImGui::Combo(skCrypt("Conditions"), &current_category, skCrypt("Default\0Moving\0Jumping\0Ducking\0Duck-jumping\0Slow-walking\0"));
    if (current_category > 0)
        ImGui::Checkbox(skCrypt("Override settings"), current_category == 1 ? &config->condAA.moving : current_category == 2 ? &config->condAA.jumping : current_category == 3 ? &config->condAA.chrouch : current_category == 4 ? &config->condAA.cjump :&config->condAA.slowwalk);
    ImGui::Combo(skCrypt("Pitch"), &config->rageAntiAim[current_category].pitch, skCrypt("Off\0Down\0Zero\0Up\0Flick-up\0Random\0"));
    ImGui::Combo(skCrypt("Yaw base"), reinterpret_cast<int*>(&config->rageAntiAim[current_category].yawBase), skCrypt("Off\0Forward\0Backward\0Right\0Left\0"));
    ImGui::Combo(skCrypt("Yaw modifier"), reinterpret_cast<int*>(&config->rageAntiAim[current_category].yawModifier), skCrypt("Off\0Centered\0Offset\0Random\0Three-Way\0Five-Way\0Spin\0"));
    ImGui::SliderInt(skCrypt("Yaw add"), &config->rageAntiAim[current_category].yawAdd, -180, 180, "%d");
    if (config->rageAntiAim[current_category].yawModifier == 1 || config->rageAntiAim[current_category].yawModifier == 2 || config->rageAntiAim[current_category].yawModifier == 7) //Jitter
    {
        ImGui::PushItemWidth(190.0f);
        ImGui::SliderInt(skCrypt("Jitter range"), &config->rageAntiAim[current_category].jitterRange, -90, 90, "%d");
         ImGui::PopItemWidth();
    }
    if (config->rageAntiAim[current_category].yawModifier == 3) //Random
    {
        ImGui::PushItemWidth(190.0f);
        ImGui::SliderInt(skCrypt("Random range"), &config->rageAntiAim[current_category].randomRange, 0, 180, "%d");
        ImGui::PopItemWidth();
    }
    if (config->rageAntiAim[current_category].yawModifier == 4 || config->rageAntiAim[current_category].yawModifier == 5)
    {
        ImGui::PushItemWidth(190.0f);
        ImGui::SliderInt(skCrypt("Range"), &config->rageAntiAim[current_category].jitterRange, -90, 90, "%d");
        ImGui::PopItemWidth();
    }
    if (config->rageAntiAim[current_category].yawModifier == 6) //Spin
    {
        ImGui::PushItemWidth(190.0f);
        ImGui::SliderFloat(skCrypt("Spin base"), &config->rageAntiAim[current_category].spinBase, -180, 180, "%.f");
        ImGui::PopItemWidth();
    }
    if (config->rageAntiAim[current_category].yawModifier == 7)
    {
        ImGui::PushItemWidth(190.0f);
        ImGui::SliderInt(skCrypt("Tick delay"), &config->rageAntiAim[current_category].tickDelays, 2, 16);
        ImGui::PopItemWidth();
    }
    ImGui::Checkbox(skCrypt("At targets"), &config->rageAntiAim[current_category].atTargets);
    ImGui::Checkbox(skCrypt("Fake flick"), &config->rageAntiAim[current_category].fakeFlick);
    ImGui::PushID(skCrypt("Fake flickerino"));
    ImGui::SameLine();
    ImGui::hotkey2("", config->fakeFlickOnKey, 80.f);
    if (config->rageAntiAim[current_category].fakeFlick)
    {
        ImGui::SliderInt(skCrypt("Fake flick rate"), &config->rageAntiAim[current_category].fakeFlickRate, 5, 64, "%d");
        ImGui::hotkey2(skCrypt("Fake flick flip"), config->flipFlick);
    }
    ImGui::PopItemWidth();
    ImGui::PopID();
    ImGui::hotkey2("Forward", config->manualForward, 60.f);
    ImGui::SameLine();
    ImGui::hotkey2("Backward", config->manualBackward, 60.f);
    ImGui::hotkey2("Right", config->manualRight, 60.f);
    ImGui::SameLine();
    ImGui::hotkey2("Left", config->manualLeft, 60.f);
    ImGui::Checkbox(skCrypt("Freestand"), &config->rageAntiAim[current_category].freestand);
    ImGui::SameLine();
    ImGui::PushID(skCrypt("cevaasdeasd"));
    ImGui::hotkey2("", config->freestandKey);
    ImGui::PopID();
    ImGui::NextColumn();
    ImGui::Checkbox(skCrypt("Enable Desync"), &config->rageAntiAim[current_category].desync);
    ImGui::SameLine();
    ImGui::hotkey2(skCrypt("Invert Key"), config->invert, 80.0f);
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt(skCrypt("Left limit"), &config->rageAntiAim[current_category].leftLimit, 0, 60, "%d");
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt(skCrypt("Right limit"), &config->rageAntiAim[current_category].rightLimit, 0, 60, "%d");
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(220.0f);
    ImGui::Combo(c_xor("Peek Mode"), &config->rageAntiAim[current_category].peekMode, skCrypt("Off\0Peek real\0Peek fake\0Jitter\0"));
    ImGui::Combo(c_xor("LBY mode"), &config->rageAntiAim[current_category].lbyMode, skCrypt("Normal\0Opposite\0Sway\0"));
    ImGui::Checkbox(c_xor("Roll Angles"), &config->rageAntiAim[current_category].roll.enabled);
    ImGui::SameLine();
    ImGui::PushID("roll");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");
      
    if (ImGui::BeginPopup(""))
    {
        ImGui::SliderFloat(skCrypt("Roll Add"), &config->rageAntiAim[current_category].roll.add, -90, 90, "%.0f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(c_xor("Caution : going over -50 or 50 roll will get u detected on most servers"));
        ImGui::EndPopup(); 
    }
     ImGui::PopID();
     ImGui::Combo(c_xor("Fake-lag"), &config->fakelag.mode, skCrypt("Static\0Adaptive\0Random\0"));
     ImGui::PushItemWidth(220.0f);
     ImGui::SliderInt(c_xor("Fake-lag Limit"), &config->fakelag.limit, 1, 15, "%d");
     ImGui::PushID("Slide type lmfao");
     ImGui::Combo(skCrypt("Leg movement"), &config->misc.moonwalk_style, skCrypt("No slide\0Normal slide\0Forward slide\0Backward slide\0Allah legs\0"));
     ImGui::PopID();
     ImGui::Checkbox(skCrypt("Fakeduck (VAC)"), &config->misc.fakeduck);
     ImGui::SameLine();
     ImGui::PushID(skCrypt("Fakeduck Key"));
     ImGui::hotkey2("", config->misc.fakeduckKey);
     ImGui::PopID();
     ImGui::Checkbox(skCrypt("Slowwalk"), &config->misc.slowwalk);
     ImGui::SameLine();
     ImGui::PushID(skCrypt("Slowwalk Key"));
     ImGui::hotkey2("", config->misc.slowwalkKey);
     ImGui::PopID();
     if (config->misc.slowwalk) {
        ImGui::SliderInt(skCrypt("Slowwalk Amount"), &config->misc.slowwalkAmnt, 0, 50, config->misc.slowwalkAmnt ? "%d u/s" : "Default");
     }
     static const char* hitboxes[]{ "Static legs in air", "Zero pitch on land", "No animations", "Walk in air" };
     static bool hitbox[ARRAYSIZE(hitboxes)] = { false, false, false };
     static std::string previewvalue = "";
     bool once = false;
     for (size_t i = 0; i < ARRAYSIZE(hitbox); i++)
     {
         hitbox[i] = (config->condAA.animBreakers & 1 << i) == 1 << i;
     }
     if (ImGui::BeginCombo(skCrypt("Animation breakers"), previewvalue.c_str()))
     {
         previewvalue = "";
         for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
         {
             ImGui::Selectable(hitboxes[i], &hitbox[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
         }
         ImGui::EndCombo();
     }
     for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
     {
         if (!once)
         {
             previewvalue = "";
             once = true;
         }
         if (hitbox[i])
         {
             previewvalue += previewvalue.size() ? std::string(", ") + hitboxes[i] : hitboxes[i];
             config->condAA.animBreakers |= 1 << i;
         }
         else
         {
             config->condAA.animBreakers &= ~(1 << i);
         }
     }
     ImGui::PopItemWidth();
}

void GUI::renderChamsWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 370.0f);
    ImGui::Separator();

    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);

    static int material = 1;

    if (ImGui::Combo("", &currentCategory, skCrypt("Allies\0Enemies\0Local player\0Weapons\0Hands\0Backtrack\0Sleeves\0Desync\0Ragdolls\0Fake lag\0Attachements\0"), -1, false))
        material = 1;

    ImGui::PopID();

    ImGui::SameLine();

    if (material <= 1)
        ImGuiCustom::arrowButtonDisabled("##left", ImGuiDir_Left);
    else if (ImGui::ArrowButton("##left", ImGuiDir_Left))
        --material;

    ImGui::SameLine();
    ImGui::Text(skCrypt("Layer :"));
    ImGui::SameLine();
    ImGui::Text(skCrypt("%d"), material);

    constexpr std::array categories{ "Allies", "Enemies", "Local player", "Weapons", "Hands", "Backtrack", "Sleeves", "Desync", "Ragdolls", "Fake lag", "Attachements" };

    ImGui::SameLine();

    if (material >= int(config->chams[categories[currentCategory]].materials.size()))
        ImGuiCustom::arrowButtonDisabled("##right", ImGuiDir_Right);
    else if (ImGui::ArrowButton("##right", ImGuiDir_Right))
        ++material;

    ImGui::SameLine();

    auto& chams{ config->chams[categories[currentCategory]].materials[material - 1] };
    ImGui::PopItemWidth();
    ImGui::Checkbox(skCrypt("Enabled"), &chams.enabled);
    ImGui::Separator();
    ImGui::Checkbox(skCrypt("Health based"), &chams.healthBased); 
    ImGui::Checkbox(skCrypt("Blinking"), &chams.blinking);
    ImGui::PushItemWidth(130.0f);
    ImGui::Combo(skCrypt("Material"), &chams.material, skCrypt("Normal\0Flat\0Animated\0Animated 2\0Glass\0Chrome\0Crystal\0Plastic\0Glow\0Fractal\0Pearlescent\0Metallic\0Snowflakes\0Tie-dye smoke\0"));
    ImGui::PopItemWidth();
    ImGui::Checkbox(skCrypt("Wireframe"), &chams.wireframe);
    ImGui::Checkbox(skCrypt("Cover"), &chams.cover);
    ImGui::Checkbox(skCrypt("Behind Wall"), &chams.ignorez);
    if (currentCategory == 5)
        ImGui::Checkbox(skCrypt("All ticks"), &config->backtrack.allticks);
    ImGuiCustom::colorPicker("Color", chams);
    ImGui::NextColumn();
    ImGui::Columns(1);
}

void GUI::renderGlowWindow() noexcept
{
    ImGui::Separator();

    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    ImGui::SetCursorPosX(9.0f);
    constexpr std::array categories{ "Allies", "Enemies", "Local Player", "Weapons", "C4", "Planted C4", "Chickens", "Defuse Kits", "Projectiles", "Hostages" };
    ImGui::Combo("", &currentCategory, categories.data(), categories.size(), -1, false);
    ImGui::PopID();
    Config::GlowItem* currentItem{};
    if (currentCategory <= 1) {
        ImGui::SameLine();
        static int currentType{ 0 };
        ImGui::PushID(1);
        ImGui::Combo("", &currentType, skCrypt("All\0Visible\0Occluded\0"), - 1, false);
        ImGui::PopID();
        auto& cfg = config->playerGlow[categories[currentCategory]];
        switch (currentType) {
        case 0: currentItem = &cfg.all; break;
        case 1: currentItem = &cfg.visible; break;
        case 2: currentItem = &cfg.occluded; break;
        }
    }
    else {
        currentItem = &config->glow[categories[currentCategory]];
    }

    ImGui::SameLine();
    ImGui::Checkbox(skCrypt("Enabled"), &currentItem->enabled);
    ImGui::Separator();
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 150.0f);
    ImGui::Checkbox(skCrypt("Health based"), &currentItem->healthBased);

    ImGuiCustom::colorPicker("Color", *currentItem);

    ImGui::NextColumn();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::Combo(c_xor("Style"), &currentItem->style, skCrypt("Default\0Rim3d\0Edge\0Edge Pulse\0"));

    ImGui::Columns(1);
}

static int currentCategory;
static auto currentItem = "All";
void renderESPpreview(ImDrawList* draw, ImVec2 pos) noexcept
{
    pos.x += 690;
    pos.y += 550 / 2 - 300 / 2;
    static auto Flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::SetNextWindowSize({ 185, 300 });
    ImGui::Begin("ESP PREVIEW", NULL,Flags);
    draw->AddRectFilled(pos, { pos.x + 185, pos.y + 300 }, ImColor(0.06f, 0.06f, 0.06f, 0.5f), 5.f);
    draw->AddRectFilled({ pos.x, pos.y - ImGui::CalcTextSize(c_xor("ESP preview")).y - 6 }, { pos.x + 185, pos.y + 1 }, ImColor(0.06f, 0.06f, 0.06f, 1.f), 5.f);
    draw->AddRect({ pos.x, pos.y - ImGui::CalcTextSize(c_xor("ESP preview")).y - 6 }, { pos.x + 185, pos.y + 300 }, Helpers::calculateColor(config->menu.accentColor), 5.f, 0, 2.f);
    draw->AddRectFilled({ pos.x, pos.y }, { pos.x + 185, pos.y + 2 }, Helpers::calculateColor(config->menu.accentColor));
    //draw->AddRect({ pos.x, pos.y }, { pos.x + 185, pos.y + 300 }, Helpers::calculateColor(config->menu.accentColor), 5.f, 0, 2.f);
    draw->AddText({ pos.x + 185 / 2 - ImGui::CalcTextSize(c_xor("ESP preview")).x / 2, pos.y - ImGui::CalcTextSize(c_xor("ESP preview")).y - 3 }, ImColor(1.f,1.f,1.f,1.f), c_xor("ESP preview"));
    ImGui::PushFont(gui->espFont());
    auto sizeMed = ImGui::CalcTextSize(skCrypt("Medusa.uno"));
    ImGui::PopFont();
    ImGui::PushFont(gui->weaponIcons());
    auto sizeAk = ImGui::CalcTextSize(skCrypt("W"));
    ImGui::PopFont();
    ImGui::PushFont(gui->getConsolas10Font());
    if (currentCategory == 0)
    {
        if (config->esp.enemy.name.enabled)
        {
            draw->AddText(gui->espFont(), 12.f, ImVec2{pos.x + 185 / 2 - (sizeMed.x / 2), pos.y + 25}, Helpers::calculateColor(config->esp.enemy.name), c_xor("Medusa.uno"));
        }
        if (config->esp.enemy.box.enabled)
        {
            if (config->esp.enemy.box.type == 0)
                draw->AddRect({ pos.x + 40, pos.y + 27 + sizeMed.y }, { pos.x + 185 - 43, pos.y + 247 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.box));
            if (config->esp.enemy.box.type == 1)
            {
                draw->AddLine({ pos.x + 185 - 80, pos.y + 246 + sizeMed.y }, { pos.x + 185 - 43, pos.y + 246 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.box));
                //draw->AddLine({ pos.x + 40, pos.y + 27 + sizeMed.y }, { pos.x + 80, pos.y + 27 + sizeMed.y }, Helpers::calculateColor(config->streamProofESP.enemies[currentItem].box));
                draw->AddLine({ pos.x + 40, pos.y + 246 + sizeMed.y }, { pos.x + 80, pos.y + 246 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.box));
                draw->AddLine({ pos.x + 40, pos.y + 27 + sizeMed.y }, { pos.x + 80, pos.y + 27 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.box));
                draw->AddLine({ pos.x + 185 - 80, pos.y + 27 + sizeMed.y }, { pos.x + 185 - 43, pos.y + 27 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.box));
                draw->AddLine({ pos.x + 40, pos.y + 27 + sizeMed.y }, { pos.x + 40, pos.y + 67 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.box));
                draw->AddLine({ pos.x + 40, pos.y + 206 + sizeMed.y }, { pos.x + 40, pos.y + 246 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.box));
                draw->AddLine({ pos.x + 185 - 44, pos.y + 206 + sizeMed.y }, { pos.x + 185 - 44, pos.y + 246 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.box));
                draw->AddLine({ pos.x + 185 - 44, pos.y + 27 + sizeMed.y }, { pos.x + 185 - 44, pos.y + 67 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.box));
            }
        }
        if (config->esp.enemy.health.enabled)
        {
            if (config->esp.enemy.health.style == 0)
                draw->AddRectFilled({ pos.x + 34, pos.y + 27 + sizeMed.y }, { pos.x + 38, pos.y + 247 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.health.solid));
            else if(config->esp.enemy.health.style == 1)
                draw->AddRectFilledMultiColor({ pos.x + 33, pos.y + 27 + sizeMed.y }, { pos.x + 38, pos.y + 247 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.health.top), Helpers::calculateColor(config->esp.enemy.health.top), Helpers::calculateColor(config->esp.enemy.health.bottom), Helpers::calculateColor(config->esp.enemy.health.bottom));
            else if (config->esp.enemy.health.style == 2)
                draw->AddRectFilled({ pos.x + 34, pos.y + 27 + sizeMed.y }, { pos.x + 38, pos.y + 247 + sizeMed.y }, Helpers::calculateColor(120, 225, 80, 255));
        }
        if (config->esp.enemy.ammo.enabled)
        {
            if (config->esp.enemy.ammo.style == 0)
                draw->AddRectFilled({ pos.x + 40, pos.y + 249 + sizeMed.y }, { pos.x + 185 - 43, pos.y + 253 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.ammo.solid));
            else
                draw->AddRectFilledMultiColor({ pos.x + 40, pos.y + 249 + sizeMed.y }, { pos.x + 185 - 43, pos.y + 253 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.ammo.left), Helpers::calculateColor(config->esp.enemy.ammo.right), Helpers::calculateColor(config->esp.enemy.ammo.right), Helpers::calculateColor(config->esp.enemy.ammo.left));
        }
        if (config->esp.enemy.weapon.enabled)
        {
            draw->AddText(ImVec2{ pos.x + 185 / 2 - (ImGui::CalcTextSize(c_xor("AK-47")).x / 2), pos.y + 247 + sizeMed.y + (config->esp.enemy.ammo.enabled ? 8 : 2)}, Helpers::calculateColor(config->esp.enemy.weapon), c_xor("AK-47"));
        }
        auto offset = config->esp.enemy.weapon.enabled ? ImGui::CalcTextSize(c_xor("AK-47")).y / 2 + 4 : 0;
        if (config->esp.enemy.weaponIcon.enabled)
        {
            ImGui::PushFont(gui->weaponIcons());
            draw->AddText(ImVec2{ pos.x + 185 / 2 - (ImGui::CalcTextSize(c_xor("W")).x / 2), pos.y + 247 + sizeMed.y + (config->esp.enemy.ammo.enabled ? 8 : 2) + offset }, Helpers::calculateColor(config->esp.enemy.weaponIcon), c_xor("W"));
            ImGui::PopFont();
        }
        if (config->esp.enemy.flags.enabled)
        {
            std::ostringstream flags;

            if (config->esp.enemy.showArmor)
                flags << skCrypt("HK\n");
            if (config->esp.enemy.showFD)
                flags << skCrypt("FD\n");
            if (config->esp.enemy.showBomb)
                flags << skCrypt("C4\n");
            if (config->esp.enemy.showKit)
                flags << skCrypt("KIT\n");
            if (config->esp.enemy.showScoped)
                flags << skCrypt("ZOOM\n");
            if (config->esp.enemy.showMoney)
                flags << "$1337\n";

            if (!flags.str().empty())
                draw->AddText({ pos.x + 185 - 39, pos.y + 27 + sizeMed.y }, Helpers::calculateColor(config->esp.enemy.flags), flags.str().c_str());
        }
    }
    if (currentCategory == 1)
    {
        if (config->esp.allies.name.enabled)
        {
            draw->AddText(gui->espFont(), 12.f, ImVec2{ pos.x + 185 / 2 - (sizeMed.x / 2), pos.y + 25 }, Helpers::calculateColor(config->esp.allies.name), c_xor("Medusa.uno"));
        }
        if (config->esp.allies.box.enabled)
        {
            if (config->esp.allies.box.type == 0)
                draw->AddRect({ pos.x + 40, pos.y + 27 + sizeMed.y }, { pos.x + 185 - 43, pos.y + 247 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.box));
            if (config->esp.allies.box.type == 1)
            {
                draw->AddLine({ pos.x + 185 - 80, pos.y + 246 + sizeMed.y }, { pos.x + 185 - 43, pos.y + 246 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.box));
                //draw->AddLine({ pos.x + 40, pos.y + 27 + sizeMed.y }, { pos.x + 80, pos.y + 27 + sizeMed.y }, Helpers::calculateColor(config->streamProofESP.enemies[currentItem].box));
                draw->AddLine({ pos.x + 40, pos.y + 246 + sizeMed.y }, { pos.x + 80, pos.y + 246 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.box));
                draw->AddLine({ pos.x + 40, pos.y + 27 + sizeMed.y }, { pos.x + 80, pos.y + 27 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.box));
                draw->AddLine({ pos.x + 185 - 80, pos.y + 27 + sizeMed.y }, { pos.x + 185 - 43, pos.y + 27 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.box));
                draw->AddLine({ pos.x + 40, pos.y + 27 + sizeMed.y }, { pos.x + 40, pos.y + 67 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.box));
                draw->AddLine({ pos.x + 40, pos.y + 206 + sizeMed.y }, { pos.x + 40, pos.y + 246 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.box));
                draw->AddLine({ pos.x + 185 - 44, pos.y + 206 + sizeMed.y }, { pos.x + 185 - 44, pos.y + 246 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.box));
                draw->AddLine({ pos.x + 185 - 44, pos.y + 27 + sizeMed.y }, { pos.x + 185 - 44, pos.y + 67 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.box));
            }
        }
        if (config->esp.allies.health.enabled)
        {
            if (config->esp.allies.health.style == 0)
                draw->AddRectFilled({ pos.x + 34, pos.y + 27 + sizeMed.y }, { pos.x + 38, pos.y + 247 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.health.solid));
            else if (config->esp.allies.health.style == 1)
                draw->AddRectFilledMultiColor({ pos.x + 34, pos.y + 27 + sizeMed.y }, { pos.x + 38, pos.y + 247 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.health.top), Helpers::calculateColor(config->esp.allies.health.top), Helpers::calculateColor(config->esp.allies.health.bottom), Helpers::calculateColor(config->esp.allies.health.bottom));
            else if (config->esp.allies.health.style == 2)
                draw->AddRectFilled({ pos.x + 34, pos.y + 27 + sizeMed.y }, { pos.x + 38, pos.y + 247 + sizeMed.y }, Helpers::calculateColor(120, 225, 80, 255));
        }
        if (config->esp.allies.ammo.enabled)
        {
            if (config->esp.allies.ammo.style == 0)
                draw->AddRectFilled({ pos.x + 40, pos.y + 249 + sizeMed.y }, { pos.x + 185 - 43, pos.y + 253 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.ammo.solid));
            else
                draw->AddRectFilledMultiColor({ pos.x + 40, pos.y + 249 + sizeMed.y }, { pos.x + 185 - 43, pos.y + 253 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.ammo.left), Helpers::calculateColor(config->esp.allies.ammo.right), Helpers::calculateColor(config->esp.allies.ammo.right), Helpers::calculateColor(config->esp.allies.ammo.left));
        }
        if (config->esp.allies.weapon.enabled)
        {
            draw->AddText(ImVec2{ pos.x + 185 / 2 - (ImGui::CalcTextSize(c_xor("AK-47")).x / 2), pos.y + 247 + sizeMed.y + (config->esp.allies.ammo.enabled ? 8 : 2) }, Helpers::calculateColor(config->esp.allies.weapon), c_xor("AK-47"));
        }
        auto offset = config->esp.allies.weapon.enabled ? ImGui::CalcTextSize(c_xor("AK-47")).y / 2 + 4 : 0;
        if (config->esp.allies.weaponIcon.enabled)
        {
            ImGui::PushFont(gui->weaponIcons());
            draw->AddText(ImVec2{ pos.x + 185 / 2 - (ImGui::CalcTextSize(c_xor("W")).x / 2), pos.y + 247 + sizeMed.y + (config->esp.allies.ammo.enabled ? 8 : 2) + offset }, Helpers::calculateColor(config->esp.allies.weaponIcon), c_xor("W"));
            ImGui::PopFont();
        }
        if (config->esp.allies.flags.enabled)
        {
            std::ostringstream flags;

            if (config->esp.allies.showArmor)
                flags << skCrypt("HK\n");
            if (config->esp.allies.showFD)
                flags << skCrypt("FD\n");
            if (config->esp.allies.showBomb)
                flags << skCrypt("C4\n");
            if (config->esp.allies.showKit)
                flags << skCrypt("KIT\n");
            if (config->esp.allies.showScoped)
                flags << skCrypt("ZOOM\n");
            if (config->esp.allies.showMoney)
                flags << "$1337\n";

            if (!flags.str().empty())
                draw->AddText({ pos.x + 185 - 39, pos.y + 27 + sizeMed.y }, Helpers::calculateColor(config->esp.allies.flags), flags.str().c_str());
        }
    }
    if (currentCategory == 4)
    {
        float offset = 0.0f;
        if (config->esp.projectiles.name.enabled)
        {
            draw->AddText({ pos.x + 185 / 2 - ImGui::CalcTextSize(c_xor("HE GRENADE")).x / 2, pos.y + 300 / 2 - ImGui::CalcTextSize(c_xor("HE GRENADE")).y / 2 - 50 }, Helpers::calculateColor(config->esp.projectiles.name), c_xor("HE GRENADE"));
            offset -= ImGui::CalcTextSize(c_xor("HE GRENADE")).y;
        }
        if (config->esp.projectiles.icon.enabled)
        {
            //pos.x + 185 / 2 - ImGui::CalcTextSize(c_xor("HE GRENADE")).x / 2, pos.y + 300 / 2 - ImGui::CalcTextSize(c_xor("HE GRENADE")).y / 2 - 50
            //ImGui::CalcTextSize(c_xor("j")).x
            ImGui::PushFont(gui->weaponIcons());
            draw->AddText({ pos.x + 185 / 2 - ImGui::CalcTextSize(c_xor("j")).x / 2, pos.y + 300 / 2 - ImGui::CalcTextSize(c_xor("HE GRENADE")).y - 50 - ImGui::CalcTextSize(c_xor("j")).y / 2}, Helpers::calculateColor(config->esp.projectiles.icon), skCrypt("j"));
            ImGui::PopFont();
        }
        if (config->esp.projectiles.box.enabled)
        {
            //if (config->esp.projectiles.box.fill.enabled)
                //draw->AddRectFilled({ pos.x + 60, pos.y + 300 / 2 - 50 + ImGui::CalcTextSize(c_xor("HE GRENADE")).y / 2 }, { pos.x + 185 - 63,pos.y + 300 / 2 + ImGui::CalcTextSize(c_xor("HE GRENADE")).y / 2 }, Helpers::calculateColor(config->streamProofESP.projectiles[currentItem].box.fill), config->streamProofESP.projectiles[currentItem].box.rounding);
            if (config->esp.projectiles.box.enabled)
                draw->AddRect({ pos.x + 60, pos.y + 300 / 2 - 50 + ImGui::CalcTextSize(c_xor("HE GRENADE")).y / 2 }, { pos.x + 185 - 63,pos.y + 300 / 2 + ImGui::CalcTextSize(c_xor("HE GRENADE")).y / 2 }, Helpers::calculateColor(config->esp.projectiles.box));
        }
    }
    if (currentCategory == 3)
    {
        float offset = (config->esp.weapons.ammo.enabled ? 6 : 0);
        if (config->esp.weapons.name.enabled)
        {
            draw->AddText({ pos.x + 185 / 2 - ImGui::CalcTextSize(c_xor("R8 REVOLVER")).x / 2, pos.y + 300 / 2 + ImGui::CalcTextSize(c_xor("8 / 8")).y / 2 + 2 + (config->esp.weapons.ammo.enabled ? 6 : 0) }, Helpers::calculateColor(config->esp.weapons.name), c_xor("R8 REVOLVER"));
            offset += ImGui::CalcTextSize(c_xor("R8 REVOLVER")).y;
        }
        if (config->esp.weapons.icon.enabled)
        {
            ImGui::PushFont(gui->weaponIcons());
            draw->AddText({ pos.x + 185 / 2 - ImGui::CalcTextSize(c_xor("J")).x / 2, pos.y + 300 / 2 + ImGui::CalcTextSize(c_xor("8 / 8")).y / 2 + offset + 1}, Helpers::calculateColor(config->esp.weapons.icon), skCrypt("J"));
            ImGui::PopFont();
        }
        if (config->esp.weapons.box.enabled)
        {
            draw->AddRect({ pos.x + 40, pos.y + 300 / 2 - 50 + ImGui::CalcTextSize(c_xor("R8 REVOLVER")).y / 2 }, { pos.x + 185 - 43,pos.y + 300 / 2 + ImGui::CalcTextSize(c_xor("R8 REVOLVER")).y / 2 }, Helpers::calculateColor(config->esp.weapons.box));
        }
        if (config->esp.weapons.ammo.enabled)
        {
            draw->AddRectFilled({ pos.x + 40, pos.y + 300 / 2 + ImGui::CalcTextSize(c_xor("R8 REVOLVER")).y / 2 + 2 }, { pos.x + 185 - 43, pos.y + 300 / 2 + ImGui::CalcTextSize(c_xor("R8 REVOLVER")).y / 2 + 6 }, Helpers::calculateColor(config->esp.weapons.ammo));
        }
    }
    ImGui::End();
    ImGui::PopFont();
}

void GUI::renderStreamProofESPWindow() noexcept
{
    ImGui::SetCursorPosX(8.f);
    if (ImGui::BeginChild(skCrypt("ESP")))
    {
        static int category = 0;
        ImGui::PushItemWidth(200.f);
        ImGui::Combo("for", &currentCategory, skCrypt("Enemies\0Allies\0Local player\0Weapons\0Projectiles\0"), -1, false);
        constexpr auto getConfigPlayer = [](std::size_t currentCategory) noexcept -> Config::NewESP::Player& {
            switch (currentCategory) {
            case 0: default: return config->esp.enemy;
            case 1: return config->esp.allies;
            }
        };
        ImGui::PushItemWidth(150.f);
        ImGui::SameLine();
        auto& pCfg = getConfigPlayer(currentCategory);
        Config::Misc::Offscreen& pCfg1 = currentCategory == 0 ? config->misc.offscreenEnemies : config->misc.offscreenAllies;
        Config::LightsConfig::delightColor& pCfg2 = currentCategory == 0 ? config->dlightConfig.enemy : config->dlightConfig.teammate;
        if (currentCategory <= 1)
        {
            ImGui::Checkbox(skCrypt("Enable"), &pCfg.enable);
            ImGuiCustom::colorPicker(skCrypt("Name"), pCfg.name);
            ImGuiCustom::colorPicker(skCrypt("Weapon name"), pCfg.weapon);
            ImGuiCustom::colorPicker(skCrypt("Weapon icon"), pCfg.weaponIcon);
            ImGuiCustom::colorPicker(skCrypt("Snaplines"), pCfg.snapline);
            ImGuiCustom::colorPicker(skCrypt("Bounding box"), pCfg.box);
            ImGui::SameLine();
            ImGui::Combo("box type", &pCfg.box.type, skCrypt("Full\0Corners\0"), -1, false);
            ImGui::Checkbox(skCrypt("Dormant"), &pCfg.dormant);
            ImGui::SliderFloat(skCrypt("Dormant fade time"), &pCfg.dormantTime, 1.f, 10.f, skCrypt("%.1f"));       
            ImGui::Checkbox(c_xor("Dlights"), &pCfg2.enabled);
            ImGui::PushID("KURWA MAC");
            ImGui::SameLine();
            if (ImGui::Button("..."))
                ImGui::OpenPopup("");

            if (ImGui::BeginPopup("")) {
                ImGuiCustom::colorPicker(c_xor("Color"), pCfg2.asColor3);
                ImGui::SliderFloat(c_xor("Radius"), &pCfg2.raduis, 5.f, 150.f, "%.2f");
                ImGui::SliderInt(c_xor("Exponent"), &pCfg2.exponent, 1, 20, "%d");
                ImGui::EndPopup();
            }
            ImGui::PopID();
            if (currentCategory == 0)
            {
                ImGuiCustom::colorPicker(skCrypt("Footstep beams"), config->visuals.footstepBeamsE);
                ImGui::PushID("Footstep beams");
                ImGui::SameLine();
                if (ImGui::Button("..."))
                    ImGui::OpenPopup("");

                if (ImGui::BeginPopup("")) {
                    ImGui::SliderInt(skCrypt("Footstep beam radius"), &config->visuals.footstepBeamRadiusE, 5, 400, "%d");
                    ImGui::SliderInt(skCrypt("Footstep beam thickness"), &config->visuals.footstepBeamThicknessE, 1, 5, "%d");
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
            if (currentCategory == 1)
            {
                ImGuiCustom::colorPicker(skCrypt("Footstep beams"), config->visuals.footstepBeamsA);
                ImGui::PushID("Footstep beams");
                ImGui::SameLine();
                if (ImGui::Button("..."))
                    ImGui::OpenPopup("");

                if (ImGui::BeginPopup("")) {
                    ImGui::SliderInt(skCrypt("Footstep beam radius"), &config->visuals.footstepBeamRadiusA, 5, 400, "%d");
                    ImGui::SliderInt(skCrypt("Footstep beam thickness"), &config->visuals.footstepBeamThicknessA, 1, 5, "%d");
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
            ImGuiCustom::colorPicker(skCrypt("Offscreen Arrows"), pCfg1);
            ImGui::PushID("Offscreen Enemies");
            ImGui::SameLine();
            if (ImGui::Button("..."))
                ImGui::OpenPopup("");

            if (ImGui::BeginPopup("")) {
                ImGui::SliderInt(skCrypt("Triangle size"), &pCfg1.size, 0, 100);
                ImGui::SliderInt(skCrypt("Triangle offset"), &pCfg1.offset, 0, 1000);
                ImGui::Combo(skCrypt("Type"), &pCfg1.type, "Triangle\0Circle\0Arc\0");
                ImGui::EndPopup();
            }
            ImGui::PopID();
            ImGuiCustom::colorPicker(skCrypt("Skeleton"), pCfg.skeleton);
            ImGuiCustom::colorPicker(skCrypt("Flags"), pCfg.flags);
            ImGui::SameLine();
            ImGui::PushID(skCrypt("pemperino"));

            if (ImGui::Button("..."))
                ImGui::OpenPopup("");

            if (ImGui::BeginPopup("")) {
                ImGui::Checkbox(skCrypt("Show armor"), &pCfg.showArmor);
                ImGui::Checkbox(skCrypt("Show bomb carrier"), &pCfg.showBomb);
                ImGui::Checkbox(skCrypt("Show kit carrier"), &pCfg.showKit);
                //ImGui::Checkbox(skCrypt("Show pin pull"), &pCfg.showPin);
                ImGui::Checkbox(skCrypt("Show money"), &pCfg.showMoney);
                ImGui::Checkbox(skCrypt("Show fake duck"), &pCfg.showFD);
                ImGui::Checkbox(skCrypt("Show scoped"), &pCfg.showScoped);
                ImGui::Checkbox(skCrypt("Show flashed"), &pCfg.showFlashed);
                ImGui::EndPopup();
            }
            ImGui::PopID();
            ImGui::Checkbox(skCrypt("Health bar"), &pCfg.health.enabled);
            ImGui::SameLine();
            static int healthType = 0;
            ImGui::PushID(skCrypt("pemperinos"));
            if (ImGui::Button("..."))
                ImGui::OpenPopup("");
            if (ImGui::BeginPopup("")) {
                ImGui::Combo(skCrypt("Health bar type"), &pCfg.health.style, skCrypt("Solid\0Gradient\0Health-based\0"));
                if (pCfg.health.style == 0)
                    ImGuiCustom::colorPicker(skCrypt("Solid color"), pCfg.health.solid);
                else if (pCfg.health.style == 1)
                {
                    ImGuiCustom::colorPicker(skCrypt("Top color"), pCfg.health.top);
                    ImGuiCustom::colorPicker(skCrypt("Bottom color"), pCfg.health.bottom);
                }
                ImGui::Checkbox(skCrypt("Outline"), &pCfg.health.outline);
                ImGui::Checkbox(skCrypt("Background"), &pCfg.health.background);
                ImGuiCustom::colorPicker(skCrypt("Text"), pCfg.health.text);
                ImGui::EndPopup();
            }
            ImGui::PopID();
            ImGui::Checkbox(skCrypt("Ammo bar"), &pCfg.ammo.enabled);
            ImGui::SameLine();
            static int ammoType = 0;
            ImGui::PushID(skCrypt("pemperinoss"));
            if (ImGui::Button("..."))
                ImGui::OpenPopup("");
            if (ImGui::BeginPopup("")) {
                ImGui::Combo(skCrypt("Ammo bar type"), &pCfg.ammo.style, skCrypt("Solid\0Gradient\0"));
                if (pCfg.ammo.style == 0)
                    ImGuiCustom::colorPicker(skCrypt("Solid color"), pCfg.ammo.solid);
                else if (pCfg.ammo.style == 1)
                {
                    ImGuiCustom::colorPicker(skCrypt("Left color"), pCfg.ammo.left);
                    ImGuiCustom::colorPicker(skCrypt("Right color"), pCfg.ammo.right);
                }
                ImGui::Checkbox(skCrypt("Outline"), &pCfg.ammo.outline);
                ImGui::Checkbox(skCrypt("Background"), &pCfg.ammo.background);
                ImGuiCustom::colorPicker(skCrypt("Text"), pCfg.ammo.text);
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        if (currentCategory == 2)
        {
            ImGui::Checkbox(skCrypt("Enable"), &config->esp.local.enable);
            ImGuiCustom::colorPicker(skCrypt("Name"), config->esp.local.name);
            ImGuiCustom::colorPicker(skCrypt("Bounding box"), config->esp.local.box);
            ImGui::SameLine();
            ImGui::Combo("box type", &config->esp.local.box.type, skCrypt("Full\0Corners\0"), -1, false);
            ImGuiCustom::colorPicker(skCrypt("Skeleton"), config->esp.local.skeleton);
            ImGui::Checkbox(c_xor("Dlights"), &config->dlightConfig.local.enabled);
            ImGui::PushID("KURWA MAC");
            ImGui::SameLine();
            if (ImGui::Button("..."))
                ImGui::OpenPopup("");

            if (ImGui::BeginPopup("")) {
                ImGuiCustom::colorPicker(c_xor("Color"), config->dlightConfig.local.asColor3);
                ImGui::SliderFloat(c_xor("Radius"), &config->dlightConfig.local.raduis, 5.f, 150.f, "%.2f");
                ImGui::SliderInt(c_xor("Exponent"), &config->dlightConfig.local.exponent, 1, 20, "%d");
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        if (currentCategory == 3)
        {
            ImGui::Checkbox(skCrypt("Enable"), &config->esp.weapons.enable);
            ImGuiCustom::colorPicker(skCrypt("Weapon name"), config->esp.weapons.name);
            ImGuiCustom::colorPicker(skCrypt("Bounding box"), config->esp.weapons.box);
            ImGui::SameLine();
            ImGui::Combo("box type", &config->esp.weapons.box.type, skCrypt("Full\0Corners\0"), -1, false);
            ImGuiCustom::colorPicker(skCrypt("Ammo bar"), config->esp.weapons.ammo);
            ImGuiCustom::colorPicker(skCrypt("Weapon icon"), config->esp.weapons.icon);
        }
        if (currentCategory == 4)
        {
            ImGui::Checkbox(skCrypt("Enable"), &config->esp.projectiles.enable);
            ImGuiCustom::colorPicker(skCrypt("Name"), config->esp.projectiles.name);
            ImGuiCustom::colorPicker(skCrypt("Bounding box"), config->esp.projectiles.box);
            ImGui::SameLine();
            ImGui::Combo("box type", &config->esp.projectiles.box.type, skCrypt("Full\0Corners\0"), -1, false);
        }
        ImGui::EndChild();
    }
}

void GUI::renderVisualsWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 334.0f);
    constexpr auto playerModels = "Default\0Special Agent Ava | FBI\0Operator | FBI SWAT\0Markus Delrow | FBI HRT\0Michael Syfers | FBI Sniper\0SAS\0B Squadron Officer | SAS\0D Squadron Officer | NZSAS\0Seal Team 6 Soldier | NSWC SEAL\0Buckshot | NSWC SEAL\0Lt. Commander Ricksaw | NSWC SEAL\0Third Commando Company | KSK\0'Two Times' McCoy | USAF TACP\0Primeiro Tenente | Brazilian 1st Battalion\0Cmdr. Davida 'Goggles' Fernandez | SEAL Frogman\0Cmdr. Frank 'Wet Sox' Baroud | SEAL Frogman\0Lieutenant Rex Krikey | SEAL Frogman\0Dragomir | Sabre\0Rezan The Ready | Sabre\0'The Doctor' Romanov | Sabre\0Maximus | Sabre\0Blackwolf | Sabre\0The Elite Mr. Muhlik | Elite Crew\0Ground Rebel | Elite Crew\0Osiris | Elite Crew\0Prof. Shahmat | Elite Crew\0Enforcer | Phoenix\0Slingshot | Phoenix\0Soldier | Phoenix\0Pirate\0Pirate Variant A\0Pirate Variant B\0Pirate Variant C\0Pirate Variant D\0Anarchist\0Anarchist Variant A\0Anarchist Variant B\0Anarchist Variant C\0Anarchist Variant D\0Balkan Variant A\0Balkan Variant B\0Balkan Variant C\0Balkan Variant D\0Balkan Variant E\0Jumpsuit Variant A\0Jumpsuit Variant B\0Jumpsuit Variant C\0GIGN\0GIGN Variant A\0GIGN Variant B\0GIGN Variant C\0GIGN Variant D\0GSG9\0The proffesional Variant A\0The proffesional Variant B\0The proffesional Variant C\0The proffesional Variant D\0Street Soldier | Phoenix\0'Blueberries' Buckshot | NSWC SEAL\0'Two Times' McCoy | TACP Cavalry\0Rezan the Redshirt | Sabre\0Dragomir | Sabre Footsoldier\0Cmdr. Mae 'Dead Cold' Jamison | SWAT\0001st Lieutenant Farlow | SWAT\0John 'Van Healen' Kask | SWAT\0Bio-Haz Specialist | SWAT\0Sergeant Bombson | SWAT\0Chem-Haz Specialist | SWAT\0Lieutenant 'Tree Hugger' Farlow | SWAT\0Sir Bloody Miami Darryl | The Professionals\0Sir Bloody Silent Darryl | The Professionals\0Sir Bloody Skullhead Darryl | The Professionals\0Sir Bloody Darryl Royale | The Professionals\0Sir Bloody Loudmouth Darryl | The Professionals\0Safecracker Voltzmann | The Professionals\0Little Kev | The Professionals\0Number K | The Professionals\0Getaway Sally | The Professionals\0Trapper | Guerrilla Warfare\0Trapper Aggressor | Guerrilla Warfare\0Vypa Sista of the Revolution | Guerrilla Warfare\0Col. Mangos Dabisi | Guerrilla Warfare\0Arno The Overgrown | Guerrilla Warfare\0'Medium Rare' Crasswater | Guerrilla Warfare\0Crasswater The Forgotten | Guerrilla Warfare\0Elite Trapper Solman | Guerrilla Warfare\0Sous-Lieutenant Medic | Gendarmerie Nationale\0Chem-Haz Capitaine | Gendarmerie Nationale\0Chef d'Escadron Rouchard | Gendarmerie Nationale\0Aspirant | Gendarmerie Nationale\0Officer Jacques Beltram | Gendarmerie Nationale\0";
    ImGui::PushItemWidth(250.0f);
    ImGui::Combo(skCrypt("T Player Model"), &config->visuals.playerModelT, playerModels);
    ImGui::Combo(skCrypt("CT Player Model"), &config->visuals.playerModelCT, playerModels);
    ImGui::InputText(c_xor("Custom Player Model"), config->visuals.playerModel, sizeof(config->visuals.playerModel), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SliderFloat(c_xor("Aspect Ratio"), &config->misc.aspectratio, 0.0f, 5.0f, skCrypt("%.2f"));
    ImGui::SliderInt(c_xor("Fov"), &config->visuals.fov, -60, 60, skCrypt("%d"));
    ImGui::SliderInt(skCrypt("Flash reduction"), &config->visuals.flashReduction, 0, 100, "%d%%");
    ImGui::Checkbox(skCrypt("Grenade Prediction"), &config->misc.nadePredict);
    ImGui::SameLine();
    ImGui::PushID("Grenade Prediction");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGuiCustom::colorPicker(skCrypt("Damage"), config->misc.nadeDamagePredict);
        ImGuiCustom::colorPicker(skCrypt("Trail"), config->misc.nadeTrailPredict);
        ImGuiCustom::colorPicker(skCrypt("Circle"), config->misc.nadeCirclePredict);
        ImGuiCustom::colorPicker(skCrypt("Outline"), config->misc.nadeGlowPredict, &config->misc.nadeGlowPredict.enabled);
        ImGui::EndPopup();
    }
    ImGui::Checkbox(skCrypt("Custom post-processing"), &config->visuals.PostEnabled);
    ImGui::SameLine();
    ImGui::PushID("something");
    bool ppPopup = ImGui::Button("...");

    if (ppPopup)
        ImGui::OpenPopup("##pppopup");

    if (ImGui::BeginPopup("##pppopup")) {
        ImGui::SliderFloat(skCrypt("World exposure"), &config->visuals.worldExposure, 0.0f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat(skCrypt("Model ambient"), &config->visuals.modelAmbient, 0.0f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat(skCrypt("Bloom scale"), &config->visuals.bloomScale, 0.0f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGuiCustom::colorPicker(skCrypt("Fog controller"), config->visuals.fog);
    ImGui::SameLine();

    ImGui::PushID(skCrypt("Fog controller"));
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {

        ImGui::PushItemWidth(290.0f);
        ImGui::SliderFloat(skCrypt("Start"), &config->visuals.fogOptions.start, 0.0f, 5000.0f, skCrypt("Start: %.2f"));
        ImGui::PushItemWidth(290.0f);
        ImGui::SliderFloat(skCrypt("End"), &config->visuals.fogOptions.end, 0.0f, 5000.0f, skCrypt("End: %.2f"));
        ImGui::PushItemWidth(290.0f);
        ImGui::SliderFloat(skCrypt("Density"), &config->visuals.fogOptions.density, 0.001f, 1.0f, skCrypt("Density: %.3f"));

        ImGui::EndPopup();
    }
    ImGuiCustom::colorPicker(skCrypt("World color"), config->visuals.world);
    ImGuiCustom::colorPicker(skCrypt("Props color"), config->visuals.props);
    ImGuiCustom::colorPicker(skCrypt("Sky color"), config->visuals.sky);
    ImGui::Checkbox(skCrypt("Noscope crosshair"), &config->misc.noscopeCrosshair);
    ImGui::Checkbox(skCrypt("Recoil crosshair"), &config->misc.recoilCrosshair);
    ImGuiCustom::colorPicker(skCrypt("Taser range"), config->visuals.TaserRange, &config->visuals.TaserRange.enabled);
    ImGuiCustom::colorPicker(skCrypt("Knife range"), config->visuals.KnifeRange, &config->visuals.KnifeRange.enabled);
    ImGui::Checkbox(skCrypt("Custom ragdoll gravity"), &config->visuals.inverseRagdollGravity);
    ImGui::PushID("gravity");
    if (config->visuals.inverseRagdollGravity)
        ImGui::SliderInt(c_xor("Gravity amount"), &config->misc.ragdollGravity, -1500, 1500, "%d");
    ImGui::PopID();
    ImGui::Checkbox(skCrypt("Deagle spinner"), &config->visuals.deagleSpinner);
    ImGui::Checkbox(skCrypt("Viewmodel in scope"), &config->visuals.viewmodelInScope);
    ImGui::PushID("COCK EXPLOIT");
    ImGuiCustom::colorPicker(skCrypt("Local Player Trail"), config->visuals.playerTrailColor);
    ImGui::PopID();
    ImGui::Combo(skCrypt("Scope Type"), &config->visuals.scope.type, skCrypt("Dynamic scope\0Static Scope\0Custom scope\0No scope\0"));
    if (config->visuals.scope.type == 2)
    {
        auto sex = ImGui::GetFrameHeight();
        auto textSize = ImGui::CalcTextSize("Scope Type");
        ImGui::SameLine();
        ImGui::PushID(skCrypt("Better Scope"));
        //ImGui::SetCursorPosY(sex + textSize.y);
        if (ImGui::Button("..."))
            ImGui::OpenPopup("");

        if (ImGui::BeginPopup("")) {
            ImGuiCustom::colorPicker(skCrypt("Custom scope color"), config->visuals.scope.color);
            ImGui::Checkbox(skCrypt("Scope fade"), &config->visuals.scope.fade);
            ImGui::PushItemWidth(350.0f);
            ImGui::SliderInt(skCrypt("Lines offset"), &config->visuals.scope.offset, -500.0, 500.0, skCrypt("%d"));
            ImGui::SliderInt(skCrypt("Lines length"), &config->visuals.scope.length, 0.0f, 1000.0, skCrypt("%d"));
            ImGui::Checkbox(c_xor("Remove top line"), &config->visuals.scope.removeTop);
            ImGui::SameLine();
            ImGui::Checkbox(c_xor("Remove bottom line"), &config->visuals.scope.removeBottom);
            ImGui::Checkbox(c_xor("Remove left line"), &config->visuals.scope.removeLeft);
            ImGui::SameLine();
            ImGui::Checkbox(c_xor("Remove right line"), &config->visuals.scope.removeRight);
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    ImGui::Checkbox(skCrypt("Shadow changer"), &config->visuals.shadowsChanger.enabled);
    ImGui::SameLine();

    ImGui::PushID("Shadow changer");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::PushItemWidth(290.0f);
        ImGui::SliderInt(c_xor("X rotation"), &config->visuals.shadowsChanger.x, 0, 360, skCrypt("%d"));
        ImGui::PushItemWidth(290.0f);
        ImGui::SliderInt(c_xor("Y rotation"), &config->visuals.shadowsChanger.y, 0, 360, skCrypt("%d"));
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::Checkbox(skCrypt("Motion Blur"), &config->visuals.motionBlur.enabled);
    ImGui::SameLine();

    ImGui::PushID("Motion Blur");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {

        ImGui::Checkbox(skCrypt("Forward enabled"), &config->visuals.motionBlur.forwardEnabled);

        ImGui::PushItemWidth(290.0f);
        ImGui::SliderFloat(c_xor("Falling min"), &config->visuals.motionBlur.fallingMin, 0.0f, 50.0f, "%.2f");
        ImGui::PushItemWidth(290.0f);
        ImGui::SliderFloat(c_xor("Falling max"), &config->visuals.motionBlur.fallingMax, 0.0f, 50.0f, "%.2f");
        ImGui::PushItemWidth(290.0f);
        ImGui::SliderFloat(c_xor("Falling intesity"), &config->visuals.motionBlur.fallingIntensity, 0.0f, 8.0f, "%.2f");
        ImGui::PushItemWidth(290.0f);
        ImGui::SliderFloat(c_xor("Rotation intensity"), &config->visuals.motionBlur.rotationIntensity, 0.0f, 8.0f, "%.2f");
        ImGui::PushItemWidth(290.0f);
        ImGui::SliderFloat(c_xor("Strength"), &config->visuals.motionBlur.strength, 0.0f, 8.0f, "%.2f");

        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox(skCrypt("Full bright"), &config->visuals.fullBright);
    ImGui::PushID("Removals");
    if (ImGui::Button("Removals"))
        ImGui::OpenPopup("Removals");
    if (ImGui::BeginPopup("Removals"))
    {
        ImGui::PushItemWidth(290.0f);
        ImGui::Text(skCrypt("Removals"));
        ImGui::Checkbox(skCrypt("Server side ads"), &config->misc.adBlock);
        ImGui::Checkbox(skCrypt("Post-processing"), &config->visuals.disablePostProcessing);
        ImGui::Checkbox(skCrypt("Jiggle bones"), &config->visuals.disableJiggleBones);
        ImGui::Checkbox(skCrypt("Fog"), &config->visuals.noFog);
        ImGui::SameLine();
        ImGui::Checkbox(skCrypt("No 3d sky"), &config->visuals.no3dSky);
        ImGui::SameLine();
        ImGui::Checkbox(skCrypt("Blur"), &config->visuals.noBlur);
        ImGui::Checkbox(skCrypt("Aim punch"), &config->visuals.noAimPunch);
        ImGui::SameLine();
        ImGui::Checkbox(skCrypt("View punch"), &config->visuals.noViewPunch);
        ImGui::Checkbox(skCrypt("View bob"), &config->visuals.noViewBob);
        ImGui::Checkbox(skCrypt("Hands"), &config->visuals.noHands);
        ImGui::SameLine();
        ImGui::Checkbox(skCrypt("Sleeves"), &config->visuals.noSleeves);
        ImGui::Checkbox(skCrypt("Weapons"), &config->visuals.noWeapons);
        ImGui::Checkbox(skCrypt("Scope Zoom"), &config->visuals.keepFov);
        ImGui::SameLine();
        ImGui::Checkbox(skCrypt("Scope overlay"), &config->visuals.noScopeOverlay);
        ImGui::Checkbox(skCrypt("Grass"), &config->visuals.noGrass);
        ImGui::Checkbox(skCrypt("Model occlusion"), &config->misc.disableModelOcclusion);
        ImGui::Checkbox(skCrypt("HUD blur"), &config->misc.disablePanoramablur);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::NextColumn();
    ImGui::PushItemWidth(250.0f);
    ImGui::Checkbox(skCrypt("Zoom"), &config->visuals.zoom);
    ImGui::SameLine();
    ImGui::PushID(skCrypt("Zoom Key"));
    ImGui::hotkey2("", config->visuals.zoomKey);
    ImGui::PopID();
    ImGui::Checkbox(skCrypt("Thirdperson"), &config->visuals.thirdperson.enable);
    {
        ImGui::SameLine();
        ImGui::PushID("?1'thirdperson'1?tf is u looking in hex for 'medusa.uno");
        if (ImGui::Button("..."))
            ImGui::OpenPopup("ttt");
        if (ImGui::BeginPopup("ttt"))
        {
            ImGui::PushItemWidth(250.0f);
            ImGui::PushID(0);
            ImGui::SliderInt(c_xor("Thirdperson distance"), &config->visuals.thirdperson.distance, 0, 200, skCrypt("%d"));
            ImGui::SliderFloat(c_xor("Transparency in scope"), &config->visuals.thirdperson.thirdpersonTransparency, 0.f, 100.f, "%.1f");
            ImGui::Checkbox(skCrypt("While dead"), &config->visuals.thirdperson.whileDead);
            ImGui::Checkbox(c_xor("Disable on grenade"), &config->visuals.thirdperson.disableOnGrenade);
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    ImGui::SameLine();
    ImGui::PushID(skCrypt("Thirdperson Key"));
    ImGui::hotkey2("", config->visuals.thirdperson.key);
    ImGui::PopID();
    //ImGui::Checkbox(skCrypt("Mask changer"), &config->misc.mask);
    ImGui::Checkbox(skCrypt("Party mode"), &config->visuals.partyMode);
    ImGui::PopID();
    ImGui::PushItemWidth(250.0f);
    ImGui::PushID("Skyboxer");
    ImGui::Combo(skCrypt("Skybox"), &config->visuals.skybox, Visuals::skyboxList.data(), Visuals::skyboxList.size());
    if (config->visuals.skybox == 26) {
        ImGui::InputText(skCrypt("Skybox filename"), &config->visuals.customSkybox);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(skCrypt("skybox files must be put in csgo/materials/skybox/ "));
    }
    ImGui::PopID();
    ImGui::PushID(13);
    ImGui::SliderInt(c_xor("Asus walls"), &config->visuals.asusWalls, 0, 100, skCrypt("%d"));
    ImGui::PopID();
    ImGui::PushID(14);
    ImGui::SliderInt(c_xor("Asus props"), &config->visuals.asusProps, 0, 100, skCrypt("%d"));
    ImGui::PopID();
    ImGui::Combo(skCrypt("Screen effect"), &config->visuals.screenEffect, skCrypt("None\0Drone cam\0Drone cam with noise\0Underwater\0Healthboost\0Dangerzone\0"));
    ImGui::Combo(skCrypt("Hit effect"), &config->visuals.hitEffect, skCrypt("None\0Drone cam\0Drone cam with noise\0Underwater\0Healthboost\0Dangerzone\0"));
    if (config->visuals.hitEffect > 0)
    ImGui::SliderFloat(c_xor("Hit effect time"), &config->visuals.hitEffectTime, 0.1f, 1.5f, "%.2fs");
    ImGuiCustom::colorPicker(skCrypt("Console color"), config->visuals.console);
    ImGuiCustom::colorPicker(skCrypt("Hitmarker"), config->visuals.hitMarkerColor);
    ImGui::SameLine();
    ImGui::PushID("sexyieafioawsd");
    if (ImGui::Button("..."))
        ImGui::OpenPopup(("htiqwi"));

    if (ImGui::BeginPopup(("htiqwi")))
    {
        ImGui::Combo(skCrypt("Hit marker type"), &config->visuals.hitMarker, skCrypt("Default (Cross)\0Swastika\0"));
        ImGui::SliderFloat(c_xor("Hit marker time"), &config->visuals.hitMarkerTime, 0.1f, 1.5f, "%.2fs");
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGuiCustom::colorPicker(skCrypt("Bullet Tracers"), config->visuals.bulletTracers.color.data(), &config->visuals.bulletTracers.color[3], nullptr, nullptr, &config->visuals.bulletTracers.enabled);
    ImGui::SameLine();
    ImGui::PushID("safafw");
    if (ImGui::Button("..."))
        ImGui::OpenPopup(("hs"));

    if (ImGui::BeginPopup(("hs")))
    {
        ImGui::SliderFloat(c_xor("Width"), &config->visuals.bulletWidth, 0.1f, 3.f, "%.2f");
        ImGui::Combo(skCrypt("Sprite"), &config->visuals.bulletSprite, skCrypt("White\0Purplelaser\0Laserbeam\0Physbeam\0"));
        ImGui::Combo(skCrypt("Effects"), &config->visuals.bulletEffects, skCrypt("None\0Noise\0Spiral\0"));
        switch (config->visuals.bulletEffects)
        {
        case 1:
            ImGui::SliderFloat("##amplitude", &config->visuals.amplitude, 0.0f, 10.0f, "Noise %.3f");
            break;
        case 2:
            ImGui::SliderFloat("##amplitude", &config->visuals.amplitude, 0.0f, 10.0f, "Radius %.3f");
            break;
        default:
            break;
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGuiCustom::colorPicker(skCrypt("Bullet Impacts"), config->visuals.bulletImpacts.color.data(), &config->visuals.bulletImpacts.color[3], nullptr, nullptr, &config->visuals.bulletImpacts.enabled);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Server side only bullet impacts");
    //ImGuiCustom::colorPicker(skCrypt("Damager indicator"), config->visuals.damageMarker);
    ImGui::SliderFloat(c_xor("Bullet Impacts time"), &config->visuals.bulletImpactsTime, 0.1f, 5.0f, "%.2fs");
    ImGui::Checkbox(skCrypt("On hit model"), &config->visuals.onHitHitbox.enabled);
    ImGui::SameLine();
    ImGui::PushID("not on hit trust");
    if (ImGui::Button("..."))
        ImGui::OpenPopup(" ");

    if (ImGui::BeginPopup(" "))
    {
        ImGuiCustom::colorPicker(skCrypt("Color"), config->visuals.onHitHitbox.color);
        ImGui::SliderFloat(c_xor("Duration"), &config->visuals.onHitHitbox.duration, 0.5f, 15.f, "%.1f");
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::PushID("Molly");
    if (ImGui::Button("Edit Molotov"))
        ImGui::OpenPopup("Molly");

    if (ImGui::BeginPopup("Molly"))
    {
        ImGuiCustom::colorPicker(skCrypt("Molotov color modulation"), config->visuals.molotovColor);
        ImGuiCustom::colorPicker(skCrypt("Molotov Polygon"), config->visuals.molotovPolygon);
        ImGuiCustom::colorPicker(skCrypt("Molotov Hull"), config->visuals.molotovHull);
        ImGui::Checkbox(skCrypt("Wireframe molotov"), &config->visuals.wireframeMolotov);
        ImGui::Checkbox(skCrypt("Remove Molotov"), &config->visuals.noMolotov);
        ImGui::Checkbox(skCrypt("Molotov Timer"), &config->visuals.molotovTimer);
        if (config->visuals.molotovTimer)
        {
            ImGuiCustom::colorPicker(skCrypt("BackGround color"), config->visuals.molotovTimerBG);
            ImGuiCustom::colorPicker(skCrypt("Text color"), config->visuals.molotovTimerText);
            ImGuiCustom::colorPicker(skCrypt("Timer color"), config->visuals.molotovTimerTimer);
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::PushID("Smokee");
    if (ImGui::Button(skCrypt("Edit Smoke")))
        ImGui::OpenPopup("Smokee");

    if (ImGui::BeginPopup("Smokee"))
    {
        ImGuiCustom::colorPicker(skCrypt("Smoke color modulation"), config->visuals.smokeColor);
        ImGuiCustom::colorPicker(skCrypt("Smoke Hull"), config->visuals.smokeHull);
        ImGui::Checkbox(skCrypt("Wireframe smoke"), &config->visuals.wireframeSmoke);
        ImGui::Checkbox(skCrypt("Remove Smoke"), &config->visuals.noSmoke);
        ImGui::Checkbox(skCrypt("Smoke Timer"), &config->visuals.smokeTimer);
        if (config->visuals.smokeTimer)
        {
            ImGuiCustom::colorPicker(skCrypt("BackGround color"), config->visuals.smokeTimerBG);
            ImGuiCustom::colorPicker(skCrypt("Text color"), config->visuals.smokeTimerText);
            ImGuiCustom::colorPicker(skCrypt("Timer color"), config->visuals.smokeTimerTimer);
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGuiCustom::colorPicker(skCrypt("Spread circle"), config->visuals.spreadCircle);

    ImGui::Checkbox(skCrypt("Viewmodel"), &config->visuals.viewModel.enabled);
    ImGui::SameLine();

    if (bool ccPopup = ImGui::Button(skCrypt("Edit")))
        ImGui::OpenPopup("##viewmodel");

    if (ImGui::BeginPopup("##viewmodel"))
    {
        ImGui::PushItemWidth(290.0f);
        ImGui::SliderFloat(skCrypt("X"), &config->visuals.viewModel.x, -20.0f, 20.0f, skCrypt("%.4f"));
        ImGui::SliderFloat(skCrypt("Y"), &config->visuals.viewModel.y, -20.0f, 20.0f, skCrypt("%.4f"));
        ImGui::SliderFloat(skCrypt("Z"), &config->visuals.viewModel.z, -20.0f, 20.0f, skCrypt("%.4f"));
        ImGui::SliderInt(skCrypt("Viewmodel FOV"), &config->visuals.viewModel.fov, -60, 60, skCrypt("%d"));
        ImGui::SliderFloat(skCrypt("Viewmodel roll"), &config->visuals.viewModel.roll, -90.0f, 90.0f, skCrypt(" %.2f"));
        ImGui::PopItemWidth();
        ImGui::EndPopup();
    }
    ImGui::PopItemWidth();
    ImGui::Columns(1);
}

void GUI::renderMovementWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 365.0f);
    ImGui::Checkbox(skCrypt("Bunny hop"), &config->misc.bunnyHop);
    //ImGui::SliderInt(skCrypt("Bunny hop hitchance"), &config->misc.bhHc, 1, 100, "%d");
    ImGui::Checkbox(skCrypt("Auto strafe"), &config->misc.autoStrafe);
    ImGui::SliderInt(skCrypt("Auto strafer smoothness"), &config->misc.auto_smoothnes, 0, 100, "%d");
    ImGui::PushID(1253);
    ImGui::PopID();
    ImGui::Checkbox(skCrypt("Fast duck (VAC)"), &config->misc.fastDuck);
    ImGui::Checkbox(skCrypt("Fast Stop"), &config->misc.fastStop);
    ImGui::Checkbox(skCrypt("Mini jump"), &config->misc.miniJump);
    ImGui::SameLine();
    ImGui::PushID("Mini jump Key");
    ImGui::hotkey2("", config->misc.miniJumpKey);
    ImGui::PopID();
    if (config->misc.miniJump) {
        ImGui::SliderInt("Crouch lock", &config->misc.miniJumpCrouchLock, 0, 12, "%d ticks");
    }
    ImGui::PushID("Jump stats");
    ImGui::Checkbox(skCrypt("Jump stats"), &config->misc.jumpStats.enabled);
    ImGui::SameLine();

    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox(skCrypt("Show fails"), &config->misc.jumpStats.showFails);
        ImGui::Checkbox(skCrypt("Show color on fails"), &config->misc.jumpStats.showColorOnFail);
        ImGui::Checkbox(skCrypt("Simplify naming"), &config->misc.jumpStats.simplifyNaming);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::AlignTextToFramePadding();
    ImGui::Text(skCrypt("Edge bug detection"));
    ImGui::SameLine();
    ImGui::PushID("stop using hex u dirty hoe, medusa.uno");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");
    if (ImGui::BeginPopup(""))
    {
        ImGui::Checkbox(skCrypt("Show in chat"), &config->misc.ebdetect.chat);
        ImGui::Checkbox(skCrypt("Effect"), &config->misc.ebdetect.effect);
        ImGui::SliderFloat("Effect time", &config->misc.ebdetect.effectTime, 0.5f, 2.5f, "%.2f");
        ImGui::Combo(c_xor("Sound"), &config->misc.ebdetect.sound, skCrypt("None\0Medusa.uno\0Metal\0Gamesense\0Bell\0Glass\0Coins\0Custom\0"));      
        if (config->misc.ebdetect.sound == 7) {
            ImGui::InputText(c_xor("Hit Sound filename"), &config->misc.customEBsound);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(c_xor("audio file must be put in csgo/sound/ directory, also make sure it's .wav"));
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::NextColumn();
    ImGui::PushItemWidth(150.f);
    ImGui::Checkbox(skCrypt("Block bot"), &config->misc.blockBot);
    ImGui::SameLine();
    ImGui::PushID("Block bot Key");
    ImGui::hotkey2("", config->misc.blockBotKey);
    ImGui::PopID();
    ImGui::Checkbox(skCrypt("Edge Jump"), &config->misc.edgeJump);
    ImGui::SameLine();
    ImGui::PushID("Edge Jump Key");
    ImGui::hotkey2("", config->misc.edgeJumpKey);
    ImGui::PopID();
    ImGui::Checkbox(skCrypt("Jump Bug"), &config->misc.jumpBug);
    ImGui::SameLine();
    ImGui::PushID("Jump Bug Key");
    ImGui::hotkey2("", config->misc.jumpBugKey);
    ImGui::PopID();
    ImGui::Checkbox(skCrypt("Edge Bug"), &config->misc.edgeBug);
    ImGui::SameLine();
    ImGui::PushID("Edge Bug Key");
    ImGui::hotkey2("", config->misc.edgeBugKey);
    ImGui::PopID();
    if (config->misc.edgeBug) {
        ImGui::SliderInt(skCrypt("Pred Amount"), &config->misc.edgeBugPredAmnt, 0, 64, "%d ticks");
        ImGui::Combo(skCrypt("Mouse Lock type"), &config->misc.edgeBugLockType, skCrypt("None\0Static\0Dynamic\0"));
        if (config->misc.edgeBugLockType != 0)
            ImGui::SliderFloat(skCrypt("Lock strength"), &config->misc.edgeBugLock, 0.f, 1.f, "%.3f%");
    }
    ImGuiCustom::colorPicker(skCrypt("Auto peek"), config->misc.autoPeek.color.data(), &config->misc.autoPeek.color[3], &config->misc.autoPeek.rainbow, &config->misc.autoPeek.rainbowSpeed, &config->misc.autoPeek.enabled);
    ImGui::SameLine();
    ImGui::PushID("Auto peek Key");
    ImGui::hotkey2("", config->misc.autoPeekKey);
    ImGui::Combo(skCrypt("Auto peek style"), &config->misc.autoPeekStyle, skCrypt("Outlined circle\0Filled circle\0Particles\0Sparks\0Gradient in\0Gradient out\0Medusa logo\0"));
    if (config->misc.autoPeekStyle == 0 || config->misc.autoPeekStyle == 1 || config->misc.autoPeekStyle == 4 || config->misc.autoPeekStyle == 5)
        ImGui::SliderFloat(skCrypt("Circle radius"), &config->misc.autoPeekRadius, 1.f, 35.f, "%.2f");
    ImGui::Checkbox(skCrypt("Auto pixel surf"), &config->misc.autoPixelSurf);
    ImGui::SameLine();
    ImGui::PushID("Auto pixel surf Key");
    ImGui::hotkey2("", config->misc.autoPixelSurfKey);
    ImGui::PopID();
    if (config->misc.autoPixelSurf) {
        ImGui::SliderInt("Prediction Amnt", &config->misc.autoPixelSurfPredAmnt, 2, 4, "%d ticks");
    }
    ImGui::PopItemWidth();
}

void GUI::renderSkinChangerWindow() noexcept
{
    ImGui::PushItemWidth(245.f);
    static auto itemIndex = 0;
    ImGui::Combo("##1", &itemIndex, [](void* data, int idx, const char** out_text) {
        *out_text = SkinChanger::weapon_names[idx].name;
        return true;
        }, nullptr, SkinChanger::weapon_names.size(), 5, false);
    ImGui::PopItemWidth();

    auto& selected_entry = config->skinChanger[itemIndex];
    selected_entry.itemIdIndex = itemIndex;

    constexpr auto rarityColor = [](int rarity) {
        constexpr auto rarityColors = std::to_array<ImU32>({
            IM_COL32(0,     0,   0,   0),
            IM_COL32(176, 195, 217, 255),
            IM_COL32(94, 152, 217, 255),
            IM_COL32(75, 105, 255, 255),
            IM_COL32(136,  71, 255, 255),
            IM_COL32(211,  44, 230, 255),
            IM_COL32(235,  75,  75, 255),
            IM_COL32(228, 174,  57, 255)
            });
        return rarityColors[static_cast<std::size_t>(rarity) < rarityColors.size() ? rarity : 0];
    };

    constexpr auto passesFilter = [](const std::wstring& str, std::wstring filter) {
        constexpr auto delimiter = L" ";
        wchar_t* _;
        wchar_t* token = std::wcstok(filter.data(), delimiter, &_);
        while (token) {
            if (!std::wcsstr(str.c_str(), token))
                return false;
            token = std::wcstok(nullptr, delimiter, &_);
        }
        return true;
    };
    ImGui::PushItemWidth(245.f);
    {
        ImGui::SameLine();
        ImGui::Checkbox(skCrypt("Enabled"), &selected_entry.enabled);
        ImGui::Separator();
        ImGui::Columns(2, nullptr, false);
        ImGui::InputInt(skCrypt("Seed"), &selected_entry.seed);
        ImGui::InputInt(skCrypt("StatTrak\u2122"), &selected_entry.stat_trak);
        selected_entry.stat_trak = (std::max)(selected_entry.stat_trak, -1);
        ImGui::SliderFloat(skCrypt("Wear"), &selected_entry.wear, FLT_MIN, 1.f, "%.10f", ImGuiSliderFlags_Logarithmic);

        const auto& kits = itemIndex == 1 ? SkinChanger::getGloveKits() : SkinChanger::getSkinKits();

        if (ImGui::BeginCombo(skCrypt("Paint Kit"), kits[selected_entry.paint_kit_vector_index].name.c_str())) {
            ImGui::PushID("Paint Kit");
            ImGui::PushID("Search");
            ImGui::SetNextItemWidth(-1.0f);
            static std::array<std::string, SkinChanger::weapon_names.size()> filters;
            auto& filter = filters[itemIndex];
            ImGui::InputTextWithHint("", "Search", &filter, ImGuiInputTextFlags_EnterReturnsTrue);
            if (ImGui::IsItemHovered() || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
                ImGui::SetKeyboardFocusHere(-1);
            ImGui::PopID();

            const std::wstring filterWide = Helpers::toUpper(Helpers::toWideString(filter));
            if (ImGui::BeginChild("##scrollarea", { 0, 6 * ImGui::GetTextLineHeightWithSpacing() })) {
                for (std::size_t i = 0; i < kits.size(); ++i) {
                    if (filter.empty() || passesFilter(kits[i].nameUpperCase, filterWide)) {
                        ImGui::PushID(i);
                        const auto selected = i == selected_entry.paint_kit_vector_index;
                        if (ImGui::SelectableWithBullet(kits[i].name.c_str(), rarityColor(kits[i].rarity), selected)) {
                            selected_entry.paint_kit_vector_index = i;
                            ImGui::CloseCurrentPopup();
                        }

                        if (ImGui::IsItemHovered()) {
                            if (const auto icon = SkinChanger::getItemIconTexture(kits[i].iconPath)) {
                                ImGui::BeginTooltip();
                                ImGui::Image(icon, { 200.0f, 150.0f });
                                ImGui::EndTooltip();
                            }
                        }
                        if (selected && ImGui::IsWindowAppearing())
                            ImGui::SetScrollHereY();
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndChild();
            ImGui::PopID();
            ImGui::EndCombo();
        }

        ImGui::Combo("Quality", &selected_entry.entity_quality_vector_index, [](void* data, int idx, const char** out_text) {
            *out_text = SkinChanger::getQualities()[idx].name.c_str(); // safe within this lamba
            return true;
            }, nullptr, SkinChanger::getQualities().size(), 5);

        if (itemIndex == 0) {
            ImGui::Combo("Knife", &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text) {
                *out_text = SkinChanger::getKnifeTypes()[idx].name.c_str();
                return true;
                }, nullptr, SkinChanger::getKnifeTypes().size(), 5);
        }
        else if (itemIndex == 1) {
            ImGui::Combo("Glove", &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text) {
                *out_text = SkinChanger::getGloveTypes()[idx].name.c_str();
                return true;
                }, nullptr, SkinChanger::getGloveTypes().size(), 5);
        }
        else {
            static auto unused_value = 0;
            selected_entry.definition_override_vector_index = 0;
            ImGui::Combo("Unavailable", &unused_value, "For knives or gloves\0");
        }

        ImGui::InputText("Name Tag", selected_entry.custom_name, 32);
    }

    ImGui::NextColumn();

    {
        ImGui::PushID("sticker");

        static std::size_t selectedStickerSlot = 0;

        ImGui::PushItemWidth(245.f);
        ImVec2 size;
        size.x = 0.0f;
        size.y = ImGui::GetTextLineHeightWithSpacing() * 5.25f + ImGui::GetStyle().FramePadding.y * 2.0f;

        if (ImGui::BeginListBox("", size)) {
            for (int i = 0; i < 5; ++i) {
                ImGui::PushID(i);

                const auto kit_vector_index = config->skinChanger[itemIndex].stickers[i].kit_vector_index;
                const std::string text = '#' + std::to_string(i + 1) + "  " + SkinChanger::getStickerKits()[kit_vector_index].name;

                if (ImGui::Selectable(text.c_str(), i == selectedStickerSlot))
                    selectedStickerSlot = i;

                ImGui::PopID();
            }
            ImGui::EndListBox();
        }


        auto& selected_sticker = selected_entry.stickers[selectedStickerSlot];

        const auto& kits = SkinChanger::getStickerKits();
        if (ImGui::BeginCombo("Sticker", kits[selected_sticker.kit_vector_index].name.c_str())) {
            ImGui::PushID("Sticker");
            ImGui::PushID("Search");
            static std::array<std::string, SkinChanger::weapon_names.size()> filters;
            auto& filter = filters[itemIndex];
            ImGui::InputTextWithHint("", "Search", &filter);
            if (ImGui::IsItemHovered() || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
                ImGui::SetKeyboardFocusHere(-1);
            ImGui::PopID();

            const std::wstring filterWide = Helpers::toUpper(Helpers::toWideString(filter));
            if (ImGui::BeginChild("##scrollarea", { 0, 6 * ImGui::GetTextLineHeightWithSpacing() })) {
                for (std::size_t i = 0; i < kits.size(); ++i) {
                    if (filter.empty() || passesFilter(kits[i].nameUpperCase, filterWide)) {
                        ImGui::PushID(i);
                        const auto selected = i == selected_sticker.kit_vector_index;
                        if (ImGui::SelectableWithBullet(kits[i].name.c_str(), rarityColor(kits[i].rarity), selected)) {
                            selected_sticker.kit_vector_index = i;
                            ImGui::CloseCurrentPopup();
                        }
                        if (ImGui::IsItemHovered()) {
                            if (const auto icon = SkinChanger::getItemIconTexture(kits[i].iconPath)) {
                                ImGui::BeginTooltip();
                                ImGui::Image(icon, { 200.0f, 150.0f });
                                ImGui::EndTooltip();
                            }
                        }
                        if (selected && ImGui::IsWindowAppearing())
                            ImGui::SetScrollHereY();
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndChild();
            ImGui::PopID();
            ImGui::EndCombo();
            ImGui::PopItemWidth();
        }

        ImGui::SliderFloat("Wear", &selected_sticker.wear, FLT_MIN, 1.0f, "%.10f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Scale", &selected_sticker.scale, 0.1f, 5.0f);
        ImGui::SliderFloat("Rotation", &selected_sticker.rotation, 0.0f, 360.0f);

        ImGui::PopID();
    }
    selected_entry.update();

    ImGui::Columns(1);

    ImGui::Separator();

    if (ImGui::Button("Update", { 130.0f, 30.0f }))
        SkinChanger::scheduleHudUpdate();

    ImGui::PopItemWidth();
}

/*void renderNadeHelperWindow() noexcept
{
    ImGui::BeginChild("##nh1", { ImGui::GetContentRegionAvail().x * 0.5f ,-1 });
    ImGui::BeginChildGroup("Grenade Helper", nullptr, gui->fonts.bold24px, ImGui::GetContentRegionAvail().x, 6);
    ImGui::Checkbox2("Enable nade helper", &nHConfig.enabled);
    ImGui::InputF("Render distance", &nHConfig.renderDistance, 200.0f, 20.f, 80.f, "%.0f");
    nHConfig.renderDistance = max(nHConfig.renderDistance, 100.0f);
    ImGui::Checkbox2("Load data on map change", &nHConfig.autoLoad);
    ImGui::hotkey3(Keybinds::keybindsMap[nadehelperKey], 2, "Aim key");
    if (Keybinds::keybindsMap[nadehelperKey].mode == KeybindMode::off)
        ImGui::BeginDisabled(true);
    ImGui::InputF("Activation distance", &nHConfig.aimDistance, 200.0f, 5.f, 10.f, "%.0f");
    ImGui::InputF("Aim FOV", &nHConfig.aimFov, 200.0f, 1.f, 2.f, "%.0f");
    ImGui::InputF("Aim Smoothing", &nHConfig.aimSmoothing, 200.0f, 1.f, 2.f, "%.0f");
    if (Keybinds::keybindsMap[nadehelperKey].mode == KeybindMode::off)
        ImGui::EndDisabled();
    ImGui::EndChildGroup();

    ImGui::BeginChildGroup("Colors", nullptr, gui->fonts.bold24px, ImGui::GetContentRegionAvail().x, 7);
    ImGuiCustom::colorPicker("Ground ring", nHConfig.groundRing);
    ImGuiCustom::colorPicker("Ground text", nHConfig.groundText);

    ImGuiCustom::colorPicker("Trajectory line", nHConfig.trajectory);
    ImGuiCustom::colorPicker("Trajectory box", nHConfig.trajectoryBox);
    ImGuiCustom::colorPicker("Trajectory text", nHConfig.trajectoryText);
    ImGuiCustom::colorPicker("Trajectory ring", nHConfig.trajectoryRing);
    ImGui::EndChildGroup();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChildGroup("#Grenades", nullptr, gui->fonts.bold24px, ImGui::GetContentRegionAvail().x, 0, ImGui::GetContentRegionAvail().y);


    if (ImGui::Button(ICON_FA_FOLDER_OPEN" Load"))
        ImGui::OpenPopup("load config");

    if (ImGui::BeginPopup("load config"))
    {
        if (ImGui::BeginChildFrame('L', { 200.f,200.f }))
        {
            for (const auto& configFile : configFiles)
            {

                if (ImGui::ListItem(configFile.first.c_str(), configFile.second ? ICON_FA_FILE_ALT : ICON_FA_FOLDER))
                {
                    loadData(configFile.first.c_str());
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndChildFrame();
        }
        ImGui::EndPopup();
    }

    timeToNextConfigRefresh -= ImGui::GetIO().DeltaTime;

    if (timeToNextConfigRefresh <= 0.0f) {
        configFiles.clear();

        std::error_code ec;
        std::transform(std::filesystem::directory_iterator{ config->path / ".." / "grenades", ec },
            std::filesystem::directory_iterator{ },
            std::back_inserter(configFiles),
            [](const auto& entry)
            {
                return std::make_pair<std::string, bool>(entry.path().filename().string(), entry.is_regular_file());

            }
        );
        timeToNextConfigRefresh = 0.1f;
    }

    ImGui::SameLine();

    static std::string savefilename;
    if (GrenadeData.empty())
        ImGui::BeginDisabled();
    if (ImGui::Button(ICON_FA_SAVE " save"))
    {
        savefilename.clear();
        ImGui::OpenPopup("save");
    }

    if (ImGui::BeginPopupModal("save", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::LabelText("", "File name:");
        ImGui::InputText("", &savefilename);
        if (savefilename.empty())
            ImGui::BeginDisabled();
        if (ImGui::Button("save"))
        {
            saveData(savefilename.c_str());
            ImGui::CloseCurrentPopup();
        }
        if (savefilename.empty())
            ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_TRASH_ALT" Clear"))
        GrenadeData.clear();
    if (GrenadeData.empty())
        ImGui::EndDisabled();
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.f);



    if (ImGui::Button(ICON_FA_PLUS_SQUARE " Add", { -1,0.f }))
    {
        addNade();
    }

    int scheduled_delete = -1;

    ImGui::BeginChildFrame('n', { -1,-1 });

    for (size_t i = GrenadeData.size(); i--;)
    {
        ImGui::PushID(i + 696);
        if (ImGui::ListItem(GrenadeData[i].name.c_str(),
            !GrenadeData[i].nadeType ? reinterpret_cast<const char*>(GetWeaponIcon(WeaponId::SmokeGrenade))
            : GrenadeData[i].nadeType == 1 ? reinterpret_cast<const char*>(GetWeaponIcon(WeaponId::Flashbang))
            : reinterpret_cast<const char*>(GetWeaponIcon(WeaponId::Molotov))
        ))
            ImGui::OpenPopup("Edit Nade");
        if (ImGui::BeginPopupModal("Edit Nade", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushID("##info");
            ImGui::LabelText("", "Name");
            ImGui::InputText("", &GrenadeData[i].name);
            ImGui::PopID();

            ImGui::PushID("##type");
            ImGui::LabelText("", "Type");

            ImGui::Combo("", &GrenadeData[i].nadeType, nade_types.data(), nade_types.size());
            ImGui::PopID();

            ImGui::PushID("##mod");
            ImGui::LabelText("", "Throw mode");

            ImGui::Combo("", &GrenadeData[i].throwMode, throw_modes.data(), throw_modes.size());
            ImGui::PopID();

            ImGui::LabelText("", "Ring radius");
            ImGui::InputFloat("", &GrenadeData[i].ringRadius);
            if (ImGui::Button("Delete"))
            {
                scheduled_delete = (int)i;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Close"))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    ImGui::EndChildFrame();

    if (scheduled_delete >= 0)
    {
        GrenadeData.erase(GrenadeData.begin() + scheduled_delete);
    }
    ImGui::EndChildGroup();
}*/
#define InsertSlider(x1,x2,x3,x4,x5) ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::NewLine(); ImGui::SameLine(42.f); ImGui::PushItemWidth(159.f); ImGui::SliderFloat(x1, &x2, x3, x4, x5); ImGui::PopItemWidth();
void GUI::renderMiscWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 350.0f);
    hotkey3(skCrypt("Menu Key"), config->misc.menuKey);
    ImGui::AlignTextToFramePadding();
    ImGui::Text(skCrypt("Reveals"));
    ImGui::SameLine();
    ImGui::PushID("stop using hex u dirty hoe, medusa.uno");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");
    if (ImGui::BeginPopup(""))
    {
        ImGui::Checkbox(skCrypt("Reveal ranks"), &config->misc.revealRanks);
        ImGui::Checkbox(skCrypt("Reveal money"), &config->misc.revealMoney);
        ImGui::Checkbox(skCrypt("Reveal suspect"), &config->misc.revealSuspect);
        ImGui::Checkbox(skCrypt("Reveal votes"), &config->misc.revealVotes);
        ImGui::Checkbox(skCrypt("Reveal enemy chat"), &config->misc.chatReveavler);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::Checkbox(skCrypt("Radar hack"), &config->misc.radarHack);
    ImGui::Checkbox(skCrypt("Spectator list"), &config->misc.spectatorList.enabled);
    ImGui::SameLine();
    ImGui::Checkbox(skCrypt("Keybinds list"), &config->misc.keybindList.enabled);

    ImGui::Checkbox(skCrypt("Bomb timer"), &config->misc.bombTimer.enabled);
    ImGui::SameLine();
    ImGuiCustom::colorPicker(skCrypt("Hurt indicator"), config->misc.hurtIndicator);
    ImGuiCustom::colorPicker(skCrypt("Event Logger"), config->misc.logger);
    ImGui::SameLine();

    ImGui::PushID("Logger");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {

        static bool modes[2]{ false, false };
        static const char* mode[]{ "Console", "On screen" };
        static std::string previewvaluemode = "";
        for (size_t i = 0; i < ARRAYSIZE(modes); i++)
        {
            modes[i] = (config->misc.loggerOptions.modes & 1 << i) == 1 << i;
        }
        if (ImGui::BeginCombo("Log output", previewvaluemode.c_str()))
        {
            previewvaluemode = "";
            for (size_t i = 0; i < ARRAYSIZE(modes); i++)
            {
                ImGui::Selectable(mode[i], &modes[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
            }
            ImGui::EndCombo();
        }
        for (size_t i = 0; i < ARRAYSIZE(modes); i++)
        {
            if (i == 0)
                previewvaluemode = "";

            if (modes[i])
            {
                previewvaluemode += previewvaluemode.size() ? std::string(", ") + mode[i] : mode[i];
                config->misc.loggerOptions.modes |= 1 << i;
            }
            else
            {
                config->misc.loggerOptions.modes &= ~(1 << i);
            }
        }

        static bool events[5]{ false, false, false, false, false };
        static const char* event[]{ "Damage dealt", "Damage received", "Hostage taken", "Bomb plants", "Purchases" };
        static std::string previewvalueevent = "";
        for (size_t i = 0; i < ARRAYSIZE(events); i++)
        {
            events[i] = (config->misc.loggerOptions.events & 1 << i) == 1 << i;
        }
        if (ImGui::BeginCombo("Log events", previewvalueevent.c_str()))
        {
            previewvalueevent = "";
            for (size_t i = 0; i < ARRAYSIZE(events); i++)
            {
                ImGui::Selectable(event[i], &events[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
            }
            ImGui::EndCombo();
        }
        for (size_t i = 0; i < ARRAYSIZE(events); i++)
        {
            if (i == 0)
                previewvalueevent = "";

            if (events[i])
            {
                previewvalueevent += previewvalueevent.size() ? std::string(", ") + event[i] : event[i];
                config->misc.loggerOptions.events |= 1 << i;
            }
            else
            {
                config->misc.loggerOptions.events &= ~(1 << i);
            }
        }
        ImGui::Combo(skCrypt("Position on screen"), &config->misc.loggerOptions.position, skCrypt("Upper-left\0Under crosshair\0"));
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::PushID(c_xor("Watermarky"));
    ImGui::Checkbox(c_xor("Watermark"), &config->misc.wm.enabled);
    ImGui::SameLine();
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox(c_xor("Show username"), &config->misc.wm.showUsername);
        ImGui::Checkbox(c_xor("Show time"), &config->misc.wm.showTime);
        ImGui::Checkbox(c_xor("Show FPS"), &config->misc.wm.showFps);
        ImGui::Checkbox(c_xor("Show ping"), &config->misc.wm.showPing);
        ImGui::Checkbox(c_xor("Show tickrate"), &config->misc.wm.showTicks);
        ImGui::Checkbox(c_xor("Show spotify"), &config->misc.wm.showSpotify);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::PushID("Player List");
    ImGui::Checkbox(skCrypt("Player List"), &config->misc.playerList.enabled);
    ImGui::SameLine();

    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox(skCrypt("Steam ID"), &config->misc.playerList.steamID);
        ImGui::Checkbox(skCrypt("Rank"), &config->misc.playerList.rank);
        ImGui::Checkbox(skCrypt("Wins"), &config->misc.playerList.wins);
        ImGui::Checkbox(skCrypt("Health"), &config->misc.playerList.health);
        ImGui::Checkbox(skCrypt("Armor"), &config->misc.playerList.armor);
        ImGui::Checkbox(skCrypt("Money"), &config->misc.playerList.money);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::Checkbox(skCrypt("Draw velocity"), &config->misc.velocity.enabled);
    ImGui::SameLine();

    ImGui::PushID("Draw velocity");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::SliderFloat("Position", &config->misc.velocity.position, 0.0f, 1.0f);
        ImGui::SliderFloat("Alpha", &config->misc.velocity.alpha, 0.0f, 1.0f);
        ImGuiCustom::colorPicker("Force color", config->misc.velocity.color.color.data(), nullptr, &config->misc.velocity.color.rainbow, &config->misc.velocity.color.rainbowSpeed, &config->misc.velocity.color.enabled);
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox(skCrypt("Keyboard display"), &config->misc.keyBoardDisplay.enabled);
    ImGui::SameLine();

    ImGui::PushID("Keyboard display");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::SliderFloat("Position", &config->misc.keyBoardDisplay.position, 0.0f, 1.0f);
        ImGui::Checkbox(skCrypt("Show key tiles"), &config->misc.keyBoardDisplay.showKeyTiles);
        ImGuiCustom::colorPicker("Color", config->misc.keyBoardDisplay.color);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::Checkbox(skCrypt("Indicators"), &config->misc.indicators.enabled);
    ImGui::SameLine();
    ImGui::PushID("COCKASJIFHAEWIGHUW");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        static const char* toShow[]{ "Double tap","On Shot","Damage override","Hitchance override","Fake flick","Desync side","Desync amount","Manual anti-aim","Freestand","Force baim","Trigger bot","Auto peek","Fake duck","Edge bug","Edge jump","Mini jump","Pixel surf","Jump bug","Block bot","Door spam", "Fake lag"};
        static bool toShow1[ARRAYSIZE(toShow)] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false ,false, false };
        static std::string previewvalue = "";
        bool once = false;
        ImGuiCustom::colorPicker(skCrypt("Color"), config->misc.indicators.color);
        ImGui::Combo(skCrypt("Indicators style"), &config->misc.indicators.style, skCrypt("Gamesense\0Under crosshair\0"));
        for (size_t i = 0; i < ARRAYSIZE(toShow1); i++)
        {
            toShow1[i] = (config->misc.indicators.toShow & 1 << i) == 1 << i;
        }
        if (ImGui::BeginCombo(skCrypt("What to show"), previewvalue.c_str()))
        {
            previewvalue = "";
            for (size_t i = 0; i < ARRAYSIZE(toShow); i++)
            {
                ImGui::Selectable(toShow[i], &toShow1[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
            }
            ImGui::EndCombo();
        }
        for (size_t i = 0; i < ARRAYSIZE(toShow); i++)
        {
            if (!once)
            {
                previewvalue = "";
                once = true;
            }
            if (toShow1[i])
            {
                previewvalue += previewvalue.size() ? std::string(", ") + toShow[i] : toShow[i];
                config->misc.indicators.toShow |= 1 << i;
            }
            else
            {
                config->misc.indicators.toShow &= ~(1 << i);
            }
        }
        /*ImGui::Checkbox(skCrypt("Show desync side"), &config->misc.IshowDesync);
        ImGui::Checkbox(skCrypt("Show anti-aim"), &config->misc.IshowManual);
        ImGui::Checkbox(skCrypt("Show misc"), &config->misc.IshowMisc);
        ImGui::Checkbox(skCrypt("Show legitbot"), &config->misc.IshowLegitBot);
        ImGui::Checkbox(skCrypt("Show ragebot"), &config->misc.IshowRageBot);*/
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGuiCustom::colorPicker(skCrypt("AA arrows"), config->condAA.visualize, &config->condAA.visualize.enabled);
    ImGui::PushID("aa arrowss");
    ImGui::SameLine();
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Combo(skCrypt("Arrows type"), &config->condAA.visualizeType, skCrypt("Desync\0Manuals\0"));
        ImGui::SliderFloat("", &config->condAA.visualizeOffset, 0.f, 250.f, skCrypt("Arrows offsets: %.2f"));
        ImGui::SliderFloat(" ", &config->condAA.visualizeSize, 0.f, 250.f, skCrypt("Arrows Size: %.2f"));
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::Checkbox(skCrypt("Anti-Aim lines"), &config->visuals.antiAimLines.enabled);
    ImGui::SameLine();
    ImGui::PushID("FOAGDOAS");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup(""))
    {
        ImGuiCustom::colorPicker(c_xor("Real color"), config->visuals.antiAimLines.real);
        ImGuiCustom::colorPicker(c_xor("Fake color"), config->visuals.antiAimLines.fake);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::Text(skCrypt(" "));
    ImGui::Text(skCrypt("Spammers"));
    ImGui::Checkbox(skCrypt("Clantag"), &config->misc.clantag);
    ImGui::Checkbox(skCrypt("Kill say"), &config->misc.killMessage);
    ImGui::Checkbox(skCrypt("Door spam"), &config->misc.doorSpam);
    ImGui::SameLine();
    ImGui::PushID("Door spam Key");
    ImGui::hotkey2("", config->misc.doorSpamKey);
    ImGui::PopID();
    //ImGui::Checkbox(skCrypt("Kill Say", &config->misc.killSay);
    ImGui::Checkbox(skCrypt("Name stealer"), &config->misc.nameStealer);
    ImGui::NextColumn();
    ImGui::PushItemWidth(220);
    ImGui::Checkbox(skCrypt("Fake latency"), &config->backtrack.fakeLatency);
    if (config->backtrack.fakeLatency)
        ImGui::SliderInt(skCrypt("Amount"), &config->backtrack.fakeLatencyAmount, 1, 200, "%d ms");
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(120.0f);
    ImGui::Checkbox(skCrypt("Anti AFK kick"), &config->misc.antiAfkKick);
    ImGui::Checkbox(skCrypt("Prepare revolver"), &config->misc.prepareRevolver);
    ImGui::SameLine();
    ImGui::PushID("Prepare revolver Key");
    ImGui::hotkey2("", config->misc.prepareRevolverKey);
    ImGui::PopID();

    ImGui::Checkbox(skCrypt("Auto pistol"), &config->misc.autoPistol);
    ImGui::SameLine();
    ImGui::Checkbox(skCrypt("Auto accept"), &config->misc.autoAccept);

    ImGui::PopID();


    ImGui::Combo(c_xor("Hit Sound"), &config->misc.hitSound, skCrypt("None\0Medusa.uno\0Metal\0Gamesense\0Bell\0Glass\0Coins\0Custom\0"));
    if (config->misc.hitSound == 7) {
        ImGui::InputText(c_xor("Hit Sound filename"), &config->misc.customHitSound);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(c_xor("audio file must be put in csgo/sound/ directory, also make sure it's .wav"));
    }
    ImGui::PushID(2);
    ImGui::Combo(c_xor("Kill Sound"), &config->misc.killSound, skCrypt("None\0Medusa.uno\0Metal\0Gamesense\0Bell\0Glass\0Coins\0Custom\0"));
    if (config->misc.killSound == 7) {
        ImGui::InputText(c_xor("Kill Sound filename"), &config->misc.customKillSound);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(c_xor("audio file must be put in csgo/sound/ directory"));
    }
    ImGui::PopID();
    ImGui::Checkbox(skCrypt("Fix tablet signal"), &config->misc.fixTabletSignal);
    ImGui::Checkbox(skCrypt("Sv pure bypass"), &config->misc.svPureBypass);
    ImGui::Checkbox(skCrypt("Unlock inventory"), &config->misc.inventoryUnlocker);
    ImGui::Checkbox(skCrypt("Preserve Killfeed"), &config->misc.preserveKillfeed.enabled);
    ImGui::SameLine();

    ImGui::PushID("Preserve Killfeed");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox(skCrypt("Only Headshots"), &config->misc.preserveKillfeed.onlyHeadshots);
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox(skCrypt("Killfeed changer"), &config->misc.killfeedChanger.enabled);
    ImGui::SameLine();

    ImGui::PushID("Killfeed changer");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox(skCrypt("Headshot"), &config->misc.killfeedChanger.headshot);
        ImGui::Checkbox(skCrypt("Dominated"), &config->misc.killfeedChanger.dominated);
        ImGui::SameLine();
        ImGui::Checkbox(skCrypt("Revenge"), &config->misc.killfeedChanger.revenge);
        ImGui::Checkbox(skCrypt("Penetrated"), &config->misc.killfeedChanger.penetrated);
        ImGui::Checkbox(skCrypt("Noscope"), &config->misc.killfeedChanger.noscope);
        ImGui::Checkbox(skCrypt("Thrusmoke"), &config->misc.killfeedChanger.thrusmoke);
        ImGui::Checkbox(skCrypt("Attackerblind"), &config->misc.killfeedChanger.attackerblind);
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox(skCrypt("Reportbot"), &config->misc.reportbot.enabled);
    ImGui::SameLine();
    ImGui::PushID("Reportbot");

    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::PushItemWidth(80.0f);
        ImGui::Combo("Target", &config->misc.reportbot.target, "Enemies\0Allies\0All\0");
        ImGui::InputInt("Delay (s)", &config->misc.reportbot.delay);
        config->misc.reportbot.delay = (std::max)(config->misc.reportbot.delay, 1);
        ImGui::InputInt("Rounds", &config->misc.reportbot.rounds);
        config->misc.reportbot.rounds = (std::max)(config->misc.reportbot.rounds, 1);
        ImGui::PopItemWidth();
        ImGui::Checkbox(skCrypt("Abusive Communications"), &config->misc.reportbot.textAbuse);
        ImGui::Checkbox(skCrypt("Griefing"), &config->misc.reportbot.griefing);
        ImGui::Checkbox(skCrypt("Wall Hacking"), &config->misc.reportbot.wallhack);
        ImGui::Checkbox(skCrypt("Aim Hacking"), &config->misc.reportbot.aimbot);
        ImGui::Checkbox(skCrypt("Other Hacking"), &config->misc.reportbot.other);
        if (ImGui::Button("Reset"))
            Misc::resetReportbot();
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox(skCrypt("Autobuy"), &config->misc.autoBuy.enabled);
    ImGui::SameLine();

    ImGui::PushID("Autobuy");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup(""))
    {
        ImGui::Combo("Primary weapon", &config->misc.autoBuy.primaryWeapon, skCrypt("None\0MAC-10 | MP9\0MP7 | MP5-SD\0UMP-45\0P90\0PP-Bizon\0Galil AR | FAMAS\0AK-47 | M4A4 | M4A1-S\0SSG 08\0SG553 |AUG\0AWP\0G3SG1 | SCAR-20\0Nova\0XM1014\0Sawed-Off | MAG-7\0M249\0Negev\0"));
        ImGui::Combo("Secondary weapon", &config->misc.autoBuy.secondaryWeapon, skCrypt("None\0Glock-18 | P2000 | USP-S\0Dual Berettas\0P250\0CZ75-Auto | Five-SeveN | Tec-9\0Desert Eagle | R8 Revolver\0"));
        ImGui::Combo("Armor", &config->misc.autoBuy.armor, skCrypt("None\0Kevlar\0Kevlar + Helmet\0"));

        static bool utilities[2]{ false, false };
        static const char* utility[]{ "Defuser","Taser" };
        static std::string previewvalueutility = "";
        for (size_t i = 0; i < ARRAYSIZE(utilities); i++)
        {
            utilities[i] = (config->misc.autoBuy.utility & 1 << i) == 1 << i;
        }
        if (ImGui::BeginCombo(skCrypt("Utility"), previewvalueutility.c_str()))
        {
            previewvalueutility = "";
            for (size_t i = 0; i < ARRAYSIZE(utilities); i++)
            {
                ImGui::Selectable(utility[i], &utilities[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
            }
            ImGui::EndCombo();
        }
        for (size_t i = 0; i < ARRAYSIZE(utilities); i++)
        {
            if (i == 0)
                previewvalueutility = "";

            if (utilities[i])
            {
                previewvalueutility += previewvalueutility.size() ? std::string(", ") + utility[i] : utility[i];
                config->misc.autoBuy.utility |= 1 << i;
            }
            else
            {
                config->misc.autoBuy.utility &= ~(1 << i);
            }
        }

        static bool nading[5]{ false, false, false, false, false };
        static const char* nades[]{ "HE Grenade","Smoke Grenade","Molotov","Flashbang","Decoy" };
        static std::string previewvaluenades = "";
        for (size_t i = 0; i < ARRAYSIZE(nading); i++)
        {
            nading[i] = (config->misc.autoBuy.grenades & 1 << i) == 1 << i;
        }
        if (ImGui::BeginCombo(skCrypt("Nades"), previewvaluenades.c_str()))
        {
            previewvaluenades = "";
            for (size_t i = 0; i < ARRAYSIZE(nading); i++)
            {
                ImGui::Selectable(nades[i], &nading[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
            }
            ImGui::EndCombo();
        }
        for (size_t i = 0; i < ARRAYSIZE(nading); i++)
        {
            if (i == 0)
                previewvaluenades = "";

            if (nading[i])
            {
                previewvaluenades += previewvaluenades.size() ? std::string(", ") + nades[i] : nades[i];
                config->misc.autoBuy.grenades |= 1 << i;
            }
            else
            {
                config->misc.autoBuy.grenades &= ~(1 << i);
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    if (ImGui::Button(skCrypt("Unlock hidden cvars")))
        Misc::unlockHiddenCvars();

    if (ImGui::Button(skCrypt("Join Discord server")))
       ShellExecuteA(NULL, NULL, skCrypt("https://discord.gg/XnGWg3uYED"), NULL, NULL, SW_SHOWNORMAL);

    if (ImGui::Button(skCrypt("RAGE QUIT!")))
        hooks->uninstall();
    ImGui::PopItemWidth();
    ImGui::Columns(1);
}

bool endswith(const char* string, char* end)
{
    int s_length = strlen(string);
    int end_length = strlen(end);

    if (end_length > s_length)
        return false;

    for (int i; i < end_length; i++)
    {
        if (string[s_length - i] != end[end_length - i])
            return false;
        return true;
    }
}

void GUI::renderConfigWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 260.0f);

    ImGui::PushItemWidth(250.0f);

    auto& configItems = config->getConfigs();
    static int currentConfig = -1;
    char end[] = ".cfg";
    static std::string buffer;

    timeToNextConfigRefresh -= ImGui::GetIO().DeltaTime;
    if (timeToNextConfigRefresh <= 0.0f) {
        config->listConfigs();
        if (const auto it = std::find(configItems.begin(), configItems.end(), buffer); it != configItems.end())
            currentConfig = std::distance(configItems.begin(), it);
        timeToNextConfigRefresh = 0.1f;
    }

    if (static_cast<std::size_t>(currentConfig) >= configItems.size())
        currentConfig = -1;

    if (ImGui::ListBox("", &currentConfig, [](void* data, int idx, const char** out_text) {
        auto& vector = *static_cast<std::vector<std::string>*>(data);
        *out_text = vector[idx].c_str();
        return true;
    }, &configItems, configItems.size(), 5) && currentConfig != -1)
        buffer = configItems[currentConfig];

        ImGui::PushID(0);
        if (ImGui::InputTextWithHint("", skCrypt("config name"), &buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (currentConfig != -1)
                config->rename(currentConfig, buffer.c_str());
        }
        ImGui::PopID();

        if (ImGui::Button(skCrypt("Open config directory"), ImVec2{ 250.0f, 25.0f }))
            config->openConfigDir();

        if (ImGui::Button(skCrypt("Create config"), ImVec2{ 250.0f, 25.0f }))
            config->add(buffer.c_str());

        if (currentConfig != -1) {
            if (ImGui::Button(skCrypt("Load selected"), ImVec2{ 250.0f, 25.0f })) {
                config->load(currentConfig, false);
                SkinChanger::scheduleHudUpdate();
                Misc::updateClanTag(true);
            }
            ImGui::PushID("SAVE");
            if (ImGui::Button(skCrypt("Save selected"), ImVec2{ 250.0f, 25.0f }))
                ImGui::OpenPopup(skCrypt("##reallySave"));
            if (ImGui::BeginPopup(skCrypt("##reallySave")))
            {
                ImGui::TextUnformatted(skCrypt("Are you sure?"));
                if (ImGui::Button(skCrypt("No"), { 45.0f, 0.0f }))
                    ImGui::CloseCurrentPopup();
                ImGui::SameLine();
                if (ImGui::Button(skCrypt("Yes"), { 45.0f, 0.0f }))
                {
                    config->save(currentConfig);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
            ImGui::PushID("DELETE");
            if (ImGui::Button(skCrypt("Delete selected"), ImVec2{ 250.0f, 25.0f }))
                ImGui::OpenPopup(skCrypt("##reallyDelete"));
            if (ImGui::BeginPopup(skCrypt("##reallyDelete")))
            {
                ImGui::TextUnformatted(skCrypt("Are you sure?"));
                if (ImGui::Button(skCrypt("No"), { 45.0f, 0.0f }))
                    ImGui::CloseCurrentPopup();
                ImGui::SameLine();
                if (ImGui::Button1(skCrypt("Yes"), { 45.0f, 0.0f }))
                {
                    config->remove(currentConfig);
                    if (static_cast<std::size_t>(currentConfig) < configItems.size())
                        buffer = configItems[currentConfig];
                    else
                        buffer.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }

        if (ImGui::Button(skCrypt("Reset selected"), ImVec2{ 250.0f, 25.0f }))
            ImGui::OpenPopup(skCrypt("Config to reset"));

        if (ImGui::BeginPopup(skCrypt("Config to reset"))) {
            static constexpr const char* names[]{ "Whole", "Legitbot", "Ragebot", "Anti aim", "Fakelag", "Backtrack", "Triggerbot", "Glow", "Chams", "ESP", "Visuals", "Skin changer", "Misc" };
            for (int i = 0; i < IM_ARRAYSIZE(names); i++) {
                if (i == 1) ImGui::Separator();

                if (ImGui::Selectable(names[i])) {
                    switch (i) {
                    case 0: config->reset(); Misc::updateClanTag(true); SkinChanger::scheduleHudUpdate(); break;
                    case 1: config->legitbob = { }; config->lgb = { }; config->legitbotKey.reset(); break;
                    case 2: config->ragebot = { }; config->rageBot = { }; config->ragebotKey.reset();  break;
                    case 3: config->rageAntiAim = { }; config->condAA = { }; break;
                    case 4: config->fakelag = { }; break;
                    case 5: config->backtrack = { }; break;
                    case 6: config->triggerbotKey.reset(); break;
                    case 7: Glow::resetConfig(); break;
                    case 8: config->chams = { }; break;
                    case 9: config->esp = { }; break;
                    case 10: config->visuals = { }; break;
                    case 11: config->skinChanger = { }; SkinChanger::scheduleHudUpdate(); break;
                    case 12: config->misc = { };  Misc::updateClanTag(true); break;
                    }
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();
        ImGuiCustom::colorPicker(skCrypt("Accent color"), config->menu.accentColor);
        ImGui::Combo(skCrypt("Window style"), &config->menu.windowStyle, skCrypt("Rounded\0Top Line\0Bottom Line\0Glow\0Modern\0"));
        ImGui::SliderFloat(skCrypt("Window transparency"), &config->menu.transparency, 0, 100, "%.f");
        //ImGui::Checkbox(skCrypt("Window borders"), &config->menu.accentColor);
        //ImGui::Checkbox(skCrypt("PassatHook prediction"), &config->predTest);
        //if (ImGui::IsItemHovered())
            //ImGui::SetTooltip(skCrypt("This feature is mostly experimental, imo it hits better without it"));
}

//void Active() { ImGuiStyle* Style = &ImGui::GetStyle(); Style->Colors[ImGuiCol_Button] = ImColor(config->menu.accentColor.color[0] * 0.35f, config->menu.accentColor.color[1] * 0.35f, config->menu.accentColor.color[2] * 0.35f, 255.f); Style->Colors[ImGuiCol_ButtonActive] = ImColor(config->menu.accentColor.color[0] * 0.35f, config->menu.accentColor.color[1] * 0.35f, config->menu.accentColor.color[2] * 0.35f, 255.f); Style->Colors[ImGuiCol_ButtonHovered] = ImColor(config->menu.accentColor.color[0] * 0.35f, config->menu.accentColor.color[1] * 0.35f, config->menu.accentColor.color[2] * 0.35f, 255.f); }
//void Hovered() { ImGuiStyle* Style = &ImGui::GetStyle(); Style->Colors[ImGuiCol_Button] = ImColor(14, 14, 14); Style->Colors[ImGuiCol_ButtonActive] = ImColor(14, 14, 14); Style->Colors[ImGuiCol_ButtonHovered] = ImColor(5, 5, 5); }
void ActiveTab() {
    ImGuiStyle* Style = &ImGui::GetStyle();
    Style->Colors[ImGuiCol_Text] = ImColor(Helpers::calculateColor(config->menu.accentColor));
    Style->Colors[ImGuiCol_Button] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.35f));
    Style->Colors[ImGuiCol_ButtonActive] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.35f));
    Style->Colors[ImGuiCol_ButtonHovered] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.35f));
}

void InactiveTab() {
    ImGuiStyle* Style = &ImGui::GetStyle();
    Style->Colors[ImGuiCol_Text] = ImColor(255.f, 255.f, 255.f, 255.f);
    Style->Colors[ImGuiCol_Button] = ImColor(14.f / 255, 14.f / 255, 14.f / 255);
    Style->Colors[ImGuiCol_ButtonActive] = ImColor(14.f / 255, 14.f / 255, 14.f / 255);
    Style->Colors[ImGuiCol_ButtonHovered] = ImColor(14.f / 255, 14.f / 255, 14.f / 255);
}
#include "Hacks/NadePredEXP.h"
void GUI::renderGuiStyle() noexcept
{
    ImColor colorbar = Helpers::calculateColor(config->menu.accentColor);
    ImDrawList* draw;
    std::vector<Snowflake::Snowflake> snow;
    posGui = ImGui::GetWindowPos();
    ImGuiStyle* Style = &ImGui::GetStyle();
    Style->Colors[ImGuiCol_Text] = ImColor(255, 255, 255);
    Style->WindowRounding = 0.0f;
    Style->ChildRounding = 0.0;
    Style->FrameBorderSize = 0.0f;
    Style->WindowBorderSize = 1.f;
    Style->Colors[ImGuiCol_Header] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.35f));
    Style->Colors[ImGuiCol_HeaderHovered] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.4f));
    Style->Colors[ImGuiCol_PopupBg] = ImColor(10, 10, 10);
    Style->Colors[ImGuiCol_WindowBg] = ImColor(0, 0, 0, 0);
    Style->Colors[ImGuiCol_ChildBg] = ImColor(31, 31, 31);
    Style->Colors[ImGuiCol_CheckMark] = ImColor(colorbar);
    Style->Colors[ImGuiCol_TextSelectedBg] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.45f));
    Style->Colors[ImGuiCol_FrameBg] = ImColor(22, 22, 22);
    Style->Colors[ImGuiCol_ScrollbarBg] = ImColor(14, 14, 14);
    Style->Colors[ImGuiCol_SliderGrabActive] = ImColor(Helpers::calculateColor(config->menu.accentColor, 1.f));
    Style->Colors[ImGuiCol_FrameBgHovered] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.4f));
    Style->Colors[ImGuiCol_FrameBgActive] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.2f));
    Style->Colors[ImGuiCol_HeaderActive] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.2f));
    Style->Colors[ImGuiCol_TitleBg] = ImColor(0, 0, 0);
    Style->Colors[ImGuiCol_TitleBgActive] = ImColor(0, 0, 0);
    Style->Colors[ImGuiCol_Border] = ImColor(255, 255, 255, 0);

    Style->Colors[ImGuiCol_ScrollbarGrab] = ImColor(colorbar);
    Style->Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(colorbar);
    Style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(colorbar);
    Style->FramePadding = { 4.5,4.5 };
    static auto Name = skCrypt("Medusa.uno");
    static auto Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    static int activeSubTabLegitbot = 1;
    static int activeSubTabRagebot = 1;
    static int activeSubTabAntiAim = 1;
    static int activeSubTabVisuals = 1;
    static int activeSubTabPlayers = 1;
    static int activeSubTabMisc = 1;
    static int activeSubTabConfigs = 1;
    static int activeSubTabSkin = 1;
    static int activeSubTabStyle = 1;
    if (ImGui::Begin(Name, NULL, Flags))
    {
        Style->ChildRounding = 3.f;
        Style->Colors[ImGuiCol_ChildBg] = colorbar;
        Style->Colors[ImGuiCol_Border] = ImColor(0,0,0,0);
        ImGui::BeginChild(skCrypt("##Back"), ImVec2{ 754- 70, 559 }, false);
        {
            ImGui::SetCursorPos(ImVec2{ 2, 2 });
            Style->Colors[ImGuiCol_ChildBg] = ImColor(14, 14, 14);
            Style->Colors[ImGuiCol_Button] = ImColor(14, 14, 14);
            Style->Colors[ImGuiCol_Border] = ImColor(14, 14, 14, 0);
            Style->Colors[ImGuiCol_ButtonHovered] = ImColor(35, 35, 35);
            Style->Colors[ImGuiCol_ButtonActive] = ImColor(65, 65, 65);
            Style->ChildRounding = 0.0;
            ImGui::BeginChild(skCrypt("##Main"), ImVec2{ 750 - 70, 555 }, false);
            {
                ImGui::BeginChild(skCrypt("##UP"), ImVec2{ 750 - 70, 40 }, false);
                ImVec2 pos;
                const char* text;
                const char* text1;
                text = " Medusa.uno";
                text1 = " z";              
                if (text == " Medusa.uno")
                {
                    pos = ImGui::GetWindowPos();
                    draw = ImGui::GetWindowDrawList();
                    ImVec2 size = ImGui::GetWindowSize();
                    {
                        draw->AddText(fonts.logo, 20.f, ImVec2(pos.x - 16.5f + 1, pos.y + 2), Helpers::calculateColor(config->menu.accentColor, 0.5f), skCrypt(" z"));
                        draw->AddText(fonts.logo, 20.f, ImVec2(pos.x - 16.5f, pos.y + 3), ImColor(255.f, 255.f, 255.f, 255.f), skCrypt(" z"));
                        draw->AddText(fonts.tahoma34, 24.f, ImVec2(pos.x + 26.5f + 1, pos.y + 2), Helpers::calculateColor(config->menu.accentColor, 0.5f), skCrypt(" Medusa.uno"));
                        draw->AddText(fonts.tahoma34, 24.f, ImVec2(pos.x + 26.5f, pos.y + 1), ImColor(255.f, 255.f, 255.f, 255.f), skCrypt(" Medusa.uno"));
                        ImGui::SameLine();
                        ImGui::PushFont(fonts.verdana);
                        if (activeTab == 4)
                        {
                            ImGui::SetCursorPosX(490.0f - 70 - 50 - 5);
                            if (activeSubTabVisuals == 1) ActiveTab(); else InactiveTab();
                            if (ImGui::Button1(skCrypt("World"), ImVec2{ 70, 30 })) activeSubTabVisuals = 1;
                            ImGui::SameLine();
                            if (activeSubTabVisuals == 2) ActiveTab(); else InactiveTab();
                            if (ImGui::Button1(skCrypt("ESP"), ImVec2{ 70, 30 })) activeSubTabVisuals = 2;
                            ImGui::SameLine();
                            if (activeSubTabVisuals == 3) ActiveTab(); else InactiveTab();
                            if (ImGui::Button1(skCrypt("Chams"), ImVec2{ 70, 30 })) activeSubTabVisuals = 3;
                            ImGui::SameLine();
                            if (activeSubTabVisuals == 4) ActiveTab(); else InactiveTab();
                            if (ImGui::Button1(skCrypt("Glow"), ImVec2{ 70, 30 })) activeSubTabVisuals = 4;
                        }
                        if (activeTab == 5)
                        {                            
                            ImGui::SetCursorPosX(530.0f - 70 - 50 - 10);
                            if (activeSubTabMisc == 1) ActiveTab(); else InactiveTab();
                            if (ImGui::Button1(skCrypt("Misc"), ImVec2{ 80, 30 })) activeSubTabMisc = 1;
                            ImGui::SameLine();
                            if (activeSubTabMisc == 2) ActiveTab(); else InactiveTab();
                            if (ImGui::Button1(skCrypt("Movement"), ImVec2{ 80, 30 })) activeSubTabMisc = 2;
                            ImGui::SameLine();
                            if (activeSubTabMisc == 3) ActiveTab(); else InactiveTab();
                            if (ImGui::Button1(skCrypt("Skin Changer"), ImVec2{ 90, 30 })) activeSubTabMisc = 3;
                        }
                        draw->AddRectFilled(ImVec2(pos.x + 0, pos.y + 28), ImVec2(pos.x + 7000, pos.y + 30), colorbar);
                        ImGui::PopFont();
                        ImGui::EndChild();
                        Style->Colors[ImGuiCol_ChildBg] = ImColor(14, 14, 14);
                        Style->Colors[ImGuiCol_Text] = ImColor(config->menu.accentColor.color[0], config->menu.accentColor.color[1], config->menu.accentColor.color[2], 255.f);
                        Style->Colors[ImGuiCol_Button] = ImColor(14, 14, 14);
                        Style->Colors[ImGuiCol_Border] = ImColor(255, 255, 255, 0);
                        Style->Colors[ImGuiCol_ButtonHovered] = ImColor(35, 35, 35);
                        Style->Colors[ImGuiCol_ButtonActive] = ImColor(65, 65, 65);
                        ImGui::SetCursorPos(ImVec2{ 0, 30 });
                        ImGui::BeginChild(skCrypt("##Childs"), ImVec2{ 750 - 70, 525 }, false);
                        {
                            auto frameRoundingB = Style->FrameRounding;
                            ImGui::BeginChild(skCrypt("##Left"), ImVec2{ 41, 525 }, false);
                            {
                                ImDrawList* draw;
                                ImVec2 pos;
                                pos = ImGui::GetWindowPos();
                                draw = ImGui::GetWindowDrawList();
                                int disabled = Helpers::calculateColor(config->menu.accentColor);
                                //Style->Colors[ImGuiCol_TextDisabled] = ImColor(Helpers::calculateColor(static_cast<float>(disabled * 0.75)));
                                ImGui::PushFont(fonts.fIcons);
                                Style->FrameRounding = 0.0f;
                                if (activeTab == 1) ActiveTab(); else InactiveTab();
                                if (ImGui::Button1(skCrypt("R"), ImVec2{ 39, 32 })) activeTab = 1;
                                //ImGui::Spacing();
                                if (activeTab == 2) ActiveTab(); else InactiveTab();
                                if (ImGui::Button1(skCrypt("L"), ImVec2{ 39, 32 })) activeTab = 2;
                                if (activeTab == 3) ActiveTab(); else InactiveTab();
                                if (ImGui::Button1(skCrypt("A"), ImVec2{ 39, 32 })) activeTab = 3;
                                if (activeTab == 4) ActiveTab(); else InactiveTab();
                                if (ImGui::Button1(skCrypt("P"), ImVec2{ 39, 32 })) activeTab = 4;
                                ImGui::PopFont();
                                ImGui::PushFont(fonts.tab_ico);
                                if (activeTab == 5) ActiveTab(); else InactiveTab();
                                if (ImGui::Button1(skCrypt("D"), ImVec2{ 39, 32 })) activeTab = 5;
                                if (activeTab == 6) ActiveTab(); else InactiveTab();
                                if (ImGui::Button1(skCrypt("E"), ImVec2{ 39, 32 })) activeTab = 6;
                                if (activeTab == 7 )ActiveTab(); else InactiveTab();
                                if (ImGui::Button1(skCrypt("M"), ImVec2{ 39, 32 })) activeTab = 7;
                                ImGui::PopFont();
                                draw->AddRectFilled(ImVec2(pos.x + 39, pos.y + 0), ImVec2(pos.x + 41, pos.y + 1000), colorbar);
                            }
                            ImGui::EndChild();
                            Style->FrameRounding = 3.f;
                            Style->ChildRounding = 0;
                            ImGui::SetCursorPos(ImVec2{ 41, 0 });
                            Style->Colors[ImGuiCol_ChildBg] = ImColor(14, 14, 14);
                            Style->Colors[ImGuiCol_Text] = ImColor(255, 255, 255, 255);
                            Style->Colors[ImGuiCol_Button] = ImColor(25, 25, 25);
                            Style->Colors[ImGuiCol_ButtonHovered] = ImColor(65, 65, 65);
                            Style->Colors[ImGuiCol_ButtonActive] = ImColor(45, 45, 45);
                            Style->Colors[ImGuiCol_TextDisabled] = ImColor(144, 144, 144);
                            ImGui::PushFont(fonts.verdana);
                            ImGui::BeginChild(skCrypt("##SubMain"), ImVec2{ 769 - 70 - 50, 525 }, false);
                            {
                                ImGui::SetCursorPos(ImVec2{ 14, 12 });
                                {
                                    //Style->Colors[ImGuiCol_Border] = ImColor(255, 255, 255, 145);
                                    switch (activeTab)
                                    {
                                    case 1: //Legitbot
                                         renderLegitBotWindow();
                                         break;
                                    case 2:
                                        renderRageBotWindow(draw);
                                        break;
                                    case 3: //AntiAim
                                        renderRageAntiAimWindow();
                                        break;
                                    case 4: // Visuals
                                        switch (activeSubTabVisuals)
                                        {
                                        case 1:
                                            //Main
                                            renderVisualsWindow();
                                            break;
                                        case 2:
                                            //Esp
                                            renderStreamProofESPWindow();
                                            renderESPpreview(ImGui::GetBackgroundDrawList(), pos);
                                            break;
                                        case 3:
                                            //Chams
                                            renderChamsWindow();
                                            break;
                                        case 4:
                                            //Glow
                                            renderGlowWindow();
                                            break;
                                        default:
                                            break;
                                        }
                                        break;

                                    case 5:
                                        switch (activeSubTabMisc)
                                        {
                                        case 1:
                                            //Main
                                            renderMiscWindow();
                                            break;
                                        case 2:
                                            //Movemint
                                            renderMovementWindow();
                                            break;
                                        case 3:
                                            //Skins
                                            renderSkinChangerWindow();
                                            break;
                                        default:
                                            break;
                                        }
                                        break;
                                    case 6:
                                        //Configs
                                        renderConfigWindow();
                                        break;
                                    case 7:
                                        NadePrediction::drawGUI();
                                        break;
                                    default:
                                        break;
                                    }
                                }
                                //if (config->misc.borders)
                                   // draw->AddRect({ 0,0 }, { 750 - 70, 555 }, colorbar, 5.f, 0, 2.f);
                                ImGui::EndChild();
                            }
                            ImGui::EndChild();
                        }
                        ImGui::EndChild();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndChild();
            }
            ImGui::End();
        }
        ImGui::PopFont();
        Style->Colors[ImGuiCol_WindowBg] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.5f };
    }
}

void GUI::renderGuiStyle1() noexcept
{
    ImGuiStyle* Style = &ImGui::GetStyle();
    Style->Colors[ImGuiCol_Text] = ImColor(255, 255, 255);
    Style->WindowRounding = 0.0f;
    Style->ChildRounding = 0.0;
    Style->FrameBorderSize = 0.0f;
    Style->WindowBorderSize = 1.f;
    Style->Colors[ImGuiCol_Header] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.35f));
    Style->Colors[ImGuiCol_HeaderHovered] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.4f));
    Style->Colors[ImGuiCol_PopupBg] = ImColor(10, 10, 10);
    Style->Colors[ImGuiCol_WindowBg] = ImColor(0, 0, 0, 0);
    Style->Colors[ImGuiCol_ChildBg] = ImColor(15,15,15);
    Style->Colors[ImGuiCol_CheckMark] = ImColor(Helpers::calculateColor(config->menu.accentColor));
    Style->Colors[ImGuiCol_TextSelectedBg] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.45f));
    Style->Colors[ImGuiCol_FrameBg] = ImColor(22, 22, 22);
    Style->Colors[ImGuiCol_ScrollbarBg] = ImColor(14, 14, 14);
    Style->Colors[ImGuiCol_SliderGrabActive] = ImColor(Helpers::calculateColor(config->menu.accentColor, 1.f));
    Style->Colors[ImGuiCol_FrameBgHovered] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.4f));
    Style->Colors[ImGuiCol_FrameBgActive] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.2f));
    Style->Colors[ImGuiCol_HeaderActive] = ImColor(Helpers::calculateColor(config->menu.accentColor, 0.2f));
    Style->Colors[ImGuiCol_TitleBg] = ImColor(0, 0, 0);
    Style->Colors[ImGuiCol_TitleBgActive] = ImColor(0, 0, 0);
    Style->Colors[ImGuiCol_Border] = ImColor(255, 255, 255, 0);

    Style->Colors[ImGuiCol_ScrollbarGrab] = ImColor(Helpers::calculateColor(config->menu.accentColor));
    Style->Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(Helpers::calculateColor(config->menu.accentColor));
    Style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(Helpers::calculateColor(config->menu.accentColor));
    Style->FramePadding = { 0.f, 0.f };
    static auto Name = skCrypt("Medusa.uno");
    static auto Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    if (!ImGui::Begin(Name, NULL, Flags))
        return;

    Style->ChildRounding = 5.f;
    Style->FramePadding = { 4.5f, 4.5f };
    ImGui::BeginChild(skCrypt("##Background"), { 650, 580 });
    {
        ImGui::PushFont(fonts.logo);
        auto logoSize = ImGui::CalcTextSize(skCrypt("z"));
        ImGui::PopFont();
        auto pos = ImGui::GetWindowPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRect(pos, { pos.x + 650, pos.y + 580 }, Helpers::calculateColor(config->menu.accentColor), 5.f, 0, 2.f);
        draw->AddLine({ pos.x, pos.y + 40 }, { pos.x + 650, pos.y + 40 }, Helpers::calculateColor(config->menu.accentColor), 2.f);
        draw->AddLine({ pos.x, pos.y + 520 }, { pos.x + 650, pos.y + 520 }, Helpers::calculateColor(config->menu.accentColor), 2.f);
        draw->AddText(fonts.logo, 25, { pos.x + 650 / 2 - logoSize.x / 2, pos.y + 7 }, Helpers::calculateColor(config->menu.accentColor), "z");
        ImGui::SetCursorPosY(42);
        ImGui::SetCursorPosX(2);
        ImGui::BeginChild(skCrypt("##Main"), { 646, 478 });
        {
            ImGui::SetCursorPos({2,3});
            ImGui::BeginChild(skCrypt("##Offset"), { 640, 472 });
            renderLegitBotWindow();
            ImGui::End();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
    ImGui::End();
}

