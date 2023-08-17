#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <algorithm>
#include "imgui/imgui.h"
struct ImFont;

class GUI {
public:
    bool isTypingBool{ false };
    GUI() noexcept;
    ImVec2 posGui;
    ImFont* weaponIcons() const noexcept;
    ImFont* espFont() const noexcept;
    ImFont* indicators() const noexcept;
    ImFont* getFIconsFont() const noexcept;
    ImFont* loggerFont() const noexcept;
    ImFont* WiconsFonts() const noexcept;
    ImFont* grenades() const noexcept;
    void render() noexcept;
    ImFont* getUnicodeFont() const noexcept;
    void handleToggle() noexcept;
    bool isOpen() noexcept { return open; }
    ImFont* getTahoma28Font() const noexcept;
    ImFont* ver11() const noexcept;
    ImFont* getConsolas10Font() const noexcept;
    ImFont* getTabIcoFont() const noexcept;
    ImFont* getVerdanaFont() const noexcept;
    struct {
        ImFont* normal15px = nullptr;
        ImFont* consolas10 = nullptr;
        ImFont* tahoma34 = nullptr;
        ImFont* tahoma24 = nullptr;
        ImFont* tahoma28 = nullptr;
        ImFont* espFont = nullptr;
        ImFont* weaponIcons = nullptr;
        ImFont* tahoma16 = nullptr;
        ImFont* tahoma9 = nullptr;
        ImFont* verdana11 = nullptr;
        ImFont* tab_ico = nullptr;
        ImFont* fIcons = nullptr;
        ImFont* unicodeFont = nullptr;
        ImFont* verdana = nullptr;
        ImFont* logo = nullptr;
        ImFont* logoBig = nullptr;
        ImFont* indicators = nullptr;
        ImFont* nazi = nullptr;
        ImFont* nades = nullptr;
        ImFont* smallfonts = nullptr;
    } fonts;
private :
    bool open = true;
    void renderGuiStyle() noexcept;
    void renderGuiStyle1() noexcept;
    void renderRageAntiAimWindow() noexcept;
    void renderDebugWindow() noexcept;
    void renderChamsWindow() noexcept;
    void renderGlowWindow() noexcept;
    void renderStreamProofESPWindow() noexcept;
    void renderVisualsWindow() noexcept;
    void renderMovementWindow() noexcept;
    void renderSkinChangerWindow() noexcept;
    void renderMiscWindow() noexcept;
    void renderConfigWindow() noexcept;

    float timeToNextConfigRefresh = 0.1f;
};

inline std::unique_ptr<GUI> gui;
