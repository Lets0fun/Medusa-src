#pragma once

#include <array>

enum class FrameStage;
class GameEvent;
struct ImDrawList;
struct ViewSetup;
struct UserCmd;
struct HitMarkerInfo {
    float hitMarkerExpTime;
    int hitMarkerDmg;
};

namespace Visuals
{
    static Vector ebpos = { 0,0,0 };
    void getCmd(UserCmd* cmd);
    void shadowChanger() noexcept;
    void drawHitboxMatrix(GameEvent* event = nullptr) noexcept;
    void drawSmokeTimerEvent(GameEvent* event) noexcept;
    void drawSmokeTimer(ImDrawList* drawList) noexcept;
    void drawMolotovTimerEvent(GameEvent* event) noexcept;
    void molotovExtinguishEvent(GameEvent* event) noexcept;
    void drawMolotovTimer(ImDrawList* drawList) noexcept;
    void visualizeSpread(ImDrawList* drawList) noexcept;
    void drawAimbotFov(ImDrawList* drawList) noexcept;
    void fullBright() noexcept;
    void FootstepESP(GameEvent* event) noexcept;
    void playerModel(FrameStage stage) noexcept;
    void modifySmoke(FrameStage stage) noexcept;
    void modifyMolotov(FrameStage stage) noexcept;
    void thirdperson() noexcept;
    void thirdperson1() noexcept;
    void removeVisualRecoil(FrameStage stage) noexcept;
    void removeBlur(FrameStage stage) noexcept;
    void removeGrass(FrameStage stage) noexcept;
    void remove3dSky() noexcept;
    void colorConsole(int reset = 0) noexcept;
    void removeShadows() noexcept;
    void applyZoom(FrameStage stage) noexcept;
    void applyScreenEffects() noexcept;
    void hitEffect(GameEvent* event = nullptr) noexcept;
    void transparentWorld(int resetType = -1) noexcept;
    void hitMarker(GameEvent* event, ImDrawList* drawList = nullptr) noexcept;
    void dmgMarker(GameEvent* event) noexcept;
    void drawMolotovPolygon(ImDrawList* drawList) noexcept;
    void motionBlur(ViewSetup* setup) noexcept;
    void disablePostProcessing(FrameStage stage) noexcept;
    bool removeHands(const char* modelName) noexcept;
    bool removeSleeves(const char* modelName) noexcept;
    bool removeWeapons(const char* modelName) noexcept;
    void skybox(FrameStage stage) noexcept;
    void TaserRange(ImDrawList* drawList);
    void updateShots(UserCmd* cmd) noexcept;
    void doBloomEffects() noexcept;
    void Trail() noexcept;
    void bulletTracer(GameEvent* event = nullptr) noexcept;
    void MaskChanger() noexcept;
    void drawBulletImpacts() noexcept;
    void bulletImpact(GameEvent& event) noexcept;
    void drawMolotovHull(ImDrawList* drawList) noexcept;
    void aaLines() noexcept;
    void drawSmokeHull(ImDrawList* drawList) noexcept;
    void penetrationCrosshair(ImDrawList* drawList) noexcept;
    inline constexpr std::array skyboxList{ "Default", "cs_baggage_skybox_", "cs_tibet", "embassy", "italy", "jungle", "nukeblank", "office", "sky_cs15_daylight01_hdr", "sky_cs15_daylight02_hdr", "sky_cs15_daylight03_hdr", "sky_cs15_daylight04_hdr", "sky_csgo_cloudy01", "sky_csgo_night_flat", "sky_csgo_night02", "sky_day02_05_hdr", "sky_day02_05", "sky_dust", "sky_l4d_rural02_ldr", "sky_venice", "vertigo_hdr", "vertigo", "vertigoblue_hdr", "vietnam", "sky_lunacy", "sky_hr_aztec", "Custom" };

    void updateEventListeners(bool forceRemove = false) noexcept;
    void updateInput() noexcept;
    void reset(int resetType) noexcept;
}
