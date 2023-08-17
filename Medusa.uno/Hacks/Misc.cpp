#include <mutex>
#include <numeric>
#include <sstream>
#include <string>

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"
#include "../postprocessing.h"
#include "../xor.h"
#include "DLight.h"
#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"
#include "../GUI.h"
#include "../Helpers.h"
#include "../GameData.h"
#include "Tickbase.h"
#include "AntiAim.h"
#include "../render.hpp"
#include "../Hooks.h"
#include "EnginePrediction.h"
#include "Misc.h"
#include "../SDK/ViewRenderBeams.h"
#include "../IEffects.h"
#include "../SDK/Client.h"
#include "../SDK/ClientMode.h"
#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GameEvent.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Input.h"
#include "../SDK/ItemSchema.h"
#include "../Localize.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/Material.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/Panorama.h"
#include "../SDK/Prediction.h"
#include "../SDK/Surface.h"
#include "../SDK/UserCmd.h"
#include "../SDK/ViewSetup.h"
#include "../SDK/WeaponData.h"
#include "../SDK/WeaponSystem.h"
#include "../SDK/Steam.h"
#include "../imguiCustom.h"
#include <TlHelp32.h>
#include "../includes.hpp"
#include "../postprocessing.h"
#include "Visuals.h"
UserCmd* cmd1;
int goofy;
void Misc::getCmd(UserCmd* cmd) noexcept
{
    cmd1 = cmd;
    if (localPlayer && localPlayer->isAlive())
    goofy = get_moving_flag(cmd);
}

bool Misc::isInChat() noexcept
{
    if (!localPlayer)
        return false;

    const auto hudChat = memory->findHudElement(memory->hud, skCrypt("CCSGO_HudChat"));
    if (!hudChat)
        return false;

    const bool isInChat = *(bool*)((uintptr_t)hudChat + 0xD);

    return isInChat;
}

std::string currentForwardKey = "";
std::string currentBackKey = "";
std::string currentRightKey = "";
std::string currentLeftKey = "";
int currentButtons = 0;

void Misc::gatherDataOnTick(UserCmd* cmd) noexcept
{
    currentButtons = cmd->buttons;
}

void Misc::handleKeyEvent(int keynum, const char* currentBinding) noexcept
{
    if (!currentBinding || keynum <= 0 || keynum >= ARRAYSIZE(ButtonCodes))
        return;

    const auto buttonName = ButtonCodes[keynum];

    switch (fnv::hash(currentBinding))
    {
    case fnv::hash("+forward"):
        currentForwardKey = std::string(buttonName);
        break;
    case fnv::hash("+back"):
        currentBackKey = std::string(buttonName);
        break;
    case fnv::hash("+moveright"):
        currentRightKey = std::string(buttonName);
        break;
    case fnv::hash("+moveleft"):
        currentLeftKey = std::string(buttonName);
        break;
    default:
        break;
    }
}

void Misc::drawKeyDisplay(ImDrawList* drawList) noexcept
{
    if (!config->misc.keyBoardDisplay.enabled)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    int screenSizeX, screenSizeY;
    interfaces->engine->getScreenSize(screenSizeX, screenSizeY);
    const float Ypos = static_cast<float>(screenSizeY) * config->misc.keyBoardDisplay.position;

    std::string keys[3][2];
    if (config->misc.keyBoardDisplay.showKeyTiles)
    {
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                keys[i][j] = "_";
            }
        }
    }

    if (currentButtons & UserCmd::IN_DUCK)
        keys[0][0] = skCrypt("C");
    if (currentButtons & UserCmd::IN_FORWARD)
        keys[1][0] = currentForwardKey;
    if (currentButtons & UserCmd::IN_JUMP)
        keys[2][0] = skCrypt("J");
    if (currentButtons & UserCmd::IN_MOVELEFT)
        keys[0][1] = currentLeftKey;
    if (currentButtons & UserCmd::IN_BACK)
        keys[1][1] = currentBackKey;
    if (currentButtons & UserCmd::IN_MOVERIGHT)
        keys[2][1] = currentRightKey;

    const float positions[3] =
    {
       -35.0f, 0.0f, 35.0f
    };

    ImGui::PushFont(gui->getTahoma28Font());
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            if (keys[i][j] == "")
                continue;

            const auto size = ImGui::CalcTextSize(keys[i][j].c_str());
            drawList->AddText(ImVec2{ (static_cast<float>(screenSizeX) / 2 - size.x / 2 + positions[i]) + 1, (Ypos + (j * 25)) + 1 }, Helpers::calculateColor(Color4{ 0.0f, 0.0f, 0.0f, config->misc.keyBoardDisplay.color.color[3] }), keys[i][j].c_str());
            drawList->AddText(ImVec2{ static_cast<float>(screenSizeX) / 2 - size.x / 2 + positions[i], Ypos + (j * 25) }, Helpers::calculateColor(config->misc.keyBoardDisplay.color), keys[i][j].c_str());
        }
    }

    ImGui::PopFont();
}

void Misc::drawVelocity(ImDrawList* drawList) noexcept
{
    if (!config->misc.velocity.enabled)
        return;

    if (!localPlayer)
        return;

    const auto entity = localPlayer->isAlive() ? localPlayer.get() : localPlayer->getObserverTarget();
    if (!entity)
        return;

    int screenSizeX, screenSizeY;
    interfaces->engine->getScreenSize(screenSizeX, screenSizeY);
    const float Ypos = static_cast<float>(screenSizeY) * config->misc.velocity.position;

    static float colorTime = 0.f;
    static float takeOffTime = 0.f;

    static auto lastVelocity = 0;
    const auto velocity = static_cast<int>(round(entity->velocity().length2D()));

    static auto takeOffVelocity = 0;
    static bool lastOnGround = true;
    const bool onGround = entity->flags() & 1;
    if (lastOnGround && !onGround)
    {
        takeOffVelocity = velocity;
        takeOffTime = memory->globalVars->realtime + 2.f;
    }

    const bool shouldDrawTakeOff = !onGround || (takeOffTime > memory->globalVars->realtime);
    const std::string finalText = std::to_string(velocity);

    const Color4 trueColor = config->misc.velocity.color.enabled ? Color4{ config->misc.velocity.color.color[0], config->misc.velocity.color.color[1], config->misc.velocity.color.color[2], config->misc.velocity.alpha, config->misc.velocity.color.rainbowSpeed, config->misc.velocity.color.rainbow }
        : (velocity == lastVelocity ? Color4{ 1.0f, 0.78f, 0.34f, config->misc.velocity.alpha } : velocity < lastVelocity ? Color4{ 1.0f, 0.46f, 0.46f, config->misc.velocity.alpha } : Color4{ 0.11f, 1.0f, 0.42f, config->misc.velocity.alpha });

    ImGui::PushFont(gui->getTahoma28Font());

    const auto size = ImGui::CalcTextSize(finalText.c_str());
    drawList->AddText(ImVec2{ (static_cast<float>(screenSizeX) / 2 - size.x / 2) + 1, Ypos + 1.0f }, Helpers::calculateColor(Color4{ 0.0f, 0.0f, 0.0f, config->misc.velocity.alpha }), finalText.c_str());
    drawList->AddText(ImVec2{ static_cast<float>(screenSizeX) / 2 - size.x / 2, Ypos }, Helpers::calculateColor(trueColor), finalText.c_str());

    if (shouldDrawTakeOff)
    {
        const std::string bottomText = "(" + std::to_string(takeOffVelocity) + ")";
        const Color4 bottomTrueColor = config->misc.velocity.color.enabled ? Color4{ config->misc.velocity.color.color[0], config->misc.velocity.color.color[1], config->misc.velocity.color.color[2], config->misc.velocity.alpha, config->misc.velocity.color.rainbowSpeed, config->misc.velocity.color.rainbow }
            : (takeOffVelocity <= 250.0f ? Color4{ 0.75f, 0.75f, 0.75f, config->misc.velocity.alpha } : Color4{ 0.11f, 1.0f, 0.42f, config->misc.velocity.alpha });
        const auto bottomSize = ImGui::CalcTextSize(bottomText.c_str());
        drawList->AddText(ImVec2{ (static_cast<float>(screenSizeX) / 2 - bottomSize.x / 2) + 1, Ypos + 20.0f + 1 }, Helpers::calculateColor(Color4{ 0.0f, 0.0f, 0.0f, config->misc.velocity.alpha }), bottomText.c_str());
        drawList->AddText(ImVec2{ static_cast<float>(screenSizeX) / 2 - bottomSize.x / 2, Ypos + 20.0f }, Helpers::calculateColor(bottomTrueColor), bottomText.c_str());
    }

    ImGui::PopFont();

    if (colorTime <= memory->globalVars->realtime)
    {
        colorTime = memory->globalVars->realtime + 0.1f;
        lastVelocity = velocity;
    }
    lastOnGround = onGround;
}

class JumpStatsCalculations
{
private:
    static const auto white = '\x01';
    static const auto violet = '\x03';
    static const auto green = '\x04';
    static const auto red = '\x07';
    static const auto golden = '\x09';
public:
    void resetStats() noexcept
    {
        units = 0.0f;
        strafes = 0;
        pre = 0.0f;
        maxVelocity = 0.0f;
        maxHeight = 0.0f;
        jumps = 0;
        bhops = 0;
        sync = 0.0f;
        startPosition = Vector{ };
        landingPosition = Vector{ };
    }

    bool show() noexcept
    {
        if (!onGround || jumping || jumpbugged)
            return false;

        if (!shouldShow)
            return false;

        units = (startPosition - landingPosition).length2D() + (isLadderJump ? 0.0f : 32.0f);

        const float z = fabsf(startPosition.z - landingPosition.z) - (isJumpbug ? 9.0f : 0.0f);
        const bool fail = z >= (isLadderJump ? 32.0f : (jumps > 0 ? (jumps > 1 ? 46.0f : 2.0f) : 46.0f));
        const bool simplifyNames = config->misc.jumpStats.simplifyNaming;

        std::string jump = "null";

        //Values taken from
        //https://github.com/KZGlobalTeam/gokz/blob/33a3a49bc7a0e336e71c7f59c14d26de4db62957/cfg/sourcemod/gokz/gokz-jumpstats-tiers.cfg
        auto color = white;
        switch (jumps)
        {
        case 1:
            if (!isJumpbug)
            {
                jump = simplifyNames ? skCrypt("LJ") : skCrypt("Long jump");
                if (units < 230.0f)
                    color = white;
                else if (units >= 230.0f && units < 235.0f)
                    color = violet;
                else if (units >= 235.0f && units < 238.0f)
                    color = green;
                else if (units >= 238.0f && units < 240.0f)
                    color = red;
                else if (units >= 240.0f)
                    color = golden;
            }
            else
            {
                jump = simplifyNames ? "JB" : "Jump bug";
                if (units < 250.0f)
                    color = white;
                else if (units >= 250.0f && units < 260.0f)
                    color = violet;
                else if (units >= 260.0f && units < 265.0f)
                    color = green;
                else if (units >= 265.0f && units < 270.0f)
                    color = red;
                else if (units >= 270.0f)
                    color = golden;
            }
            break;
        case 2:
            jump = simplifyNames ? "BH" : "Bunnyhop";
            if (units < 230.0f)
                color = white;
            else if (units >= 230.0f && units < 233.0f)
                color = violet;
            else if (units >= 233.0f && units < 235.0f)
                color = green;
            else if (units >= 235.0f && units < 240.0f)
                color = red;
            else if (units >= 240.0f)
                color = golden;
            break;
        default:
            if (jumps >= 3)
            {
                jump = simplifyNames ? "MBH" : "Multi Bunnyhop";
                if (units < 230.0f)
                    color = white;
                else if (units >= 230.0f && units < 233.0f)
                    color = violet;
                else if (units >= 233.0f && units < 235.0f)
                    color = green;
                else if (units >= 235.0f && units < 240.0f)
                    color = red;
                else if (units >= 240.0f)
                    color = golden;
            }
            break;
        }
        if (isLadderJump)
        {
            jump = simplifyNames ? "LAJ" : "Ladder jump";
            if (units < 80.0f)
                color = white;
            else if (units >= 80.0f && units < 90.0f)
                color = violet;
            else if (units >= 90.0f && units < 105.0f)
                color = green;
            else if (units >= 105.0f && units < 109.0f)
                color = red;
            else if (units >= 109.0f)
                color = golden;
        }

        if (!config->misc.jumpStats.showColorOnFail && fail)
            color = white;

        if (fail)
            jump += simplifyNames ? "-F" : " Failed";

        const bool show = (isLadderJump ? units >= 50.0f : units >= 186.0f) && (!(!config->misc.jumpStats.showFails && fail) || (config->misc.jumpStats.showFails));
        if (show && config->misc.jumpStats.enabled)
        {
            //Certain characters are censured on printf
            if (jumps > 2)
                memory->clientMode->getHudChat()->printf(0,
                    skCrypt(" [Medusa.uno] %c%s: %.2f units \x01[\x05%d\x01 Strafes | \x05%.0f\x01 Pre | \x05%.0f\x01 Max | \x05%.1f\x01 Height | \x05%d\x01 Bhops | \x05%.0f\x01 Sync]"),
                    color, jump.c_str(),
                    jumpStatsCalculations.units, jumpStatsCalculations.strafes, jumpStatsCalculations.pre, jumpStatsCalculations.maxVelocity, jumpStatsCalculations.maxHeight, jumpStatsCalculations.jumps, jumpStatsCalculations.sync);
            else
                memory->clientMode->getHudChat()->printf(0,
                    skCrypt(" [Medusa.uno] %c%s: %.2f units \x01[\x05%d\x01 Strafes | \x05%.0f\x01 Pre | \x05%.0f\x01 Max | \x05%.1f\x01 Height | \x05%.0f\x01 Sync]"),
                    color, jump.c_str(),
                    jumpStatsCalculations.units, jumpStatsCalculations.strafes, jumpStatsCalculations.pre, jumpStatsCalculations.maxVelocity, jumpStatsCalculations.maxHeight, jumpStatsCalculations.sync);
        }

        shouldShow = false;
        return true;
    }

    void run(UserCmd* cmd) noexcept
    {
        if (localPlayer->moveType() == MoveType::NOCLIP)
        {
            resetStats();
            shouldShow = false;
            return;
        }

        velocity = localPlayer->velocity().length2D();
        origin = localPlayer->getAbsOrigin();
        onGround = localPlayer->flags() & 1;
        onLadder = localPlayer->moveType() == MoveType::LADDER;
        jumping = cmd->buttons & UserCmd::IN_JUMP && !(lastButtons & UserCmd::IN_JUMP) && onGround;
        jumpbugged = !jumpped && hasJumped;

        //We jumped so we should show this jump
        if (jumping || jumpbugged)
            shouldShow = true;

        if (onLadder)
        {
            startPosition = origin;
            pre = velocity;
            startedOnLadder = true;
        }

        if (onGround)
        {
            if (!onLadder)
            {
                if (jumping)
                {
                    //We save pre velocity and the starting position
                    startPosition = origin;
                    pre = velocity;
                    jumps++;
                    startedOnLadder = false;
                    isLadderJump = false;
                }
                else
                {
                    landingPosition = origin;
                    //We reset our jumps after logging them, and incase we do log our jumps and need to reset anyways we do this
                    if (!shouldShow)
                        jumps = 0;

                    if (startedOnLadder)
                    {
                        isLadderJump = true;
                        shouldShow = true;
                    }
                    startedOnLadder = false;
                }
            }

            //Calculate sync
            if (ticksInAir > 0 && !jumping)
                sync = (static_cast<float>(ticksSynced) / static_cast<float>(ticksInAir)) * 100.0f;

            //Reset both counters used for calculating sync
            ticksInAir = 0;
            ticksSynced = 0;
        }
        else if (!onGround && !onLadder)
        {
            if (jumpbugged)
            {
                if (oldOrigin.notNull())
                    startPosition = oldOrigin;
                pre = oldVelocity;
                jumps = 1;
                isJumpbug = true;
                jumpbugged = false;
            }
            //Check for strafes
            if (cmd->mousedx != 0 && cmd->sidemove != 0.0f)
            {
                if (cmd->mousedx > 0 && lastMousedx <= 0.0f && cmd->sidemove > 0.0f)
                {
                    strafes++;
                }
                if (cmd->mousedx < 0 && lastMousedx >= 0.0f && cmd->sidemove < 0.0f)
                {
                    strafes++;
                }
            }

            //If we gain velocity, we gain more sync
            if (oldVelocity != 0.0f)
            {
                float deltaSpeed = velocity - oldVelocity;
                bool gained = deltaSpeed > 0.000001f;
                bool lost = deltaSpeed < -0.000001f;
                if (gained)
                {
                    ticksSynced++;
                }
            }

            //Get max height and max velocity
            maxHeight = max(fabsf(startPosition.z - origin.z), maxHeight);
            maxVelocity = max(velocity, maxVelocity);

            ticksInAir++; //We are in air
            sync = 0; //We dont calculate sync yet
        }

        lastMousedx = cmd->mousedx;
        lastOnGround = onGround;
        lastButtons = cmd->buttons;
        oldVelocity = velocity;
        oldOrigin = origin;
        jumpped = jumping;
        hasJumped = false;

        if (show())
            resetStats();

        if (onGround && !onLadder)
        {
            isJumpbug = false;
        }
        isLadderJump = false;
    }

    //Last values
    short lastMousedx{ 0 };
    bool lastOnGround{ false };
    int lastButtons{ 0 };
    float oldVelocity{ 0.0f };
    bool jumpped{ false };
    Vector oldOrigin{ };
    Vector startPosition{ };

    //Current values
    float velocity{ 0.0f };
    bool onLadder{ false };
    bool onGround{ false };
    bool jumping{ false };
    bool jumpbugged{ false };
    bool isJumpbug{ false };
    bool hasJumped{ false };
    bool startedOnLadder{ false };
    bool isLadderJump{ false };
    bool shouldShow{ false };
    int jumps{ 0 };
    Vector origin{ };
    Vector landingPosition{ };
    int ticksInAir{ 0 };
    int ticksSynced{ 0 };

    //Final values
    float units{ 0.0f };
    int strafes{ 0 };
    float pre{ 0.0f };
    float maxVelocity{ 0.0f };
    float maxHeight{ 0.0f };
    int bhops{ 0 };
    float sync{ 0.0f };
} jumpStatsCalculations;

void Misc::gotJump() noexcept
{
    jumpStatsCalculations.hasJumped = true;
}

void Misc::jumpStats(UserCmd* cmd) noexcept
{
    if (!localPlayer)
        return;

    if (!localPlayer->isAlive())
    {
        jumpStatsCalculations = { };
        return;
    }

    static bool once = true;
    if ((!*memory->gameRules || (*memory->gameRules)->freezePeriod()) || localPlayer->flags() & (1 << 6))
    {
        if (once)
        {
            jumpStatsCalculations = { };
            once = false;
        }
        return;
    }

    jumpStatsCalculations.run(cmd);

    once = true;
}

void Misc::miniJump(UserCmd* cmd) noexcept
{
    static int lockedTicks = 0;
    if (!config->misc.miniJump || (!config->misc.miniJumpKey.isActive() && config->misc.miniJumpCrouchLock <= 0))
    {
        lockedTicks = 0;
        return;
    }

    if (!localPlayer || !localPlayer->isAlive())
    {
        lockedTicks = 0;
        return;
    }
    
    if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER)
    {
        lockedTicks = 0;
        return;
    }

    if (lockedTicks >= 1)
    {
        cmd->buttons |= UserCmd::IN_DUCK;
        lockedTicks--;
    }
    
    if ((EnginePrediction::getFlags() & 1) && !(localPlayer->flags() & 1))
    {
        cmd->buttons |= UserCmd::IN_JUMP;
        cmd->buttons |= UserCmd::IN_DUCK;
        lockedTicks = config->misc.miniJumpCrouchLock;
    }
}

std::array<std::pair<bool, Vector>, 4> pointsss;

std::pair<int, Vector> get_align_side()
{
    float originx = localPlayer->origin().x;
    float originy = localPlayer->origin().y;
    float one_minus_x_floor = 1.f - (originx - floor(originx));
    float one_minus_y_floor = 1.f - (originy - floor(originy));
    float x_floor = originx - floor(originx);
    float y_floor = originy - floor(originy);

    Vector mins = localPlayer->getCollideable()->obbMins();
    Vector maxs = localPlayer->getCollideable()->obbMaxs();
    const Vector org_origin = localPlayer->origin();

    Trace info[4];

    for (int i = 0; i < 4; i++)
    {
        Vector startpos = org_origin;
        Vector endpos = org_origin;
        switch (i)
        {
        case 0:
            startpos.y += mins.y; // adding mins.y so were on the edge of bbox
            endpos.y = floor(startpos.y); // flooring so we trace to the nearest whole hammer unit
            pointsss.at(i).second = org_origin + Vector{ mins.x, mins.y, 0.f };
            break;
        case 1:
            startpos.x += maxs.x;
            endpos.x = floor(startpos.x) + 1.f;
            pointsss.at(i).second = org_origin + Vector{ maxs.x, mins.y, 0.f };
            break;
        case 2:
            startpos.y += maxs.y;
            endpos.y = floor(startpos.y) + 1.f; //flooring y and adding 1 so we trace to opposite side
            pointsss.at(i).second = org_origin + Vector{ maxs.x, maxs.y, 0.f };
            break;
        case 3: //flooring x
            startpos.x += mins.x;
            endpos.x = floor(startpos.x); // negative x from origin
            pointsss.at(i).second = org_origin + Vector{ mins.x, maxs.y, 0.f };
            break;
        }

        Trace tr;
        TraceFilter fil{ localPlayer.get() };
        Ray ray{ Ray(org_origin, endpos) };
        interfaces->engineTrace->traceRay(ray, MASK_PLAYERSOLID, fil, tr);
        //pointsss.at(i) = { tr.startpos, tr.endpos };
        info[i] = tr;
    }

    float min_frac = 1.f;
    int bestind = -1;
    for (int i = 0; i < 4; i++)
    {
        auto& tr = info[i];
        if ((tr.fraction < 1.f || tr.allSolid || tr.startSolid) && (tr.entity ? !tr.entity->isPlayer() : true))
        {
            min_frac = tr.fraction;
            bestind = i;
            switch (i)
            {
            case 0:
                if (y_floor < 0.03125)
                    pointsss.at(i).first = true;
                else
                    pointsss.at(i).first = false;
                break;

            case 1:
                if (one_minus_x_floor < 0.03125)
                    pointsss.at(i).first = true;
                else
                    pointsss.at(i).first = false;
                break;
            case 2:
                if (one_minus_y_floor < 0.03125)
                    pointsss.at(i).first = true;
                else
                    pointsss.at(i).first = false;
                break;
            case 3:
                if (x_floor < 0.03125)
                    pointsss.at(i).first = true;
                else
                    pointsss.at(i).first = false;
                break;
            }
        }
        else
        {
            pointsss.at(i).first = false;
        }
    }

    if (bestind != -1)
    {
        return { bestind, info[bestind].endpos };
    }


    return { bestind, {0,0,0} };
}

void autoalign_adjustfsmove(UserCmd* cmd, Vector align_angle)
{
    Vector align_vector_forward = { 0.f,0.f,0.f }, align_vector_right = { 0.f,0.f,0.f }, align_vector_up = { 0.f,0.f,0.f };
    Helpers::AngleVectors(align_angle, &align_vector_forward, &align_vector_right, &align_vector_up);
    float nor_length2d_alignvecforward = 1.f / (align_vector_forward.length2D() + FLT_EPSILON); // normalized length2d of forward align vector
    float nor_length2d_alignvecright = 1.f / (align_vector_right.length2D() + FLT_EPSILON); // normalized length2d of forward align vector
    float up_vecz_align_angle = align_vector_up.z;

    Vector va_vector_forward = { 0.f,0.f,0.f }, va_vector_right = { 0.f,0.f,0.f }, va_vector_up = { 0.f,0.f,0.f };
    Helpers::AngleVectors(cmd->viewangles, &va_vector_forward, &va_vector_right, &va_vector_up);
    //do tha shit with forward
    float nor_length2d_vavecforward = 1.f / (va_vector_forward.length2D() + FLT_EPSILON);
    float v35 = nor_length2d_vavecforward * va_vector_forward.x;
    float v32 = nor_length2d_vavecforward * va_vector_forward.y;
    //do tha shit with right
    float nor_length2d_vavecright = 1.f / (va_vector_right.length2D() + FLT_EPSILON);
    float v34 = nor_length2d_vavecright * va_vector_right.x;
    float v39 = nor_length2d_vavecright * va_vector_right.y;
    //do tha shit with up
    float nor_length2d_vavecup = 1.f / (va_vector_up.z + FLT_EPSILON); // this is v27

    //save fmove, smove
    float saved_fmove = cmd->forwardmove;
    float saved_smove = cmd->sidemove;

    //calculate the desired fmove, smove, umove

    float modified_smove_by_y = saved_smove * nor_length2d_alignvecright * align_vector_right.y; // this is v2
    float modified_smove_by_x = saved_smove * nor_length2d_alignvecright * align_vector_right.x; // this is v45.m128i_i64[1]

    float modified_fmove_by_y = saved_fmove * nor_length2d_alignvecforward * align_vector_forward.y; // this is v29.m128i_i64[1]
    float modified_fmove_by_x = saved_fmove * nor_length2d_alignvecforward * align_vector_forward.x; // this is v30.m128i_i64[1]

    float modified_umove_by_z = cmd->upmove * (1.f / (up_vecz_align_angle + FLT_EPSILON)) * up_vecz_align_angle;

    //further calculation

    float calculated_forwardmove = v35 * modified_fmove_by_x + v35 * modified_smove_by_x + v32 * modified_fmove_by_y + v32 * modified_fmove_by_y;
    float calculated_sidemove = v39 * modified_smove_by_y + v34 * modified_smove_by_x + v34 * modified_fmove_by_x + v39 * modified_fmove_by_y;

    //TODO: DETERMINE WHEN TO ACTUALLY SET THIS

    if ((align_angle.y == 90.f || align_angle.y == -90.f) && fabsf(saved_smove) > 0.01f)
    {
        cmd->forwardmove = 450.f + calculated_forwardmove;
    }
    else
    {
        cmd->forwardmove = calculated_forwardmove;
    }

    //cmd->forwardmove = calculated_forwardmove

    if ((align_angle.y == 0.f || align_angle.y == 180.f) && fabsf(saved_fmove) > 0.01f)
    {
        cmd->sidemove = 450.f + calculated_sidemove;
    }
    else
    {
        cmd->sidemove = calculated_sidemove;
    }

    cmd->forwardmove = std::clamp(cmd->forwardmove, -450.f, 450.f);
    cmd->sidemove = std::clamp(cmd->sidemove, -450.f, 450.f);
}

static bool detectedPixelSurf = false;
void Misc::autoPixelSurf(UserCmd* cmd) noexcept
{
    if (!config->misc.autoPixelSurf || !config->misc.autoPixelSurfKey.isActive())
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (EnginePrediction::getFlags() & 1)
        return;
    
    if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER)
        return;

    //Restore before prediction
    memory->restoreEntityToPredictedFrame(0, interfaces->prediction->split->commandsPredicted - 1);

    for (int i = 0; i < config->misc.autoPixelSurfPredAmnt; i++)
    {
        auto backupButtons = cmd->buttons;
        cmd->buttons |= UserCmd::IN_DUCK;

        EnginePrediction::run(cmd);

        cmd->buttons = backupButtons;

        detectedPixelSurf = (localPlayer->velocity().z == -6.25f || localPlayer->velocity().z == -6.f || localPlayer->velocity().z == -3.125) && !(localPlayer->flags() & 1) && localPlayer->moveType() != MoveType::LADDER;

        if (detectedPixelSurf)
            break;
    }

    if (detectedPixelSurf)
    {
        cmd->buttons |= UserCmd::IN_DUCK;
    }

    memory->restoreEntityToPredictedFrame(0, interfaces->prediction->split->commandsPredicted - 1);
    EnginePrediction::run(cmd);
}

#include "../SDK/PlayerResource.h"

void Misc::chatRevealer(GameEvent& event, GameEvent* events) noexcept
{
    if (!config->misc.chatReveavler)
        return;

    if (!localPlayer)
        return;

    const auto entity = interfaces->entityList->getEntity(interfaces->engine->getPlayerFromUserID(events->getInt("userid")));
    if (!entity)
        return;

    std::string output = "";

    auto team = entity->getTeamNumber();
    bool isAlive = entity->isAlive();
    bool dormant = entity->isDormant();
    if (dormant) {
        if (const auto pr = *memory->playerResource)
            isAlive = pr->getIPlayerResource()->isAlive(entity->index());
    }

    const char* text = event.getString(skCrypt("text"));
    const char* lastLocation = entity->lastPlaceName();
    const std::string name = entity->getPlayerName();

    if (team == localPlayer->getTeamNumber())
        return;

    if (!isAlive && team != Team::Spectators && team != Team::None)
        output += "*DEAD* ";

    switch (team)
    {
    case Team::None:
        output += "";
        break;
    case Team::Spectators:
        output += skCrypt("*SPEC* ");
        break;
    case Team::CT:
        output += skCrypt("(Counter-Terrorist) ");
        break;
    case Team::TT:
        output += skCrypt("(Terrorist) ");
        break;
    }

    output = output + name + (dormant || !isAlive ? "" : (" @ " + std::string(lastLocation))) + " : " + text;
    memory->clientMode->getHudChat()->printf(0, output.c_str());
}

void Misc::PixelSurfAlign(UserCmd* cmd)
{
    if (!localPlayer || detectedPixelSurf || !localPlayer->isAlive() || localPlayer->moveType() == MoveType::NOCLIP || (localPlayer->flags() & FL_ONGROUND) || localPlayer->moveType() == MoveType::LADDER || !config->misc.autoPixelSurf || !config->misc.autoPixelSurfKey.isActive())
        return;

    float originx = localPlayer->origin().x;
    float originy = localPlayer->origin().y;
    float one_minus_x_floor = 1.f - (originx - floor(originx));
    float one_minus_y_floor = 1.f - (originy - floor(originy));
    float x_floor = originx - floor(originx);
    float y_floor = originy - floor(originy);
    float forward_move = cmd->forwardmove;
    float side_move = cmd->sidemove;

    int alignside;

    auto al_res = get_align_side();

    alignside = al_res.first;

    //if (alignside != -1 && ((one_minus_x_floor >= 0.00050000002 && one_minus_x_floor <= 0.03125) || (one_minus_y_floor >= 0.00050000002 && one_minus_y_floor <= 0.03125) || (x_floor >= 0.00050000002 && x_floor <= 0.03125) || (y_floor >= 0.00050000002 && y_floor <= 0.03125)) && !(csgo->flags & FL_ONGROUND))
    if ((alignside == 0 && y_floor >= 0.03125) || (alignside == 1 && one_minus_x_floor >= 0.03125) || (alignside == 2 && one_minus_y_floor >= 0.03125) || (alignside == 3 && x_floor >= 0.03125))
    {
        Vector orig = cmd->viewangles;//original angle

        float yang = 0;
        //TrolOLOsoososososososooSOOSOSOSOSOSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
        switch (alignside)
        {
        case 0:
            yang = -90.f;
            break;
        case 1:
            yang = 0.f;
            break;
        case 2:
            yang = 90.f;
            break;
        case 3:
            yang = 180.f;
            break;
        }

        Vector ang_diff = { cmd->viewangles.x, yang, 0.f };

        //This right here if just movement fix that one of our funny devs spent 2 weeks reverse engineering (:D) but you can make it work easily if you know basic trigonometry 
        autoalign_adjustfsmove(cmd, ang_diff);
    }
}

static std::vector<std::pair<Vector, Vector>> detectionpositions;
bool actualebdetection(Vector& old_velocity, Vector& predicted_velocity, Vector& afterpredicted_velocity)
{
    static auto Sv_gravity = interfaces->cvar->findVar(skCrypt("sv_gravity"));
    auto sv_gravity = Sv_gravity->getFloat();

    if (old_velocity.z < -6.0f && predicted_velocity.z > old_velocity.z && predicted_velocity.z < -6.0f && old_velocity.length2D() <= predicted_velocity.length2D())
    {
        const float gravity_vel_const = roundf(-sv_gravity * memory->globalVars->intervalPerTick + predicted_velocity.z);

        if (gravity_vel_const == roundf(afterpredicted_velocity.z))
            return true;
    }

    if (predicted_velocity.length2D() <= afterpredicted_velocity.length2D())
    {
        float ebzvel = sv_gravity * 0.5f * memory->globalVars->intervalPerTick;

        if (-ebzvel > predicted_velocity.z && round(afterpredicted_velocity.z) == round(-ebzvel))
        {
            return true;
        }
    }


    return false;
}

void Misc::edgeBug(UserCmd* cmd, float lasttickyaw)
{
    if (!localPlayer || !localPlayer->isAlive())
    {
        detectdata.detecttick = 0;
        detectdata.edgebugtick = 0;
        return;
    }

    if (!config->misc.edgeBug || !config->misc.edgeBugKey.isActive())
    {
        detectdata.detecttick = 0;
        detectdata.edgebugtick = 0;
        return;
    }

    if (!interfaces->engine->isConnected() || !interfaces->engine->isInGame())
    {
        detectdata.detecttick = 0;
        detectdata.edgebugtick = 0;
        return;
    }
    if (localPlayer->flags() & 1)
    {
        detectdata.detecttick = 0;
        detectdata.edgebugtick = 0;
        return;
    }

    if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER)
    {
        detectdata.detecttick = 0;
        detectdata.edgebugtick = 0;
        return;
    }

    if (memory->globalVars->tickCount >= detectdata.detecttick && memory->globalVars->tickCount <= detectdata.edgebugtick)
    {

        if (detectdata.crouched)
            cmd->buttons |= UserCmd::IN_DUCK;
        else
            cmd->buttons &= ~UserCmd::IN_DUCK;


        if (detectdata.strafing)
        {
            cmd->forwardmove = detectdata.forwardmove;
            cmd->sidemove = detectdata.sidemove;
            cmd->viewangles.y = Helpers::normalizeYaw(detectdata.startingyaw + (detectdata.yawdelta * (memory->globalVars->tickCount - detectdata.detecttick))); // extrapolate the viewangle using a static delta and the amount of ticks that have passed from detection
            //interfaces.engine->SetViewAngles(cmd->viewangles);
        }
        else
        {
            cmd->forwardmove = 0.f;
            cmd->sidemove = 0.f;
        }

        return;
    }

    Vector originalpos = localPlayer->origin();
    Vector originalvel = localPlayer->velocity();
    int originalflags = localPlayer->flags();
    float originalfmove = cmd->forwardmove;
    float originalsmove = cmd->sidemove;
    Vector originalangles = cmd->viewangles;

    float yawdelta = std::clamp(cmd->viewangles.y - lasttickyaw, -(180.f / config->misc.edgeBugPredAmnt), 180.f / config->misc.edgeBugPredAmnt);
    //prediction

    if (memory->globalVars->tickCount < detectdata.detecttick || memory->globalVars->tickCount > detectdata.edgebugtick)
    {
        const int desiredrounds = (config->misc.advancedDetectionEB && yawdelta < 0.1f) ? 4 : 4;

        for (int predRound = 0; predRound < desiredrounds; predRound++)
        {

            memory->restoreEntityToPredictedFrame(0, interfaces->prediction->split->commandsPredicted - 1);

            //create desired cmd
            UserCmd predictcmd = *cmd;

            detectdata.startingyaw = originalangles.y;

            if (predRound == 0)
            {
                detectdata.crouched = true;
                predictcmd.buttons |= UserCmd::IN_DUCK;
                detectdata.strafing = false;
                predictcmd.forwardmove = 0.f;
                predictcmd.sidemove = 0.f;

            }
            else if (predRound == 1)
            {
                detectdata.crouched = false;
                predictcmd.buttons &= ~UserCmd::IN_DUCK;
                detectdata.strafing = false;
                predictcmd.forwardmove = 0.f;
                predictcmd.sidemove = 0.f;

            }
            else if (predRound == 2)
            {
                detectdata.crouched = true;
                predictcmd.buttons |= UserCmd::IN_DUCK;
                detectdata.strafing = true;
                predictcmd.forwardmove = originalfmove;
                predictcmd.sidemove = originalsmove;
            }
            else if (predRound == 3)
            {
                detectdata.crouched = false;
                predictcmd.buttons &= ~UserCmd::IN_DUCK;
                detectdata.strafing = true;
                predictcmd.forwardmove = originalfmove;
                predictcmd.sidemove = originalsmove;
            }


            detectionpositions.clear();
            detectionpositions.push_back(std::pair<Vector, Vector>(localPlayer->origin(), localPlayer->velocity()));



            for (int ticksPredicted = 0; ticksPredicted < config->misc.edgeBugPredAmnt; ticksPredicted++)
            {
                Vector old_velocity = localPlayer->velocity();
                int old_flags = localPlayer->flags();
                Vector old_pos = localPlayer->origin();

                if (detectdata.strafing)
                {
                    predictcmd.viewangles.y = Helpers::normalizeYaw(originalangles.y + (yawdelta * ticksPredicted));
                }


                EnginePrediction::run(&predictcmd); // predict 1 more tick
                Vector predicted_velocity = localPlayer->velocity();
                int predicted_flags = localPlayer->flags();
                detectionpositions.push_back(std::pair<Vector, Vector>(localPlayer->origin(), localPlayer->velocity()));

                if ((old_flags & 1) || (predicted_flags & 1) || round(predicted_velocity.length2D()) == 0.f || round(old_velocity.length2D()) == 0.f || localPlayer->moveType() == MoveType::LADDER || old_velocity.z > 0.f)
                {
                    detectdata.detecttick = 0;
                    detectdata.edgebugtick = 0;
                    break;
                }

                if (detectionpositions.size() > 2)
                {
                    if (actualebdetection(detectionpositions.at(detectionpositions.size() - 3).second, detectionpositions.at(detectionpositions.size() - 2).second, detectionpositions.at(detectionpositions.size() - 1).second))
                    {

                        detectdata.detecttick = memory->globalVars->tickCount;
                        detectdata.edgebugtick = memory->globalVars->tickCount + (ticksPredicted + 1);

                        detectdata.forwardmove = originalfmove;
                        detectdata.sidemove = originalsmove;

                        detectdata.yawdelta = yawdelta;

                        if (predRound < 2)
                        {
                            cmd->forwardmove = 0.f;
                            cmd->sidemove = 0.f;
                        }
                        else
                        {
                            cmd->forwardmove = originalfmove;
                            cmd->sidemove = originalsmove;
                        }

                        if (predRound == 0 || predRound == 2)
                        {
                            cmd->buttons |= UserCmd::IN_DUCK;
                        }
                        else
                        {
                            cmd->buttons &= ~UserCmd::IN_DUCK;
                        }

                        Visuals::ebpos = old_pos;

                        return;
                    }

                }
            }
        }

    }
}

struct DATAFORDETECT
{
    Vector velocity;
    bool onground;
};
static float lastEBDETECT = 0.f;
std::deque<DATAFORDETECT> VelocitiesForDetection;

void Misc::ebdetections(UserCmd* cmd)
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!interfaces->engine->isConnected() || !interfaces->engine->isInGame())
        return;

    if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER)
    {
        VelocitiesForDetection.clear();
        return;
    }

    DATAFORDETECT d;
    d.velocity = localPlayer->velocity();
    d.onground = localPlayer->flags() & FL_ONGROUND;

    VelocitiesForDetection.push_front(d);

    if (VelocitiesForDetection.size() > 2)
        VelocitiesForDetection.pop_back();

    static auto sv_Gravity = interfaces->cvar->findVar(skCrypt("sv_gravity"));
    float negativezvel = sv_Gravity->getFloat() * -0.5f * memory->globalVars->intervalPerTick;

    if (VelocitiesForDetection.size() == 2 && ((round(negativezvel * 100.f) == round(VelocitiesForDetection.at(0).velocity.z * 100.f) && VelocitiesForDetection.at(1).velocity.z < negativezvel && !VelocitiesForDetection.at(1).onground && !VelocitiesForDetection.at(0).onground) || detectdata.edgebugtick == memory->globalVars->tickCount))
    {
        VelocitiesForDetection.clear();
        lastEBDETECT = memory->globalVars->realtime;
        if (config->misc.ebdetect.chat)
            memory->clientMode->getHudChat()->printf(0, skCrypt("[Medusa.uno] Edge bugged"));
        if (config->misc.ebdetect.effect)
            localPlayer->HealthBoostTime() = memory->globalVars->currenttime + config->misc.ebdetect.effectTime;
        if (config->misc.ebdetect.sound == 0)
            return;
        else if (config->misc.ebdetect.sound == 1)
            interfaces->engine->clientCmdUnrestricted(c_xor("play survival/paradrop_idle_01.wav"));
        else if (config->misc.ebdetect.sound == 2)
            interfaces->engine->clientCmdUnrestricted(c_xor("play physics/metal/metal_solid_impact_bullet2"));
        else if (config->misc.ebdetect.sound == 3)
            interfaces->engine->clientCmdUnrestricted(c_xor("play buttons/arena_switch_press_02"));
        else if (config->misc.ebdetect.sound == 4)
            interfaces->engine->clientCmdUnrestricted(c_xor("play training/timer_bell"));
        else if (config->misc.ebdetect.sound == 5)
            interfaces->engine->clientCmdUnrestricted(c_xor("play physics/glass/glass_impact_bullet1"));
        else if (config->misc.ebdetect.sound == 6)
            interfaces->engine->clientCmdUnrestricted(c_xor("play survival/money_collect_04"));
        else if (config->misc.ebdetect.sound == 7)
            interfaces->engine->clientCmdUnrestricted(("play " + config->misc.customEBsound).c_str());
    }
}

void textEllipsisInTableCell(const char* text) noexcept
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImGuiTable* table = g.CurrentTable;
    IM_ASSERT(table != NULL && "Need to call textEllipsisInTableCell() after BeginTable()!");
    IM_ASSERT(table->CurrentColumn != -1);

    const char* textEnd = ImGui::FindRenderedTextEnd(text);
    ImVec2 textSize = ImGui::CalcTextSize(text, textEnd, true);
    ImVec2 textPos = window->DC.CursorPos;
    float textHeight = ImMax(textSize.y, table->RowMinHeight - table->CellPaddingY * 2.0f);

    float ellipsisMax = ImGui::TableGetCellBgRect(table, table->CurrentColumn).Max.x;
    ImGui::RenderTextEllipsis(window->DrawList, textPos, ImVec2(ellipsisMax, textPos.y + textHeight + g.Style.FramePadding.y), ellipsisMax, ellipsisMax, text, textEnd, &textSize);

    ImRect bb(textPos, textPos + textSize);
    ImGui::ItemSize(textSize, 0.0f);
    ImGui::ItemAdd(bb, 0);
}

void Misc::drawPlayerList() noexcept
{
    if (!config->misc.playerList.enabled)
        return;

    if (config->misc.playerList.pos != ImVec2{}) {
        ImGui::SetNextWindowPos(config->misc.playerList.pos);
        config->misc.playerList.pos = {};
    }

    static bool changedName = true;
    static std::string nameToChange = "";

    if (!changedName && nameToChange != "")
        changedName = changeName(false, (nameToChange + '\x1').c_str(), 1.0f);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (!gui->isOpen())
    {
        windowFlags |= ImGuiWindowFlags_NoInputs;
        return;
    }

    GameData::Lock lock;
    if ((GameData::players().empty()) && !gui->isOpen())
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.5f);
    if (config->misc.borders)
    ImGui::PushStyleColor(ImGuiCol_Border, { config->menu.accentColor.color[0], config->menu.accentColor.color[1], config->menu.accentColor.color[2], config->menu.accentColor.color[3] });
    ImGui::SetNextWindowSize(ImVec2(300.0f, 300.0f), ImGuiCond_Once);


    if (ImGui::Begin("Player List", nullptr, windowFlags)) {
        PostProcessing::performFullscreenBlur(ImGui::GetWindowDrawList(), 1.f);
        if (ImGui::beginTable("", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 120.0f);
            ImGui::TableSetupColumn("Steam ID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("Wins", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("Health", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("Armor", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("Money", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetColumnEnabled(2, config->misc.playerList.steamID);
            ImGui::TableSetColumnEnabled(3, config->misc.playerList.rank);
            ImGui::TableSetColumnEnabled(4, config->misc.playerList.wins);
            ImGui::TableSetColumnEnabled(5, config->misc.playerList.health);
            ImGui::TableSetColumnEnabled(6, config->misc.playerList.armor);
            ImGui::TableSetColumnEnabled(7, config->misc.playerList.money);

            ImGui::TableHeadersRow();

            std::vector<std::reference_wrapper<const PlayerData>> playersOrdered{ GameData::players().begin(), GameData::players().end() };
            std::ranges::sort(playersOrdered, [](const PlayerData& a, const PlayerData& b) {
                // enemies first
                if (a.enemy != b.enemy)
                    return a.enemy && !b.enemy;

                return a.handle < b.handle;
                });

            ImGui::PushFont(gui->getUnicodeFont());

            for (const PlayerData& player : playersOrdered) {
                ImGui::TableNextRow();
                ImGui::PushID(ImGui::TableGetRowIndex());

                if (ImGui::TableNextColumn())
                    ImGui::Text("%d", player.userId);

                if (ImGui::TableNextColumn())
                {
                    ImGui::Image(player.getAvatarTexture(), { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() });
                    ImGui::SameLine();
                    textEllipsisInTableCell(player.name.c_str());
                }

                if (ImGui::TableNextColumn() && ImGui::smallButtonFullWidth("Copy", player.steamID == 0))
                    ImGui::SetClipboardText(std::to_string(player.steamID).c_str());

                if (ImGui::TableNextColumn()) {
                    ImGui::Image(player.getRankTexture(), { 2.45f /* -> proportion 49x20px */ * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() });
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::PushFont(nullptr);
                        ImGui::TextUnformatted(player.getRankName().data());
                        ImGui::PopFont();
                        ImGui::EndTooltip();
                    }
                    
                }

                if (ImGui::TableNextColumn())
                    ImGui::Text("%d", player.competitiveWins);

                if (ImGui::TableNextColumn()) {
                    if (!player.alive)
                        ImGui::TextColored({ 1.0f, 0.0f, 0.0f, 1.0f }, "%s", "Dead");
                    else
                        ImGui::Text("%d HP", player.health);
                }

                if (ImGui::TableNextColumn())
                    ImGui::Text("%d", player.armor);

                if (ImGui::TableNextColumn())
                    ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "$%d", player.money);

                if (ImGui::TableNextColumn()){
                    if (ImGui::smallButtonFullWidth("...", false))
                        ImGui::OpenPopup("");

                    if (ImGui::BeginPopup("")) {
                        if (ImGui::Button("Steal name"))
                        {
                            changedName = changeName(false, (std::string{ player.name } + '\x1').c_str(), 1.0f);
                            nameToChange = player.name;

                            if (PlayerInfo playerInfo; interfaces->engine->isInGame() && localPlayer
                                && interfaces->engine->getPlayerInfo(localPlayer->index(), playerInfo) && (playerInfo.name == std::string{ "?empty" } || playerInfo.name == std::string{ "\n\xAD\xAD\xAD" }))
                                changedName = false;
                        }

                        if (ImGui::Button("Steal clantag"))
                            memory->setClanTag(player.clanTag.c_str(), player.clanTag.c_str());

                        if (GameData::local().exists && player.team == GameData::local().team && player.steamID != 0)
                        {
                            if (ImGui::Button("Kick"))
                            {
                                const std::string cmd = "callvote kick " + std::to_string(player.userId);
                                interfaces->engine->clientCmdUnrestricted(cmd.c_str());
                            }
                        }

                        ImGui::EndPopup();
                    }
                }

                ImGui::PopID();
            }

            ImGui::PopFont();
            ImGui::EndTable();
        }
    }
    ImGui::PopStyleVar();
    if (config->misc.borders)
    ImGui::PopStyleColor();
    ImGui::End();
}

void Misc::blockBot(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    static int blockTargetHandle = 0;

    if (!config->misc.blockBot || !config->misc.blockBotKey.isActive())
    {
        blockTargetHandle = 0;
        return;
    }

    float best = 1024.0f;
    if (!blockTargetHandle)
    {
        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
        {
            Entity* entity = interfaces->entityList->getEntity(i);

            if (!entity || !entity->isPlayer() || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
                continue;

            const auto distance = entity->getAbsOrigin().distTo(localPlayer->getAbsOrigin());
            if (distance < best)
            {
                best = distance;
                blockTargetHandle = entity->handle();
            }
        }
    }

    const auto target = interfaces->entityList->getEntityFromHandle(blockTargetHandle);
    if (target && target->isPlayer() && target != localPlayer.get() && !target->isDormant() && target->isAlive())
    {
        const auto targetVec = (target->getAbsOrigin() + target->velocity() * memory->globalVars->intervalPerTick - localPlayer->getAbsOrigin());
        const auto z1 = target->getAbsOrigin().z - localPlayer->getEyePosition().z;
        const auto z2 = target->getEyePosition().z - localPlayer->getAbsOrigin().z;
        if (z1 >= 0.0f || z2 <= 0.0f)
        {
            Vector fwd = Vector::fromAngle2D(cmd->viewangles.y);
            Vector side = fwd.crossProduct(Vector::up());
            Vector move = Vector{ fwd.dotProduct2D(targetVec), side.dotProduct2D(targetVec), 0.0f };
            move *= 45.0f;

            const float l = move.length2D();
            if (l > 450.0f)
                move *= 450.0f / l;

            cmd->forwardmove = move.x;
            cmd->sidemove = move.y;
        }
        else
        {
            Vector fwd = Vector::fromAngle2D(cmd->viewangles.y);
            Vector side = fwd.crossProduct(Vector::up());
            Vector tar = (targetVec / targetVec.length2D()).crossProduct(Vector::up());
            tar = tar.snapTo4();
            tar *= tar.dotProduct2D(targetVec);
            Vector move = Vector{ fwd.dotProduct2D(tar), side.dotProduct2D(tar), 0.0f };
            move *= 45.0f;

            const float l = move.length2D();
            if (l > 450.0f)
                move *= 450.0f / l;

            cmd->forwardmove = move.x;
            cmd->sidemove = move.y;
        }
    }
}

static int buttons = 0;

void Misc::runFreeCam(UserCmd* cmd, Vector viewAngles) noexcept
{
    static Vector currentViewAngles = Vector{ };
    static Vector realViewAngles = Vector{ };
    static bool wasCrouching = false;
    static bool wasHoldingAttack = false;
    static bool wasHoldingUse = false;
    static bool hasSetAngles = false;

    buttons = cmd->buttons;
    if (!config->visuals.freeCam || !config->visuals.freeCamKey.isActive())
    {
        if (hasSetAngles)
        {
            interfaces->engine->setViewAngles(realViewAngles);
            cmd->viewangles = currentViewAngles;
            if (wasCrouching)
                cmd->buttons |= UserCmd::IN_DUCK;
            if (wasHoldingAttack)
                cmd->buttons |= UserCmd::IN_ATTACK;
            if (wasHoldingUse)
                cmd->buttons |= UserCmd::IN_USE;
            wasCrouching = false;
            wasHoldingAttack = false;
            wasHoldingUse = false;
            hasSetAngles = false;
        }
        currentViewAngles = Vector{};
        return;
    }

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (currentViewAngles.null())
    {
        currentViewAngles = cmd->viewangles;
        realViewAngles = viewAngles;
        wasCrouching = cmd->buttons & UserCmd::IN_DUCK;
    }

    cmd->forwardmove = 0;
    cmd->sidemove = 0;
    cmd->buttons = 0;
    if (wasCrouching)
        cmd->buttons |= UserCmd::IN_DUCK;
    if (wasHoldingAttack)
        cmd->buttons |= UserCmd::IN_ATTACK;
    if (wasHoldingUse)
        cmd->buttons |= UserCmd::IN_USE;
    cmd->viewangles = currentViewAngles;
    hasSetAngles = true;
}

void Misc::freeCam(ViewSetup* setup) noexcept
{
    static Vector newOrigin = Vector{ };

    if (!config->visuals.freeCam || !config->visuals.freeCamKey.isActive())
    {
        newOrigin = Vector{ };
        return;
    }

    if (!localPlayer || !localPlayer->isAlive())
        return;

    float freeCamSpeed = fabsf(static_cast<float>(config->visuals.freeCamSpeed));

    if (newOrigin.null())
        newOrigin = setup->origin;

    Vector forward{ }, right{ }, up{ };

    Vector::fromAngleAll(setup->angles, &forward, &right, &up);

    const bool backBtn = buttons & UserCmd::IN_BACK;
    const bool forwardBtn = buttons & UserCmd::IN_FORWARD;
    const bool rightBtn = buttons & UserCmd::IN_MOVERIGHT;
    const bool leftBtn = buttons & UserCmd::IN_MOVELEFT;
    const bool shiftBtn = buttons & UserCmd::IN_SPEED;
    const bool duckBtn = buttons & UserCmd::IN_DUCK;
    const bool jumpBtn = buttons & UserCmd::IN_JUMP;

    if (duckBtn)
        freeCamSpeed *= 0.45f;

    if (shiftBtn)
        freeCamSpeed *= 1.65f;

    if (forwardBtn)
        newOrigin += forward * freeCamSpeed;

    if (rightBtn)
        newOrigin += right * freeCamSpeed;

    if (leftBtn)
        newOrigin -= right * freeCamSpeed;

    if (backBtn)
        newOrigin -= forward * freeCamSpeed;

    if (jumpBtn)
        newOrigin += up * freeCamSpeed;

    setup->origin = newOrigin;
}

static void shadeVertsHSVColorGradientKeepAlpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1)
{
    ImVec2 gradient_extent = gradient_p1 - gradient_p0;
    float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;

    ImVec4 col0HSV = ImGui::ColorConvertU32ToFloat4(col0);
    ImVec4 col1HSV = ImGui::ColorConvertU32ToFloat4(col1);
    ImGui::ColorConvertRGBtoHSV(col0HSV.x, col0HSV.y, col0HSV.z, col0HSV.x, col0HSV.y, col0HSV.z);
    ImGui::ColorConvertRGBtoHSV(col1HSV.x, col1HSV.y, col1HSV.z, col1HSV.x, col1HSV.y, col1HSV.z);
    ImVec4 colDelta = col1HSV - col0HSV;

    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        float d = ImDot(vert->pos - gradient_p0, gradient_extent);
        float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);

        float h = col0HSV.x + colDelta.x * t;
        float s = col0HSV.y + colDelta.y * t;
        float v = col0HSV.z + colDelta.z * t;

        ImVec4 rgb;
        ImGui::ColorConvertHSVtoRGB(h, s, v, rgb.x, rgb.y, rgb.z);
        vert->col = (ImGui::ColorConvertFloat4ToU32(rgb) & ~IM_COL32_A_MASK) | (vert->col & IM_COL32_A_MASK);
    }
}
void Misc::viewModelChanger(ViewSetup* setup) noexcept
{
    if (!localPlayer)
        return;

    auto activeWeapon = localPlayer->getActiveWeapon();

    if (!activeWeapon)
        return;

    if ((activeWeapon->itemDefinitionIndex2() == WeaponId::Aug || activeWeapon->itemDefinitionIndex2() == WeaponId::Sg553) && localPlayer->isScoped())
        return;

    constexpr auto setViewmodel = [](Entity* viewModel, const Vector& angles) constexpr noexcept
    {
        if (viewModel)
        {
            Vector forward = Vector::fromAngle(angles);
            Vector up = Vector::fromAngle(angles - Vector{ 90.0f, 0.0f, 0.0f });
            Vector side = forward.cross(up);
            Vector offset = side * config->visuals.viewModel.x + forward * config->visuals.viewModel.y + up * config->visuals.viewModel.z;
            memory->setAbsOrigin(viewModel, viewModel->getRenderOrigin() + offset);
            memory->setAbsAngle(viewModel, angles + Vector{ 0.0f, 0.0f, config->visuals.viewModel.roll });
        }
    };

    if (localPlayer->isAlive())
    {
        if (config->visuals.viewModel.enabled && !memory->input->isCameraInThirdPerson)
            setViewmodel(interfaces->entityList->getEntityFromHandle(localPlayer->viewModel()), setup->angles);
    }
    else if (auto observed = localPlayer->getObserverTarget(); observed && localPlayer->getObserverMode() == ObsMode::InEye)
    {
        if (config->visuals.viewModel.enabled && !observed->isScoped())
            setViewmodel(interfaces->entityList->getEntityFromHandle(observed->viewModel()), setup->angles);
    }
}

static Vector peekPosition{};
static bool hasShot = false;
void Misc::drawAutoPeekSex() noexcept
{
    if (!config->misc.autoPeek.enabled)
        return;
    if (!config->misc.autoPeekKey.isActive())
        return;
    if (peekPosition.notNull())
    {
        constexpr float step = 3.141592654f * 2.0f / 20.0f;
        std::vector<ImVec2> points;
        for (float lat = 0.f; lat <= 3.141592654f * 2.0f; lat += step)
        {
            const auto& point3d = Vector{ std::sin(lat), std::cos(lat), 0.f } *config->misc.autoPeekRadius;
            ImVec2 point2d;
            if (Helpers::worldToScreen(peekPosition + point3d, point2d))
                points.push_back(point2d);
        }
        if (config->misc.autoPeekStyle == 2)
        {
            interfaces->m_Effects->EnergySplash(peekPosition, Vector(0, 0, 0), false);
            if (hasShot)
                interfaces->m_Effects->EnergySplash(localPlayer->getAbsOrigin(), Vector(0, 0, 0), false);
            auto material = interfaces->materialSystem->findMaterial(skCrypt("effects/spark"));
            if (!material)
                return;
            if (!config->misc.autoPeek.rainbow)
                material->colorModulate(config->misc.autoPeek.color[0], config->misc.autoPeek.color[1], config->misc.autoPeek.color[2]);
            else
                material->colorModulate(rainbowColor(config->misc.autoPeek.rainbowSpeed));
        }
        if (config->misc.autoPeekStyle == 3)
        {
            interfaces->m_Effects->MetalSparks(peekPosition, Vector(0, 0, 0));
            auto material = interfaces->materialSystem->findMaterial(skCrypt("effects/spark"));
            if (!material)
                return;
            if (!config->misc.autoPeek.rainbow)
                material->colorModulate(config->misc.autoPeek.color[0], config->misc.autoPeek.color[1], config->misc.autoPeek.color[2]);
            else
                material->colorModulate(rainbowColor(config->misc.autoPeek.rainbowSpeed));
        }
    }
}
void Misc::drawAutoPeekD() noexcept
{
    if (!config->misc.autoPeek.enabled)
        return;
    if (!config->misc.autoPeekKey.isActive())
        return;
    if (peekPosition.notNull())
    {  
        if (config->misc.autoPeekStyle == 0)
            Render::Draw3DCircle(peekPosition, config->misc.autoPeekRadius, Color(Helpers::calculateColor(config->misc.autoPeek)));
    }
}

void RadialGradient3D( Vector pos, float radius, Color in, Color out ) noexcept
{
    ImVec2 center; ImVec2 g_pos;
    auto drawList = ImGui::GetBackgroundDrawList();
    Helpers::worldToScreen(Vector(pos), g_pos);
    center = ImVec2(g_pos.x, g_pos.y);
    drawList->_PathArcToFastEx(center, radius, 0, 128, 0);
    const int count = drawList->_Path.Size - 1;
    float step = (3.141592654f * 2.0f) / (count + 1);
    std::vector<ImVec2> point;
    for (float lat = 0.f; lat <= 3.141592654f * 2.0f; lat += step)
    {
        const auto& point3d = Vector(sin(lat), cos(lat), 0.f) * radius;
        ImVec2 point2d;
        if (Helpers::worldToScreen(Vector(pos) + point3d, point2d))
            point.push_back(ImVec2(point2d.x, point2d.y));
    }
    if (in.a() == 0 && out.a() == 0 || radius < 0.5f || point.size() < count + 1)
        return;

    unsigned int vtx_base = drawList->_VtxCurrentIdx;
    drawList->PrimReserve(count * 3, count + 1);

    // Submit vertices
    const ImVec2 uv = drawList->_Data->TexUvWhitePixel;
    drawList->PrimWriteVtx(center, uv, ImColor(in.r(), in.g(), in.b(), in.a()));
    for (int n = 0; n < count; n++)
        drawList->PrimWriteVtx(point[n + 1], uv, ImColor(out.r(), out.g(), out.b(), out.a()));

    // Submit a fan of triangles
    for (int n = 0; n < count; n++)
    {
        drawList->PrimWriteIdx((ImDrawIdx)(vtx_base));
        drawList->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + n));
        drawList->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((n + 1) % count)));
    }
    drawList->_Path.Size = 0;
}

void Misc::drawAutoPeek(ImDrawList* drawList) noexcept
{
    if (!config->misc.autoPeek.enabled)
        return;

    if (peekPosition.notNull())
    {
        constexpr float step = 3.141592654f * 2.0f / 40.0f;
        std::vector<ImVec2> points;
        for (float lat = 0.f; lat <= 3.141592654f * 2.0f; lat += step)
        {
            const auto& point3d = Vector{ std::sin(lat), std::cos(lat), 0.f } *config->misc.autoPeekRadius;
            ImVec2 point2d;
            if (Helpers::worldToScreen(peekPosition + point3d, point2d))
                points.push_back(point2d);
        }
        ImVec2 ceva;
        Helpers::worldToScreen(peekPosition, ceva);
        const ImU32 color = (Helpers::calculateColor(config->misc.autoPeek));
        if (config->misc.autoPeekStyle == 1)
        {
            drawList->AddConvexPolyFilled(points.data(), points.size(), color);
            drawList->AddPolyline(points.data(), points.size(), color, true, 2.f);
        }
        else if (config->misc.autoPeekStyle == 4)
            RadialGradient3D(peekPosition, config->misc.autoPeekRadius, Color(Helpers::calculateColor(config->misc.autoPeek)), Color(Helpers::calculateColor(config->misc.autoPeek, 0.f)));
        else if (config->misc.autoPeekStyle == 5)
            RadialGradient3D(peekPosition, config->misc.autoPeekRadius, Color(Helpers::calculateColor(config->misc.autoPeek, 0.f)), Color(Helpers::calculateColor(config->misc.autoPeek)));
        else if (config->misc.autoPeekStyle == 6)
            drawList->AddText(gui->fonts.logoBig, 50.f, ceva, color, "z");
    }
}

void Misc::autoPeek(UserCmd* cmd, Vector currentViewAngles) noexcept
{

    if (!localPlayer)
        return;

    if (!config->misc.autoPeek.enabled)
    {
        hasShot = false;
        peekPosition = Vector{};
        return;
    }

    if (!localPlayer->isAlive())
    {
        hasShot = false;
        peekPosition = Vector{};
        return;
    }

    auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    if (const auto mt = localPlayer->moveType(); mt == MoveType::LADDER || mt == MoveType::NOCLIP || !(localPlayer->flags() & 1))
        return;

    if (config->misc.autoPeekKey.isActive())
    {
        if (peekPosition.null())
            peekPosition = localPlayer->getRenderOrigin();

        auto revolver_shoot = activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver && !AntiAim::r8Working && (UserCmd::IN_ATTACK | UserCmd::IN_ATTACK2);

        if (revolver_shoot)
            hasShot = true;

        if (cmd->buttons & UserCmd::IN_ATTACK)
            hasShot = true;

        if (hasShot)
        {
            const float yaw = currentViewAngles.y;
            const auto difference = localPlayer->getRenderOrigin() - peekPosition;

            if (difference.length2D() > 2.5f)
            {
                const auto velocity = Vector{
                    difference.x * std::cos(yaw / 180.0f * 3.141592654f) + difference.y * std::sin(yaw / 180.0f * 3.141592654f),
                    difference.y * std::cos(yaw / 180.0f * 3.141592654f) - difference.x * std::sin(yaw / 180.0f * 3.141592654f),
                    difference.z };

                cmd->forwardmove = -velocity.x * 20.f;
                cmd->sidemove = velocity.y * 20.f;
            }
            else
            {
                hasShot = false;
                peekPosition = Vector{};
            }
        }
    }
    else
    {
        hasShot = false;
        peekPosition = Vector{};
    }
}

void Misc::jumpBug(UserCmd* cmd) noexcept
{
    if (!config->misc.jumpBug || !config->misc.jumpBugKey.isActive())
        return;

    if (!interfaces->engine->isConnected())
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto mt = localPlayer->moveType(); mt == MoveType::LADDER || mt == MoveType::NOCLIP)
        return;

    if (!(EnginePrediction::getFlags() & 1) && (localPlayer->flags() & 1))
    {
        if(config->misc.fastDuck)
            cmd->buttons &= ~UserCmd::IN_BULLRUSH;
        cmd->buttons |= UserCmd::IN_DUCK;
    }

    if (localPlayer->flags() & 1)
        cmd->buttons &= ~UserCmd::IN_JUMP;
}

void Misc::unlockHiddenCvars() noexcept
{
    auto iterator = **reinterpret_cast<conCommandBase***>(interfaces->cvar + 0x34);
    for (auto c = iterator->next; c != nullptr; c = c->next)
    {
        conCommandBase* cmd = c;
        cmd->flags &= ~(1 << 1);
        cmd->flags &= ~(1 << 4);
    }
}

void Misc::fakeDuck(UserCmd* cmd, bool& sendPacket) noexcept
{
    if (!config->misc.fakeduck || !config->misc.fakeduckKey.isActive())
        return;

    if (!interfaces->engine->isConnected())
        return;

    if (const auto gameRules = (*memory->gameRules); gameRules)
        if ((getGameMode() == GameMode::Casual || getGameMode() == GameMode::Deathmatch || getGameMode() == GameMode::ArmsRace || getGameMode() == GameMode::WarGames || getGameMode() == GameMode::Demolition) && gameRules->isValveDS())
            return;

    if (!localPlayer || !localPlayer->isAlive() || !(localPlayer->flags() & 1))
        return;

    const auto netChannel = interfaces->engine->getNetworkChannel();
    if (!netChannel)
        return;

    cmd->buttons |= UserCmd::IN_BULLRUSH;
    const bool crouch = netChannel->chokedPackets >= (maxUserCmdProcessTicks / 2);
    if (crouch)
        cmd->buttons |= UserCmd::IN_DUCK;
    else
        cmd->buttons &= ~UserCmd::IN_DUCK;
    sendPacket = netChannel->chokedPackets >= maxUserCmdProcessTicks;
}


void Misc::edgejump(UserCmd* cmd) noexcept
{
    if (!config->misc.edgeJump || !config->misc.edgeJumpKey.isActive())
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto mt = localPlayer->moveType(); mt == MoveType::LADDER || mt == MoveType::NOCLIP)
        return;

    if ((EnginePrediction::getFlags() & 1) && !(localPlayer->flags() & 1))
        cmd->buttons |= UserCmd::IN_JUMP;
}

void Misc::slowwalk(UserCmd* cmd) noexcept
{
    if (!config->misc.slowwalk || !config->misc.slowwalkKey.isActive())
        return;

    if (!interfaces->engine->isConnected())
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    const auto weaponData = activeWeapon->getWeaponData();
    if (!weaponData)
        return;

    const float maxSpeed = config->misc.slowwalkAmnt ? config->misc.slowwalkAmnt : (localPlayer->isScoped() ? weaponData->maxSpeedAlt : weaponData->maxSpeed) / 3;

    if (cmd->forwardmove && cmd->sidemove) {
        const float maxSpeedRoot = maxSpeed * static_cast<float>(M_SQRT1_2);
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeedRoot : maxSpeedRoot;
    } else if (cmd->forwardmove) {
        cmd->forwardmove = cmd->forwardmove < 0.0f ? -maxSpeed : maxSpeed;
    } else if (cmd->sidemove) {
        cmd->sidemove = cmd->sidemove < 0.0f ? -maxSpeed : maxSpeed;
    }
}

void Misc::inverseRagdollGravity() noexcept
{
    static auto ragdollGravity = interfaces->cvar->findVar(skCrypt("cl_ragdoll_gravity"));
    static auto backup = ragdollGravity->getInt();
    ragdollGravity->setValue(config->visuals.inverseRagdollGravity ? config->misc.ragdollGravity : backup);
}

enum
{
    IDoubleTap = 0,
    IOnShot = 1,
    IDmgOverride = 2,
    IHcOverride = 3,
    IFakeFlick = 4,
    IDesyncSide = 5,
    IDesyncAmnt = 6,
    IManual = 7,
    IFreestand = 8,
    IBaim = 9,
    ITriggerBot = 10,
    IAutoPeek = 11,
    IFakeDuck = 12,
    IEdgeBug = 13,
    IEdgeJump = 14,
    IMiniJump = 15,
    IPixelSurf = 16,
    IJumpBug = 17,
    IBlockBot = 18,
    IDoorSpam = 19,
    IFakeLag = 20
};

void Misc::Indictators() noexcept
{
    if (!config->misc.indicators.enabled)
        return;

    ImVec2 s = ImGui::GetIO().DisplaySize;
    if (config->misc.indicators.style == 0)
    {
        const auto [width, height] = interfaces->surface->getScreenSize();
        int h = 0;
        static auto percent_col = [](int per) -> Color {
            int red = per < 50 ? 255 : floorf(255 - (per * 2 - 100) * 255.f / 100.f);
            int green = per > 50 ? 255 : floorf((per * 2) * 255.f / 100.f);

            return Color(red, green, 0);
        };
        if ((config->misc.indicators.toShow & 1 << ITriggerBot) == 1 << ITriggerBot)
        {
            if (config->triggerbotKey.isActive() && config->lgb.enableTriggerbot)
            {
                std::ostringstream ss; ss << skCrypt("TRIGGER");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, ss.str().c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, ss.str().c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IDesyncSide) == 1 << IDesyncSide)
        {
            if (config->rageAntiAim[goofy].desync && config->rageAntiAim[static_cast<int>(goofy)].peekMode != 3)
            {
                std::string side = config->invert.isActive() ? " L" : " R";
                std::ostringstream ss; ss << skCrypt("DESYNC") << side;
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, ss.str().c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, ss.str().c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IManual) == 1 << IManual)
        {
            if (config->manualBackward.isActive())
            {
                std::string indicator = c_xor("AA BACKWARD");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
            if (config->manualForward.isActive())
            {
                std::string indicator = c_xor("AA FORWARD");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
            if (config->manualLeft.isActive())
            {
                std::string indicator = c_xor("AA LEFT");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
            if (config->manualRight.isActive())
            {
                std::string indicator = c_xor("AA RIGHT");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IFakeFlick) == 1 << IFakeFlick)
        {
            if (localPlayer && localPlayer->isAlive())
            {
                if (config->rageAntiAim[goofy].fakeFlick && config->fakeFlickOnKey.isActive())
                {
                    std::string side = !config->flipFlick.isActive() ? " R" : " L";
                    std::ostringstream ss; ss << skCrypt("FLICK ") << side;
                    Color color = Color(255, 255, 255, 200);
                    Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                    Render::gradient(14 + Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                    Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, ss.str().c_str());
                    Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, ss.str().c_str());
                    h += 36;
                }
            }
        }
        if ((config->misc.indicators.toShow & 1 << IFakeLag) == 1 << IFakeLag)
        {
            if (localPlayer && localPlayer->isAlive())
            {
                const auto netChannel = interfaces->engine->getNetworkChannel();
                if (netChannel)
                {
                    int choke1 = memory->clientState->chokedCommands;
                    std::ostringstream ss; ss << c_xor("FL ") << choke1;
                    Color color = Color(134, 120, 243, 255);
                    Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                    Render::gradient(14 + Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, ss.str().c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                    Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, ss.str().c_str());
                    Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, ss.str().c_str());
                    h += 36;
                }
            }
        }
        if ((config->misc.indicators.toShow & 1 << IDoubleTap) == 1 << IDoubleTap)
        {
            if (config->tickbase.doubletap.isActive())
            {
                std::string indicator = c_xor("DT");
                Color color;
                if (memory->globalVars->realtime - Tickbase::realTime > 0.24625f
                    && localPlayer && localPlayer->isAlive()
                    && localPlayer->getActiveWeapon()
                    && localPlayer->getActiveWeapon()->nextPrimaryAttack() <= (localPlayer->tickBase() - Tickbase::getTargetTickShift()) * memory->globalVars->intervalPerTick
                    && (config->misc.fakeduck && !config->misc.fakeduckKey.isActive()))
                    color = Color(255, 255, 255, 200);
                else
                    color = Color(255, 0, 0, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IOnShot) == 1 << IOnShot)
        {
            if (config->tickbase.hideshots.isActive())
            {
                std::string indicator = c_xor("ONSHOT");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IFreestand) == 1 << IFreestand)
        {
            if (config->freestandKey.isActive() && config->rageAntiAim[goofy].freestand)
            {
                std::string indicator = c_xor("FS");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IBaim) == 1 << IBaim)
        {
            if (config->forceBaim.isActive())
            {
                std::string indicator = c_xor("BAIM");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IHcOverride) == 1 << IHcOverride)
        {
            if (config->hitchanceOverride.isActive())
            {
                std::string indicator = c_xor("HC OVR");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IDmgOverride) == 1 << IDmgOverride)
        {
            if (config->minDamageOverrideKey.isActive())
            {
                std::string indicator = c_xor("DMG");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IAutoPeek) == 1 << IAutoPeek)
        {
            if (config->misc.autoPeek.enabled && config->misc.autoPeekKey.isActive())
            {
                std::string indicator = c_xor("PEEK");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IFakeDuck) == 1 << IFakeDuck)
        {
            if (config->misc.fakeduck && config->misc.fakeduckKey.isActive())
            {
                std::string indicator = c_xor("DUCK");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IEdgeBug) == 1 << IEdgeBug)
        {
            if (config->misc.edgeBug && config->misc.edgeBugKey.isActive())
            {
                std::string indicator = c_xor("EB");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IEdgeJump) == 1 << IEdgeJump)
        {
            if (config->misc.edgeJump && config->misc.edgeJumpKey.isActive())
            {
                std::string indicator = c_xor("EJ");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IPixelSurf) == 1 << IPixelSurf)
        {
            if (config->misc.autoPixelSurf && config->misc.autoPixelSurfKey.isActive())
            {
                std::string indicator = c_xor("PIXEL");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IJumpBug) == 1 << IJumpBug)
        {
            if (config->misc.jumpBug && config->misc.jumpBugKey.isActive())
            {
                std::string indicator = c_xor("JB");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }

        }
        if ((config->misc.indicators.toShow & 1 << IMiniJump) == 1 << IMiniJump) 
        {
            if (config->misc.miniJump && config->misc.miniJumpKey.isActive())
            {
                std::string indicator = c_xor("MJ");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IBlockBot) == 1 << IBlockBot)
        {
            if (config->misc.blockBot && config->misc.blockBotKey.isActive())
            {
                std::string indicator = c_xor("BLOCK");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IDoorSpam) == 1 << IDoorSpam)
        {
            if (config->misc.doorSpam && config->misc.doorSpamKey.isActive())
            {
                std::string indicator = c_xor("DOOR");
                Color color = Color(255, 255, 255, 200);
                Render::gradient(14, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::gradient(14 + Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, height - 340 - h - 3, Render::textWidth(hooks->IndShadow, indicator.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), Render::GradientType::GRADIENT_HORIZONTAL);
                Render::drawText(hooks->IndShadow, Color(0, 0, 0, 200), ImVec2{ 23 + 1, (float)(height - 340 - h + 1) }, indicator.c_str());
                Render::drawText(hooks->IndFont, color, ImVec2{ 23, (float)(height - 340 - h) }, indicator.c_str());
                h += 36;
            }
        }     
    }
    if (config->misc.indicators.style == 1)
    {
        int offset = 1;
        {
            interfaces->surface->setTextColor(255, 255, 255, 255);
            interfaces->surface->setTextFont(hooks->smallFonts);
            interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset);
            std::string text1 = c_xor("MEDUSA");
            interfaces->surface->printText(std::wstring(text1.begin(), text1.end()));
            auto textSize = interfaces->surface->getTextSize(hooks->smallFonts, L"MEDUSA");
            if (!config->misc.indicators.color.rainbow)
                interfaces->surface->setTextColor(255 * config->misc.indicators.color.color[0], 255 * config->misc.indicators.color.color[1], 255 * config->misc.indicators.color.color[2], 255 * (std::sin(memory->globalVars->currenttime * 5) * 0.5f + 0.5f));
            else
                interfaces->surface->setTextColor(rainbowColor(config->misc.indicators.color.rainbowSpeed));
            interfaces->surface->setTextFont(hooks->smallFonts);
            interfaces->surface->setTextPosition(s.x / 2 + 7 + textSize.first, s.y / 2 + 9 * offset);
            std::string text2 = c_xor(".UNO");
            interfaces->surface->printText(std::wstring(text2.begin(), text2.end()));
            offset += 1;
        }
        interfaces->surface->setDrawColor(255, 255, 255, 255);
        interfaces->surface->drawLine(s.x / 2 + 7, s.y / 2 + interfaces->surface->getTextSize(hooks->smallFonts, L"MEDUSA.UNO").second + 9, s.x / 2 + interfaces->surface->getTextSize(hooks->smallFonts, L"MEDUSA.UNO").first + 7, s.y / 2 + 9 + interfaces->surface->getTextSize(hooks->smallFonts, L"MEDUSA.UNO").second);
        if ((config->misc.indicators.toShow & 1 << ITriggerBot) == 1 << ITriggerBot)
        {
            if (config->triggerbotKey.isActive() && config->lgb.enableTriggerbot)
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("TRIGGERBOT");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IDesyncSide) == 1 << IDesyncSide)
        {
            if (config->rageAntiAim[goofy].desync && config->rageAntiAim[static_cast<int>(goofy)].peekMode != 3)
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("DESYNC ");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                auto textSize = interfaces->surface->getTextSize(hooks->smallFonts, L"DESYNC ");
                interfaces->surface->setTextColor(255, 255, 255, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7 + textSize.first, s.y / 2 + 9 * offset + 3);
                auto invert = config->invert.isActive() ? L" L" : L" R";
                interfaces->surface->printText(invert);
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IManual) == 1 << IManual)
        {
            if (config->manualBackward.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("MANUAL BACK");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
            if (config->manualForward.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("MANUAL  FORWARD");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
            if (config->manualLeft.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("MANUAL  LEFT");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
            if (config->manualRight.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("MANUAL  RIGHT");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IFakeFlick) == 1 << IFakeFlick)
        {
            if (localPlayer && localPlayer->isAlive())
            {
                if (config->rageAntiAim[goofy].fakeFlick && config->fakeFlickOnKey.isActive())
                {
                    interfaces->surface->setTextColor(255, 255, 2552, 255);
                    interfaces->surface->setTextFont(hooks->smallFonts);
                    interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                    interfaces->surface->printText(L"FAKE  FLICK ");
                    auto textSize = interfaces->surface->getTextSize(hooks->smallFonts, L"FAKE  FLICK ");
                    interfaces->surface->setTextColor(255, 255, 255, 255);
                    interfaces->surface->setTextFont(hooks->smallFonts);
                    interfaces->surface->setTextPosition(s.x / 2 + 7 + textSize.first, s.y / 2 + 9 * offset + 3);
                    std::wstring side = !config->flipFlick.isActive() ? L" R" : L" L";
                    interfaces->surface->printText(side);
                    offset += 1;
                }
            }
        }
        if ((config->misc.indicators.toShow & 1 << IDoubleTap) == 1 << IDoubleTap)
        {
            if (config->tickbase.doubletap.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("DOUBLE  TAP");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IOnShot) == 1 << IOnShot)
        {
            if (config->tickbase.hideshots.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("HIDE  SHOTS");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IFreestand) == 1 << IFreestand)
        {
            if (config->freestandKey.isActive() && config->rageAntiAim[goofy].freestand)
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("FREESTAND");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IBaim) == 1 << IBaim)
        {
            if (config->forceBaim.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("FORCE  BAIM");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IHcOverride) == 1 << IHcOverride)
        {
            if (config->hitchanceOverride.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("HC  OVERRIDE");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IDmgOverride) == 1 << IDmgOverride)
        {
            if (config->minDamageOverrideKey.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("DMG  OVERRIDE");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IAutoPeek) == 1 << IAutoPeek)
        {
            if (config->misc.autoPeek.enabled && config->misc.autoPeekKey.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("AUTO  PEEK");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IFakeDuck) == 1 << IFakeDuck)
        {
            if (config->misc.fakeduck && config->misc.fakeduckKey.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("FAKE  DUCK");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IEdgeBug) == 1 << IEdgeBug)
        {
            if (config->misc.edgeBug && config->misc.edgeBugKey.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("EDGE  BUG");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IEdgeJump) == 1 << IEdgeJump)
        {
            if (config->misc.edgeJump && config->misc.edgeJumpKey.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("EDGE  JUMP");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;

            }
        }
        if ((config->misc.indicators.toShow & 1 << IPixelSurf) == 1 << IPixelSurf)
        {
            if (config->misc.autoPixelSurf && config->misc.autoPixelSurfKey.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("PIXEL  SURF");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IJumpBug) == 1 << IJumpBug)
        {
            if (config->misc.jumpBug && config->misc.jumpBugKey.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("JUMP  BUG");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IMiniJump) == 1 << IMiniJump)
        {
            if (config->misc.miniJump && config->misc.miniJumpKey.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("MINI  JUMP");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IBlockBot) == 1 << IBlockBot)
        {
            if (config->misc.blockBot && config->misc.blockBotKey.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("BLOCK  BOT");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
        if ((config->misc.indicators.toShow & 1 << IDoorSpam) == 1 << IDoorSpam)
        {
            if (config->misc.doorSpam && config->misc.doorSpamKey.isActive())
            {
                interfaces->surface->setTextColor(255, 255, 2552, 255);
                interfaces->surface->setTextFont(hooks->smallFonts);
                interfaces->surface->setTextPosition(s.x / 2 + 7, s.y / 2 + 9 * offset + 3);
                std::string text = c_xor("DOOR  SPAM");
                interfaces->surface->printText(std::wstring(text.begin(), text.end()));
                offset += 1;
            }
        }
    }
}

static void staticScope(ImDrawList* drawList, const ImVec2& pos, ImU32 c) noexcept
{
    //left
    drawList->AddRectFilled(ImVec2{ pos.x + 0, pos.y + 0 }, ImVec2{ pos.x - 3900, pos.y + 1 }, c);
    //right
    drawList->AddRectFilled(ImVec2{ pos.x + 0, pos.y + 0 }, ImVec2{ pos.x + 3900, pos.y + 1 }, c);
    //top
    drawList->AddRectFilled(ImVec2{ pos.x + 0, pos.y + 0 }, ImVec2{ pos.x + 1, pos.y - 3900 }, c);
    //bottom
    drawList->AddRectFilled(ImVec2{ pos.x + 0, pos.y + 0 }, ImVec2{ pos.x + 1, pos.y + 3900 }, c);
}

void drawTriangleFromCenter(ImDrawList* drawList, const ImVec2& pos, unsigned color, bool outline) noexcept
{
    const auto l = std::sqrtf(ImLengthSqr(pos));
    if (!l) return;
    const auto posNormalized = pos / l;
    const auto center = ImGui::GetIO().DisplaySize / 2 + pos;

    const ImVec2 trianglePoints[] = {
            center + ImVec2{  0.5f * posNormalized.y, -0.5f * posNormalized.x } *config->condAA.visualizeSize,
            center + ImVec2{  1.0f * posNormalized.x,  1.0f * posNormalized.y } *config->condAA.visualizeSize,
            center + ImVec2{ -0.5f * posNormalized.y,  0.5f * posNormalized.x } *config->condAA.visualizeSize,
    };

    drawList->AddConvexPolyFilled(trianglePoints, 3, color);
    if (outline)
        drawList->AddPolyline(trianglePoints, 3, color | IM_COL32_A_MASK, ImDrawFlags_Closed, 1.5f);
}

void Misc::aaArrows(ImDrawList* drawList) noexcept 
{
    if (!config->condAA.visualize.enabled)
        return;

    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (memory->input->isCameraInThirdPerson)
        return;

    bool invert;
    bool isInvertToggled = config->invert.isActive();
    if (config->rageAntiAim[static_cast<int>(goofy)].peekMode != 3)
        invert = isInvertToggled;
    ImVec2 pos{ ImGui::GetIO().DisplaySize / 2 };
    ImU32 col{ Helpers::calculateColor(static_cast<Color4>(config->condAA.visualize)) };
    ImU32 col1{ Helpers::calculateColor(0, 0, 0, static_cast<int>(config->condAA.visualize.color[3] * 255.f))};
    if (config->condAA.visualizeType == 0)
    {
        if (config->rageAntiAim[static_cast<int>(goofy)].desync && config->rageAntiAim[static_cast<int>(goofy)].peekMode != 3)
        //if (config->fakeAngle.enabled && config->fakeAngle.peekMode != 3)
        {
            if (!invert)
                drawTriangleFromCenter(drawList, { config->condAA.visualizeOffset, 0 }, col, config->condAA.visualize.outline);
            else if (invert)
                drawTriangleFromCenter(drawList, { -config->condAA.visualizeOffset, 0 }, col, config->condAA.visualize.outline);
        }
    }
    if (config->condAA.visualizeType == 1)
    {
        if (config->manualForward.isActive())
            drawTriangleFromCenter(drawList, { 0, -config->condAA.visualizeOffset }, col, config->condAA.visualize.outline);
        if (config->manualBackward.isActive())
            drawTriangleFromCenter(drawList, { 0, config->condAA.visualizeOffset }, col, config->condAA.visualize.outline);
        if (config->manualRight.isActive() || (AntiAim::auto_direction_yaw == 1 && config->freestandKey.isActive()))
            drawTriangleFromCenter(drawList, { config->condAA.visualizeOffset, 0 }, col, config->condAA.visualize.outline);
        if (config->manualLeft.isActive() || (AntiAim::auto_direction_yaw == -1 && config->freestandKey.isActive()))
            drawTriangleFromCenter(drawList, { -config->condAA.visualizeOffset, 0 }, col, config->condAA.visualize.outline);
    }
}

void Misc::customScope() noexcept
{
    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!localPlayer->getActiveWeapon())
        return;

    if (localPlayer->getActiveWeapon()->itemDefinitionIndex2() == WeaponId::Sg553 || localPlayer->getActiveWeapon()->itemDefinitionIndex2() == WeaponId::Aug)
        return;

    if (config->visuals.scope.type == 0)
    {
        return;
    }
    if (config->visuals.scope.type == 1)
    {
        if (localPlayer->isScoped())
        {
            const auto [width, height] = interfaces->surface->getScreenSize();
            interfaces->surface->setDrawColor(0, 0, 0, 255);
            interfaces->surface->drawFilledRect(0, height / 2, width, height / 2 + 1);
            interfaces->surface->setDrawColor(0, 0, 0, 255);
            interfaces->surface->drawFilledRect(width / 2, 0, width / 2 + 1, height);
        }
    }
    if (config->visuals.scope.type == 2)
    {
        if (localPlayer->isScoped() && config->visuals.scope.fade)
        {
            auto offset = config->visuals.scope.offset;
            auto leng = config->visuals.scope.length;
            auto accent = Color(config->visuals.scope.color.color[0], config->visuals.scope.color.color[1], config->visuals.scope.color.color[2], config->visuals.scope.color.color[3]);
            auto accent2 = Color(config->visuals.scope.color.color[0], config->visuals.scope.color.color[1], config->visuals.scope.color.color[2], 0.f);
            const auto [width, height] = interfaces->surface->getScreenSize();
            //right
            if (!config->visuals.scope.removeRight)
            Render::gradient(width / 2 + offset, height / 2, leng, 1, accent, accent2, Render::GradientType::GRADIENT_HORIZONTAL);
            //left
            if (!config->visuals.scope.removeLeft)
            Render::gradient(width / 2 - leng - offset, height / 2, leng, 1, accent2, accent, Render::GradientType::GRADIENT_HORIZONTAL);
            //bottom
            if (!config->visuals.scope.removeBottom)
            Render::gradient(width / 2, height / 2 + offset, 1, leng, accent, accent2, Render::GradientType::GRADIENT_VERTICAL);
            //top
            if (!config->visuals.scope.removeTop)
            Render::gradient(width / 2, height / 2 - leng - offset, 1, leng, accent2, accent, Render::GradientType::GRADIENT_VERTICAL);
        }
        else if (localPlayer->isScoped() && !config->visuals.scope.fade)
        {
            auto offset = config->visuals.scope.offset;
            auto leng = config->visuals.scope.length;
            auto accent = Color(config->visuals.scope.color.color[0], config->visuals.scope.color.color[1], config->visuals.scope.color.color[2], config->visuals.scope.color.color[3]);
            const auto [width, height] = interfaces->surface->getScreenSize();
            //right
            if (!config->visuals.scope.removeRight)
            Render::rectFilled(width / 2 + offset, height / 2, leng, 1, accent);
            //left
            if (!config->visuals.scope.removeLeft)
            Render::rectFilled(width / 2 - leng - offset, height / 2, leng, 1, accent);
            //bottom
            if (!config->visuals.scope.removeBottom)
            Render::rectFilled(width / 2, height / 2 + offset, 1, leng, accent);
            //top
            if (!config->visuals.scope.removeTop)
            Render::rectFilled(width / 2, height / 2 - leng - offset, 1, leng, accent);
        }
    }
    if (config->visuals.scope.type == 3)
    {
        return;
    }
}

void Misc::updateClanTag(bool tagChanged) noexcept
{
    static bool wasEnabled = false;
    static auto clanId = interfaces->cvar->findVar("cl_clanid");
    if (!config->misc.clantag && wasEnabled)
    {
        interfaces->engine->clientCmdUnrestricted(("cl_clanid " + std::to_string(clanId->getInt())).c_str());
        wasEnabled = false;
    }
    if (!config->misc.clantag)
    {
        return;
    }

    static std::string clanTag;

    static int lastTime= 0;
    int time = memory->globalVars->currenttime * M_PI;
    if (config->misc.clantag)
    {
        wasEnabled = true;
        if (time != lastTime)
        {
            switch (time % 24)
            {
            case 0: { memory->setClanTag(skCrypt("卐 Medusa.un "), skCrypt("卐 Medusa.un ")); break; }
            case 1: { memory->setClanTag(skCrypt("卐 Medusa.uno "), skCrypt("卐 Medusa.uno ")); break; }
            case 2: { memory->setClanTag(skCrypt("卐 Medusa.uno "), skCrypt("卐 Medusa.uno ")); break; }
            case 3: { memory->setClanTag(skCrypt("卐 Medusa.uno "), skCrypt("卐 Medusa.uno ")); break; }
            case 4: { memory->setClanTag(skCrypt("卐 Medusa.uno "), skCrypt("卐 Medusa.uno ")); break; }
            case 5: { memory->setClanTag(skCrypt("卐 Medusa.uno "), skCrypt("卐 Medusa.uno ")); break; }
            case 6: { memory->setClanTag(skCrypt("卐 Medusa.uno "), skCrypt("卐 Medusa.uno ")); break; }
            case 7: { memory->setClanTag(skCrypt("卐 Medusa.un "), skCrypt("卐 Medusa.un ")); break; }
            case 8: { memory->setClanTag(skCrypt("卐 Medusa.u "), skCrypt("卐 Medusa.u ")); break; }
            case 9: { memory->setClanTag(skCrypt("卐 Medusa "), skCrypt("卐 Medusa ")); break; }
            case 10: { memory->setClanTag(skCrypt("卐 Medus "), skCrypt("卐 Medus ")); break; }
            case 11: { memory->setClanTag(skCrypt("卐 Medu "), skCrypt("卐 Medu ")); break; }
            case 12: { memory->setClanTag(skCrypt("卐 Med "), skCrypt("卐 Med ")); break; }
            case 13: { memory->setClanTag(skCrypt("卐 Me "), skCrypt("卐 Me ")); break; }
            case 14: { memory->setClanTag(skCrypt("卐 M "), skCrypt("卐 M ")); break; }
            case 15: { memory->setClanTag("卐", "卐"); break; }
            case 16: { memory->setClanTag(skCrypt("卐 M "), skCrypt("卐 M ")); break; }
            case 17: { memory->setClanTag(skCrypt("卐 Me "), skCrypt("卐 Me ")); break; }
            case 18: { memory->setClanTag(skCrypt("卐 Med "), skCrypt("卐 Med ")); break; }
            case 19: { memory->setClanTag(skCrypt("卐 Medu "), skCrypt("卐 Medu ")); break; }
            case 20: { memory->setClanTag(skCrypt("卐 Medus "), skCrypt("卐 Medus ")); break; }
            case 21: { memory->setClanTag(skCrypt("卐 Medusa "), skCrypt("卐 Medusa ")); break; }
            case 22: { memory->setClanTag(skCrypt("卐 Medusa. "), skCrypt("卐 Medusa. ")); break; }
            case 23: { memory->setClanTag(skCrypt("卐 Medusa.u "), skCrypt("卐 Medusa.u ")); break; }
            }

        }
        lastTime = time;
    }
}

const bool anyActiveKeybinds() noexcept
{
    const bool rageBot = config->ragebotKey.canShowKeybind();
    const bool minDamageOverride = config->minDamageOverrideKey.canShowKeybind();
    const bool fakeAngle = config->rageAntiAim[static_cast<int>(goofy)].desync && config->invert.canShowKeybind();
    const bool antiAimManualForward = config->condAA.global && config->manualForward.canShowKeybind();
    const bool antiAimManualBackward = config->condAA.global && config->manualBackward.canShowKeybind();
    const bool antiAimManualRight = config->condAA.global && config->manualRight.canShowKeybind();
    const bool antiAimManualLeft = config->condAA.global && config->manualLeft.canShowKeybind();
    const bool doubletap = config->tickbase.doubletap.canShowKeybind();
    const bool hideshots = config->tickbase.hideshots.canShowKeybind();
    const bool legitBot = config->legitbotKey.canShowKeybind();
    const bool triggerBot = config->triggerbotKey.canShowKeybind();

    const bool zoom = config->visuals.zoom && config->visuals.zoomKey.canShowKeybind();
    //const bool thirdperson = config->visuals.thirdperson && config->visuals.thirdpersonKey.canShowKeybind();
    const bool freeCam = config->visuals.freeCam && config->visuals.freeCamKey.canShowKeybind();

    const bool blockbot = config->misc.blockBot && config->misc.blockBotKey.canShowKeybind();
    const bool edgejump = config->misc.edgeJump && config->misc.edgeJumpKey.canShowKeybind();
    const bool minijump = config->misc.miniJump && config->misc.miniJumpKey.canShowKeybind();
    const bool jumpBug = config->misc.jumpBug && config->misc.jumpBugKey.canShowKeybind();
    const bool edgebug = config->misc.edgeBug && config->misc.edgeBugKey.canShowKeybind();
    const bool autoPixelSurf = config->misc.autoPixelSurf && config->misc.autoPixelSurfKey.canShowKeybind();
    const bool slowwalk = config->misc.slowwalk && config->misc.slowwalkKey.canShowKeybind();
    const bool fakeduck = config->misc.fakeduck && config->misc.fakeduckKey.canShowKeybind();
    const bool autoPeek = config->misc.autoPeek.enabled && config->misc.autoPeekKey.canShowKeybind();
    const bool prepareRevolver = config->misc.prepareRevolver && config->misc.prepareRevolverKey.canShowKeybind();
    const bool doorSpam = config->misc.doorSpam && config->misc.doorSpamKey.canShowKeybind();
    const bool fakeFlick = config->fakeFlickOnKey.canShowKeybind();
    const bool flipFlick = config->flipFlick.canShowKeybind();
    const bool baim = config->forceBaim.canShowKeybind();
    const bool freestand = config->rageAntiAim[static_cast<int>(goofy)].freestand && config->freestandKey.canShowKeybind();
    return rageBot || minDamageOverride || fakeAngle || antiAimManualForward || antiAimManualBackward || antiAimManualRight  || antiAimManualLeft 
        || doubletap || hideshots || legitBot || triggerBot
        || zoom || freeCam || blockbot || edgejump || minijump || jumpBug || edgebug || autoPixelSurf || slowwalk || fakeduck || autoPeek || prepareRevolver || doorSpam || fakeFlick || flipFlick || baim || freestand;
}

void Misc::showKeybinds() noexcept
{
    if (!config->misc.keybindList.enabled)
        return;

    if (!anyActiveKeybinds && !gui->isOpen())
        return;

    if (config->misc.keybindList.pos != ImVec2{}) {
        ImGui::SetNextWindowPos(config->misc.keybindList.pos);
        config->misc.keybindList.pos = {};
    }
    auto windowFlags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    if (!gui->isOpen())
        windowFlags |= ImGuiWindowFlags_NoInputs;
    ImGui::SetNextWindowSize({150.f, 0.f}, ImGuiCond_Once);
    ImGui::PushFont(gui->getFIconsFont());
    auto size = ImGui::CalcTextSize(c_xor("Keybinds"));
    ImGui::SetNextWindowSizeConstraints({ 150.f, 0.f }, { 150.f, size.y * 2 - 2 });
    ImGui::GetBackgroundDrawList();
    ImGui::Begin(c_xor("Keybinds list"), nullptr, windowFlags);
    {
        //ImGui::PushFont(gui->getFIconsFont());
        auto draw_list = ImGui::GetBackgroundDrawList();
        auto p = ImGui::GetWindowPos();
        // set keybinds color
        auto bg_clr = ImColor(0.f, 0.f, 0.f, config->menu.transparency / 100);
        auto line_clr = Helpers::calculateColor(config->menu.accentColor);
        auto text_clr = ImColor(255, 255, 255, 255);
        auto glow = Helpers::calculateColor(config->menu.accentColor, 0.0f);
        auto glow1 = Helpers::calculateColor(config->menu.accentColor, config->menu.accentColor.color[3] * 0.5f);
        auto glow2 = Helpers::calculateColor(config->menu.accentColor);
        auto offset = 2;
        draw_list->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 150, p.y + size.y * 2 - 2), bg_clr, config->menu.windowStyle == 0 ? 5.f : 0.0f);
        // draw line
        if (config->menu.windowStyle == 0)
            draw_list->AddRect(ImVec2(p.x - 1, p.y - 1), ImVec2(p.x + 151, p.y + size.y * 2 - 1), line_clr, 5.f, 0, 1.5f);
        else if (config->menu.windowStyle == 1)
            draw_list->AddLine({ p.x, p.y - 1 }, { p.x + 150, p.y - 1 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 2)
            draw_list->AddLine({ p.x, p.y + size.y * 2 - 3 }, { p.x + 150, p.y + size.y * 2 - 3 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 3)
            draw_list->AddLine({ p.x, p.y - 1 }, { p.x + 150, p.y - 1 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 4)
            draw_list->AddLine({ p.x, p.y - 1 }, { p.x + 150, p.y - 1 }, line_clr, 2.f);
        // draw text
        draw_list->AddText(
            ImVec2(p.x + 75 - size.x / 2, p.y + size.y / 2 - 1),
            text_clr,
            c_xor("Keybinds")
        );
        if (config->menu.windowStyle == 3)
            draw_list->AddRectFilledMultiColor({ p.x, p.y }, ImVec2(p.x + 150, p.y + size.y),
                glow1,
                glow1,
                glow,
                glow);
        else if (config->menu.windowStyle == 4)
        {
            draw_list->AddRectFilledMultiColor(ImVec2(p.x, p.y), ImVec2(p.x + 2, p.y + size.y * 2), glow2, glow2, glow, glow);
            draw_list->AddRectFilledMultiColor(ImVec2(p.x + 148, p.y), ImVec2(p.x + 150, p.y + size.y * 2), glow2, glow2, glow, glow);
        }
        auto textO = ImGui::CalcTextSize(c_xor("[toggle]"));
        auto textH = ImGui::CalcTextSize(c_xor("[hold]"));
        auto textA = ImGui::CalcTextSize(c_xor("[on]"));
        //legitbot
        if (config->lgb.enabled && config->legitbotKey.isActive())
        {
            if (!config->legitbotKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Legitbot"));
                if (config->legitbotKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->legitbotKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
            if (config->lgb.enableTriggerbot && config->triggerbotKey.isActive())
            {
                if (config->triggerbotKey.CSB())
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Triggerbot"));
                    draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                    offset = offset + 1;
                }
                else
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Triggerbot"));
                    if (config->triggerbotKey.isDown())
                        draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                    else if (config->triggerbotKey.isToggled())
                        draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                    offset = offset + 1;
                }
            }
        }
        //ragebot
        if (config->ragebotKey.isActive() && config->ragebot.enabled)
        {
            if (!config->ragebotKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Ragebot"));
                if (config->ragebotKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->ragebotKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
            if (config->hitchanceOverride.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("HC override"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else if (config->hitchanceOverride.isActive2())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("HC override"));
                if (config->hitchanceOverride.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->hitchanceOverride.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //dt
        if (config->tickbase.doubletap.isActive())
        {
            if (config->tickbase.doubletap.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Double tap"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Double tap"));
                if (config->tickbase.doubletap.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->tickbase.doubletap.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //hs
        if (config->tickbase.hideshots.isActive())
        {
            if (config->tickbase.hideshots.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Hide shots"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Hide shots"));
                if (config->tickbase.hideshots.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->tickbase.hideshots.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }        
        //dmg
        if (config->minDamageOverrideKey.isActive())
        {
            if (config->minDamageOverrideKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Override DMG"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Override DMG"));
                if (config->minDamageOverrideKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->minDamageOverrideKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //baim
        if (config->forceBaim.isActive())
        {    
            if (config->forceBaim.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Force baim"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Force baim"));
                if (config->forceBaim.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->forceBaim.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //fakeflick
        if (config->fakeFlickOnKey.isActive() && config->rageAntiAim[static_cast<int>(goofy)].fakeFlick)
        {
            if (config->fakeFlickOnKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Fake flick"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Fake flick"));
                if (config->fakeFlickOnKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->fakeFlickOnKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
            if (config->flipFlick.isActive())
            {
                if (config->flipFlick.CSB())
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Fake flick flip"));
                    draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                    offset = offset + 1;
                }
                else if (!config->flipFlick.CSB())
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Fake flick flip"));
                    if (config->flipFlick.isDown())
                        draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                    else if (config->flipFlick.isToggled())
                        draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                    offset = offset + 1;
                }
            }
        }
        //fs
        if (config->freestandKey.isActive() && config->rageAntiAim[static_cast<int>(goofy)].freestand)
        {
            if (config->freestandKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Freestand"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Freestand"));
                if (config->freestandKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->freestandKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //manual
        if (config->condAA.global)
        {
            if (config->manualBackward.isActive())
            {
                if (config->manualBackward.CSB())
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Backward"));
                    draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                    offset = offset + 1;
                }
                else
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Backward"));
                    if (config->manualBackward.isDown())
                        draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                    else if (config->manualBackward.isToggled())
                        draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                    offset = offset + 1;
                }
            }
            if (config->manualForward.isActive())
            {
                if (config->manualForward.CSB())
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Forward"));
                    draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                    offset = offset + 1;
                }
                else
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Forward"));
                    if (config->manualForward.isDown())
                        draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                    else if (config->manualForward.isToggled())
                        draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                    offset = offset + 1;
                }
            }
            if (config->manualLeft.isActive())
            {
                if (config->manualLeft.CSB())
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Left"));
                    draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                    offset = offset + 1;
                }
                else
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Left"));
                    if (config->manualLeft.isDown())
                        draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                    else if (config->manualLeft.isToggled())
                        draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                    offset = offset + 1;
                }
            }
            if (config->manualRight.isActive())
            {
                if (config->manualRight.CSB())
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Right"));
                    draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                    offset = offset + 1;
                }
                else
                {
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Right"));
                    if (config->manualRight.isDown())
                        draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                    else if (config->manualRight.isToggled())
                        draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                    offset = offset + 1;
                }
            }
        }
        //invert
        if (config->invert.isActive() && config->rageAntiAim[static_cast<int>(goofy)].desync)
        {
            if (config->invert.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Desync invert"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            { 
            draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Desync invert"));
            if (config->invert.isDown())
                draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
            else if (config->invert.isToggled())
                draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
            offset = offset + 1;
            }
        }
        //fake duck
        if (config->misc.fakeduckKey.isActive() && config->misc.fakeduck)
        {
            if (config->misc.fakeduckKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Fake duck"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else if (!config->misc.fakeduckKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Fake duck"));
                if (config->misc.fakeduckKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->misc.fakeduckKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //auto peek
        if (config->misc.autoPeek.enabled && config->misc.autoPeekKey.isActive())
        {
            if (config->misc.autoPeekKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Auto peek"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Auto peek"));
                if (config->misc.autoPeekKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->misc.autoPeekKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //edge bug
        if (config->misc.edgeBug && config->misc.edgeBugKey.isActive())
        {
            if (config->misc.edgeBugKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Edge bug"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Edge bug"));
                if (config->misc.edgeBugKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->misc.edgeBugKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //jump bug
        if (config->misc.jumpBug && config->misc.jumpBugKey.isActive())
        {
            if (config->misc.jumpBugKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Jump bug"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Jump bug"));
                if (config->misc.jumpBugKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->misc.jumpBugKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //zoom
        if (config->visuals.zoom && config->visuals.zoomKey.isActive())
        {
            if (config->visuals.zoomKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Zoom"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Zoom"));
                if (config->visuals.zoomKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->visuals.zoomKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //freecam
        if (config->visuals.freeCam && config->visuals.freeCamKey.isActive())
        {
            if (config->visuals.freeCamKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Freecam"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Freecam"));
                if (config->visuals.freeCamKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->visuals.freeCamKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //auto pixel surf
        if (config->misc.autoPixelSurf && config->misc.autoPixelSurfKey.isActive())
        {
            if (config->misc.autoPixelSurfKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Auto pixel surf"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Auto pixel surf"));
                if (config->misc.autoPixelSurfKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->misc.autoPixelSurfKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //block bot
        if (config->misc.blockBot && config->misc.blockBotKey.isActive())
        {
            if (config->misc.blockBotKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Block bot"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Block bot"));
                if (config->misc.blockBotKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->misc.blockBotKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //edge jump
        if (config->misc.edgeJump && config->misc.edgeJumpKey.isActive())
        {
            if (config->misc.edgeJumpKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Edge jump"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Edge jump"));
                if (config->misc.edgeJumpKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->misc.edgeJumpKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //minijump
        if (config->misc.miniJump && config->misc.miniJumpKey.isActive())
        {
            if (config->misc.miniJumpKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Mini jump"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Mini jump"));
                if (config->misc.miniJumpKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->misc.miniJumpKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //slowwalk
        if (config->misc.slowwalk && config->misc.slowwalkKey.isActive())
        {
            if (config->misc.slowwalkKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Slow walk"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Slow walk"));
                if (config->misc.slowwalkKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->misc.slowwalkKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //r8
        if (config->misc.prepareRevolver && config->misc.prepareRevolverKey.isActive())
        {
            if (!config->misc.prepareRevolverKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Cocking R8"));
                if (config->misc.prepareRevolverKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->misc.prepareRevolverKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        //door spam
        if (config->misc.doorSpam && config->misc.doorSpamKey.isActive())
        {
            if (config->misc.doorSpamKey.CSB())
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Door spam"));
                draw_list->AddText(ImVec2(p.x - textA.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[on]"));
                offset = offset + 1;
            }
            else
            {
                draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("Door spam"));
                if (config->misc.doorSpamKey.isDown())
                    draw_list->AddText(ImVec2(p.x - textH.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[hold]"));
                else if (config->misc.doorSpamKey.isToggled())
                    draw_list->AddText(ImVec2(p.x - textO.x - 4.5f + 150, p.y + 13.5f *offset), ImColor(1.f, 1.f, 1.f, 1.f), c_xor("[toggle]"));
                offset = offset + 1;
            }
        }
        ImGui::PopFont();
    }
    ImGui::End();
}

void Misc::doorSpam(UserCmd* cmd) noexcept {

    if (!config->misc.doorSpam && (!localPlayer || !localPlayer->isAlive() || localPlayer->isDefusing()))
        return;

    constexpr auto doorRange = 200.0f;

    if (config->misc.doorSpamKey.isActive() && config->misc.doorSpamKey.isSet())
    {
        Trace trace;
        const auto startPos = localPlayer->getEyePosition();
        const auto endPos = startPos + Vector::fromAngle(cmd->viewangles) * doorRange;
        interfaces->engineTrace->traceRay({ startPos, endPos }, 0x46004009, localPlayer.get(), trace);


        if (trace.entity && trace.entity->getClientClass()->classId == ClassId::PropDoorRotating)
            if (cmd->buttons & UserCmd::IN_USE && cmd->tickCount & 1)
                cmd->buttons &= ~UserCmd::IN_USE;
    }
}

void Misc::spectatorList() noexcept
{
    if (!config->misc.spectatorList.enabled)
        return;

    GameData::Lock lock;

    const auto& observers = GameData::observers();

    if (std::ranges::none_of(observers, [](const auto& obs) { return obs.targetIsLocalPlayer; }) && !gui->isOpen())
        return;

    if (config->misc.spectatorList.pos != ImVec2{}) {
        ImGui::SetNextWindowPos(config->misc.spectatorList.pos);
        config->misc.spectatorList.pos = {};
    }

    ImGui::SetNextWindowSize({ 150.f, 0.f }, ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints({ 150.f, 0.f }, { 150.f, FLT_MAX });

    auto windowFlags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    if (!gui->isOpen())
        windowFlags += ImGuiWindowFlags_NoInputs;

    windowFlags |= ImGuiWindowFlags_NoTitleBar;
    ImGui::Begin(skCrypt("Spectator list"), nullptr, windowFlags);
    {        
        ImGui::PushFont(gui->getFIconsFont());
        auto draw_list = ImGui::GetBackgroundDrawList();
        auto p = ImGui::GetWindowPos();
        // set keybinds color
        auto bg_clr = ImColor(0.f, 0.f, 0.f, config->menu.transparency / 100);
        auto line_clr = Helpers::calculateColor(config->menu.accentColor);
        auto text_clr = ImColor(255, 255, 255, 255);
        auto glow = Helpers::calculateColor(config->menu.accentColor, 0.0f);
        auto glow1 = Helpers::calculateColor(config->menu.accentColor, config->menu.accentColor.color[3] * 0.5f);
        auto glow2 = Helpers::calculateColor(config->menu.accentColor);
        auto size = ImGui::CalcTextSize(skCrypt("Spectators"));
        auto offset = 2;
        // draw bg
        draw_list->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 150, p.y + size.y * 2 - 2), bg_clr, config->menu.windowStyle == 0 ? 5.f : 0.0f);
        // draw line
        if (config->menu.windowStyle == 0)
            draw_list->AddRect(ImVec2(p.x - 1, p.y - 1), ImVec2(p.x + 151, p.y + size.y * 2 - 1), line_clr, 5.f, 0, 1.5f);
        else if (config->menu.windowStyle == 1)
            draw_list->AddLine({ p.x, p.y - 1 }, { p.x + 150, p.y - 1 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 2)
            draw_list->AddLine({ p.x, p.y + size.y * 2 - 1 }, { p.x + 150, p.y + size.y * 2 - 3 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 3)
            draw_list->AddLine({ p.x, p.y - 1 }, { p.x + 150, p.y - 1 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 4)
            draw_list->AddLine({ p.x, p.y - 1 }, { p.x + 150, p.y - 1 }, line_clr, 2.f);
        // draw text
        draw_list->AddText(
            ImVec2(p.x + 75 - size.x / 2, p.y + size.y / 2 - 1),
            text_clr,
            c_xor("Spectators"));
        if (config->menu.windowStyle == 3)
            draw_list->AddRectFilledMultiColor({ p.x, p.y }, ImVec2(p.x + 150, p.y + size.y),
                glow1,
                glow1,
                glow,
                glow);
        else if (config->menu.windowStyle == 4)
        {
            draw_list->AddRectFilledMultiColor(ImVec2(p.x, p.y), ImVec2(p.x + 2, p.y + size.y * 2), glow2, glow2, glow, glow);
            draw_list->AddRectFilledMultiColor(ImVec2(p.x + 148, p.y), ImVec2(p.x + 150, p.y + size.y * 2), glow2, glow2, glow, glow);
        }
        for (const auto& observer : observers) {
            if (!observer.targetIsLocalPlayer)
                continue;

            if (const auto it = std::ranges::find(GameData::players(), observer.playerHandle, &PlayerData::handle); it != GameData::players().cend()) {
                    auto obsMode{ "" };
                    ImVec2 text;
                    if (it->observerMode == ObsMode::InEye)
                    {
                        obsMode = "1st";
                        text = ImGui::CalcTextSize("1st");
                    }                  
                    else if (it->observerMode == ObsMode::Chase)
                    {
                        obsMode = "3rd";
                        text = ImGui::CalcTextSize("3rd");
                    }
                    else if (it->observerMode == ObsMode::Roaming)
                    {
                        obsMode = "Freecam";
                        text = ImGui::CalcTextSize("Freecam");
                    }
                    //draw_list->AddText(ImVec2(p.x + 150.f - 4.5f - text.x, p.y + 15 * offset), ImColor(1.f, 1.f, 1.f, 1.f), obsMode);
                    draw_list->AddText(ImVec2(p.x + 4.5f, p.y + 13.5 * offset), ImColor(1.f, 1.f, 1.f, 1.f), it->name.c_str());
                    offset = offset + 1;
            }
        }
        ImGui::PopFont();
    }
    ImGui::End();
}

void Misc::noscopeCrosshair() noexcept
{
    static auto showSpread = interfaces->cvar->findVar(skCrypt("weapon_debug_spread_show"));
    showSpread->setValue(config->misc.noscopeCrosshair && localPlayer && !localPlayer->isScoped() ? 3 : 0);
}

void Misc::recoilCrosshair() noexcept
{
    static auto recoilCrosshair = interfaces->cvar->findVar(skCrypt("cl_crosshair_recoil"));
    recoilCrosshair->setValue(config->misc.recoilCrosshair ? 1 : 0);
}

bool offsetSpot{ false };

void Misc::watermark() noexcept
{
    if (!config->misc.wm.enabled)
    {
        return;
    }

    auto draw_list = ImGui::GetBackgroundDrawList();
    const char* name = interfaces->engine->getSteamAPIContext()->steamFriends->getPersonaName();
    std::string namey = name;
    static auto lastTime = 0.0f;
    const auto time = std::time(nullptr);
    static auto frameRate = 1.0f;
    frameRate = 0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime;

    float latency = 0.0f;
    if (auto networkChannel = interfaces->engine->getNetworkChannel(); networkChannel && networkChannel->getLatency(0) > 0.0f) {
        latency = networkChannel->getLatency(0);
    }
    std::time_t t = std::time(nullptr);
    auto localTime = std::localtime(&time);
    const auto [screenWidth, screenHeight] = interfaces->surface->getScreenSize();
    char s[11];
    s[0] = '\0';
    snprintf(s, sizeof(s), skCrypt("%02d:%02d:%02d"), localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
    std::string fuck = s;
    std::string fps{ std::to_string(static_cast<int>(1 / frameRate)) + " fps" };
    std::string ping{ interfaces->engine->isConnected() ? std::to_string(static_cast<int>(latency * 1000)) + " ms" : "Not connected" };
    const auto tick = static_cast<int>(1.f / memory->globalVars->intervalPerTick);
    std::string ticks{ interfaces->engine->isConnected() ? std::to_string(tick) : "" };
    std::ostringstream text; text << c_xor("Medusa.uno") << (config->misc.wm.showUsername ? " | " + namey : "") << (config->misc.wm.showTime ? " | " + fuck : "") << (config->misc.wm.showFps ? " | " + fps : "") << (config->misc.wm.showPing ? " | " + ping : "") << (interfaces->engine->isConnected() && config->misc.wm.showTicks ? " | " + std::to_string(tick) + " Ticks" : "");
    ImGui::Begin(c_xor("##WIM"), NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_::ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    {
        ImGui::PushFont(gui->getFIconsFont());
        // get window pos
        auto p = ImGui::GetWindowPos();

        // set watermark color
        auto bg_clr = ImColor(0.f, 0.f, 0.f, config->menu.transparency / 100);
        auto line_clr = Helpers::calculateColor(config->menu.accentColor);
        auto text_clr = ImColor(255, 255, 255, 255);
        auto glow = Helpers::calculateColor(config->menu.accentColor, 0.0f);
        auto glow1 = Helpers::calculateColor(config->menu.accentColor, config->menu.accentColor.color[3] * 0.5f);
        auto glow2 = Helpers::calculateColor(config->menu.accentColor);

        // draw bg
        auto calcText = ImGui::CalcTextSize(text.str().c_str());
        ImGui::SetWindowPos({ screenWidth - calcText.x - 19, 9 });
        draw_list->AddRectFilled(ImVec2(p.x - 4, p.y - 2), ImVec2(p.x + calcText.x + 14, p.y + calcText.y * 2 - 4), bg_clr, config->menu.windowStyle == 0 ? 5.f : 0.0f);
        // draw line
        if (config->menu.windowStyle == 0)
            draw_list->AddRect(ImVec2(p.x - 5, p.y - 3), ImVec2(p.x + calcText.x + 15, p.y + calcText.y * 2 - 3), line_clr, 5.f, 0, 1.5f);
        else if (config->menu.windowStyle == 1)
            draw_list->AddLine({ p.x - 4, p.y - 2 - 1 }, { p.x + calcText.x + 14, p.y - 2 - 1 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 2)
            draw_list->AddLine({ p.x - 4, p.y + calcText.y * 2 - 3 }, { p.x + calcText.x + 14, p.y + calcText.y * 2 - 5 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 3)
            draw_list->AddLine({ p.x - 4, p.y - 2 - 1 }, { p.x + calcText.x + 14, p.y - 2 - 1 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 4)
            draw_list->AddLine({ p.x - 4, p.y - 2 - 1 }, { p.x + calcText.x + 14, p.y - 2 - 1 }, line_clr, 2.f);
        if (config->menu.windowStyle == 3)
            draw_list->AddRectFilledMultiColor(ImVec2(p.x - 4, p.y - 2), ImVec2(p.x + calcText.x + 14, p.y + calcText.y - 2),
                glow1,
                glow1,
                glow,
                glow);
        else if (config->menu.windowStyle == 4)
        {
            draw_list->AddRectFilledMultiColor(ImVec2(p.x - 4, p.y - 2), ImVec2(p.x - 2, p.y + calcText.y * 2 - 4), glow2, glow2, glow, glow);
            draw_list->AddRectFilledMultiColor(ImVec2(p.x + calcText.x + 12, p.y - 2), ImVec2(p.x + calcText.x + 14, p.y + calcText.y * 2 - 4), glow2, glow2, glow, glow);
        }
        // draw text
        draw_list->AddText(
            ImVec2(p.x + 5, p.y + calcText.y / 2 - 3.f),
            text_clr,
            text.str().c_str()
        );
        offsetSpot = true;
        ImGui::PopFont();
    }
    ImGui::End();
}

void Misc::spotifyInd() noexcept
{
    if (!config->misc.wm.showSpotify)
        return;

    const auto [screenWidth, screenHeight] = interfaces->surface->getScreenSize();
    auto draw_list = ImGui::GetBackgroundDrawList();
    ImGui::Begin(c_xor("##SPT"), NULL, ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_::ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    {
        ImGui::PushFont(gui->getFIconsFont());
        // get window pos
        auto p = ImGui::GetWindowPos();

        // set watermark color
        auto bg_clr = ImColor(0.f, 0.f, 0.f, config->menu.transparency / 100);
        auto line_clr = Helpers::calculateColor(config->menu.accentColor);
        auto text_clr = ImColor(255, 255, 255, 255);
        auto glow = Helpers::calculateColor(config->menu.accentColor, 0.0f);
        auto glow1 = Helpers::calculateColor(config->menu.accentColor, config->menu.accentColor.color[3] * 0.5f);
        auto glow2 = Helpers::calculateColor(config->menu.accentColor);
        // draw bg
        auto calcText = ImGui::CalcTextSize(Misc::spotifytitle.c_str());
        ImGui::SetWindowPos({ screenWidth - calcText.x - 19.f, 9 + (config->misc.wm.enabled ? calcText.y * 2 + 5 : 0) });
        draw_list->AddRectFilled(ImVec2(p.x - 4, p.y - 2), ImVec2(p.x + calcText.x + 14, p.y + calcText.y * 2 - 4), bg_clr, config->menu.windowStyle == 0 ? 5.f : 0.0f);
        // draw line
        if (config->menu.windowStyle == 0)
            draw_list->AddRect(ImVec2(p.x - 5, p.y - 3), ImVec2(p.x + calcText.x + 15, p.y + calcText.y * 2 - 3), line_clr, 5.f, 0, 1.5f);
        else if (config->menu.windowStyle == 1)
            draw_list->AddLine({ p.x - 4, p.y - 2 - 1 }, { p.x + calcText.x + 14, p.y - 2 - 1 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 2)
            draw_list->AddLine({ p.x - 4, p.y + calcText.y * 2 - 3 }, { p.x + calcText.x + 14, p.y + calcText.y * 2 - 5 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 3)
            draw_list->AddLine({ p.x - 4, p.y - 2 - 1 }, { p.x + calcText.x + 14, p.y - 2 - 1 }, line_clr, 2.f);
        else if (config->menu.windowStyle == 4)
            draw_list->AddLine({ p.x - 4, p.y - 2 - 1 }, { p.x + calcText.x + 14, p.y - 2 - 1 }, line_clr, 2.f);
        if (config->menu.windowStyle == 3)
            draw_list->AddRectFilledMultiColor(ImVec2(p.x - 4, p.y - 2), ImVec2(p.x + calcText.x + 14, p.y + calcText.y - 2),
                glow1,
                glow1,
                glow,
                glow);
        else if (config->menu.windowStyle == 4)
        {
            draw_list->AddRectFilledMultiColor(ImVec2(p.x - 4, p.y - 2), ImVec2(p.x - 2, p.y + calcText.y * 2 - 4), glow2, glow2, glow, glow);
            draw_list->AddRectFilledMultiColor(ImVec2(p.x + calcText.x + 12, p.y - 2), ImVec2(p.x + calcText.x + 14, p.y + calcText.y * 2 - 4), glow2, glow2, glow, glow);
        }
        // draw text
        draw_list->AddText(
            ImVec2(p.x + 5, p.y + calcText.y / 2 - 3.f),
            text_clr,
            Misc::spotifytitle.c_str()
        );
        ImGui::PopFont();
    }
    ImGui::End();
}

void Misc::prepareRevolver(UserCmd* cmd) noexcept
{
    if (!config->misc.prepareRevolver || !config->misc.prepareRevolverKey.isActive())
        return;

    if (!localPlayer)
        return;

    if (cmd->buttons & UserCmd::IN_ATTACK)
        return;

    constexpr float revolverPrepareTime{ 0.234375f };

    if (auto activeWeapon = localPlayer->getActiveWeapon(); activeWeapon && activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver)
    {
        const auto time = memory->globalVars->serverTime();
        AntiAim::r8Working = true;
        if (localPlayer->nextAttack() > time)
            return;

        cmd->buttons &= ~UserCmd::IN_ATTACK2;

        static auto readyTime = time + revolverPrepareTime;
        if (activeWeapon->nextPrimaryAttack() <= time)
        {
            if (readyTime <= time)
            {
                if (activeWeapon->nextSecondaryAttack() <= time)
                    readyTime = time + revolverPrepareTime;
                else
                    cmd->buttons |= UserCmd::IN_ATTACK2;
            }
            else
                cmd->buttons |= UserCmd::IN_ATTACK;
        }
        else
            readyTime = time + revolverPrepareTime;
    }
}

void Misc::fastStop(UserCmd* cmd) noexcept
{
    if (!config->misc.fastStop)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER || !(localPlayer->flags() & 1) || cmd->buttons & UserCmd::IN_JUMP)
        return;

    if (cmd->buttons & (UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT | UserCmd::IN_FORWARD | UserCmd::IN_BACK))
        return;
    
    const auto velocity = localPlayer->velocity();
    const auto speed = velocity.length2D();
    if (speed < 15.0f)
        return;
    
    Vector direction = velocity.toAngle();
    direction.y = cmd->viewangles.y - direction.y;

    const auto negatedDirection = Vector::fromAngle(direction) * -speed;
    cmd->forwardmove = negatedDirection.x;
    cmd->sidemove = negatedDirection.y;
}

void Misc::drawBombTimer() noexcept
{
    if (!config->misc.bombTimer.enabled)
        return;

    GameData::Lock lock;

    const auto& plantedC4 = GameData::plantedC4();
    if (plantedC4.blowTime == 0.0f && !gui->isOpen())
        return;

    if (!gui->isOpen()) {
        ImGui::SetNextWindowBgAlpha(0.5f);
    }
    static float windowWidth = 200.0f;
    ImGui::SetNextWindowPos({ (ImGui::GetIO().DisplaySize.x - 170.0f) / 2.0f, 60.0f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ 150, 0 }, ImGuiCond_Once);

    if (!gui->isOpen())
        ImGui::SetNextWindowSize({ 170, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, { 255, 255, 255, 255 });
    if (config->misc.borders)
        ImGui::PushStyleColor(ImGuiCol_Border, { config->menu.accentColor.color[0], config->menu.accentColor.color[1], config->menu.accentColor.color[2], config->menu.accentColor.color[3] });
    ImGui::SetNextWindowSizeConstraints({ 0, -1 }, { FLT_MAX, -1 });
    ImGui::Begin(skCrypt("Bomb Timer"), nullptr, ImGuiWindowFlags_NoTitleBar | (gui->isOpen() ? 0 : ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize));
    PostProcessing::performFullscreenBlur(ImGui::GetWindowDrawList(), 1.f);
    ImDrawList* draw;
    ImVec2 pos;
    pos = ImGui::GetWindowPos();
    draw = ImGui::GetWindowDrawList();
    if (!plantedC4.bombsite)
        Misc::bombSiteCeva = "A";
    else
        Misc::bombSiteCeva = "B";
    draw->AddText(gui->getTahoma28Font(), 28.f, ImVec2(pos.x + 10, pos.y + 3), ImColor(255, 255, 255), !plantedC4.bombsite ? "A" : "B");
    std::ostringstream ss; ss << skCrypt("Time : ") << (std::max)(static_cast<int>(plantedC4.blowTime - memory->globalVars->currenttime), static_cast<int>(0)) << " s";
    //draw->AddRect()
    ImGui::textUnformattedCentered(ss.str().c_str());
    if (plantedC4.defuserHandle != -1) {
        const bool canDefuse = plantedC4.blowTime >= plantedC4.defuseCountDown;

        if (plantedC4.defuserHandle == GameData::local().handle) {
            if (canDefuse) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                ImGui::textUnformattedCentered(skCrypt("Defusabale"));
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                ImGui::textUnformattedCentered(skCrypt("RUN!!!!"));
            }
            ImGui::PopStyleColor();
        }
        else if (const auto defusingPlayer = GameData::playerByHandle(plantedC4.defuserHandle)) {
            std::ostringstream ss; ss <<  skCrypt(" Someone is defusing: ") << std::fixed << std::showpoint << std::setprecision(3) << (std::max)(plantedC4.defuseCountDown - memory->globalVars->currenttime, static_cast<float>(0)) << " s";

            ImGui::textUnformattedCentered(ss.str().c_str());

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, canDefuse ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
            ImGui::progressBarFullWidth((plantedC4.defuseCountDown - memory->globalVars->currenttime) / plantedC4.defuseLength, 5.0f);
            ImGui::PopStyleColor();
        }
    }

    windowWidth = ImGui::GetCurrentWindow()->SizeFull.x;
    if (config->misc.borders)
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::End();
}

void Misc::hurtIndicator() noexcept
{
    if (!config->misc.hurtIndicator.enabled)
        return;

    GameData::Lock lock;
    const auto& local = GameData::local();
    if ((!local.exists || !local.alive) && !gui->isOpen())
        return;

    if (local.velocityModifier >= 0.99f && !gui->isOpen())
        return;

    if (gui->isOpen()) {
        ImGui::SetNextWindowBgAlpha(0.3f);
    }
    else
        ImGui::SetNextWindowBgAlpha(0.0f);
    static float windowWidth = 140.0f;
    ImGui::SetNextWindowPos({ (ImGui::GetIO().DisplaySize.x - 140.0f) / 2.0f, 260.0f }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ windowWidth, 0 }, ImGuiCond_Once);

    if (!gui->isOpen())
        ImGui::SetNextWindowSize({ windowWidth, 0 });

    ImGui::SetNextWindowSizeConstraints({ 0, -1 }, { FLT_MAX, -1 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.5f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{ 0.0f, 0.0f, 0.0f, 1.0f });
    //if (config->misc.borders)
    //ImGui::PushStyleColor(ImGuiCol_Border, { config->menu.accentColor.color[0], config->menu.accentColor.color[1], config->menu.accentColor.color[2], config->menu.accentColor.color[3] });
    ImGui::Begin(skCrypt("Hurt Indicator"), nullptr, ImGuiWindowFlags_NoTitleBar | (gui->isOpen() ? 0 : ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration));

    std::ostringstream ss; ss << skCrypt("Slowed down ") << static_cast<int>(local.velocityModifier * 100.f) << "%";
    ImGui::textUnformattedCentered(ss.str().c_str());


    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Helpers::calculateColor(config->misc.hurtIndicator));

    
    ImGui::progressBarFullWidth(local.velocityModifier, 1.0f);

    windowWidth = ImGui::GetCurrentWindow()->SizeFull.x;

    if (config->misc.borders)
    ImGui::PopStyleColor();
    //ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
    ImGui::End();
}

void Misc::stealNames() noexcept
{
    if (!config->misc.nameStealer)
        return;

    if (!localPlayer)
        return;

    static std::vector<int> stolenIds;

    for (int i = 1; i <= memory->globalVars->maxClients; ++i) {
        const auto entity = interfaces->entityList->getEntity(i);

        if (!entity || entity == localPlayer.get())
            continue;

        PlayerInfo playerInfo;
        if (!interfaces->engine->getPlayerInfo(entity->index(), playerInfo))
            continue;

        if (playerInfo.fakeplayer || std::find(stolenIds.cbegin(), stolenIds.cend(), playerInfo.userId) != stolenIds.cend())
            continue;

        if (changeName(false, (std::string{ playerInfo.name } +'\x1').c_str(), 1.0f))
            stolenIds.push_back(playerInfo.userId);

        return;
    }
    stolenIds.clear();
}

void Misc::disablePanoramablur() noexcept
{
    static auto blur = interfaces->cvar->findVar(skCrypt("@panorama_disable_blur"));
    blur->setValue(config->misc.disablePanoramablur);
}

bool Misc::changeName(bool reconnect, const char* newName, float delay) noexcept
{
    static auto exploitInitialized{ false };

    static auto name{ interfaces->cvar->findVar("name") };

    if (reconnect) {
        exploitInitialized = false;
        return false;
    }

    if (!exploitInitialized && interfaces->engine->isInGame()) {
        if (PlayerInfo playerInfo; localPlayer && interfaces->engine->getPlayerInfo(localPlayer->index(), playerInfo) && (!strcmp(playerInfo.name, "?empty") || !strcmp(playerInfo.name, "\n\xAD\xAD\xAD"))) {
            exploitInitialized = true;
        } else {
            name->onChangeCallbacks.size = 0;
            name->setValue("\n\xAD\xAD\xAD");
            return false;
        }
    }

    static auto nextChangeTime{ 0.0f };
    if (nextChangeTime <= memory->globalVars->realtime) {
        name->setValue(newName);
        nextChangeTime = memory->globalVars->realtime + delay;
        return true;
    }
    return false;
}

void Misc::bunnyHop(UserCmd* cmd) noexcept
{
    if (!localPlayer)
        return;

    static bool hasLanded = true;
    static int bhopInSeries = 1;
    static float lastTimeInAir{};
    static int chanceToHit = config->misc.bhHc;

    if (config->misc.jumpBug && config->misc.jumpBugKey.isActive())
        return;

    static auto wasLastTimeOnGround{ localPlayer->flags() & 1 };

    chanceToHit = config->misc.bhHc;

    if (bhopInSeries <= 1) {
        chanceToHit = chanceToHit * 1.5;
    }
    if (config->misc.bunnyHop && !(localPlayer->flags() & 1) && localPlayer->moveType() != MoveType::LADDER && !wasLastTimeOnGround)
        if (rand() % 100 <= chanceToHit) {
            cmd->buttons &= ~UserCmd::IN_JUMP;
        }
    //memory->globalVars->realtime - lastTimeInAir <= 2 &&
    if (!wasLastTimeOnGround && hasLanded) {
        bhopInSeries++;
        lastTimeInAir = memory->globalVars->realtime;
        hasLanded = false;
    }
    if (wasLastTimeOnGround) {
        hasLanded = true;
        if (memory->globalVars->realtime - lastTimeInAir >= 3) {
            bhopInSeries = 0;
        }
    }
    wasLastTimeOnGround = localPlayer->flags() & 1;
}

void Misc::fixTabletSignal() noexcept
{
    if (config->misc.fixTabletSignal && localPlayer) {
        if (auto activeWeapon{ localPlayer->getActiveWeapon() }; activeWeapon && activeWeapon->getClientClass()->classId == ClassId::Tablet)
            activeWeapon->tabletReceptionIsBlocked() = false;
    }
}

void Misc::killfeedChanger(GameEvent& event) noexcept
{
    if (!config->misc.killfeedChanger.enabled)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt(skCrypt("attacker")) != localUserId || event.getInt(skCrypt("userid")) == localUserId)
        return;

    if (config->misc.killfeedChanger.headshot)
        event.setInt("headshot", 1);

    if (config->misc.killfeedChanger.dominated)
        event.setInt("Dominated", 1);

    if (config->misc.killfeedChanger.revenge)
        event.setInt("Revenge", 1);

    if (config->misc.killfeedChanger.penetrated)
        event.setInt("penetrated", 1);

    if (config->misc.killfeedChanger.noscope)
        event.setInt("noscope", 1);

    if (config->misc.killfeedChanger.thrusmoke)
        event.setInt("thrusmoke", 1);

    if (config->misc.killfeedChanger.attackerblind)
        event.setInt("attackerblind", 1);
}

void Misc::killMessage(GameEvent& event) noexcept
{
    if (!config->misc.killMessage)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt(skCrypt("attacker")) != localUserId || event.getInt(skCrypt("userid")) == localUserId)
        return;

    srand(time(0));
    auto randomMessage = rand() % 9;
    std::string killMessage = "";

    switch (randomMessage)
    {
    case 0:
        killMessage = skCrypt("OwO, what is this? Medusa.uno just made your kd worse");
        break;
    case 1:
        killMessage = skCrypt("YOUR DEAD, IMAGINE, GET 1'ED BY MEDUSA.UNO");
        break;
    case 2:
        killMessage = skCrypt("Get good, sting to death with Medusa.uno");
        break;
    case 3:
        killMessage = skCrypt("I just made you my little furry bitch with Medusa.uno >w<");
        break;
    case 4:
        killMessage = skCrypt("Go deeper daddy OwO or u can't cause all i see is a 1 on my screen");
        break;
    case 5:
        killMessage = skCrypt("UwU *stings you to death cutely* using Medusa.uno");
        break;
    case 6:
        killMessage = skCrypt("Medusa.uno just stinged ur ass $$$$$");
        break;
    case 7:
        killMessage = skCrypt("Just saw something, a 1 in my killfeed caused by Medusa.uno");
        break;
    case 8:
        killMessage = skCrypt("Ugh~...don't stop Master! I know...-huff-...how much you wanted to die to Medusa.uno!");
        break;
    }

    std::string cmd = c_xor("say \"");
    cmd += killMessage;
    cmd += '"';
    interfaces->engine->clientCmdUnrestricted(cmd.c_str());
}

void Misc::fixMovement(UserCmd* cmd, float yaw) noexcept
{
    float oldYaw = yaw + (yaw < 0.0f ? 360.0f : 0.0f);
    float newYaw = cmd->viewangles.y + (cmd->viewangles.y < 0.0f ? 360.0f : 0.0f);
    float yawDelta = newYaw < oldYaw ? fabsf(newYaw - oldYaw) : 360.0f - fabsf(newYaw - oldYaw);
    yawDelta = 360.0f - yawDelta;

    const float forwardmove = cmd->forwardmove;
    const float sidemove = cmd->sidemove;
    cmd->forwardmove = std::cos(Helpers::deg2rad(yawDelta)) * forwardmove + std::cos(Helpers::deg2rad(yawDelta + 90.0f)) * sidemove;
    cmd->sidemove = std::sin(Helpers::deg2rad(yawDelta)) * forwardmove + std::sin(Helpers::deg2rad(yawDelta + 90.0f)) * sidemove;
    if (localPlayer->moveType() != MoveType::LADDER && (config->misc.moonwalk_style == 0 || config->misc.moonwalk_style == 4))
        cmd->buttons &= ~(UserCmd::IN_FORWARD | UserCmd::IN_BACK | UserCmd::IN_MOVERIGHT | UserCmd::IN_MOVELEFT);
}

void Misc::antiAfkKick(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;
    if (localPlayer->velocity().length2D() >= 5.f)
        return;
    if (config->misc.antiAfkKick && cmd->commandNumber % 2)
        cmd->buttons |= 1 << 27;
}

void Misc::fixAnimationLOD(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START)
        return;

    if (!localPlayer)
        return;

    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
        Entity* entity = interfaces->entityList->getEntity(i);
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
            continue;
        *reinterpret_cast<int*>(entity + 0xA28) = 0;
        *reinterpret_cast<int*>(entity + 0xA30) = memory->globalVars->framecount;
    }
}

void Misc::autoPistol(UserCmd* cmd) noexcept
{
    if (config->misc.autoPistol && localPlayer) {
        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (activeWeapon && activeWeapon->isPistol() && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime()) {
            if (activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver)
                cmd->buttons &= ~UserCmd::IN_ATTACK2;
            else
                cmd->buttons &= ~UserCmd::IN_ATTACK;
        }
    }
}

void Misc::autoReload(UserCmd* cmd) noexcept
{
    if (config->misc.autoReload && localPlayer) {
        const auto activeWeapon = localPlayer->getActiveWeapon();
        if (activeWeapon && getWeaponIndex(activeWeapon->itemDefinitionIndex2()) && !activeWeapon->clip())
            cmd->buttons &= ~(UserCmd::IN_ATTACK | UserCmd::IN_ATTACK2);
    }
}

void Misc::revealRanks(UserCmd* cmd) noexcept
{
    if (config->misc.revealRanks && cmd->buttons & UserCmd::IN_SCORE)
        interfaces->client->dispatchUserMessage(50, 0, 0, nullptr);
}

void RotateMovement(UserCmd* cmd, float yaw)
{
    Vector viewangles = interfaces->engine->getViewAngles();

    float rotation = Helpers::rad2deg(viewangles.y - yaw);

    float cos_rot = cos(rotation);
    float sin_rot = sin(rotation);

    float new_forwardmove = cos_rot * cmd->forwardmove - sin_rot * cmd->sidemove;
    float new_sidemove = sin_rot * cmd->forwardmove + cos_rot * cmd->sidemove;

    cmd->forwardmove = new_forwardmove;
    cmd->sidemove = new_sidemove;
}

void Misc::autoStrafe(UserCmd* cmd, Vector& currentViewAngles) noexcept
{
    if (!config->misc.autoStrafe)
        return;
    
    if (!localPlayer || !localPlayer->isAlive())
        return;

    if ((EnginePrediction::getFlags() & 1) || localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER)
        return;

    {
        const float speed = localPlayer->velocity().length2D();
        if (speed < 5.0f)
            return;

        static float angle = 0.f;

        const bool back = cmd->buttons & UserCmd::IN_BACK;
        const bool forward = cmd->buttons & UserCmd::IN_FORWARD;
        const bool right = cmd->buttons & UserCmd::IN_MOVERIGHT;
        const bool left = cmd->buttons & UserCmd::IN_MOVELEFT;

        if (back) {
            angle = -180.f;
            if (left)
                angle -= 45.f;
            else if (right)
                angle += 45.f;
        }
        else if (left) {
            angle = 90.f;
            if (back)
                angle += 45.f;
            else if (forward)
                angle -= 45.f;
        }
        else if (right) {
            angle = -90.f;
            if (back)
                angle -= 45.f;
            else if (forward)
                angle += 45.f;
        }
        else {
            angle = 0.f;
        }

        //If we are on ground, noclip or in a ladder return
        if ((EnginePrediction::getFlags() & 1) || localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER)
            return;

        currentViewAngles.y += angle;

        cmd->forwardmove = 0.f;
        cmd->sidemove = 0.f;
        float smooth = (1.f - (0.15f * (1.f - config->misc.auto_smoothnes * 0.01f)));
        const auto delta = Helpers::normalizeYaw(currentViewAngles.y - Helpers::rad2deg(std::atan2(EnginePrediction::getVelocity().y, EnginePrediction::getVelocity().x)));

        cmd->sidemove = delta > 0.f ? -450.f : 450.f;

        currentViewAngles.y = Helpers::normalizeYaw(currentViewAngles.y - delta * smooth);
    }
}

void Misc::partyMode() noexcept
{
    static auto partyVar = interfaces->cvar->findVar(c_xor("sv_party_mode"));
    if (config->visuals.partyMode)
        partyVar->setValue(1);
    else
        partyVar->setValue(0);
}

void Misc::removeCrouchCooldown(UserCmd* cmd) noexcept
{
    if (const auto gameRules = (*memory->gameRules); gameRules)
        if (gameRules->isValveDS())
            return;

    if (config->misc.fastDuck)
        cmd->buttons |= UserCmd::IN_BULLRUSH;
}

int RandomInt_new(int min, int max) noexcept
{
    return (min + 1) + (((int)rand()) / (int)RAND_MAX) * (max - (min + 1));
}

void Misc::moonwalk(UserCmd* cmd, bool& sendPacket) noexcept
{
    const auto netChannel = interfaces->engine->getNetworkChannel();
    if (!netChannel)
        return;
    if (config->misc.moonwalk_style > 0 && config->misc.moonwalk_style != 4 && localPlayer && localPlayer->moveType() != MoveType::LADDER)
        if (!sendPacket)
            cmd->buttons ^= UserCmd::IN_FORWARD | UserCmd::IN_BACK | UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT;
}

void Misc::playHitSound(GameEvent& event) noexcept
{
    if (!config->misc.hitSound)
        return;

    if (!localPlayer)
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt(c_xor("attacker")) != localUserId || event.getInt(c_xor("userid")) == localUserId)
        return;

    if (config->misc.hitSound == 0)
        return;
    else if (config->misc.hitSound == 1)
        interfaces->engine->clientCmdUnrestricted(c_xor("play survival/paradrop_idle_01.wav"));
    else if (config->misc.hitSound == 2)
        interfaces->engine->clientCmdUnrestricted(c_xor("play physics/metal/metal_solid_impact_bullet2"));
    else if (config->misc.hitSound == 3)
        interfaces->engine->clientCmdUnrestricted(c_xor("play buttons/arena_switch_press_02"));
    else if (config->misc.hitSound == 4)
        interfaces->engine->clientCmdUnrestricted(c_xor("play training/timer_bell"));
    else if (config->misc.hitSound == 5)
        interfaces->engine->clientCmdUnrestricted(c_xor("play physics/glass/glass_impact_bullet1"));
    else if (config->misc.hitSound == 6)
        interfaces->engine->clientCmdUnrestricted(c_xor("play survival/money_collect_04"));
    else if (config->misc.hitSound == 7)
        interfaces->engine->clientCmdUnrestricted(("play " + config->misc.customHitSound).c_str());
}

void Misc::killSound(GameEvent& event) noexcept
{
    if (!config->misc.killSound)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (const auto localUserId = localPlayer->getUserId(); event.getInt(c_xor("attacker")) != localUserId || event.getInt(c_xor("userid")) == localUserId)
        return;

    if (config->misc.killSound == 0)
        return;
    else if (config->misc.killSound == 1)
        interfaces->engine->clientCmdUnrestricted(c_xor("play survival/paradrop_idle_01.wav"));
    else if (config->misc.killSound == 2)
        interfaces->engine->clientCmdUnrestricted(c_xor("play physics/metal/metal_solid_impact_bullet2"));
    else if (config->misc.killSound == 3)
        interfaces->engine->clientCmdUnrestricted(c_xor("play buttons/arena_switch_press_02"));
    else if (config->misc.killSound == 4)
        interfaces->engine->clientCmdUnrestricted(c_xor("play training/timer_bell"));
    else if (config->misc.killSound == 5)
        interfaces->engine->clientCmdUnrestricted(c_xor("play physics/glass/glass_impact_bullet1"));
    else if (config->misc.hitSound == 6)
        interfaces->engine->clientCmdUnrestricted(c_xor("play survival/money_collect_04"));
    else if (config->misc.killSound == 7)
        interfaces->engine->clientCmdUnrestricted(("play " + config->misc.customKillSound).c_str());
}

void Misc::autoBuy(GameEvent* event) noexcept
{
    static const std::array<std::string, 17> primary = {
    "",
    "mac10;buy mp9;",
    "mp7;",
    "ump45;",
    "p90;",
    "bizon;",
    "galilar;buy famas;",
    "ak47;buy m4a1;",
    "ssg08;",
    "sg556;buy aug;",
    "awp;",
    "g3sg1; buy scar20;",
    "nova;",
    "xm1014;",
    "sawedoff;buy mag7;",
    "m249; ",
    "negev;"
    };
    static const std::array<std::string, 6> secondary = {
        "",
        "glock;buy hkp2000;",
        "elite;",
        "p250;",
        "tec9;buy fiveseven;",
        "deagle;buy revolver;"
    };
    static const std::array<std::string, 3> armor = {
        "",
        "vest;",
        "vesthelm;",
    };
    static const std::array<std::string, 2> utility = {
        "defuser;",
        "taser;"
    };
    static const std::array<std::string, 5> nades = {
        "hegrenade;",
        "smokegrenade;",
        "molotov;buy incgrenade;",
        "flashbang;buy flashbang;",
        "decoy;"
    };

    if (!config->misc.autoBuy.enabled)
        return;

    std::string cmd = "";

    if (event) 
    {
        if (config->misc.autoBuy.primaryWeapon)
            cmd += "buy " + primary[config->misc.autoBuy.primaryWeapon];
        if (config->misc.autoBuy.secondaryWeapon)
            cmd += "buy " + secondary[config->misc.autoBuy.secondaryWeapon];
        if (config->misc.autoBuy.armor)
            cmd += "buy " + armor[config->misc.autoBuy.armor];

        for (size_t i = 0; i < nades.size(); i++)
        {
            if ((config->misc.autoBuy.grenades & 1 << i) == 1 << i)
                cmd += "buy " + nades[i];
        }

        for (size_t i = 0; i < utility.size(); i++)
        {
            if ((config->misc.autoBuy.utility & 1 << i) == 1 << i)
                cmd += "buy " + utility[i];
        }

        interfaces->engine->clientCmdUnrestricted(cmd.c_str());
    }
}
static std::vector<std::uint64_t> reportedPlayers;
static int reportbotRound;

void Misc::runReportbot() noexcept
{
    if (!config->misc.reportbot.enabled)
        return;

    if (!localPlayer)
        return;

    static auto lastReportTime = 0.0f;

    if (lastReportTime + config->misc.reportbot.delay > memory->globalVars->realtime)
        return;

    if (reportbotRound >= config->misc.reportbot.rounds)
        return;

    for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
        const auto entity = interfaces->entityList->getEntity(i);

        if (!entity || entity == localPlayer.get())
            continue;

        if (config->misc.reportbot.target != 2 && (entity->isOtherEnemy(localPlayer.get()) ? config->misc.reportbot.target != 0 : config->misc.reportbot.target != 1))
            continue;

        PlayerInfo playerInfo;
        if (!interfaces->engine->getPlayerInfo(i, playerInfo))
            continue;

        if (playerInfo.fakeplayer || std::find(reportedPlayers.cbegin(), reportedPlayers.cend(), playerInfo.xuid) != reportedPlayers.cend())
            continue;

        std::string report;

        if (config->misc.reportbot.textAbuse)
            report += "textabuse,";
        if (config->misc.reportbot.griefing)
            report += "grief,";
        if (config->misc.reportbot.wallhack)
            report += "wallhack,";
        if (config->misc.reportbot.aimbot)
            report += "aimbot,";
        if (config->misc.reportbot.other)
            report += "speedhack,";

        if (!report.empty()) {
            memory->submitReport(std::to_string(playerInfo.xuid).c_str(), report.c_str());
            lastReportTime = memory->globalVars->realtime;
            reportedPlayers.push_back(playerInfo.xuid);
        }
        return;
    }

    reportedPlayers.clear();
    ++reportbotRound;
}

void Misc::resetReportbot() noexcept
{
    reportbotRound = 0;
    reportedPlayers.clear();
}

void Misc::preserveKillfeed(bool roundStart) noexcept
{
    if (!config->misc.preserveKillfeed.enabled)
        return;

    static auto nextUpdate = 0.0f;

    if (roundStart) {
        nextUpdate = memory->globalVars->realtime + 10.0f;
        return;
    }

    if (nextUpdate > memory->globalVars->realtime)
        return;

    nextUpdate = memory->globalVars->realtime + 2.0f;

    const auto deathNotice = std::uintptr_t(memory->findHudElement(memory->hud, skCrypt("CCSGO_HudDeathNotice")));
    if (!deathNotice)
        return;

    const auto deathNoticePanel = (*(UIPanel**)(*reinterpret_cast<std::uintptr_t*>(deathNotice - 20 + 88) + sizeof(std::uintptr_t)));

    const auto childPanelCount = deathNoticePanel->getChildCount();

    for (int i = 0; i < childPanelCount; ++i) {
        const auto child = deathNoticePanel->getChild(i);
        if (!child)
            continue;

        if (child->hasClass(skCrypt("DeathNotice_Killer")) && (!config->misc.preserveKillfeed.onlyHeadshots || child->hasClass(skCrypt("DeathNoticeHeadShot"))))
            child->setAttributeFloat(skCrypt("SpawnTime"), memory->globalVars->currenttime);
    }
}

void Misc::voteRevealer(GameEvent& event) noexcept
{
    if (!config->misc.revealVotes)
        return;

    const auto entity = interfaces->entityList->getEntity(event.getInt(skCrypt("entityid")));
    if (!entity)
        return;

    if (!entity->isPlayer())
        return;

    const auto votedYes = event.getInt("vote_option");
    const auto isLocal = localPlayer && entity == localPlayer.get();
    const char color = !votedYes ? '\x06' : '\x07';
    if (!memory->clientMode->getHudChat())
        return;
    memory->clientMode->getHudChat()->printf(0, skCrypt(" Medusa.uno | %s voted %c%s"), isLocal ? localPlayer.get()->getPlayerName().c_str() : entity->getPlayerName().c_str(), color, !votedYes ? "Yes" : "No");
}

#include "../SDK/ProtobufReader.h"
void Misc::onVoteStart(const void* data, int size) noexcept
{
    if (!config->misc.revealVotes)
        return;

    constexpr auto voteName = [](int index) {
        switch (index) {
        case 0: return "Kick";
        case 1: return "Change Level";
        case 6: return "Surrender";
        case 13: return "Start TimeOut";
        default: return "";
        }
    };

    const auto reader = ProtobufReader{ static_cast<const std::uint8_t*>(data), size };
    const auto entityIndex = reader.readInt32(2);

    const auto entity = interfaces->entityList->getEntity(entityIndex);
    if (!entity || !entity->isPlayer())
        return;

    const auto isLocal = localPlayer && entity == localPlayer.get();

    const auto voteType = reader.readInt32(3);
    std::string cock = c_xor("Medusa.uno | ") + entity->getPlayerName() + c_xor(" called a vote to ") + voteName(voteType);
    memory->clientMode->getHudChat()->printf(0, cock.c_str());

    //memory->clientMode->getHudChat()->printf(0, " \x0C\u2022Osiris\u2022 %c%s\x01 call vote (\x06%s\x01)", isLocal ? '\x01' : '\x06', isLocal ? "You" : entity->getPlayerName().c_str(), voteName(voteType));
}

void Misc::onVotePass() noexcept
{
    if (!config->misc.revealVotes)
        return;

    memory->clientMode->getHudChat()->printf(0, skCrypt("Medusa.uno | Vote passed"));

    //memory->clientMode->getHudChat()->printf(0, " \x0C\u2022Osiris\u2022\x01 Vote\x06 PASSED");
}

void Misc::onVoteFailed() noexcept
{
    if (!config->misc.revealVotes)
        return;

    memory->clientMode->getHudChat()->printf(0, skCrypt("Medusa.uno | Vote failed"));

    //memory->clientMode->getHudChat()->printf(0, " \x0C\u2022Osiris\u2022\x01 Vote\x07 FAILED");
}

void AddRadialGradient1(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col_in, ImU32 col_out)
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

void Misc::drawOffscreenEnemies(ImDrawList* drawList) noexcept
{
    if (!config->misc.offscreenEnemies.enabled && !config->misc.offscreenAllies.enabled)
        return;

    const auto yaw = Helpers::deg2rad(interfaces->engine->getViewAngles().y);

    GameData::Lock lock;
    for (auto& player : GameData::players()) {
        if (player.dormant || !player.alive || player.inViewFrustum)
            continue;

        Config::Misc::Offscreen pCfg = player.enemy ? config->misc.offscreenEnemies : config->misc.offscreenAllies;
        if (!pCfg.enabled)
            continue;

        const auto positionDiff = GameData::local().origin - player.origin;

        auto x = std::cos(yaw) * positionDiff.y - std::sin(yaw) * positionDiff.x;
        auto y = std::cos(yaw) * positionDiff.x + std::sin(yaw) * positionDiff.y;
        if (const auto len = std::sqrt(x * x + y * y); len != 0.0f) {
            x /= len;
            y /= len;
        }
        const auto& displaySize = ImGui::GetIO().DisplaySize;
        const auto pos = ImGui::GetIO().DisplaySize / 2 + ImVec2{ x, y } * pCfg.offset;
        const auto trianglePos = pos + ImVec2{ x, y };
        const auto color = Helpers::calculateColor(pCfg);

        const ImVec2 trianglePoints[]{
            trianglePos + ImVec2{  0.5f * y, -0.5f * x } * pCfg.size,
            trianglePos + ImVec2{  1.f * x,  1.f * y } * pCfg.size,
            trianglePos + ImVec2{ -0.5f * y,  0.5f * x } * pCfg.size,
        };

        if (pCfg.type == 0)
        {
            drawList->AddConvexPolyFilled(trianglePoints, 3, color);
            if (pCfg.outline)
                drawList->AddPolyline(trianglePoints, 3, color | IM_COL32_A_MASK, ImDrawFlags_Closed, 1.5f);
        }
        else if (pCfg.type == 1)
        {
            AddRadialGradient1(drawList, pos, pCfg.size, color, Helpers::calculateColor(pCfg, 0.0f));
        }
        else if (pCfg.type == 2)
        {
            auto angle = interfaces->engine->getViewAngles().y - Helpers::calculate_angle(GameData::local().origin, player.origin).y - 90;
            //thick
            drawList->PathArcTo(displaySize / 2, pCfg.offset + 6, Helpers::deg2rad(angle - pCfg.size / 2), Helpers::deg2rad(angle + pCfg.size / 2), 32);
            drawList->PathStroke(color, false, 4.5f);
            drawList->PathArcTo(displaySize / 2, pCfg.offset, Helpers::deg2rad(angle - pCfg.size / 2), Helpers::deg2rad(angle + pCfg.size / 2), 32);
            drawList->PathStroke(color, false, 1.f);
        }
    }
}


void Misc::autoAccept(const char* soundEntry) noexcept
{
    if (!config->misc.autoAccept)
        return;

    if (std::strcmp(soundEntry, skCrypt("UIPanorama.popup_accept_match_beep")))
        return;

    if (const auto idx = memory->registeredPanoramaEvents->find(memory->makePanoramaSymbol(skCrypt("MatchAssistedAccept"))); idx != -1) {
        if (const auto eventPtr = memory->registeredPanoramaEvents->memory[idx].value.makeEvent(nullptr))
            interfaces->panoramaUIEngine->accessUIEngine()->dispatchEvent(eventPtr);
    }

    auto window = FindWindowW(L"Valve001", NULL);
    FLASHWINFO flash{ sizeof(FLASHWINFO), window, FLASHW_TRAY | FLASHW_TIMERNOFG, 0, 0 };
    FlashWindowEx(&flash);
    ShowWindow(window, SW_RESTORE);
}

void Misc::updateInput() noexcept
{
    config->misc.blockBotKey.handleToggle();
    config->misc.edgeJumpKey.handleToggle();
    config->misc.miniJumpKey.handleToggle();
    config->misc.jumpBugKey.handleToggle();
    config->misc.edgeBugKey.handleToggle();
    config->misc.autoPixelSurfKey.handleToggle();
    config->misc.slowwalkKey.handleToggle();
    config->misc.fakeduckKey.handleToggle();
    config->misc.autoPeekKey.handleToggle();
    config->misc.prepareRevolverKey.handleToggle();
    config->misc.doorSpamKey.handleToggle();
}

void Misc::reset(int resetType) noexcept
{
    if (resetType == 1)
    {
        static auto ragdollGravity = interfaces->cvar->findVar(skCrypt("cl_ragdoll_gravity"));
        static auto blur = interfaces->cvar->findVar(skCrypt("@panorama_disable_blur"));
        ragdollGravity->setValue(600);
        blur->setValue(0);
    }
    jumpStatsCalculations = JumpStatsCalculations{ };
}