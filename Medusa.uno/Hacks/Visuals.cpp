#include <array>
#include <cstring>
#include <string.h>
#include <deque>
#include <sys/stat.h>
#include <locale>
#include <functional>
#include <iostream>
#include <MLang.h>
#include "../QAngle.h"
#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"
#include "../xor.h"
#include "../fnv.h"
#include "../GameData.h"
#include "../Helpers.h"
#include "../SDK/PhysicsSurfaceProps.h"
#include "AimbotFunctions.h"
#include "Visuals.h"
#include "../SDK/EngineTrace.h"
#include "Animations.h"
#include "Backtrack.h"
#include "../IEffects.h"
#include "../SDK/ConVar.h"
#include "../SDK/DebugOverlay.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GameEvent.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Input.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/NetworkStringTable.h"
#include "../SDK/RenderContext.h"
#include "../SDK/Surface.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../SDK/ViewRenderBeams.h"
#include "../SDK/ViewSetup.h"
#include "../GUI.h"
#include "../Hooks.h"
#include "../xor.h"
#include "../Memory.h"
#include <src/base/md5.c>
#include "Animations.h"
#include "Misc.h"
#include "../render.hpp"
UserCmd* cmd2;
void Visuals::getCmd(UserCmd* cmd1)
{
    cmd2 = cmd1;
}

void trace_line(Vector start, Vector end, unsigned int mask, TraceFilter filter, Trace tr)
{
    Ray ray{ start, end };

    interfaces->engineTrace->traceRay(ray, mask, filter, tr);
}

void Visuals::shadowChanger() noexcept
{
    static auto cl_csm_rot_override = interfaces->cvar->findVar(skCrypt("cl_csm_rot_override"));
    static auto cl_csm_max_shadow_dist = interfaces->cvar->findVar(skCrypt("cl_csm_max_shadow_dist"));
    static auto cl_csm_rot_x = interfaces->cvar->findVar(skCrypt("cl_csm_rot_x"));
    static auto cl_csm_rot_y = interfaces->cvar->findVar(skCrypt("cl_csm_rot_y"));
    
    if (config->visuals.noShadows || !config->visuals.shadowsChanger.enabled)
    {
        cl_csm_rot_override->setValue(0);
        return;
    }

    cl_csm_max_shadow_dist->setValue(800);
    cl_csm_rot_override->setValue(1);
    cl_csm_rot_x->setValue(config->visuals.shadowsChanger.x);
    cl_csm_rot_y->setValue(config->visuals.shadowsChanger.y);
}

#define SMOKEGRENADE_LIFETIME 17.85f

struct smokeData
{
    float destructionTime;
    Vector pos;
};

static std::vector<smokeData> smokes;

void Visuals::drawSmokeTimerEvent(GameEvent* event) noexcept
{
    if (!event)
        return;

    smokeData data{};
    const auto time = memory->globalVars->realtime + SMOKEGRENADE_LIFETIME;
    const auto pos = Vector(event->getFloat("x"), event->getFloat("y"), event->getFloat("z"));
    data.destructionTime = time;
    data.pos = pos;
    smokes.push_back(data);
}

void Visuals::drawSmokeTimer(ImDrawList* drawList) noexcept
{
    if (!config->visuals.smokeTimer)
        return;

    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
        return;

    for (size_t i = 0; i < smokes.size(); i++) {
        const auto& smoke = smokes[i];

        auto time = smoke.destructionTime - memory->globalVars->realtime;
        std::ostringstream text; text << "y";

        ImVec2 pos;

        if (time >= 0.0f) {
            if (Helpers::worldToScreen(smoke.pos, pos)) {
                const auto radius = 20.f;
                const auto fraction = std::clamp(time / SMOKEGRENADE_LIFETIME, 0.0f, 1.0f);

                drawList->AddCircleFilled(pos, radius, Helpers::calculateColor(config->visuals.smokeTimerBG), 40);
                constexpr float pi = std::numbers::pi_v<float>;
                const auto arc270 = (3 * pi) / 2;
                drawList->PathArcTo(pos, radius, arc270 - (2 * pi * fraction), arc270, 40);
                drawList->PathStroke(Helpers::calculateColor(config->visuals.smokeTimerTimer), false, 2.45f);
                ImGui::PushFont(gui->grenades());
                auto textSize = ImGui::CalcTextSize(text.str().c_str());
                //drawList->AddText(gui->grenades(), 19.5f, ImVec2{ pos.x - (textSize.x / 2) - 8.80f, pos.y - (textSize.y / 2) - 1.f }, Helpers::calculateColor(config->visuals.smokeTimerText), text.str().c_str());
                drawList->AddText(gui->grenades(), 20.f, ImVec2{ pos.x - (textSize.x / 2) + 0.75f, pos.y - (textSize.y / 2) - 0.5f }, Helpers::calculateColor(config->visuals.smokeTimerText), text.str().c_str());
                ImGui::PopFont();
            }
        }
        else
            smokes.erase(smokes.begin() + i);
    }
}

#define MOLOTOV_LIFETIME 7.2f

struct molotovData
{
    float destructionTime;
    Vector pos;
};

static std::vector<molotovData> molotovs;

void Visuals::drawMolotovTimerEvent(GameEvent* event) noexcept
{
    if (!event)
        return;

    molotovData data{};
    const auto time = memory->globalVars->realtime + MOLOTOV_LIFETIME;
    const auto pos = Vector(event->getFloat("x"), event->getFloat("y"), event->getFloat("z"));
    data.destructionTime = time;
    data.pos = pos;
    molotovs.push_back(data);
}

void Visuals::molotovExtinguishEvent(GameEvent* event) noexcept {

    if (!event)
        return;

    if (molotovs.empty())
        return;

    molotovs.erase(molotovs.begin());
}

void Visuals::drawMolotovTimer(ImDrawList* drawList) noexcept
{
    if (!config->visuals.molotovTimer)
        return;

    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
        return;

    for (size_t i = 0; i < molotovs.size(); i++) {
        const auto& molotov = molotovs[i];

        auto time = molotov.destructionTime - memory->globalVars->realtime;
        std::ostringstream text; text << "z";
        ImGui::PushFont(gui->grenades());
        auto textSize = ImGui::CalcTextSize(text.str().c_str());
        ImGui::PopFont();
        ImVec2 pos;

        if (time >= 0.0f) {
            if (Helpers::worldToScreen(molotov.pos, pos)) {
                const auto radius = 20.0f;
                const auto fraction = std::clamp(time / MOLOTOV_LIFETIME, 0.0f, 1.0f);

                drawList->AddCircleFilled(pos, radius + 1.f, Helpers::calculateColor(config->visuals.molotovTimerBG), 40);
                constexpr float pi = std::numbers::pi_v<float>;
                const auto arc270 = (3 * pi) / 2;
                drawList->PathArcTo(pos, radius, arc270 - (2 * pi * fraction), arc270, 40);
                drawList->PathStroke(Helpers::calculateColor(config->visuals.molotovTimerTimer), false, 2.45f);               
                drawList->AddText(gui->grenades(), 20.f, ImVec2{ pos.x - (textSize.x / 2) + 1.25f, pos.y - (textSize.y / 2) - 0.60f }, Helpers::calculateColor(config->visuals.molotovTimerText), text.str().c_str());
            }
        }
    }
}

static bool worldToScreen1(const Vector& in, Vector& out) noexcept
{
    const auto& matrix = interfaces->engine->worldToScreenMatrix();
    float w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;

    if (w > 0.001f) {
        const auto [width, height] = interfaces->surface->getScreenSize();
        out.x = width / 2 * (1 + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w);
        out.y = height / 2 * (1 - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w);
        out.z = 0.0f;
        return true;
    }
    return false;
}

void Visuals::aaLines() noexcept
{
    if (!config->visuals.antiAimLines.enabled)
        return;

    auto AngleVectors = [](const Vector& angles, Vector* forward)
    {
        float	sp, sy, cp, cy;

        sy = sin(Helpers::deg2rad(angles.y));
        cy = cos(Helpers::deg2rad(angles.y));

        sp = sin(Helpers::deg2rad(angles.x));
        cp = cos(Helpers::deg2rad(angles.x));

        forward->x = cp * cy;
        forward->y = cp * sy;
        forward->z = -sp;
    };

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
        return;

    auto fakeMatrix = Animations::getFakeMatrix();
    float lbyAngle = localPlayer->lby();
    float realAngle = localPlayer.get()->getAnimstate()->footYaw;
    float fakeAngle = localPlayer->getMaxDesyncAngle();
    float absAngle = localPlayer->getAbsAngle().y;

    Vector src3D, dst3D, forward, src, dst;

    //
    // LBY
    //

    AngleVectors(Vector{ 0, lbyAngle, 0}, &forward);
    src3D = localPlayer->getAbsOrigin();
    dst3D = src3D + (forward * 40.f);

    if (!worldToScreen1(src3D, src) || !worldToScreen1(dst3D, dst))
        return;

    //interfaces->surface->setDrawColor(static_cast<int>(config->visuals.antiAimLines.fake.color[0] * 255.f), static_cast<int>(config->visuals.antiAimLines.fake.color[1] * 255.f), static_cast<int>(config->visuals.antiAimLines.fake.color[2] * 255.f), 255);
    interfaces->surface->setDrawColor(static_cast<int>(config->visuals.antiAimLines.fake.color[0] * 255.f), static_cast<int>(config->visuals.antiAimLines.fake.color[1] * 255.f), static_cast<int>(config->visuals.antiAimLines.fake.color[2] * 255.f));
    interfaces->surface->drawLine(src.x, src.y, dst.x, dst.y);

    std::string text = c_xor("FAKE");
    const auto [widthLby, heightLby] { interfaces->surface->getTextSize(hooks->smallFonts, std::wstring(text.begin(), text.end()).c_str()) };
    interfaces->surface->setTextColor(static_cast<int>(config->visuals.antiAimLines.fake.color[0] * 255.f), static_cast<int>(config->visuals.antiAimLines.fake.color[1] * 255.f), static_cast<int>(config->visuals.antiAimLines.fake.color[2] * 255.f));
    interfaces->surface->setTextFont(hooks->smallFonts);
    interfaces->surface->setTextPosition(dst.x - widthLby / 2, dst.y - heightLby / 2);
    interfaces->surface->printText(std::wstring(text.begin(), text.end()).c_str());
    //
    // Fake
    //

    AngleVectors(Vector{ 0, absAngle, 0 }, &forward);
    dst3D = src3D + (forward * 40.f);


    if (!worldToScreen1(src3D, src) || !worldToScreen1(dst3D, dst))
        return;

    AngleVectors(Vector{ 0, absAngle, 0 }, &forward);
    dst3D = src3D + (forward * 40.f);

    if (!worldToScreen1(src3D, src) || !worldToScreen1(dst3D, dst))
        return;

    interfaces->surface->setDrawColor(static_cast<int>(config->visuals.antiAimLines.real.color[0] * 255.f), static_cast<int>(config->visuals.antiAimLines.real.color[1] * 255.f), static_cast<int>(config->visuals.antiAimLines.real.color[2] * 255.f));
    interfaces->surface->drawLine(src.x, src.y, dst.x, dst.y);

    std::string text1 = c_xor("REAL");
    const auto [widthAbs, heightAbs] { interfaces->surface->getTextSize(hooks->smallFonts, std::wstring(text1.begin(), text1.end()).c_str()) };
    interfaces->surface->setTextColor(static_cast<int>(config->visuals.antiAimLines.real.color[0] * 255.f), static_cast<int>(config->visuals.antiAimLines.real.color[1] * 255.f), static_cast<int>(config->visuals.antiAimLines.real.color[2] * 255.f));
    interfaces->surface->setTextFont(hooks->smallFonts);
    interfaces->surface->setTextPosition(dst.x - widthAbs / 2, dst.y - heightAbs / 2);
    interfaces->surface->printText(std::wstring(text1.begin(), text1.end()).c_str());
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

void Visuals::visualizeSpread(ImDrawList* drawList) noexcept
{
    if (!config->visuals.spreadCircle.enabled)
        return;

    GameData::Lock lock;
    const auto& local = GameData::local();

    if (!local.exists || !local.alive || local.inaccuracy.null())
        return;

    if (memory->input->isCameraInThirdPerson)
        return;

    if (ImVec2 edge; Helpers::worldToScreen(local.inaccuracy, edge))
    {
        const auto& displaySize = ImGui::GetIO().DisplaySize;
        const auto radius = std::sqrtf(ImLengthSqr(edge - displaySize / 2.0f));

        if (radius > displaySize.x || radius > displaySize.y || !std::isfinite(radius))
            return;

        const auto color = Helpers::calculateColor(config->visuals.spreadCircle);
        AddRadialGradient(drawList, displaySize / 2, radius, Helpers::calculateColor(config->visuals.spreadCircle, 0.0f), color);
    }
}

void Visuals::drawAimbotFov(ImDrawList* drawList) noexcept
{
    if (!config->lgb.enabled)
        return;

    if (!config->lgb.legitbotFov.enabled)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    GameData::Lock lock;
    const auto& local = GameData::local();

    if (local.aimPunch.null())
        return;

    if (memory->input->isCameraInThirdPerson)
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return;

    if (!config->legitbob[weaponIndex].override)
        weaponIndex = getWeaponClass(activeWeapon->itemDefinitionIndex2());

    if (!config->legitbob[weaponIndex].override)
        weaponIndex = 0;

    if (ImVec2 pos; Helpers::worldToScreen(local.aimPunch, pos))
    {
        const auto& displaySize = ImGui::GetIO().DisplaySize;
        const auto radius = std::tan(Helpers::deg2rad(config->legitbob[weaponIndex].fov) / (16.0f / 6.0f)) / std::tan(Helpers::deg2rad(localPlayer->isScoped() ? localPlayer->fov() : (config->visuals.fov + 90.0f)) / 2.0f) * displaySize.x;
        if (radius > displaySize.x || radius > displaySize.y || !std::isfinite(radius))
            return;

        const auto color = Helpers::calculateColor(config->lgb.legitbotFov);
        drawList->AddCircleFilled(localPlayer->shotsFired() > 1 ? pos : displaySize / 2.0f, radius, color);
        if (config->lgb.legitbotFov.outline)
            drawList->AddCircle(localPlayer->shotsFired() > 1 ? pos : displaySize / 2.0f, radius, color | IM_COL32_A_MASK, 360);
    }
}

void Visuals::fullBright() noexcept
{
    static auto bright = interfaces->cvar->findVar(skCrypt("mat_fullbright"));
    bright->setValue(config->visuals.fullBright);
}

void Visuals::MaskChanger() noexcept
{
    if (!config->misc.mask)
    {
        return;
    }

    if (!interfaces->engine->isConnected())
        return;

    static int originalIdx = 0;

    if (!localPlayer) {
        originalIdx = 0;
        return;
    }

    if (!localPlayer->isAlive())
        return;

    //const char* path = interfaces->engine->getGameDirectory();
    if (config->misc.mask)
        localPlayer->AddonBits() |= 0x10000 | 0x00800;
    else
        return;
}

void Visuals::playerModel(FrameStage stage) noexcept
{
    if (stage != FrameStage::NET_UPDATE_POSTDATAUPDATE_START && stage != FrameStage::RENDER_END)
        return;

    static int originalIdx = 0;

    if (!localPlayer) {
        originalIdx = 0;
        return;
    }

    constexpr auto getModel = [](Team team) constexpr noexcept -> const char* {
        constexpr std::array models{
        "models/player/custom_player/legacy/ctm_fbi_variantb.mdl",
        "models/player/custom_player/legacy/ctm_fbi_variantf.mdl",
        "models/player/custom_player/legacy/ctm_fbi_variantg.mdl",
        "models/player/custom_player/legacy/ctm_fbi_varianth.mdl",
        "models/player/custom_player/legacy/ctm_sas.mdl",
        "models/player/custom_player/legacy/ctm_sas_variantf.mdl",
        "models/player/custom_player/legacy/ctm_sas_variantg.mdl",
        "models/player/custom_player/legacy/ctm_st6_variante.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantg.mdl",
        "models/player/custom_player/legacy/ctm_st6_varianti.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantk.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantm.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantn.mdl", // Primeiro Tenente | Brazilian 1st Battalion
        "models/player/custom_player/legacy/ctm_diver_varianta.mdl", // Cmdr. Davida 'Goggles' Fernandez | SEAL Frogman
        "models/player/custom_player/legacy/ctm_diver_variantb.mdl", // Cmdr. Frank 'Wet Sox' Baroud | SEAL Frogman
        "models/player/custom_player/legacy/ctm_diver_variantc.mdl", // Lieutenant Rex Krikey | SEAL Frogman
        "models/player/custom_player/legacy/tm_balkan_variantf.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantg.mdl",
        "models/player/custom_player/legacy/tm_balkan_varianth.mdl",
        "models/player/custom_player/legacy/tm_balkan_varianti.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantj.mdl",
        "models/player/custom_player/legacy/tm_leet_variantf.mdl",
        "models/player/custom_player/legacy/tm_leet_variantg.mdl",
        "models/player/custom_player/legacy/tm_leet_varianth.mdl",
        "models/player/custom_player/legacy/tm_leet_varianti.mdl",
        "models/player/custom_player/legacy/tm_phoenix_variantf.mdl",
        "models/player/custom_player/legacy/tm_phoenix_variantg.mdl",
        "models/player/custom_player/legacy/tm_phoenix_varianth.mdl",

        "models/player/custom_player/legacy/tm_pirate.mdl",
        "models/player/custom_player/legacy/tm_pirate_varianta.mdl",
        "models/player/custom_player/legacy/tm_pirate_variantb.mdl",
        "models/player/custom_player/legacy/tm_pirate_variantc.mdl",
        "models/player/custom_player/legacy/tm_pirate_variantd.mdl",
        "models/player/custom_player/legacy/tm_anarchist.mdl",
        "models/player/custom_player/legacy/tm_anarchist_varianta.mdl",
        "models/player/custom_player/legacy/tm_anarchist_variantb.mdl",
        "models/player/custom_player/legacy/tm_anarchist_variantc.mdl",
        "models/player/custom_player/legacy/tm_anarchist_variantd.mdl",
        "models/player/custom_player/legacy/tm_balkan_varianta.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantb.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantc.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantd.mdl",
        "models/player/custom_player/legacy/tm_balkan_variante.mdl",
        "models/player/custom_player/legacy/tm_jumpsuit_varianta.mdl",
        "models/player/custom_player/legacy/tm_jumpsuit_variantb.mdl",
        "models/player/custom_player/legacy/tm_jumpsuit_variantc.mdl",
        //gign
        "models/player/custom_player/legacy/ctm_gign.mdl",
        "models/player/custom_player/legacy/ctm_gign_varianta.mdl",
        "models/player/custom_player/legacy/ctm_gign_variantb.mdl",
        "models/player/custom_player/legacy/ctm_gign_variantc.mdl",
        "models/player/custom_player/legacy/ctm_gign_variantd.mdl",
        //gsg9
        "models/player/custom_player/legacy/ctm_gsg9.mdl",
        //the prof
        "models/player/custom_player/legacy/tm_professional_var2.mdl",
        "models/player/custom_player/legacy/tm_professional_var1.mdl",
        "models/player/custom_player/legacy/tm_professional_var3.mdl",
        "models/player/custom_player/legacy/tm_professional_var4.mdl",
        "models/player/custom_player/legacy/tm_phoenix_varianti.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantj.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantl.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantk.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantl.mdl",
        "models/player/custom_player/legacy/ctm_swat_variante.mdl",
        "models/player/custom_player/legacy/ctm_swat_variantf.mdl",
        "models/player/custom_player/legacy/ctm_swat_variantg.mdl",
        "models/player/custom_player/legacy/ctm_swat_varianth.mdl",
        "models/player/custom_player/legacy/ctm_swat_varianti.mdl",
        "models/player/custom_player/legacy/ctm_swat_variantj.mdl", // Chem-Haz Specialist | SWAT
        "models/player/custom_player/legacy/ctm_swat_variantk.mdl", // Lieutenant 'Tree Hugger' Farlow | SWAT
        "models/player/custom_player/legacy/tm_professional_varf.mdl",
        "models/player/custom_player/legacy/tm_professional_varf1.mdl",
        "models/player/custom_player/legacy/tm_professional_varf2.mdl",
        "models/player/custom_player/legacy/tm_professional_varf3.mdl",
        "models/player/custom_player/legacy/tm_professional_varf4.mdl",
        "models/player/custom_player/legacy/tm_professional_varg.mdl",
        "models/player/custom_player/legacy/tm_professional_varh.mdl",
        "models/player/custom_player/legacy/tm_professional_vari.mdl",
        "models/player/custom_player/legacy/tm_professional_varj.mdl",
        //new
        "models/player/custom_player/legacy/tm_jungle_raider_variantf2.mdl", // Trapper | Guerrilla Warfare
        "models/player/custom_player/legacy/tm_jungle_raider_variantf.mdl", // Trapper Aggressor | Guerrilla Warfare
        "models/player/custom_player/legacy/tm_jungle_raider_variante.mdl", // Vypa Sista of the Revolution | Guerrilla Warfare
        "models/player/custom_player/legacy/tm_jungle_raider_variantd.mdl", // Col. Mangos Dabisi | Guerrilla Warfare
        "models/player/custom_player/legacy/tm_jungle_raider_variantc.mdl", // Arno The Overgrown | Guerrilla Warfare
        "models/player/custom_player/legacy/tm_jungle_raider_variantb2.mdl", // 'Medium Rare' Crasswater | Guerrilla Warfare
        "models/player/custom_player/legacy/tm_jungle_raider_variantb.mdl", // Crasswater The Forgotten | Guerrilla Warfare
        "models/player/custom_player/legacy/tm_jungle_raider_varianta.mdl", // Elite Trapper Solman | Guerrilla Warfare

        "models/player/custom_player/legacy/ctm_gendarmerie_varianta.mdl", // Sous-Lieutenant Medic | Gendarmerie Nationale
        "models/player/custom_player/legacy/ctm_gendarmerie_variantb.mdl", // Chem-Haz Capitaine | Gendarmerie Nationale
        "models/player/custom_player/legacy/ctm_gendarmerie_variantc.mdl", // Chef d'Escadron Rouchard | Gendarmerie Nationale
        "models/player/custom_player/legacy/ctm_gendarmerie_variantd.mdl", // Aspirant | Gendarmerie Nationale
        "models/player/custom_player/legacy/ctm_gendarmerie_variante.mdl" // Officer Jacques Beltram | Gendarmerie Nationale
        };

        switch (team) {
        case Team::TT: return static_cast<std::size_t>(config->visuals.playerModelT - 1) < models.size() ? models[config->visuals.playerModelT - 1] : nullptr;
        case Team::CT: return static_cast<std::size_t>(config->visuals.playerModelCT - 1) < models.size() ? models[config->visuals.playerModelCT - 1] : nullptr;
        default: return nullptr;
        }
    };

    auto isValidModel = [](std::string name) noexcept -> bool
    {
        if (name.empty() || name.front() == ' ' || name.back() == ' ' || !name.ends_with(".mdl"))
            return false;

        if (!name.starts_with(c_xor("models")) && !name.starts_with(c_xor("/models")) && !name.starts_with(c_xor("\\models")))
            return false;

        //Check if file exists within directory
        std::string path = interfaces->engine->getGameDirectory();
        if (config->visuals.playerModel[0] != '\\' && config->visuals.playerModel[0] != '/')
            path += "/";
        path += config->visuals.playerModel;

        struct stat buf;
        if (stat(path.c_str(), &buf) != -1)
            return true;

        return false;
    };
    
    const bool custom = isValidModel(static_cast<std::string>(config->visuals.playerModel));

    if (const auto model = custom ? config->visuals.playerModel : getModel(localPlayer->getTeamNumber())) {
        if (stage == FrameStage::NET_UPDATE_POSTDATAUPDATE_START) {
            originalIdx = localPlayer->modelIndex();
            if (const auto modelprecache = interfaces->networkStringTableContainer->findTable(c_xor("modelprecache"))) {
                const auto index = modelprecache->addString(false, model);
                if (index == -1)
                    return;

                const auto viewmodelArmConfig = memory->getPlayerViewmodelArmConfigForPlayerModel(model);
                modelprecache->addString(false, viewmodelArmConfig[2]);
                modelprecache->addString(false, viewmodelArmConfig[3]);
            }
        }

        const auto idx = stage == FrameStage::RENDER_END && originalIdx ? originalIdx : interfaces->modelInfo->getModelIndex(model);

        localPlayer->setModelIndex(idx);

        if (const auto ragdoll = interfaces->entityList->getEntityFromHandle(localPlayer->ragdoll()))
            ragdoll->setModelIndex(idx);
    }
}

void Visuals::modifySmoke(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    constexpr std::array smokeMaterials{
        "particle/vistasmokev1/vistasmokev1_emods",
        "particle/vistasmokev1/vistasmokev1_emods_impactdust",
        "particle/vistasmokev1/vistasmokev1_fire",
        "particle/vistasmokev1/vistasmokev1_smokegrenade"
    };

    for (const auto mat : smokeMaterials) {
        const auto material = interfaces->materialSystem->findMaterial(mat);
        material->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noSmoke);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, stage == FrameStage::RENDER_START && config->visuals.wireframeSmoke);
    }
}

void Visuals::modifyMolotov(FrameStage stage) noexcept
{
    constexpr std::array fireMaterials{
        "decals/molotovscorch.vmt",
        "particle/fire_burning_character/fire_env_fire.vmt",
        "particle/fire_burning_character/fire_env_fire_depthblend.vmt",
        "particle/particle_flares/particle_flare_001.vmt",
        "particle/particle_flares/particle_flare_004.vmt",
        "particle/particle_flares/particle_flare_004b_mod_ob.vmt",
        "particle/particle_flares/particle_flare_004b_mod_z.vmt",
        "particle/fire_explosion_1/fire_explosion_1_bright.vmt",
        "particle/fire_explosion_1/fire_explosion_1b.vmt",
        "particle/fire_particle_4/fire_particle_4.vmt",
        "particle/fire_explosion_1/fire_explosion_1_oriented.vmt",
        "particle/vistasmokev1/vistasmokev1_nearcull_nodepth.vmt"
    };
    
    for (const auto mat : fireMaterials) {
        const auto material = interfaces->materialSystem->findMaterial(mat);
        material->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noMolotov);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, stage == FrameStage::RENDER_START && config->visuals.wireframeMolotov); 
    }
}

enum {
    OBS_MODE_NONE = 0,	// not in spectator mode
    OBS_MODE_DEATHCAM,	// special mode for death cam animation
    OBS_MODE_FREEZECAM,	// zooms to a target, and freeze-frames on them
    OBS_MODE_FIXED,		// view from a fixed camera position
    OBS_MODE_IN_EYE,	// follow a player in first person view
    OBS_MODE_CHASE,		// follow a player in third person view
    OBS_MODE_POI,		// PASSTIME point of interest - game objective, big fight, anything interesting; added in the middle of the enum due to tons of hard-coded "<ROAMING" enum compares
    OBS_MODE_ROAMING,	// free roaming

    NUM_OBSERVER_MODES
};

void Visuals::thirdperson() noexcept
{
    if (!config->visuals.thirdperson.enable)
        return;

    if (!localPlayer)
    {
        return;
    }

    if (!interfaces->engine->isConnected())
    {
        return;
    }

    const bool thirdPerson = config->visuals.thirdperson.enable && config->visuals.thirdperson.key.isActive();
    if (localPlayer->isAlive())
    {
        if (!localPlayer->getActiveWeapon())
            return;

        if (localPlayer->getActiveWeapon()->isGrenade() && config->visuals.thirdperson.disableOnGrenade)
        {
            memory->input->isCameraInThirdPerson = false;
        }
        else 
        {
            memory->input->isCameraInThirdPerson = thirdPerson;
        }
    }
    else if (config->visuals.thirdperson.whileDead && !localPlayer->isAlive() && localPlayer->getObserverTarget() && localPlayer->observerMode() == ObsMode::InEye)
    {
        localPlayer->observerMode() = ObsMode::Chase;
    }
}

void Visuals::colorConsole(int reset) noexcept
{

    static Material* material[5];
    if (!material[0] || !material[1] || !material[2] || !material[3] || !material[4]) {
        for (short h = interfaces->materialSystem->firstMaterial(); h != interfaces->materialSystem->invalidMaterial(); h = interfaces->materialSystem->nextMaterial(h)) {
            const auto mat = interfaces->materialSystem->getMaterial(h);

            if (!mat)
                continue;

            if (strstr(mat->getName(), "vgui_white"))
                material[0] = mat;
            else if (strstr(mat->getName(), "800corner1"))
                material[1] = mat;
            else if (strstr(mat->getName(), "800corner2"))
                material[2] = mat;
            else if (strstr(mat->getName(), "800corner3"))
                material[3] = mat;
            else if (strstr(mat->getName(), "800corner4"))
                material[4] = mat;
        }
    }
    else {
        for (unsigned int num = 0; num < 5; num++) {
            if (reset == 1)
            {
                material[num]->colorModulate(1.f, 1.f, 1.f);
                material[num]->alphaModulate(1.f);
                continue;
            }
            if (!config->visuals.console.enabled || !interfaces->engine->isConsoleVisible()) {
                material[num]->colorModulate(1.f, 1.f, 1.f);
                material[num]->alphaModulate(1.f);
                continue;
            }

            if (config->visuals.console.rainbow) {
                material[num]->colorModulate(rainbowColor(config->visuals.console.rainbowSpeed));
                material[num]->alphaModulate(config->visuals.console.color[3]);
            }
            else {
                material[num]->colorModulate(config->visuals.console.color[0], config->visuals.console.color[1], config->visuals.console.color[2]);
                material[num]->alphaModulate(config->visuals.console.color[3]);
            }
        }
    }
}

void Visuals::removeVisualRecoil(FrameStage stage) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    static Vector aimPunch;
    static Vector viewPunch;

    if (stage == FrameStage::RENDER_START) {
        aimPunch = localPlayer->aimPunchAngle();
        viewPunch = localPlayer->viewPunchAngle();

        if (config->visuals.noAimPunch)
            localPlayer->aimPunchAngle() = Vector{ };

        if (config->visuals.noViewPunch)
            localPlayer->viewPunchAngle() = Vector{ };

    } else if (stage == FrameStage::RENDER_END) {
        localPlayer->aimPunchAngle() = aimPunch;
        localPlayer->viewPunchAngle() = viewPunch;
    }
}

void Visuals::removeBlur(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    static auto blur = interfaces->materialSystem->findMaterial(skCrypt("dev/scope_bluroverlay"));
    blur->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noBlur);
}

void Visuals::removeGrass(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    constexpr auto getGrassMaterialName = []() noexcept -> const char* {
        switch (fnv::hashRuntime(interfaces->engine->getLevelName())) {
        case fnv::hash("dz_blacksite"): return "detail/detailsprites_survival";
        case fnv::hash("dz_sirocco"): return "detail/dust_massive_detail_sprites";
        case fnv::hash("coop_autumn"): return "detail/autumn_detail_sprites";
        case fnv::hash("dz_frostbite"): return "ski/detail/detailsprites_overgrown_ski";
        // dz_junglety has been removed in 7/23/2020 patch
        // case fnv::hash("dz_junglety"): return "detail/tropical_grass";
        default: return nullptr;
        }
    };

    if (const auto grassMaterialName = getGrassMaterialName())
        interfaces->materialSystem->findMaterial(grassMaterialName)->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noGrass);
}

void Visuals::remove3dSky() noexcept
{
    static auto sky = interfaces->cvar->findVar(skCrypt("r_3dsky"));
    sky->setValue(!config->visuals.no3dSky);
}

void Visuals::removeShadows() noexcept
{
    static auto shadows = interfaces->cvar->findVar(skCrypt("cl_csm_enabled"));
    shadows->setValue(!config->visuals.noShadows);
}

void Visuals::applyZoom(FrameStage stage) noexcept
{
    if (config->visuals.zoom && localPlayer) {
        if (stage == FrameStage::RENDER_START && (localPlayer->fov() == 90 || localPlayer->fovStart() == 90)) {
            if (config->visuals.zoomKey.isActive()) {
                localPlayer->fov() = 40;
                localPlayer->fovStart() = 40;
            }
        }
    }
}

#undef xor
#define DRAW_SCREEN_EFFECT(material) \
{ \
    const auto drawFunction = memory->drawScreenEffectMaterial; \
    int w, h; \
    interfaces->engine->getScreenSize(w, h); \
    __asm { \
        __asm push h \
        __asm push w \
        __asm push 0 \
        __asm xor edx, edx \
        __asm mov ecx, material \
        __asm call drawFunction \
        __asm add esp, 12 \
    } \
}

void Visuals::applyScreenEffects() noexcept
{
    if (!config->visuals.screenEffect)
        return;

    const auto material = interfaces->materialSystem->findMaterial([] {
        constexpr std::array effects{
            "effects/dronecam",
            "effects/underwater_overlay",
            "effects/healthboost",
            "effects/dangerzone_screen"
        };

        if (config->visuals.screenEffect <= 2 || static_cast<std::size_t>(config->visuals.screenEffect - 2) >= effects.size())
            return effects[0];
        return effects[config->visuals.screenEffect - 2];
    }());

    if (config->visuals.screenEffect == 1)
        material->findVar("$c0_x")->setValue(0.0f);
    else if (config->visuals.screenEffect == 2)
        material->findVar("$c0_x")->setValue(0.1f);
    else if (config->visuals.screenEffect >= 4)
        material->findVar("$c0_x")->setValue(1.0f);

    DRAW_SCREEN_EFFECT(material)
}

void Visuals::hitEffect(GameEvent* event) noexcept
{
    if (config->visuals.hitEffect && localPlayer) {
        static float lastHitTime = 0.0f;

        if (event && interfaces->engine->getPlayerFromUserID(event->getInt(c_xor("attacker"))) == localPlayer->index()) {
            lastHitTime = memory->globalVars->realtime;
            return;
        }

        if (lastHitTime + config->visuals.hitEffectTime >= memory->globalVars->realtime) {
            constexpr auto getEffectMaterial = [] {
                static constexpr const char* effects[]{
                "effects/dronecam",
                "effects/underwater_overlay",
                "effects/healthboost",
                "effects/dangerzone_screen"
                };

                if (config->visuals.hitEffect <= 2)
                    return effects[0];
                return effects[config->visuals.hitEffect - 2];
            };

           
            auto material = interfaces->materialSystem->findMaterial(getEffectMaterial());
            if (config->visuals.hitEffect == 1)
                material->findVar("$c0_x")->setValue(0.0f);
            else if (config->visuals.hitEffect == 2)
                material->findVar("$c0_x")->setValue(0.1f);
            else if (config->visuals.hitEffect >= 4)
                material->findVar("$c0_x")->setValue(1.0f);

            DRAW_SCREEN_EFFECT(material)
        }
    }
}

void Visuals::transparentWorld(int resetType) noexcept
{
    static int asus[2] = { -1, -1 };

    if (resetType >= 0)
    {
        asus[0] = -1;
        asus[1] = -1;
    }

    if (asus[0] == config->visuals.asusWalls && asus[1] == config->visuals.asusProps)
        return;

    for (short h = interfaces->materialSystem->firstMaterial(); h != interfaces->materialSystem->invalidMaterial(); h = interfaces->materialSystem->nextMaterial(h)) {
        const auto mat = interfaces->materialSystem->getMaterial(h);

        const std::string_view textureGroup = mat->getTextureGroupName();

        if (resetType == 1)
        {
            if (textureGroup.starts_with("World"))
                mat->alphaModulate(1.0f);

            if (textureGroup.starts_with("StaticProp"))
                mat->alphaModulate(1.0f);
            continue;
        }

        if (asus[0] != config->visuals.asusWalls && textureGroup.starts_with("World"))
            mat->alphaModulate(static_cast<float>(config->visuals.asusWalls) / 100.0f);

        if (asus[1] != config->visuals.asusProps && textureGroup.starts_with("StaticProp"))
            mat->alphaModulate(static_cast<float>(config->visuals.asusProps) / 100.0f);
    }
    asus[0] = config->visuals.asusWalls;
    asus[1] = config->visuals.asusProps;
}

void Visuals::hitMarker(GameEvent* event, ImDrawList* drawList) noexcept
{
    if (!config->visuals.hitMarkerColor.enabled)
        return;

    static float lastHitTime = 0.0f;
    int attacker;
    std::wstring damage;
    Vector bulletImpact;
    if (event) {
        damage = std::to_wstring(event->getInt(skCrypt("dmg_health")));
        attacker = event->getInt(skCrypt("attacker"));
        if (attacker != localPlayer->getUserId())
            return;
        bulletImpact = Vector{ event->getFloat("x"),  event->getFloat("y"),  event->getFloat("z") };
        if (localPlayer && attacker == localPlayer->getUserId())
            lastHitTime = memory->globalVars->realtime;
        return;
    }

    if (lastHitTime + config->visuals.hitMarkerTime < memory->globalVars->realtime)
        return;

    const auto& mid = ImGui::GetIO().DisplaySize / 2.0f;
    ImU32 color = Helpers::calculateColor(static_cast<int>(config->visuals.hitMarkerColor.color[0] * 255), static_cast<int>(config->visuals.hitMarkerColor.color[1] * 255), static_cast<int>(config->visuals.hitMarkerColor.color[2] * 255), static_cast<int>(Helpers::lerp(fabsf(lastHitTime + config->visuals.hitMarkerTime - memory->globalVars->realtime) / config->visuals.hitMarkerTime + FLT_EPSILON, 0.0f, 255.0f)));
    switch (config->visuals.hitMarker) {
    case 0:
        drawList->AddLine({ mid.x - 10, mid.y - 10 }, { mid.x - 4, mid.y - 4 }, color);
        drawList->AddLine({ mid.x + 10.5f, mid.y - 10.5f }, { mid.x + 4.5f, mid.y - 4.5f }, color);
        drawList->AddLine({ mid.x + 10.5f, mid.y + 10.5f }, { mid.x + 4.5f, mid.y + 4.5f }, color);
        drawList->AddLine({ mid.x - 10, mid.y + 10 }, { mid.x - 4, mid.y + 4 }, color);
        break;
    case 1:
        ImGui::PushFont(gui->getVerdanaFont());
        drawList->AddText(gui->getVerdanaFont(), 30.0f, ImVec2{ mid.x, mid.y } - ImGui::CalcTextSize("z") / 2, color, "z");
        ImGui::PopFont();
        break;
    }
}

void Visuals::dmgMarker(GameEvent* event) noexcept
{
    if (!localPlayer)
        return;
    
    static float lastHitTime = 0.0f;
    int attacker;
    std::wstring damage;
    Vector bulletImpact;
    if (event) {
        damage = std::to_wstring(event->getInt(skCrypt("dmg_health")));
        attacker = event->getInt(skCrypt("attacker"));
        if (attacker != localPlayer->getUserId())
            return;
        bulletImpact = Vector{ event->getFloat("x"),  event->getFloat("y"),  event->getFloat("z") };
        if (localPlayer && attacker == localPlayer->getUserId())
            lastHitTime = memory->globalVars->realtime;
        return;
    }

    if (lastHitTime + config->visuals.hitMarkerTime < memory->globalVars->realtime)
        return;

    if (config->visuals.damageMarker.enabled)
    {
        auto color = config->visuals.damageMarker;
        Vector screen;
        if (worldToScreen1(bulletImpact, screen))
            return;

        interfaces->surface->setTextColor(color.color[0] * 255, color.color[1] * 255, color.color[2] * 255, color.color[3] * 255);
        interfaces->surface->setTextFont(hooks->tahomaBoldAA);
        interfaces->surface->setTextPosition(screen.x, screen.y);
        interfaces->surface->printText(damage);
    }
}

void Visuals::drawHitboxMatrix(GameEvent* event) noexcept {

    if (!config->visuals.onHitHitbox.enabled) return;

    if (config->visuals.onHitHitbox.duration <= 0.f) return;

    if (!event) return;

    if (!localPlayer) return;

    const auto userID = interfaces->entityList->getEntity(interfaces->engine->getPlayerFromUserID(event->getInt("userid")));
    const auto attacker = interfaces->entityList->getEntity(interfaces->engine->getPlayerFromUserID(event->getInt("attacker")));

    if (!userID) return;

    if (!attacker) return;

    if (localPlayer->getUserId() != attacker->getUserId() || localPlayer->getUserId() == userID->getUserId()) return;

    StudioHdr* hdr = interfaces->modelInfo->getStudioModel(userID->getModel());
    StudioHitboxSet* set = hdr->getHitboxSet(0);

    auto records = Animations::getBacktrackRecords(userID->index());
    const matrix3x4* matrix = userID->getBoneCache().memory;
    auto bestFov{ 255.f };

    if (records && !records->empty()) {
        for (int i = static_cast<int>(records->size() - 1); i >= 0; i--)
        {
            if (Backtrack::valid(records->at(i).simulationTime))
            {
                for (auto& position : records->at(i).positions) {
                    auto angle = AimbotFunction::calculateRelativeAngle(localPlayer->getEyePosition(), position, interfaces->engine->getViewAngles());
                    auto fov = std::hypotf(angle.x, angle.y);
                    if (fov < bestFov) {
                        bestFov = fov;
                        matrix = records->at(i).matrix;
                    }
                }
            }
        }

    }

    int r, g, b, a;
    r = static_cast<int>(config->visuals.onHitHitbox.color.color[0] * 255.f);
    g = static_cast<int>(config->visuals.onHitHitbox.color.color[1] * 255.f);
    b = static_cast<int>(config->visuals.onHitHitbox.color.color[2] * 255.f);
    a = static_cast<int>(config->visuals.onHitHitbox.color.color[3] * 255.f);

    float d = config->visuals.onHitHitbox.duration;
    for (int i = 0; i < set->numHitboxes; i++) {
        StudioBbox* hitbox = set->getHitbox(i);

        if (!hitbox)
            continue;

        Vector vMin = hitbox->bbMin.transform(matrix[hitbox->bone]);
        Vector vMax = hitbox->bbMax.transform(matrix[hitbox->bone]);
        float size = hitbox->capsuleRadius;

        interfaces->debugOverlay->capsuleOverlay(vMin, vMax, size <= 0 ? 3.f : hitbox->capsuleRadius, r, g, b, a, d, 0, 1);
    }
}

struct MotionBlurHistory
{
    MotionBlurHistory() noexcept
    {
        lastTimeUpdate = 0.0f;
        previousPitch = 0.0f;
        previousYaw = 0.0f;
        previousPositon = Vector{ 0.0f, 0.0f, 0.0f };
        noRotationalMotionBlurUntil = 0.0f;
    }

    float lastTimeUpdate;
    float previousPitch;
    float previousYaw;
    Vector previousPositon;
    float noRotationalMotionBlurUntil;
};
// Used to sort Vectors in ccw order about a pivot.
static float ccw(const Vector& a, const Vector& b, const Vector& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

struct ccwSorter {
    const Vector& pivot;

    ccwSorter(const Vector& inPivot) : pivot(inPivot) { }

    bool operator()(const Vector& a, const Vector& b) {
        return ccw(pivot, a, b) < 0;
    }
};

static bool isLeftOf(const Vector& a, const Vector& b) {
    return (a.x < b.x || (a.x == b.x && a.y < b.y));
}

static std::vector<Vector> gift_wrapping(std::vector<Vector> v) {
    std::vector<Vector> hull;

    // There must be at least 3 points
    if (v.size() < 3)
        return hull;

    // Move the leftmost Vector to the beginning of our vector.
    // It will be the first Vector in our convext hull.
    std::swap(v[0], *min_element(v.begin(), v.end(), isLeftOf));

    // Repeatedly find the first ccw Vector from our last hull Vector
    // and put it at the front of our array. 
    // Stop when we see our first Vector again.
    do {
        hull.push_back(v[0]);
        std::swap(v[0], *min_element(v.begin() + 1, v.end(), ccwSorter(v[0])));
    } while (v[0].x != hull[0].x && v[0].y != hull[0].y);

    return hull;
}


void Visuals::drawMolotovPolygon(ImDrawList* drawList) noexcept
{
    if (!config->visuals.molotovPolygon.enabled)
        return;

    const auto color = Helpers::calculateColor(config->visuals.molotovPolygon);
    constexpr float pi = std::numbers::pi_v<float>;

    GameData::Lock lock;

    /* add the inferno position with largest possible inferno width so it's showing accurate radius. */
    auto flameCircumference = [](std::vector<Vector> points)
    {
        std::vector<Vector> new_points;

        for (size_t i = 0; i < points.size(); ++i)
        {
            const auto& pos = points[i];

            for (int j = 0; j <= 3; j++)
            {
                float p = j * (360.0f / 4.0f) * (pi / 200.0f);
                new_points.emplace_back(pos + Vector(std::cos(p) * 60.f, std::sin(p) * 60.f, 0.f));
            }
        }

        return new_points;
    };

    for (const auto& molotov : GameData::infernos())
    {
        /* we only wanted to draw the points on the edge, use giftwrap algorithm. */
        std::vector<Vector> giftWrapped = gift_wrapping(flameCircumference(molotov.points));

        /* transforms world position to screen position. */
        std::vector<ImVec2> points;

        for (size_t i = 0; i < giftWrapped.size(); ++i)
        {
            const auto& pos = giftWrapped[i];

            auto screen_pos = ImVec2();
            if (!Helpers::worldToScreen(pos, screen_pos))
                continue;

            points.emplace_back(ImVec2(screen_pos.x, screen_pos.y));
        }
        //drawList->AddPolyline(points.data(), points.size(), color, 0, 2.f);
        drawList->AddConvexPolyFilled(points.data(), points.size(), color);
    }
}
void Visuals::motionBlur(ViewSetup* setup) noexcept
{
    if (!localPlayer || !config->visuals.motionBlur.enabled)
        return;

    static MotionBlurHistory history;
    static float motionBlurValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (setup)
    {
        const float timeElapsed = memory->globalVars->realtime - history.lastTimeUpdate;

        const auto viewangles = setup->angles;

        const float currentPitch = Helpers::normalizeYaw(viewangles.x);
        const float currentYaw = Helpers::normalizeYaw(viewangles.y);

        Vector currentSideVector;
        Vector currentForwardVector;
        Vector currentUpVector;
        Vector::fromAngleAll(setup->angles, &currentForwardVector, &currentSideVector, &currentUpVector);

        Vector currentPosition = setup->origin;
        Vector positionChange = history.previousPositon - currentPosition;

        if ((positionChange.length() > 30.0f) && (timeElapsed >= 0.5f))
        {
            motionBlurValues[0] = 0.0f;
            motionBlurValues[1] = 0.0f;
            motionBlurValues[2] = 0.0f;
            motionBlurValues[3] = 0.0f;
        }
        else if (timeElapsed > (1.0f / 15.0f))
        {
            motionBlurValues[0] = 0.0f;
            motionBlurValues[1] = 0.0f;
            motionBlurValues[2] = 0.0f;
            motionBlurValues[3] = 0.0f;
        }
        else if (positionChange.length() > 50.0f)
        {
            history.noRotationalMotionBlurUntil = memory->globalVars->realtime + 1.0f;
        }
        else
        {
            const float horizontalFov = setup->fov;
            const float verticalFov = (setup->flAspectRatio <= 0.0f) ? (setup->fov) : (setup->fov / setup->flAspectRatio);
            const float viewdotMotion = currentForwardVector.dotProduct(positionChange);

            if (config->visuals.motionBlur.forwardEnabled)
                motionBlurValues[2] = viewdotMotion;

            const float sidedotMotion = currentSideVector.dotProduct(positionChange);
            float yawdiffOriginal = history.previousYaw - currentYaw;
            if (((history.previousYaw - currentYaw > 180.0f) || (history.previousYaw - currentYaw < -180.0f)) &&
                ((history.previousYaw + currentYaw > -180.0f) && (history.previousYaw + currentYaw < 180.0f)))
                yawdiffOriginal = history.previousYaw + currentYaw;

            float yawdiffAdjusted = yawdiffOriginal + (sidedotMotion / 3.0f);

            if (yawdiffOriginal < 0.0f)
                yawdiffAdjusted = std::clamp(yawdiffAdjusted, yawdiffOriginal, 0.0f);
            else
                yawdiffAdjusted = std::clamp(yawdiffAdjusted, 0.0f, yawdiffOriginal);

            const float undampenedYaw = yawdiffAdjusted / horizontalFov;
            motionBlurValues[0] = undampenedYaw * (1.0f - (fabsf(currentPitch) / 90.0f));

            const float pitchCompensateMask = 1.0f - ((1.0f - fabsf(currentForwardVector[2])) * (1.0f - fabsf(currentForwardVector[2])));
            const float pitchdiffOriginal = history.previousPitch - currentPitch;
            float pitchdiffAdjusted = pitchdiffOriginal;

            if (currentPitch > 0.0f)
                pitchdiffAdjusted = pitchdiffOriginal - ((viewdotMotion / 2.0f) * pitchCompensateMask);
            else
                pitchdiffAdjusted = pitchdiffOriginal + ((viewdotMotion / 2.0f) * pitchCompensateMask);


            if (pitchdiffOriginal < 0.0f)
                pitchdiffAdjusted = std::clamp(pitchdiffAdjusted, pitchdiffOriginal, 0.0f);
            else
                pitchdiffAdjusted = std::clamp(pitchdiffAdjusted, 0.0f, pitchdiffOriginal);

            motionBlurValues[1] = pitchdiffAdjusted / verticalFov;
            motionBlurValues[3] = undampenedYaw;
            motionBlurValues[3] *= (fabs(currentPitch) / 90.0f) * (fabs(currentPitch) / 90.0f) * (fabs(currentPitch) / 90.0f);

            if (timeElapsed > 0.0f)
                motionBlurValues[2] /= timeElapsed * 30.0f;
            else
                motionBlurValues[2] = 0.0f;

            motionBlurValues[2] = std::clamp((fabsf(motionBlurValues[2]) - config->visuals.motionBlur.fallingMin) / (config->visuals.motionBlur.fallingMax - config->visuals.motionBlur.fallingMin), 0.0f, 1.0f) * (motionBlurValues[2] >= 0.0f ? 1.0f : -1.0f);
            motionBlurValues[2] /= 30.0f;
            motionBlurValues[0] *= config->visuals.motionBlur.rotationIntensity * .15f * config->visuals.motionBlur.strength;
            motionBlurValues[1] *= config->visuals.motionBlur.rotationIntensity * .15f * config->visuals.motionBlur.strength;
            motionBlurValues[2] *= config->visuals.motionBlur.rotationIntensity * .15f * config->visuals.motionBlur.strength;
            motionBlurValues[3] *= config->visuals.motionBlur.fallingIntensity * .15f * config->visuals.motionBlur.strength;

        }

        if (memory->globalVars->realtime < history.noRotationalMotionBlurUntil)
        {
            motionBlurValues[0] = 0.0f;
            motionBlurValues[1] = 0.0f;
            motionBlurValues[3] = 0.0f;
        }
        else
        {
            history.noRotationalMotionBlurUntil = 0.0f;
        }
        history.previousPositon = currentPosition;

        history.previousPitch = currentPitch;
        history.previousYaw = currentYaw;
        history.lastTimeUpdate = memory->globalVars->realtime;
        return;
    }

    const auto material = interfaces->materialSystem->findMaterial("dev/motion_blur", "RenderTargets", false);
    if (!material)
        return;

    const auto MotionBlurInternal = material->findVar("$MotionBlurInternal", nullptr, false);

    MotionBlurInternal->setVecComponentValue(motionBlurValues[0], 0);
    MotionBlurInternal->setVecComponentValue(motionBlurValues[1], 1);
    MotionBlurInternal->setVecComponentValue(motionBlurValues[2], 2);
    MotionBlurInternal->setVecComponentValue(motionBlurValues[3], 3);

    const auto MotionBlurViewPortInternal = material->findVar("$MotionBlurViewportInternal", nullptr, false);

    MotionBlurViewPortInternal->setVecComponentValue(0.0f, 0);
    MotionBlurViewPortInternal->setVecComponentValue(0.0f, 1);
    MotionBlurViewPortInternal->setVecComponentValue(1.0f, 2);
    MotionBlurViewPortInternal->setVecComponentValue(1.0f, 3);

    DRAW_SCREEN_EFFECT(material)
}

void Visuals::disablePostProcessing(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    *memory->disablePostProcessing = stage == FrameStage::RENDER_START && config->visuals.disablePostProcessing;
}

bool Visuals::removeHands(const char* modelName) noexcept
{
    return config->visuals.noHands && std::strstr(modelName, "arms") && !std::strstr(modelName, "sleeve");
}

bool Visuals::removeSleeves(const char* modelName) noexcept
{
    return config->visuals.noSleeves && std::strstr(modelName, "sleeve");
}

bool Visuals::removeWeapons(const char* modelName) noexcept
{
    return config->visuals.noWeapons && std::strstr(modelName, skCrypt("models/weapons/v_"))
        && !std::strstr(modelName, skCrypt("arms")) && !std::strstr(modelName, skCrypt("tablet"))
        && !std::strstr(modelName, skCrypt("parachute")) && !std::strstr(modelName, skCrypt("fists"));
}

void Visuals::skybox(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    if (stage == FrameStage::RENDER_START && config->visuals.skybox > 0 && static_cast<std::size_t>(config->visuals.skybox) < skyboxList.size() - 1) {
        memory->loadSky(skyboxList[config->visuals.skybox]);
    }
    else if (config->visuals.skybox == 26 && stage == FrameStage::RENDER_START) {
        memory->loadSky(config->visuals.customSkybox.c_str());
    }
    else {
        static const auto sv_skyname = interfaces->cvar->findVar(skCrypt("sv_skyname"));
        memory->loadSky(sv_skyname->string);
    }
}

struct shotRecords
{
    shotRecords(Vector eyePosition, float time) noexcept
    {
        this->eyePosition = eyePosition;
        this->time = time;
    }
    Vector eyePosition;
    bool gotImpact{ false };
    float time{ 0.0f };
};

void Visuals::FootstepESP(GameEvent* event) noexcept
{
    if (!config->visuals.footstepBeamsA.enabled && !config->visuals.footstepBeamsE.enabled)
        return;

    if (!localPlayer)
        return;

    for (const auto& e : GameData::players())
    {
        if (!e.alive || e.handle == localPlayer->handle() || e.dormant)
            continue; 

        if (e.enemy && !config->visuals.footstepBeamsE.enabled)
            continue;
        else if (!e.enemy && !config->visuals.footstepBeamsA.enabled)
            continue;

        int thickness = e.enemy ? config->visuals.footstepBeamThicknessE : config->visuals.footstepBeamThicknessA;
        int radius = e.enemy ? config->visuals.footstepBeamRadiusE : config->visuals.footstepBeamRadiusA;
        Color4 color = e.enemy ? config->visuals.footstepBeamsE : config->visuals.footstepBeamsA;

        auto model_index = interfaces->modelInfo->getModelIndex(skCrypt("sprites/white.vmt"));

        BeamInfo info;

        info.type = TE_BEAMRINGPOINT;
        info.modelName = skCrypt("sprites/white.vmt");
        info.modelIndex = model_index;
        info.haloIndex = -1;
        info.haloScale = 3.0f;
        info.life = 2.0f;
        info.width = (thickness / 2);
        info.fadeLength = 0.0f;
        info.amplitude = 0.0f;
        info.red = color.color[0] * 255;
        info.green = color.color[1] * 255;
        info.blue = color.color[2] * 255;
        info.brightness = color.color[3] * 255;
        info.speed = 0.0f;
        info.startFrame = 0;
        info.frameRate = 60.0f;
        info.segments = -1;
        info.flags = FBEAM_FADEOUT;
        info.ringCenter = e.origin + Vector(0.0f, 0.0f, 5.0f);
        info.ringStartRadius = 5.0f;
        info.ringEndRadius = radius;
        info.renderable = true;

        Beam* beam_draw = memory->viewRenderBeams->createBeamRingPoints(info);

        if (beam_draw)
            memory->viewRenderBeams->drawBeam(beam_draw);
    }
}

std::deque<shotRecords> shotRecord;

void Visuals::updateShots(UserCmd* cmd) noexcept
{
    if (!config->visuals.bulletTracers.enabled)
        return;

    if (!localPlayer || !localPlayer->isAlive())
    {
        shotRecord.clear();
        return;
    }

    if (!(cmd->buttons & UserCmd::IN_ATTACK))
        return;

    if (localPlayer->nextAttack() > memory->globalVars->serverTime() || localPlayer->isDefusing() || localPlayer->waitForNoAttack())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto())
        return;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    if (!*memory->gameRules || (*memory->gameRules)->freezePeriod())
        return;

    if (localPlayer->flags() & (1 << 6)) //Frozen
        return;

    shotRecord.push_back(shotRecords(localPlayer->getEyePosition(), memory->globalVars->serverTime()));

    while (!shotRecord.empty() && fabsf(shotRecord.front().time - memory->globalVars->serverTime()) > 1.0f)
        shotRecord.pop_front();
}
struct ColorSex {
    std::array<float, 4> color{ 1.0f, 1.0f, 1.0f, 1.0f };
};

void Visuals::doBloomEffects() noexcept
{
    if (!localPlayer || !interfaces->engine->isConnected())
        return;

    for (int i = 0; i < 2048; i++)
    {
        Entity* ent = interfaces->entityList->getEntity(i);

        if (!ent)
            continue;

        if (!std::string(ent->getClientClass()->networkName).ends_with(skCrypt("TonemapController")))
            continue;

        bool enabled = config->visuals.PostEnabled;
        ent->useCustomAutoExposureMax() = enabled;
        ent->useCustomAutoExposureMin() = enabled;
        ent->useCustomBloomScale() = enabled;
        ConVar* modelAmbientMin = interfaces->cvar->findVar(skCrypt("r_modelAmbientMin"));
        auto backupAmbient = modelAmbientMin->getFloat();
        if (enabled)
        {
            float worldExposure = config->visuals.worldExposure;
            ent->customAutoExposureMin() = worldExposure;
            ent->customAutoExposureMax() = worldExposure;

            float bloomScale = config->visuals.bloomScale;
            ent->customBloomScale() = bloomScale;

            modelAmbientMin->setValue(config->visuals.modelAmbient);
        }
    }
}

void Visuals::Trail() noexcept
{
    if (!config->visuals.playerTrailColor.enabled)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (localPlayer->velocity().length2D() == 0.f)
        return;
    if (localPlayer->velocity().length2D() >= 5.f)
    {
        if (config->misc.autoPeekKey.isActive() && config->misc.autoPeek.enabled)
            return;

        interfaces->m_Effects->EnergySplash(localPlayer->getAbsOrigin(), Vector(0, 0, 0), false);
        auto material = interfaces->materialSystem->findMaterial(skCrypt("effects/spark"));
        if (!material)
            return;
        //if (config->misc.autoPeekStyle != 3 && config->misc.autoPeekStyle != 2)
        if (!config->visuals.playerTrailColor.rainbow)
            material->colorModulate(config->visuals.playerTrailColor.color[0], config->visuals.playerTrailColor.color[1], config->visuals.playerTrailColor.color[2]);
        else
            material->colorModulate(rainbowColor(config->visuals.playerTrailColor.rainbowSpeed));
    }    
}

void Visuals::TaserRange(ImDrawList* draw)
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    auto weapon = localPlayer->getActiveWeapon();

    if (!weapon)
        return;

    if (localPlayer->getAbsOrigin().notNull())
    {
        constexpr float step = 3.141592654f * 2.0f / 20.0f;

        const ImU32 color = Helpers::calculateColor(config->visuals.TaserRange);
        const ImU32 color1 = Helpers::calculateColor(config->visuals.KnifeRange);
        auto flags_backup = draw->Flags;
        if (config->visuals.TaserRange.enabled && weapon->itemDefinitionIndex2() == WeaponId::Taser)
        {
            std::vector<ImVec2> points;
            for (float lat = 0.f; lat <= 3.141592654f * 2.0f; lat += step)
            {
                const auto& point3d = Vector{ std::sin(lat), std::cos(lat), 0.f } * 150.f;
                ImVec2 point2d;
                if (Helpers::worldToScreen(localPlayer->getAbsOrigin() + point3d, point2d))
                    points.push_back(point2d);
            }
            draw->AddPolyline(points.data(), points.size(), color, true, 2.f);
        } 
        if (config->visuals.KnifeRange.enabled && weapon->isKnife())
        {
            std::vector<ImVec2> points;
            for (float lat = 0.f; lat <= 3.141592654f * 2.0f; lat += step)
            {
                const auto& point3d = Vector{ std::sin(lat), std::cos(lat), 0.f } * 64.f;
                ImVec2 point2d;
                if (Helpers::worldToScreen(localPlayer->getAbsOrigin() + point3d, point2d))
                    points.push_back(point2d);
            }
            draw->AddPolyline(points.data(), points.size(), color1, true, 2.f);
        }
        // why not 2 functions? cause yes
    }
}

void Visuals::bulletTracer(GameEvent* event) noexcept
{
    if (!config->visuals.bulletTracers.enabled)
    {
        shotRecord.clear();
        return;
    }

    if (!localPlayer || shotRecord.empty())
        return;

    if (!interfaces->debugOverlay)
        return;

    switch (fnv::hashRuntime(event->getName())) {
    case fnv::hash("round_start"):
        shotRecord.clear();
        break;
    case fnv::hash("weapon_fire"):
        if (event->getInt("userid") != localPlayer->getUserId())
            return;

        if (shotRecord.front().gotImpact)
            shotRecord.pop_front();
        break;
    case fnv::hash("bullet_impact"):
    {
        if (shotRecord.front().gotImpact)
            return;

        if (event->getInt("userid") != localPlayer->getUserId())
            return;

        if (shotRecord.front().eyePosition.null())
        {
            shotRecord.pop_front();
            return;
        }

        const auto bulletImpact = Vector{ event->getFloat("x"),  event->getFloat("y"),  event->getFloat("z") };
        const auto angle = AimbotFunction::calculateRelativeAngle(shotRecord.front().eyePosition, bulletImpact, Vector{ });
        const auto end = bulletImpact + Vector::fromAngle(angle) * 2000.f;

        //interfaces->debugOverlay->addLineOverlay(shotRecord.front().eyePosition, bulletImpact, 255.0f * config->visuals.bulletTracers.color[0], 255.0f * config->visuals.bulletTracers.color[1], 255.0f * config->visuals.bulletTracers.color[2], 255.0f * config->visuals.bulletTracers.color[3], 2.5f);
        BeamInfo beamInfo;

        beamInfo.start = shotRecord.front().eyePosition;
        beamInfo.end = end;
        beamInfo.type = TE_BEAMPOINTS;
        switch (config->visuals.bulletSprite)
        {
        case 0:
            beamInfo.modelName = skCrypt("sprites/white.vmt");
            break;
        case 1:
            beamInfo.modelName = skCrypt("sprites/purplelaser1.vmt");
            break;
        case 2:
            beamInfo.modelName = skCrypt("sprites/laserbeam.vmt");
            break;
        case 3:
            beamInfo.modelName = skCrypt("sprites/physbeam.vmt");
            break;
        }
        
        beamInfo.modelIndex = -1;
        beamInfo.haloName = nullptr;
        beamInfo.haloIndex = -1;
        if (const auto modelprecache = interfaces->networkStringTableContainer->findTable(skCrypt("modelprecache")))
            modelprecache->addString(false, beamInfo.modelName);
        beamInfo.red = 255.0f * config->visuals.bulletTracers.color[0];
        beamInfo.green = 255.0f * config->visuals.bulletTracers.color[1];
        beamInfo.blue = 255.0f * config->visuals.bulletTracers.color[2];
        beamInfo.brightness = 255.0f * config->visuals.bulletTracers.color[3];

        beamInfo.life = 0.0f;
        beamInfo.amplitude = 0.0f;
        beamInfo.segments = -1;
        beamInfo.renderable = true;
        beamInfo.speed = 0.2f;
        beamInfo.startFrame = 0;
        beamInfo.frameRate = 0.0f;
        beamInfo.width = config->visuals.bulletWidth;
        beamInfo.endWidth = config->visuals.bulletWidth;
        beamInfo.flags = 0x40;
        beamInfo.fadeLength = -1.f;
        switch (config->visuals.bulletEffects)
        {
        case 0:
            beamInfo.flags |= FBEAM_ONLYNOISEONCE;
            break;
        case 1:
            beamInfo.amplitude = config->visuals.amplitude * 200.0f / beamInfo.start.distTo(beamInfo.end);
            break;
        case 2:
            beamInfo.flags |= FBEAM_SINENOISE;
            beamInfo.amplitude = config->visuals.amplitude * 0.02f;
            break;
        }
        
        if (const auto beam = memory->viewRenderBeams->createBeamPoints(beamInfo)) {
            beam->flags &= ~FBEAM_FOREVER;
            beam->die = memory->globalVars->currenttime +config->visuals.bulletImpactsTime;
        }
        shotRecord.front().gotImpact = true;
    }
    }
}
//Why 2 functions when you can do it in 1?, because it causes a crash doing it in 1 function

static std::deque<Vector> positions;

void Visuals::drawBulletImpacts() noexcept
{
    if (!config->visuals.bulletImpacts.enabled)
        return;

    if (!localPlayer)
        return;

    if (!interfaces->debugOverlay)
        return;

    const int r = static_cast<int>(config->visuals.bulletImpacts.color[0] * 255.f);
    const int g = static_cast<int>(config->visuals.bulletImpacts.color[1] * 255.f);
    const int b = static_cast<int>(config->visuals.bulletImpacts.color[2] * 255.f);
    const int a = static_cast<int>(config->visuals.bulletImpacts.color[3] * 255.f);

    for (int i = 0; i < static_cast<int>(positions.size()); i++)
    {
        if (!positions.at(i).notNull())
            continue;
        interfaces->debugOverlay->boxOverlay(positions.at(i), Vector{ -2.0f, -2.0f, -2.0f }, Vector{ 2.0f, 2.0f, 2.0f }, Vector{ 0.0f, 0.0f, 0.0f }, r, g, b, a, config->visuals.bulletImpactsTime);
    }
    positions.clear();
}

void Visuals::bulletImpact(GameEvent& event) noexcept
{
    if (!config->visuals.bulletImpacts.enabled)
        return;

    if (!localPlayer)
        return;

    if (event.getInt("userid") != localPlayer->getUserId())
        return;

    Vector endPos = Vector{ event.getFloat("x"), event.getFloat("y"), event.getFloat("z") };
    positions.push_front(endPos);
}

void Visuals::drawMolotovHull(ImDrawList* drawList) noexcept
{
    if (!config->visuals.molotovHull.enabled)
        return;

    const auto color = Helpers::calculateColor(config->visuals.molotovHull);

    GameData::Lock lock;

    static const auto flameCircumference = [] {
        std::array<Vector, 72> points;
        for (std::size_t i = 0; i < points.size(); ++i) {
            constexpr auto flameRadius = 60.0f; // https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/server/cstrike15/Effects/inferno.cpp#L889
            points[i] = Vector{ flameRadius * std::cos(Helpers::deg2rad(i * (360.0f / points.size()))),
                                flameRadius * std::sin(Helpers::deg2rad(i * (360.0f / points.size()))),
                                0.0f };
        }
        return points;
    }();

    for (const auto& molotov : GameData::infernos()) {
        for (const auto& pos : molotov.points) {
            std::array<ImVec2, flameCircumference.size()> screenPoints;
            std::size_t count = 0;

            for (const auto& point : flameCircumference) {
                if (Helpers::worldToScreen(pos + point, screenPoints[count]))
                    ++count;
            }

            if (count < 1)
                continue;

            std::swap(screenPoints[0], *std::min_element(screenPoints.begin(), screenPoints.begin() + count, [](const auto& a, const auto& b) { return a.y < b.y || (a.y == b.y && a.x < b.x); }));

            constexpr auto orientation = [](const ImVec2& a, const ImVec2& b, const ImVec2& c) {
                return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
            };

            std::sort(screenPoints.begin() + 1, screenPoints.begin() + count, [&](const auto& a, const auto& b) { return orientation(screenPoints[0], a, b) > 0.0f; });
            drawList->AddConvexPolyFilled(screenPoints.data(), count, color);
        }
    }
}

void Visuals::drawSmokeHull(ImDrawList* drawList) noexcept
{
    if (!config->visuals.smokeHull.enabled)
        return;

    const auto color = Helpers::calculateColor(config->visuals.smokeHull);

    GameData::Lock lock;

    static const auto smokeCircumference = [] {
        std::array<Vector, 72> points;
        for (std::size_t i = 0; i < points.size(); ++i) {
            constexpr auto smokeRadius = 150.0f; // https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/server/cstrike15/Effects/inferno.cpp#L889
            points[i] = Vector{ smokeRadius * std::cos(Helpers::deg2rad(i * (360.0f / points.size()))),
                                smokeRadius* std::sin(Helpers::deg2rad(i * (360.0f / points.size()))),
                                0.0f };
        }
        return points;
    }();

    for (const auto& smoke : GameData::smokes())
    {
        std::array<ImVec2, smokeCircumference.size()> screenPoints;
        std::size_t count = 0;

        for (const auto& point : smokeCircumference)
        {
            if (Helpers::worldToScreen(smoke.origin + point, screenPoints[count]))
                ++count;
        }

        if (count < 1)
            continue;

        std::swap(screenPoints[0], *std::min_element(screenPoints.begin(), screenPoints.begin() + count, [](const auto& a, const auto& b) { return a.y < b.y || (a.y == b.y && a.x < b.x); }));

        constexpr auto orientation = [](const ImVec2& a, const ImVec2& b, const ImVec2& c)
        {
            return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
        };

        std::sort(screenPoints.begin() + 1, screenPoints.begin() + count, [&](const auto& a, const auto& b) { return orientation(screenPoints[0], a, b) > 0.0f; });
        drawList->AddConvexPolyFilled(screenPoints.data(), count, color);
        if (config->visuals.smokeHull.outline)
        drawList->AddPolyline(screenPoints.data(), count, color | IM_COL32_A_MASK, true, 2.f);
    }
}

void Visuals::updateEventListeners(bool forceRemove) noexcept
{
    class ImpactEventListener : public GameEventListener {
    public:
        void fireGameEvent(GameEvent* event) { 
            bulletTracer(event); 
            bulletImpact(*event);
            dmgMarker(event);
        }
    };

    static ImpactEventListener listener;
    static bool listenerRegistered = false;

    if ((config->visuals.bulletImpacts.enabled || config->visuals.bulletTracers.enabled) && !listenerRegistered) {
        interfaces->gameEventManager->addListener(&listener, skCrypt("bullet_impact"));
        listenerRegistered = true;
    } else if (((!config->visuals.bulletImpacts.enabled && !config->visuals.bulletTracers.enabled) || forceRemove) && listenerRegistered) {
        interfaces->gameEventManager->removeListener(&listener);
        listenerRegistered = false;
    }
    class FootstepEventListener : public GameEventListener {
    public:
        void fireGameEvent(GameEvent* event) {
            Visuals::FootstepESP(event);
        }
    };

    static ImpactEventListener impactListener;
    static bool impactListenerRegistered = false;

    if ((config->visuals.bulletImpacts.enabled || config->visuals.bulletTracers.enabled) && !impactListenerRegistered) {
        interfaces->gameEventManager->addListener(&impactListener, skCrypt("bullet_impact"));
        impactListenerRegistered = true;
    }
    else if (((!config->visuals.bulletImpacts.enabled && !config->visuals.bulletTracers.enabled) || forceRemove) && impactListenerRegistered) {
        interfaces->gameEventManager->removeListener(&impactListener);
        impactListenerRegistered = false;
    }

    static FootstepEventListener footstepListener;
    static bool footstepListenerRegistered = false;

    if ((config->visuals.footstepBeamsA.enabled || config->visuals.footstepBeamsE.enabled) && !footstepListenerRegistered) {
        interfaces->gameEventManager->addListener(&footstepListener, skCrypt("player_footstep"));
        footstepListenerRegistered = true;
    }
    else if (((config->visuals.footstepBeamsA.enabled || config->visuals.footstepBeamsE.enabled) || forceRemove) && footstepListenerRegistered) {
        interfaces->gameEventManager->removeListener(&footstepListener);
        footstepListenerRegistered = false;
    }
}

void Visuals::updateInput() noexcept
{
    config->visuals.freeCamKey.handleToggle();
    config->visuals.thirdperson.key.handleToggle();
    config->visuals.zoomKey.handleToggle();
}

void Visuals::reset(int resetType) noexcept
{
    shotRecord.clear();
    Visuals::transparentWorld(resetType);
    Visuals::colorConsole(resetType);
    if (resetType == 1)
    {
        static auto bright = interfaces->cvar->findVar(skCrypt("mat_fullbright"));
        static auto sky = interfaces->cvar->findVar(skCrypt("r_3dsky"));
        static auto shadows = interfaces->cvar->findVar(skCrypt("cl_csm_enabled"));
        static auto cl_csm_rot_override = interfaces->cvar->findVar(skCrypt("cl_csm_rot_override"));
        bright->setValue(0);
        sky->setValue(1);
        shadows->setValue(1);
        cl_csm_rot_override->setValue(0);

        const bool thirdPerson = config->visuals.thirdperson.enable && config->visuals.thirdperson.key.isActive() && localPlayer && localPlayer->isAlive();
        if (thirdPerson)
            memory->input->isCameraInThirdPerson = false;
    }
}
