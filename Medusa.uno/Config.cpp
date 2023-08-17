#include <fstream>
#include <Windows.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <filesystem>
#include "nlohmann/json.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "Config.h"
#include "Helpers.h"

#include "Hacks/AntiAim.h"
#include "Hacks/Backtrack.h"
#include "Hacks/Glow.h"
#include "xor.h"
#include "SDK/Platform.h"

int CALLBACK fontCallback(const LOGFONTW* lpelfe, const TEXTMETRICW*, DWORD, LPARAM lParam)
{
    const wchar_t* const fontName = reinterpret_cast<const ENUMLOGFONTEXW*>(lpelfe)->elfFullName;

    if (fontName[0] == L'@')
        return TRUE;

    if (HFONT font = CreateFontW(0, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, fontName)) {

        DWORD fontData = GDI_ERROR;

        if (HDC hdc = CreateCompatibleDC(nullptr)) {
            SelectObject(hdc, font);
            // Do not use TTC fonts as we only support TTF fonts
            fontData = GetFontData(hdc, 'fctt', 0, NULL, 0);
            DeleteDC(hdc);
        }
        DeleteObject(font);

        if (fontData == GDI_ERROR) {
            if (char buff[1024]; WideCharToMultiByte(CP_UTF8, 0, fontName, -1, buff, sizeof(buff), nullptr, nullptr))
                reinterpret_cast<std::vector<std::string>*>(lParam)->emplace_back(buff);
        }
    }
    return TRUE;
}

Config::Config() noexcept
{
    CreateDirectoryA(skCrypt("C:\\Medusa.uno\\"), NULL);
    path /= std::string(skCrypt("C:\\Medusa.uno\\Configs"));
    listConfigs();
    misc.clanTag[0] = '\0';
    misc.name[0] = '\0';
    visuals.playerModel[0] = '\0';

    load(skCrypt(u8"default.json"), false);

    LOGFONTW logfont;
    logfont.lfCharSet = ANSI_CHARSET;
    logfont.lfPitchAndFamily = DEFAULT_PITCH;
    logfont.lfFaceName[0] = L'\0';

    EnumFontFamiliesExW(GetDC(nullptr), &logfont, fontCallback, (LPARAM)&systemFonts, 0);

    std::sort(std::next(systemFonts.begin()), systemFonts.end());
}

#pragma region  Read

static void from_json(const json& j, ColorToggle& ct)
{
    from_json(j, static_cast<Color4&>(ct));
    read(j, "Enabled", ct.enabled);
}

static void from_json(const json& j, Color3& c)
{
    read(j, "Color", c.color);
    read(j, "Rainbow", c.rainbow);
    read(j, "Rainbow Speed", c.rainbowSpeed);
}

static void from_json(const json& j, ColorToggle3& ct)
{
    from_json(j, static_cast<Color3&>(ct));
    read(j, "Enabled", ct.enabled);
}

static void from_json(const json& j, ColorToggleRounding& ctr)
{
    from_json(j, static_cast<ColorToggle&>(ctr));

    read(j, "Rounding", ctr.rounding);
}

static void from_json(const json& j, ColorToggleOutline& cto)
{
    from_json(j, static_cast<ColorToggle&>(cto));

    read(j, "Outline", cto.outline);
}

static void from_json(const json& j, ColorToggleThickness& ctt)
{
    from_json(j, static_cast<ColorToggle&>(ctt));

    read(j, "Thickness", ctt.thickness);
}

static void from_json(const json& j, ColorToggleThicknessRounding& cttr)
{
    from_json(j, static_cast<ColorToggleRounding&>(cttr));

    read(j, "Thickness", cttr.thickness);
}

static void from_json(const json& j, Font& f)
{
    read<value_t::string>(j, "Name", f.name);

    if (!f.name.empty())
        config->scheduleFontLoad(f.name);

    if (const auto it = std::find_if(config->getSystemFonts().begin(), config->getSystemFonts().end(), [&f](const auto& e) { return e == f.name; }); it != config->getSystemFonts().end())
        f.index = std::distance(config->getSystemFonts().begin(), it);
    else
        f.index = 0;
}

static void from_json(const json& j, Snapline& s)
{
    from_json(j, static_cast<ColorToggleThickness&>(s));

    read(j, "Type", s.type);
}

static void from_json(const json& j, Config::Misc::Offscreen& o)
{
    from_json(j, static_cast<ColorToggleOutline&>(o));

    read(j, "SIZE", o.size);
    read(j, "OFFSET", o.offset);
}

static void from_json(const json& j, Box& b)
{
    from_json(j, static_cast<ColorToggleRounding&>(b));

    read(j, "Type", b.type);
    read<value_t::object>(j, "Fill", b.fill);
}

static void from_json(const json& j, BulletTracers& o)
{
    from_json(j, static_cast<ColorToggle&>(o));
}

static void from_json(const json& j, ImVec2& v)
{
    read(j, "X", v.x);
    read(j, "Y", v.y);
}

static void from_json(const json& j, Config::RecoilControlSystem& r)
{
    read(j, "Enabled", r.enabled);
    read(j, "Silent", r.silent);
    read(j, "Ignore Shots", r.shotsFired);
    read(j, "Horizontal", r.horizontal);
    read(j, "Vertical", r.vertical);
}
static void from_json(const json& j, Config::RagebotGlobal& rg)
{
    read(j, "Enabled", rg.enabled);
    read(j, "Knifebot", rg.knifebot);
    read(j, "Silent", rg.silent);
    read(j, "ASP", rg.autoScope);
    read(j, "Friendly fire", rg.friendlyFire);
    read(j, "Auto shot", rg.autoShot);
    read(j, "Priority", rg.priority);
    read(j, "Fov", rg.fov);
}
static void from_json(const json& j, Config::Ragebot& r)
{
    read(j, "OV CONFIG", r.enabled);
    read(j, "ASS", r.autoStop);
    read(j, "ASMS", r.autoStopMod);
    read(j, "HB", r.hitboxes);
    read(j, "HC", r.hitChance);
    read(j, "HCO", r.OvrHitChance);
    read(j, "HCOB", r.hcov);
    read(j, "DMGOB", r.dmgov);
    read(j, "MH", r.multiPointHead);
    read(j, "MB", r.multiPointBody);
    read(j, "MD", r.minDamage);
    read(j, "MDO", r.minDamageOverride);
    read(j, "BS", r.betweenShots);
}

static void from_json(const json& j, Config::RageAntiAimConfig& a)
{
    read(j, "Pitch", a.pitch);
    read(j, "Yaw base", reinterpret_cast<int&>(a.yawBase));
    read(j, "Freestand", a.freestand);
    read(j, "Fake flick", a.fakeFlick);
    read(j, "FF rate", a.fakeFlickRate);
    read(j, "Yaw modifier", a.yawModifier);
    read(j, "Yaw add", a.yawAdd);
    read(j, "Jitter Range", a.jitterRange);
    read(j, "Random Range", a.randomRange);
    read(j, "Roll add", a.roll.add);
    read(j, "Roll", a.roll.enabled);
    read(j, "Roll pitchE", a.roll.exploitPitch);
    read(j, "Roll pitch", a.roll.pitch);
    read(j, "Spin base", a.spinBase);
    read(j, "At targets", a.atTargets);
    read(j, "Desync", a.desync);
    read(j, "Left limit", a.leftLimit);
    read(j, "Right limit", a.rightLimit);
    read(j, "Peek mode", a.peekMode);
    read(j, "Lby mode", a.lbyMode);
}

static void from_json(const json& j, Config::AntiAimConditions& c)
{
    read<value_t::object>(j, "Visualize", c.visualize);
    read(j, "Visualize type", c.visualizeType);
    read(j, "Visualize offset", c.visualizeOffset);
    read(j, "Visualize size", c.visualizeSize);
    read(j, "G", c.global);
    read(j, "M", c.moving);
    read(j, "S", c.slowwalk);
    read(j, "J", c.jumping);
    read(j, "CJ", c.cjump);
    read(j, "C", c.chrouch);
    read(j, "BREAKERS", c.animBreakers);
}

static void from_json(const json& j, Config::Fakelag& f)
{
    read(j, "Mode", f.mode);
    read(j, "Limit", f.limit);
}

static void from_json(const json& j, Config::Tickbase& t)
{
    read(j, "Doubletap", t.doubletap);
    read(j, "Hideshots", t.hideshots);
    read(j, "Teleport", t.teleport);
}

static void from_json(const json& j, Config::Backtrack& b)
{
    read(j, "Enabled", b.enabled);
    read(j, "All ticks", b.allticks);
    read(j, "Time limit", b.timeLimit);
    read(j, "Fake Latency", b.fakeLatency);
    read(j, "Fake Latency Amount", b.fakeLatencyAmount);
}

static void from_json(const json& j, Config::Chams::Material& m)
{
    from_json(j, static_cast<Color4&>(m));

    read(j, "Enabled", m.enabled);
    read(j, "Health based", m.healthBased);
    read(j, "Blinking", m.blinking);
    read(j, "Wireframe", m.wireframe);
    read(j, "Cover", m.cover);
    read(j, "Ignore-Z", m.ignorez);
    read(j, "Material", m.material);
}

static void from_json(const json& j, Config::LegitbotGlobal& l)
{
    read(j, "EL", l.enabled);
    read(j, "AL", l.aimlock);
    read(j, "FF", l.friendlyFire);
    read(j, "VO", l.visibleOnly);
    read(j, "SO", l.scopedOnly);
    read(j, "IF", l.ignoreFlash);
    read(j, "KS", l.killshot);
    read(j, "ET", l.enableTriggerbot);
    read<value_t::object>(j, "FOV", l.legitbotFov);
}

static void from_json(const json& j, Config::Legitbob& l)
{
    read(j,"OV", l.override);
    read(j,"RT", l.reactionTime);
    read(j,"ST", l.smooth);
    read(j,"HB", l.hitboxes);
    read(j,"HT", l.hitchanceT);
    read(j,"MD", l.minDamage);
    read(j,"BS", l.betweenShots);
    read(j,"FOV", l.fov);
}

static void from_json(const json& j, Config::Chams& c)
{
    read_array_opt(j, "Materials", c.materials);
}

static void from_json(const json& j, Config::GlowItem& g)
{
    from_json(j, static_cast<Color4&>(g));

    read(j, "Enabled", g.enabled);
    read(j, "Health based", g.healthBased);
    read(j, "Style", g.style);
}

static void from_json(const json& j, Config::Visuals::OnHitHitbox& h)
{
    read(j, "enabled", h.enabled);
    read<value_t::object>(j, "Color", h.color);
    read(j, "Duration", h.duration);
}

static void from_json(const json& j, Config::PlayerGlow& g)
{
    read<value_t::object>(j, "All", g.all);
    read<value_t::object>(j, "Visible", g.visible);
    read<value_t::object>(j, "Occluded", g.occluded);
}
//esp
static void from_json(const json& j, Config::NewESP::Box& b)
{
    from_json(j, static_cast<ColorToggle&>(b));
    read(j, "TYPE", b.type);
}

static void from_json(const json& j, Config::NewESP::Shared& s)
{
    read(j, "ENABLED", s.enable);
    read<value_t::object>(j, "BOX", s.box);
    read<value_t::object>(j, "NAME", s.name);
    read<value_t::object>(j, "SNAPLINE", s.snapline);
}

static void from_json(const json& j, Config::NewESP::Weapon& w)
{
    from_json(j, static_cast<Config::NewESP::Shared&>(w));
    read<value_t::object>(j, "AMMO", w.ammo);
    read<value_t::object>(j, "ICON", w.icon);
}

static void from_json(const json& j, Config::NewESP::Projectiles& w)
{
    from_json(j, static_cast<Config::NewESP::Shared&>(w));
    read<value_t::object>(j, "TRAILS", w.trails);
}
static void from_json(const json& j, Config::NewESP::HealthBar& a)
{
    read(j, "ENABLED", a.enabled);
    read(j, "OUTLINE", a.outline);
    read(j, "BACKGROUND", a.background);
    read(j, "STYLE", a.style);
    read<value_t::object>(j, "TEXT", a.text);
    read<value_t::object>(j, "SOLID", a.solid);
    read<value_t::object>(j, "TOP", a.top);
    read<value_t::object>(j, "BOTTOM", a.bottom);
}
static void from_json(const json& j, Config::NewESP::AmmoBar& a)
{
    read(j, "ENABLED", a.enabled);
    read(j, "OUTLINE", a.outline);
    read(j, "BACKGROUND", a.background);
    read(j, "STYLE", a.style);
    read<value_t::object>(j, "TEXT", a.text);
    read<value_t::object>(j, "SOLID", a.solid);
    read<value_t::object>(j, "LEFT", a.left);
    read<value_t::object>(j, "RIGHT", a.right);
}

static void from_json(const json& j, Config::NewESP::Player& p)
{
    read(j, "ENABLED", p.enable);
    read<value_t::object>(j, "BOX", p.box);
    read<value_t::object>(j, "NAME", p.name);
    read<value_t::object>(j, "SNAPLINE", p.snapline);
    read<value_t::object>(j, "SKELETON", p.skeleton);
    read<value_t::object>(j, "AMMO", p.ammo);
    read<value_t::object>(j, "HEALTH", p.health);
    read<value_t::object>(j, "WEAPON", p.weapon);
    read<value_t::object>(j, "WEAPONI", p.weaponIcon);
    read<value_t::object>(j, "FLAGS", p.flags);
    read(j, "DORMANT", p.dormant);
    read(j, "DORMANT TIME", p.dormantTime);
    read(j, "SA", p.showArmor);
    read(j, "SB", p.showBomb);
    read(j, "SK", p.showKit);
    read(j, "SP", p.showPin);
    read(j, "SM", p.showMoney);
    read(j, "SF", p.showFD);
    read(j, "SS", p.showScoped);
    read(j, "SFF", p.showFlashed);
}

static void from_json(const json& j, Config::NewESP& e)
{
    read<value_t::object>(j, "ENEMY", e.enemy);
    read<value_t::object>(j, "ALLIES", e.allies);
    read<value_t::object>(j, "WEAPONS", e.weapons);
    read<value_t::object>(j, "PROJECTILES", e.projectiles);
}
//end of esp
static void from_json(const json& j, Config::Visuals::AALines& a)
{
    read(j, "enabled", a.enabled);
    read<value_t::object>(j, "Real", a.real);
    read<value_t::object>(j, "Fake", a.fake);
}

static void from_json(const json& j, Config::Visuals::Scope& s)
{
    read<value_t::object>(j, "color", s.color);
    read(j, "offset", s.offset);
    read(j, "length", s.length);
    read(j, "fade", s.fade);
    read(j, "type", s.type);
    read(j, "b", s.removeBottom);
    read(j, "t", s.removeTop);
    read(j, "l", s.removeLeft);
    read(j, "r", s.removeRight);
}

static void from_json(const json& j, Config::Visuals& v)
{
    read<value_t::object>(j, "SCOPE", v.scope);
    read<value_t::object>(j, "AA lines", v.antiAimLines);
    read(j, "Disable post-processing", v.disablePostProcessing);
    read(j, "Inverse ragdoll gravity", v.inverseRagdollGravity);
    read(j, "No fog", v.noFog);
    read<value_t::object>(j, "Fog controller", v.fog);
    read<value_t::object>(j, "Hitshitado", v.hitMarkerColor);
    read<value_t::object>(j, "Fog options", v.fogOptions);
    read(j, "No 3d sky", v.no3dSky);
    read(j, "No aim punch", v.noAimPunch);
    read(j, "No view punch", v.noViewPunch);
    read(j, "No view bob", v.noViewBob);
    read(j, "No hands", v.noHands);
    read(j, "No sleeves", v.noSleeves);
    read(j, "No weapons", v.noWeapons);
    read(j, "No smoke", v.noSmoke);
    read(j, "Wireframe smoke", v.wireframeSmoke);
    read(j, "No molotov", v.noMolotov);
    read<value_t::object>(j, "Console Color", v.console);
    read<value_t::object>(j, "Trail", v.playerTrailColor);
    read(j, "Wireframe molotov", v.wireframeMolotov);
    read(j, "No blur", v.noBlur);
    read(j, "No scope overlay", v.noScopeOverlay);
    read(j, "No grass", v.noGrass);
    read(j, "No shadows", v.noShadows);
    read<value_t::object>(j, "Motion Blur", v.motionBlur);
    read<value_t::object>(j, "Shadows changer", v.shadowsChanger);
    read(j, "Full bright", v.fullBright);
    read(j, "Zoom", v.zoom);
    read(j, "Zoom key", v.zoomKey);
    read(j, "Thirdperson", v.thirdperson.enable);
    read(j, "Thirdperson anim", v.thirdperson.animated);
    read(j, "Thirdperson key", v.thirdperson.key);
    read(j, "Thirdperson distance", v.thirdperson.distance);
    read(j, "On Nade", v.thirdperson.disableOnGrenade);
    read(j, "Transparency in scope", v.thirdperson.thirdpersonTransparency);
    read(j, "While dead", v.thirdperson.whileDead);
    read(j, "Freecam", v.freeCam);
    read(j, "Freecam key", v.freeCamKey);
    read(j, "Freecam speed", v.freeCamSpeed);
    read(j, "Keep FOV", v.keepFov);
    read(j, "FOV", v.fov);
    read(j, "Far Z", v.farZ);
    read(j, "Flash reduction", v.flashReduction);
    read(j, "Skybox", v.skybox);
    read(j, "Deagle spinner", v.deagleSpinner);
    read(j, "Screen effect", v.screenEffect);
    read(j, "Hit effect", v.hitEffect);
    read(j, "Hit effect time", v.hitEffectTime);
    read(j, "Hit marker", v.hitMarker);
    read(j, "Hit marker time", v.hitMarkerTime);
    read(j, "Playermodel T", v.playerModelT);
    read(j, "Playermodel CT", v.playerModelCT);
    read(j, "Custom Playermodel", v.playerModel, sizeof(v.playerModel));
    read(j, "Disable jiggle bones", v.disableJiggleBones);
    read<value_t::object>(j, "Bullet Tracers", v.bulletTracers);
    read(j, "SPRITE", v.bulletSprite);
    read(j, "WIDTH", v.bulletWidth);
    read(j, "TYPE", v.bulletEffects);
    read(j, "EFFECTS", v.bulletEffects);
    read(j, "AMP", v.amplitude);
    read<value_t::object>(j, "Bullet Impacts", v.bulletImpacts);
    read(j, "Bullet Impacts time", v.bulletImpactsTime);
    read<value_t::object>(j, "Molotov Hull", v.molotovHull);
    read<value_t::object>(j, "Smoke Hull", v.smokeHull);
    read<value_t::object>(j, "Viewmodel", v.viewModel);
    read<value_t::object>(j, "Molotov Polygon", v.molotovPolygon);
    read(j, "ViewmodelScope", v.viewmodelInScope);
    read<value_t::string>(j, "Custom skybox", v.customSkybox);
    read<value_t::object>(j, "Spread circle", v.spreadCircle);
    read<value_t::object>(j, "World", v.world);
    read<value_t::object>(j, "Props", v.props);
    read<value_t::object>(j, "Sky", v.sky);
    read(j, "Asus walls", v.asusWalls);
    read(j, "Asus props", v.asusProps);
    read(j, "Smoke timer", v.smokeTimer);
    read<value_t::object>(j, "Smoke timer BG", v.smokeTimerBG);
    read<value_t::object>(j, "Smoke timer TIMER", v.smokeTimerTimer);
    read<value_t::object>(j, "Smoke timer TEXT", v.smokeTimerText);
    read(j, "Molotov timer", v.molotovTimer);
    read<value_t::object>(j, "Molotov timer BG", v.molotovTimerBG);
    read<value_t::object>(j, "Molotov timer TIMER", v.molotovTimerTimer);
    read<value_t::object>(j, "Molotov timer TEXT", v.molotovTimerText);
    read<value_t::object>(j, "Smoke Color", v.smokeColor);
    read<value_t::object>(j, "Molotov Color", v.molotovColor);
    read<value_t::object>(j, "Taser range", v.TaserRange);
    read<value_t::object>(j, "Knife range", v.KnifeRange);
    read(j, "PostEnabled", v.PostEnabled);
    read(j, "World exposure", v.worldExposure);
    read(j, "Model ambient", v.modelAmbient);
    read(j, "Bloom scale", v.bloomScale);
    read<value_t::object>(j,"Footstep beamsE", v.footstepBeamsE);
    read<value_t::object>(j,"Footstep beamsA", v.footstepBeamsA);
    read(j,"Footstep beams radiusE", v.footstepBeamRadiusE);
    read(j,"Footstep beams radiusA", v.footstepBeamRadiusA);
    read(j,"Footstep beams thicknessE", v.footstepBeamThicknessE);
    read(j,"Footstep beams thicknessA", v.footstepBeamThicknessA);
    read<value_t::object>(j, "Hitbox on Hit", v.onHitHitbox);
    read(j, "Party mode", v.partyMode);
    //read<value_t::object>(j, "Molly circle", v.molotovCircle);
}

static void from_json(const json& j, sticker_setting& s)
{
    read(j, "Kit", s.kit);
    read(j, "Wear", s.wear);
    read(j, "Scale", s.scale);
    read(j, "Rotation", s.rotation);

    s.onLoad();
}

static void from_json(const json& j, item_setting& i)
{
    read(j, "Enabled", i.enabled);
    read(j, "Definition index", i.itemId);
    read(j, "Quality", i.quality);
    read(j, "Paint Kit", i.paintKit);
    read(j, "Definition override", i.definition_override_index);
    read(j, "Seed", i.seed);
    read(j, "StatTrak", i.stat_trak);
    read(j, "Wear", i.wear);
    read(j, "Custom name", i.custom_name, sizeof(i.custom_name));
    read(j, "Stickers", i.stickers);

    i.onLoad();
}

static void from_json(const json& j, PurchaseList& pl)
{
    read(j, "Enabled", pl.enabled);
    read(j, "Only During Freeze Time", pl.onlyDuringFreezeTime);
    read(j, "Show Prices", pl.showPrices);
    read(j, "No Title Bar", pl.noTitleBar);
    read(j, "Mode", pl.mode);
    read<value_t::object>(j, "Pos", pl.pos);
}

static void from_json(const json& j, Config::Misc::SpectatorList& sl)
{
    read(j, "Enabled", sl.enabled);
    read(j, "No Title Bar", sl.noTitleBar);
    read(j, "Avatars", sl.avatars);
    read<value_t::object>(j, "Pos", sl.pos);
}

static void from_json(const json& j, Config::Misc::KeyBindList& sl)
{
    read(j, "Enabled", sl.enabled);
    read(j, "No Title Bar", sl.noTitleBar);
    read<value_t::object>(j, "Pos", sl.pos);
}

static void from_json(const json& j, Config::Misc::PlayerList& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Steam ID", o.steamID);
    read(j, "Rank", o.rank);
    read(j, "Wins", o.wins);
    read(j, "Money", o.money);
    read(j, "Health", o.health);
    read(j, "Armor", o.armor);
    read<value_t::object>(j, "Pos", o.pos);
}

static void from_json(const json& j, Config::Misc::JumpStats& js)
{
    read(j, "Enabled", js.enabled);
    read(j, "Show fails", js.showFails);
    read(j, "Show color on fail", js.showColorOnFail);
    read(j, "Simplify naming", js.simplifyNaming);
}

static void from_json(const json& j, Config::Misc::Velocity& v)
{
    read(j, "Enabled", v.enabled);
    read(j, "Position", v.position);
    read(j, "Alpha", v.alpha);
    read<value_t::object>(j, "Color", v.color);
}

static void from_json(const json& j, Config::Misc::KeyBoardDisplay& kbd)
{
    read(j, "Enabled", kbd.enabled);
    read(j, "Position", kbd.position);
    read(j, "Show key Tiles", kbd.showKeyTiles);
    read<value_t::object>(j, "Color", kbd.color);
}

static void from_json(const json& j, PreserveKillfeed& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Only Headshots", o.onlyHeadshots);
}

static void from_json(const json& j, KillfeedChanger& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Headshot", o.headshot);
    read(j, "Dominated", o.dominated);
    read(j, "Revenge", o.revenge);
    read(j, "Penetrated", o.penetrated);
    read(j, "Noscope", o.noscope);
    read(j, "Thrusmoke", o.thrusmoke);
    read(j, "Attackerblind", o.attackerblind);
}

static void from_json(const json& j, AutoBuy& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Primary weapon", o.primaryWeapon);
    read(j, "Secondary weapon", o.secondaryWeapon);
    read(j, "Armor", o.armor);
    read(j, "Utility", o.utility);
    read(j, "Grenades", o.grenades);
}

static void from_json(const json& j, Config::Misc::Logger& o)
{
    read(j, "Modes", o.modes);
    read(j, "Events", o.events);
    read(j, "POS", o.position);
    read(j, "OFF", o.offset);
}

static void from_json(const json& j, Config::Visuals::MotionBlur& mb)
{
    read(j, "Enabled", mb.enabled);
    read(j, "Forward", mb.forwardEnabled);
    read(j, "Falling min", mb.fallingMin);
    read(j, "Falling max", mb.fallingMax);
    read(j, "Falling intensity", mb.fallingIntensity);
    read(j, "Rotation intensity", mb.rotationIntensity);
    read(j, "Strength", mb.strength);
}

static void from_json(const json& j, Config::Visuals::Fog& f)
{
    read(j, "Start", f.start);
    read(j, "End", f.end);
    read(j, "Density", f.density);
}

static void from_json(const json& j, Config::Visuals::ShadowsChanger& sw)
{
    read(j, "Enabled", sw.enabled);
    read(j, "X", sw.x);
    read(j, "Y", sw.y);
}

static void from_json(const json& j, Config::Visuals::Viewmodel& vxyz)
{
    read(j, "Enabled", vxyz.enabled);
    read(j, "Fov", vxyz.fov);
    read(j, "X", vxyz.x);
    read(j, "Y", vxyz.y);
    read(j, "Z", vxyz.z);
    read(j, "Roll", vxyz.roll);
}

static void from_json(const json& j, Config::Misc::Watermark& wm)
{
    read(j, "Enabled", wm.enabled);
    read(j, "Fps", wm.showFps);
    read(j, "Tickrate", wm.showTicks);
    read(j, "Time", wm.showTime);
    read(j, "Spotify", wm.showSpotify);
    read(j, "Username", wm.showUsername);
    read(j, "Ping", wm.showPing);
}

static void from_json(const json& j, Config::Misc::Indicators& i)
{
    read(j, "Enabled", i.enabled);
    read<value_t::object>(j, "Color", i.color);
    read(j, "Style", i.style);
    read(j, "Show", i.toShow);
}

static void from_json(const json& j, Config::Misc::EBDetect& e)
{
    read(j, "chat", e.chat);
    read(j, "sound", e.sound);
    read(j, "effect", e.effect);
    read(j, "effect time", e.effectTime);
}

static void from_json(const json& j, Config::Misc& m)
{
    read(j, "Chat reveavler", m.chatReveavler);
    read<value_t::object>(j, "Indicators", m.indicators);
    read(j, "DesyncI", m.IshowDesync);
    read(j, "bhHC", m.bhHc);
    read(j, "ManualI", m.IshowManual);
    read(j, "RageBotI", m.IshowRageBot);
    read(j, "LegitBotI", m.IshowLegitBot);
    read(j, "MiscI", m.IshowMisc);
    read(j, "Menu key", m.menuKey);
    read(j, "Anti AFK kick", m.antiAfkKick);
    read(j, "Adblock", m.adBlock);
    read(j, "Auto strafe", m.autoStrafe);
    read(j, "Auto strafe smoothness", m.auto_smoothnes);
    read(j, "Bunny hop", m.bunnyHop);
    read(j, "Clan tag", m.clantag);
    read(j, "Fast duck", m.fastDuck);
    read(j, "Moonwalk Mode", m.moonwalk_style);
    read(j, "Block bot", m.blockBot);
    read(j, "Block bot Key", m.blockBotKey);
    read(j, "Door spam", m.doorSpamKey);
    read(j, "Door spam key", m.doorSpam);
    read(j, "Edge Jump", m.edgeJump);
    read(j, "Edge Jump Key", m.edgeJumpKey);
    read(j, "Mini Jump", m.miniJump);
    read(j, "Mini Jump Crouch lock", m.miniJumpCrouchLock);
    read(j, "Mini Jump Key", m.miniJumpKey);
    read(j, "Jump Bug", m.jumpBug);
    read(j, "Jump Bug Key", m.jumpBugKey);
    read(j, "Edge Bug", m.edgeBug);
    read(j, "Edge Bug Key", m.edgeBugKey);
    read(j, "Edge bug lock", m.edgeBugLock);
    read(j, "Edge bug lock type", m.edgeBugLockType);
    read<value_t::object>(j, "Egg bog", m.ebdetect);
    read<value_t::string>(j, "Egg bog sound", m.customEBsound);
    read(j, "Pred Amnt", m.edgeBugPredAmnt);
    read(j, "Auto pixel surf", m.autoPixelSurf);
    read(j, "Auto pixel surf Pred Amnt", m.autoPixelSurfPredAmnt);
    read(j, "Auto pixel surf Key", m.autoPixelSurfKey);
    read<value_t::object>(j, "Velocity", m.velocity);
    read<value_t::object>(j, "Keyboard display", m.keyBoardDisplay);
    read(j, "Slowwalk", m.slowwalk);
    read(j, "Slowwalk key", m.slowwalkKey);
    read(j, "Slowwalk Amnt", m.slowwalkAmnt);
    read(j, "Fake duck", m.fakeduck);
    read(j, "Fake duck key", m.fakeduckKey);
    read<value_t::object>(j, "Auto peek", m.autoPeek);
    read(j, "Auto peek style", m.autoPeekStyle);
    read(j, "Auto peek key", m.autoPeekKey);
    read(j, "Noscope crosshair", m.noscopeCrosshair);
    read(j, "Recoil crosshair", m.recoilCrosshair);
    read(j, "Auto pistol", m.autoPistol);
    read(j, "Auto reload", m.autoReload);
    read(j, "Auto accept", m.autoAccept);
    read(j, "Radar hack", m.radarHack);
    read(j, "Reveal ranks", m.revealRanks);
    read(j, "Reveal money", m.revealMoney);
    read(j, "Reveal suspect", m.revealSuspect);
    read(j, "Reveal votes", m.revealVotes);
    read<value_t::object>(j, "Spectator list", m.spectatorList);
    read<value_t::object>(j, "Keybind list", m.keybindList);
    read<value_t::object>(j, "Player list", m.playerList);
    read<value_t::object>(j, "Jump stats", m.jumpStats);
    read<value_t::object>(j, "Watermark", m.wm);
    if (j.contains("ColorE")) {
        const auto& val = j["ColorE"];
        // old format -> [1.0f, 0.0f, 0.0f, 1.0f]
        // new format -> #ff0000 + alpha as float
        if (val.type() == value_t::array && val.size() == m.offscreenEnemies.color.size()) {
            for (std::size_t i = 0; i < val.size(); ++i) {
                if (!val[i].empty())
                    val[i].get_to(m.offscreenEnemies.color[i]);
            }
        }
        else if (val.type() == value_t::string) {
            const auto str = val.get<std::string>();
            if (str.length() == 7 && str[0] == '#') {
                const auto color = std::strtol(str.c_str() + 1, nullptr, 16);
                m.offscreenEnemies.color[0] = ((color >> 16) & 0xFF) / 255.0f;
                m.offscreenEnemies.color[1] = ((color >> 8) & 0xFF) / 255.0f;
                m.offscreenEnemies.color[2] = (color & 0xFF) / 255.0f;
            }
            read(j, "AlphaE", m.offscreenEnemies.color[3]);
        }
    }

    read(j, "RainbowE", m.offscreenEnemies.rainbow);
    read(j, "Rainbow Speed E", m.offscreenEnemies.rainbowSpeed);
    read(j, "OEE", m.offscreenEnemies.enabled);
    read(j, "OAT", m.offscreenAllies.type);
    read(j, "OET", m.offscreenEnemies.type);
    read(j, "OEOE", m.offscreenEnemies.outline);
    read(j, "SIZEE", m.offscreenEnemies.size);
    read(j, "OFFSETE", m.offscreenEnemies.offset);
    if (j.contains("ColorA")) {
        const auto& val = j["ColorA"];
        // old format -> [1.0f, 0.0f, 0.0f, 1.0f]
        // new format -> #ff0000 + alpha as float
        if (val.type() == value_t::array && val.size() == m.offscreenAllies.color.size()) {
            for (std::size_t i = 0; i < val.size(); ++i) {
                if (!val[i].empty())
                    val[i].get_to(m.offscreenAllies.color[i]);
            }
        }
        else if (val.type() == value_t::string) {
            const auto str = val.get<std::string>();
            if (str.length() == 7 && str[0] == '#') {
                const auto color = std::strtol(str.c_str() + 1, nullptr, 16);
                m.offscreenAllies.color[0] = ((color >> 16) & 0xFF) / 255.0f;
                m.offscreenAllies.color[1] = ((color >> 8) & 0xFF) / 255.0f;
                m.offscreenAllies.color[2] = (color & 0xFF) / 255.0f;
            }
            read(j, "AlphaA", m.offscreenAllies.color[3]);
        }
    }

    read(j, "RainbowA", m.offscreenAllies.rainbow);
    read(j, "Rainbow Speed A", m.offscreenAllies.rainbowSpeed);
    read(j, "OEA", m.offscreenAllies.enabled);
    read(j, "OEOA", m.offscreenAllies.outline);
    read(j, "SIZEA", m.offscreenAllies.size);
    read(j, "OFFSETA", m.offscreenAllies.offset);
    read(j, "Disable model occlusion", m.disableModelOcclusion);
    read(j, "Aspect Ratio", m.aspectratio);
    read(j, "Kill message", m.killMessage);
    read(j, "Name stealer", m.nameStealer);
    read(j, "Disable HUD blur", m.disablePanoramablur);
    read(j, "Fast Stop", m.fastStop);
    read<value_t::object>(j, "Bomb timer", m.bombTimer);
    read<value_t::object>(j, "Hurt indicator", m.hurtIndicator);
    read(j, "Prepare revolver", m.prepareRevolver);
    read(j, "Prepare revolver key", m.prepareRevolverKey);
    read(j, "Hit sound", m.hitSound);
    read(j, "Quick healthshot key", m.quickHealthshotKey);
    read(j, "Grenade predict", m.nadePredict);
    read<value_t::object>(j, "Grenade predict Damage", m.nadeDamagePredict);
    read<value_t::object>(j, "Grenade predict Trail", m.nadeTrailPredict);
    read<value_t::object>(j, "Grenade predict Circle", m.nadeCirclePredict);
    read<value_t::object>(j, "Grenade predict Glow", m.nadeGlowPredict);
    read(j, "Fix tablet signal", m.fixTabletSignal);
    read<value_t::string>(j, "Custom Hit Sound", m.customHitSound);
    read(j, "Kill sound", m.killSound);
    read<value_t::string>(j, "Custom Kill Sound", m.customKillSound);
    read<value_t::object>(j, "Purchase List", m.purchaseList);
    read<value_t::object>(j, "Reportbot", m.reportbot);
    read<value_t::object>(j, "Preserve Killfeed", m.preserveKillfeed);
    read<value_t::object>(j, "Killfeed changer", m.killfeedChanger);
    read(j, "Sv pure bypass", m.svPureBypass);
    read(j, "Inventory Unlocker", m.inventoryUnlocker);
    read<value_t::object>(j, "Autobuy", m.autoBuy);
    read<value_t::object>(j, "Logger", m.logger);
    read<value_t::object>(j, "Logger options", m.loggerOptions);
    read<value_t::object>(j, "accent", m.accentColor);
    read(j, "no borders", m.borders);
    read(j, "Radius", m.autoPeekRadius);
    read(j, "Gravity force", m.ragdollGravity);
    //read(j, "IPOS", m.indicatorPos);
}

static void from_json(const json& j, Config::Misc::Reportbot& r)
{
    read(j, "Enabled", r.enabled);
    read(j, "Target", r.target);
    read(j, "Delay", r.delay);
    read(j, "Rounds", r.rounds);
    read(j, "Abusive Communications", r.textAbuse);
    read(j, "Griefing", r.griefing);
    read(j, "Wall Hacking", r.wallhack);
    read(j, "Aim Hacking", r.aimbot);
    read(j, "Other Hacking", r.other);
}

static void from_json(const json& j, Config::LightsConfig::delightColor& d)
{
    read(j, "RADIUS", d.raduis);
    read(j, "EXPONENT", d.exponent);
    read(j, "ENABLED", d.enabled);
    read<value_t::object>(j, "COLOR", d.asColor3);
}

static void from_json(const json& j, Config::LightsConfig& d)
{
    read<value_t::object>(j, "LOCAL", d.local);
    read<value_t::object>(j, "TEAM", d.teammate);
    read<value_t::object>(j, "ENEMY", d.enemy);
}

static void from_json(const json& j, Config::Menu& m)
{
    read<value_t::object>(j, "ACCENT", m.accentColor);
    read(j, "STYLE", m.windowStyle);
    read(j, "TRANS", m.transparency);
}

void Config::load(size_t id, bool incremental) noexcept
{
    load((const char8_t*)configs[id].c_str(), incremental);
}

void Config::load(const char8_t* name, bool incremental) noexcept
{
    json j;

    if (std::ifstream in{ path / name }; in.good()) {
        j = json::parse(in, nullptr, false);
        if (j.is_discarded())
            return;
    } else {
        return;
    }

    if (!incremental)
        reset();

    //read(j, "Legitbot", legitbot);
    read(j, "Legitbot Key", legitbotKey);
    read(j, "Recoil control system", recoilControlSystem);
    read(j, "Freestand key", freestandKey);
    read(j, "Disable in freezetime", disableInFreezetime);
    read(j, "Manual forward Key", manualForward);
    read(j, "Manual backward Key", manualBackward);
    read(j, "Manual right Key", manualRight);
    read(j, "Manual left Key", manualLeft);
    read(j, "Fake flick key", fakeFlickOnKey);
    read(j, "Flip Flick", flipFlick);
    read(j, "Invert", invert);
    read(j, "HCOK", hitchanceOverride);
    read<value_t::object>(j, "SEX", condAA);
    read<value_t::object>(j, "LegitbotGlobal", lgb);
    read<value_t::object>(j, "PENIS?", esp);
    read(j, "LegitbotConfig", legitbob);
    read<value_t::object>(j, "RagebotGlobal", ragebot);
    read<value_t::array>(j, "RagebotConfig", rageBot);
    read(j, "Ragebot Key", ragebotKey);
    read(j, "Min damage override Key", minDamageOverrideKey);
    read<value_t::object>(j, "Dlights", dlightConfig);
    read(j, "Triggerbot Key", triggerbotKey);
    read(j, "Force baim", forceBaim);
    read(j, "Rage Anti aim", rageAntiAim);
    read<value_t::object>(j, "Fakelag", fakelag);
    read<value_t::object>(j, "Tickbase", tickbase);
    read<value_t::object>(j, "Backtrack", backtrack);
    read<value_t::object>(j, "Menu", menu);
    read(j["Glow"], "Items", glow);
    read(j["Glow"], "Players", playerGlow);

    read(j, "Chams", chams);
    read<value_t::object>(j, "Visuals", visuals);
    read(j, "Skin changer", skinChanger);
    read<value_t::object>(j, "Misc", misc);
}

#pragma endregion

#pragma region  Write

static void to_json(json& j, const ColorToggle& o, const ColorToggle& dummy = {})
{
    to_json(j, static_cast<const Color4&>(o), dummy);
    WRITE("Enabled", enabled);
}

static void to_json(json& j, const Color3& o, const Color3& dummy = {})
{
    WRITE("Color", color);
    WRITE("Rainbow", rainbow);
    WRITE("Rainbow Speed", rainbowSpeed);
}

static void to_json(json& j, const ColorToggle3& o, const ColorToggle3& dummy = {})
{
    to_json(j, static_cast<const Color3&>(o), dummy);
    WRITE("Enabled", enabled);
}

static void to_json(json& j, const ColorToggleRounding& o, const ColorToggleRounding& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Rounding", rounding);
}

static void to_json(json& j, const ColorToggleThickness& o, const ColorToggleThickness& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Thickness", thickness);
}

static void to_json(json& j, const ColorToggleOutline& o, const ColorToggleOutline& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Outline", outline);
}

static void to_json(json& j, const ColorToggleThicknessRounding& o, const ColorToggleThicknessRounding& dummy = {})
{
    to_json(j, static_cast<const ColorToggleRounding&>(o), dummy);
    WRITE("Thickness", thickness);
}

static void to_json(json& j, const Font& o, const Font& dummy = {})
{
    WRITE("Name", name);
}

static void to_json(json& j, const BulletTracers& o, const BulletTracers& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
}
static void to_json(json& j, const ImVec2& o, const ImVec2& dummy = {})
{
    WRITE("X", x);
    WRITE("Y", y);
}

static void to_json(json& j, const Config::RecoilControlSystem& o, const Config::RecoilControlSystem& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Silent", silent);
    WRITE("Ignore Shots", shotsFired);
    WRITE("Horizontal", horizontal);
    WRITE("Vertical", vertical);
}

static void to_json(json& j, const Config::LegitbotGlobal& o, const Config::LegitbotGlobal& dummy = {})
{
    WRITE("EL", enabled);
    WRITE("AL", aimlock);
    WRITE("FF", friendlyFire);
    WRITE("VO", visibleOnly);
    WRITE("SO", scopedOnly);
    WRITE("IF", ignoreFlash);
    WRITE("IS", ignoreSmoke);
    WRITE("KS", killshot);
    WRITE("ET", enableTriggerbot);
    WRITE("FOV", legitbotFov);
}

static void to_json(json& j, const Config::Menu& o)
{
    const Config::Menu dummy;
    WRITE("ACCENT", accentColor);
    WRITE("STYLE", windowStyle);
    WRITE("TRANS", transparency);
}

static void to_json(json& j, const Config::Legitbob& o, const Config::Legitbob& dummy = {})
{
    WRITE("OV", override);
    WRITE("RT", reactionTime);
    WRITE("ST", smooth);
    WRITE("HB", hitboxes);
    WRITE("HT", hitchanceT);
    WRITE("MD", minDamage);
    WRITE("BS", betweenShots);
    WRITE("FOV", fov);
}

static void to_json(json& j, const Config::RagebotGlobal& o, const Config::RagebotGlobal& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Knifebot", knifebot);
    WRITE("Silent", silent);
    WRITE("Friendly fire", friendlyFire);
    WRITE("Auto shot", autoShot);
    WRITE("ASP", autoScope);
    WRITE("Priority", priority);
    WRITE("Fov", fov);
}
static void to_json(json& j, const Config::Ragebot& o, const Config::Ragebot& dummy = {})
{
    WRITE("OV CONFIG", enabled);
    WRITE("ASS", autoStop);
    WRITE("ASMS", autoStopMod);
    WRITE("HB", hitboxes);
    WRITE("HC", hitChance);
    WRITE("HCO", OvrHitChance);
    WRITE("HCOB", hcov);
    WRITE("DMGOB", dmgov);
    WRITE("MH", multiPointHead);
    WRITE("MB", multiPointBody);
    WRITE("MD", minDamage);
    WRITE("MDO", minDamageOverride);
    WRITE("BS", betweenShots);
}

static void to_json(json& j, const Config::Chams::Material& o)
{
    const Config::Chams::Material dummy;

    to_json(j, static_cast<const Color4&>(o), dummy);
    WRITE("Enabled", enabled);
    WRITE("Health based", healthBased);
    WRITE("Blinking", blinking);
    WRITE("Wireframe", wireframe);
    WRITE("Cover", cover);
    WRITE("Ignore-Z", ignorez);
    WRITE("Material", material);
}

static void to_json(json& j, const Config::Chams& o)
{
    j["Materials"] = o.materials;
}

static void to_json(json& j, const Config::GlowItem& o, const Config::GlowItem& dummy = {})
{
    to_json(j, static_cast<const Color4&>(o), dummy);
    WRITE("Enabled", enabled);
    WRITE("Health based", healthBased);
    WRITE("Style", style);
}

static void to_json(json& j, const  Config::PlayerGlow& o, const  Config::PlayerGlow& dummy = {})
{
    WRITE("All", all);
    WRITE("Visible", visible);
    WRITE("Occluded", occluded);
}

static void to_json(json& j, const Config::Misc::Reportbot& o, const Config::Misc::Reportbot& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Target", target);
    WRITE("Delay", delay);
    WRITE("Rounds", rounds);
    WRITE("Abusive Communications", textAbuse);
    WRITE("Griefing", griefing);
    WRITE("Wall Hacking", wallhack);
    WRITE("Aim Hacking", aimbot);
    WRITE("Other Hacking", other);
}

static void to_json(json& j, const Config::RageAntiAimConfig& o, const Config::RageAntiAimConfig& dummy = {})
{
    WRITE("Pitch", pitch);
    WRITE_ENUM("Yaw base", yawBase);
    WRITE("Yaw modifier", yawModifier);
    WRITE("Yaw add", yawAdd);
    WRITE("Jitter Range", jitterRange);
    WRITE("Random Range", randomRange);
    WRITE("Spin base", spinBase);
    WRITE("At targets", atTargets);
    WRITE("Roll add", roll.add);
    WRITE("Roll", roll.enabled);
    WRITE("Roll pitchE", roll.exploitPitch);
    WRITE("Roll pitch", roll.pitch);
    WRITE("Fake flick", fakeFlick);
    WRITE("FF rate", fakeFlickRate);
    WRITE("Freestand", freestand);
    WRITE("Desync", desync);
    WRITE("Left limit", leftLimit);
    WRITE("Right limit", rightLimit);
    WRITE("Peek mode", peekMode);
    WRITE("Lby mode", lbyMode);
}

static void to_json(json& j, const Config::AntiAimConditions& o, const Config::AntiAimConditions& dummy = {})
{
    WRITE("Visualize type", visualizeType);
    WRITE("Visualize offset", visualizeOffset);
    WRITE("Visualize size", visualizeSize);
    WRITE("Visualize", visualize);
    WRITE("G", global);
    WRITE("M", moving);
    WRITE("S", slowwalk);
    WRITE("J", jumping);
    WRITE("CJ", cjump);
    WRITE("C", chrouch);
    WRITE("BREAKERS", animBreakers);
}

static void to_json(json& j, const Config::Fakelag& o, const Config::Fakelag& dummy = {})
{
    WRITE("Mode", mode);
    WRITE("Limit", limit);
}

static void to_json(json& j, const Config::Tickbase& o, const Config::Tickbase& dummy = {})
{
    WRITE("Doubletap", doubletap);
    WRITE("Hideshots", hideshots);
    WRITE("Teleport", teleport);
}

static void to_json(json& j, const Config::Backtrack& o, const Config::Backtrack& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("All ticks", allticks);
    WRITE("Time limit", timeLimit);
    WRITE("Fake Latency", fakeLatency);
    WRITE("Fake Latency Amount", fakeLatencyAmount);
}

static void to_json(json& j, const PurchaseList& o, const PurchaseList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Only During Freeze Time", onlyDuringFreezeTime);
    WRITE("Show Prices", showPrices);
    WRITE("No Title Bar", noTitleBar);
    WRITE("Mode", mode);

    if (const auto window = ImGui::FindWindowByName("Purchases")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const Config::Misc::SpectatorList& o, const Config::Misc::SpectatorList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("No Title Bar", noTitleBar);
    WRITE("Avatars", avatars);
    if (const auto window = ImGui::FindWindowByName("Spectator list")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const Config::Misc::KeyBindList& o, const Config::Misc::KeyBindList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("No Title Bar", noTitleBar);

    if (const auto window = ImGui::FindWindowByName("Keybinds list")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const Config::Misc::PlayerList& o, const Config::Misc::PlayerList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Steam ID", steamID);
    WRITE("Rank", rank);
    WRITE("Wins", wins);
    WRITE("Money", money);
    WRITE("Health", health);
    WRITE("Armor", armor);

    if (const auto window = ImGui::FindWindowByName("Player List")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const Config::Misc::JumpStats& o, const Config::Misc::JumpStats& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Show fails", showFails);
    WRITE("Show color on fail", showColorOnFail);
    WRITE("Simplify naming", simplifyNaming);
}

static void to_json(json& j, const Config::Misc::Velocity& o, const Config::Misc::Velocity& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Position", position);
    WRITE("Alpha", alpha);
    WRITE("Color", color);
}

static void to_json(json& j, const Config::Misc::KeyBoardDisplay& o, const Config::Misc::KeyBoardDisplay& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Position", position);
    WRITE("Show key Tiles", showKeyTiles);
    WRITE("Color", color);
}

static void to_json(json& j, const PreserveKillfeed& o, const PreserveKillfeed& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Only Headshots", onlyHeadshots);
}

static void to_json(json& j, const KillfeedChanger& o, const KillfeedChanger& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Headshot", headshot);
    WRITE("Dominated", dominated);
    WRITE("Revenge", revenge);
    WRITE("Penetrated", penetrated);
    WRITE("Noscope", noscope);
    WRITE("Thrusmoke", thrusmoke);
    WRITE("Attackerblind", attackerblind);
}

static void to_json(json& j, const AutoBuy& o, const AutoBuy& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Primary weapon", primaryWeapon);
    WRITE("Secondary weapon", secondaryWeapon);
    WRITE("Armor", armor);
    WRITE("Utility", utility);
    WRITE("Grenades", grenades);
}

static void to_json(json& j, const Config::Misc::Logger& o, const Config::Misc::Logger& dummy = {})
{
    WRITE("Modes", modes);
    WRITE("Events", events);
    WRITE("OFF", offset);
    WRITE("POS", position);
}

static void to_json(json& j, const Config::Visuals::MotionBlur& o, const Config::Visuals::MotionBlur& dummy)
{
    WRITE("Enabled", enabled);
    WRITE("Forward", forwardEnabled);
    WRITE("Falling min", fallingMin);
    WRITE("Falling max", fallingMax);
    WRITE("Falling intensity", fallingIntensity);
    WRITE("Rotation intensity", rotationIntensity);
    WRITE("Strength", strength);
}

static void to_json(json& j, const Config::Visuals::Fog& o, const Config::Visuals::Fog& dummy)
{
    WRITE("Start", start);
    WRITE("End", end);
    WRITE("Density", density);
}

static void to_json(json& j, const Config::Visuals::ShadowsChanger& o, const Config::Visuals::ShadowsChanger& dummy)
{
    WRITE("Enabled", enabled);
    WRITE("X", x);
    WRITE("Y", y);
}

static void to_json(json& j, const Config::Visuals::Viewmodel& o, const Config::Visuals::Viewmodel& dummy)
{
    WRITE("Enabled", enabled);
    WRITE("Fov", fov);
    WRITE("X", x);
    WRITE("Y", y);
    WRITE("Z", z);
    WRITE("Roll", roll);
}

static void to_json(json& j, const Config::Misc::Watermark& o, const Config::Misc::Watermark& dummy)
{
    WRITE("Enabled", enabled);
    WRITE("Fps", showFps);
    WRITE("Tickrate", showTicks);
    WRITE("Spotify", showSpotify);
    WRITE("Time", showTime);
    WRITE("Username", showUsername);
    WRITE("Ping", showPing);
}

static void to_json(json& j, const Config::LightsConfig::delightColor& o, const Config::LightsConfig::delightColor& dummy)
{
    WRITE("ENABLED", enabled);
    WRITE("RADIUS", raduis);
    WRITE("EXPONENT", exponent);
    WRITE("COLOR", asColor3);
}

static void to_json(json& j, const Config::LightsConfig& o)
{
    const Config::LightsConfig dummy;
    WRITE("LOCAL", local);
    WRITE("TEAM", teammate);
    WRITE("ENEMY", enemy);
}

static void to_json(json& j, const Config::Misc::Indicators& o, const Config::Misc::Indicators& dummy)
{
    WRITE("Enabled", enabled);
    WRITE("Color", color);
    WRITE("Style", style);
    WRITE("Show", toShow);
}

static void to_json(json& j, const Config::Misc::EBDetect& o, const Config::Misc::EBDetect& dummy)
{
    /*    read(j, "chat", e.chat);
    read(j, "sound", e.sound);
    read(j, "effect", e.effect);
    read(j, "effect time", e.effectTime);*/
    WRITE("chat", chat);
    WRITE("sound", sound);
    WRITE("effect", effect);
    WRITE("effect time", effectTime);
}

static void to_json(json& j, const Config::Misc& o)
{
    const Config::Misc dummy;

    WRITE("Watermark", wm);
    WRITE("Menu key", menuKey);
    WRITE("Anti AFK kick", antiAfkKick);
    WRITE("Adblock", adBlock);
    WRITE("Auto strafe", autoStrafe);
    WRITE("Auto strafe smoothness", auto_smoothnes);
    WRITE("Bunny hop", bunnyHop);
    WRITE("Clan tag", clantag);
    WRITE("Fast duck", fastDuck);
    WRITE("Moonwalk Mode", moonwalk_style);
    WRITE("Block bot", blockBot);
    WRITE("Block bot Key", blockBotKey);
    WRITE("Door spam", doorSpamKey);
    WRITE("Door spam key", doorSpam);
    WRITE("Edge Jump", edgeJump);
    WRITE("Edge Jump Key", edgeJumpKey);
    WRITE("Mini Jump", miniJump);
    WRITE("Mini Jump Crouch lock", miniJumpCrouchLock);
    WRITE("Mini Jump Key", miniJumpKey);
    WRITE("Jump Bug", jumpBug);
    WRITE("Jump Bug Key", jumpBugKey);
    WRITE("Edge Bug", edgeBug);
    WRITE("Edge Bug Key", edgeBugKey);
    WRITE("Edge bug lock", edgeBugLock);
    WRITE("Edge bug lock type", edgeBugLockType);
    WRITE("Egg bog", ebdetect);
    WRITE("Egg bog sound", customEBsound);
    WRITE("Pred Amnt", edgeBugPredAmnt);
    WRITE("Auto pixel surf", autoPixelSurf);
    WRITE("Auto pixel surf Pred Amnt", autoPixelSurfPredAmnt);
    WRITE("Auto pixel surf Key", autoPixelSurfKey);
    WRITE("Velocity", velocity);
    WRITE("Keyboard display", keyBoardDisplay);
    WRITE("Slowwalk", slowwalk);
    WRITE("Slowwalk key", slowwalkKey);
    WRITE("Slowwalk Amnt", slowwalkAmnt);
    WRITE("Fake duck", fakeduck);
    WRITE("Fake duck key", fakeduckKey);
    WRITE("Auto peek", autoPeek);
    WRITE("Auto peek style", autoPeekStyle);
    WRITE("Auto peek key", autoPeekKey);
    WRITE("Noscope crosshair", noscopeCrosshair);
    WRITE("Recoil crosshair", recoilCrosshair);
    WRITE("Auto pistol", autoPistol);
    WRITE("Auto reload", autoReload);
    WRITE("Auto accept", autoAccept);
    WRITE("Radar hack", radarHack);
    WRITE("Reveal ranks", revealRanks);
    WRITE("Reveal money", revealMoney);
    WRITE("Reveal suspect", revealSuspect);
    WRITE("Reveal votes", revealVotes);
    WRITE("Chat reveavler", chatReveavler);
    WRITE("Spectator list", spectatorList);
    WRITE("Keybind list", keybindList);
    WRITE("Player list", playerList);
    WRITE("Jump stats", jumpStats);
    WRITE("OEE", offscreenEnemies.enabled);
    if (o.offscreenEnemies.color != dummy.offscreenEnemies.color) {
        std::ostringstream s;
        s << '#' << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(o.offscreenEnemies.color[0] * 255) << std::setw(2) << static_cast<int>(o.offscreenEnemies.color[1] * 255) << std::setw(2) << static_cast<int>(o.offscreenEnemies.color[2] * 255);
        j["ColorE"] = s.str();
        j["AlphaE"] = o.offscreenEnemies.color[3];
    }
    if (o.offscreenAllies.color != dummy.offscreenAllies.color) {
        std::ostringstream s;
        s << '#' << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(o.offscreenAllies.color[0] * 255) << std::setw(2) << static_cast<int>(o.offscreenAllies.color[1] * 255) << std::setw(2) << static_cast<int>(o.offscreenAllies.color[2] * 255);
        j["ColorA"] = s.str();
        j["AlphaA"] = o.offscreenAllies.color[3];
    }
    WRITE("RainbowE", offscreenEnemies.rainbow);
    WRITE("RainbowA", offscreenAllies.rainbow);
    WRITE("Rainbow Speed E", offscreenEnemies.rainbowSpeed);
    WRITE("Rainbow Speed A", offscreenAllies.rainbowSpeed);
    WRITE("OEA", offscreenAllies.enabled);
    WRITE("OAT", offscreenAllies.type);
    WRITE("OET", offscreenEnemies.type);
    WRITE("OEOE", offscreenEnemies.outline);
    WRITE("OEOA", offscreenAllies.outline);
    WRITE("SIZEE", offscreenEnemies.size);
    WRITE("OFFSETE", offscreenEnemies.offset);
    WRITE("SIZEA", offscreenAllies.size);
    WRITE("OFFSETA", offscreenAllies.offset);
    WRITE("Disable model occlusion", disableModelOcclusion);
    WRITE("Aspect Ratio", aspectratio);
    WRITE("Kill message", killMessage);
    WRITE("Name stealer", nameStealer);
    WRITE("Disable HUD blur", disablePanoramablur);
    WRITE("Fast Stop", fastStop);
    WRITE("Bomb timer", bombTimer);
    WRITE("Hurt indicator", hurtIndicator);
    WRITE("Prepare revolver", prepareRevolver);
    WRITE("Prepare revolver key", prepareRevolverKey);
    WRITE("Hit sound", hitSound);
    WRITE("Quick healthshot key", quickHealthshotKey);
    WRITE("Grenade predict", nadePredict);
    WRITE("Grenade predict Damage", nadeDamagePredict);
    WRITE("Grenade predict Trail", nadeTrailPredict);
    WRITE("Grenade predict Circle", nadeCirclePredict);
    WRITE("Grenade predict Glow", nadeGlowPredict);
    WRITE("Fix tablet signal", fixTabletSignal);
    WRITE("Custom Hit Sound", customHitSound);
    WRITE("Kill sound", killSound);
    WRITE("Custom Kill Sound", customKillSound);
    WRITE("Purchase List", purchaseList);
    WRITE("Reportbot", reportbot);
    WRITE("Preserve Killfeed", preserveKillfeed);
    WRITE("Killfeed changer", killfeedChanger);
    WRITE("Sv pure bypass", svPureBypass);
    WRITE("Inventory Unlocker", inventoryUnlocker);
    WRITE("Autobuy", autoBuy);
    WRITE("Logger", logger);
    WRITE("Logger options", loggerOptions);
    WRITE("accent", accentColor);
    WRITE("no borders", borders);
    WRITE("Radius", autoPeekRadius);
    WRITE("Gravity force", ragdollGravity);
    WRITE("Indicators", indicators);
    //WRITE("IPOS", indicatorPos);
}

static void to_json(json& j, const Config::Visuals::OnHitHitbox& o, const Config::Visuals::OnHitHitbox& dummy)
{
    WRITE("enabled", enabled);
    WRITE("Color", color);
    WRITE("Duration", duration);
}

static void to_json(json& j, const Config::Visuals::AALines& o, const Config::Visuals::AALines& dummy)
{
    WRITE("enabled", enabled);
    WRITE("Real", real);
    WRITE("Fake", fake);
}

static void to_json(json& j, const Config::Visuals::Scope& o, const Config::Visuals::Scope& dummy)
{
    /*read<value_t::object>(j, "color", s.color);
    read(j, "offset", s.offset);
    read(j, "length", s.length);
    read(j, "fade", s.fade);
    read(j, "type", s.type);
    read(j, "b", s.removeBottom);
    read(j, "t", s.removeTop);
    read(j, "l", s.removeLeft);
    read(j, "r", s.removeRight);*/
    WRITE("color", color);
    WRITE("offset", offset);
    WRITE("length", length);
    WRITE("fade", fade);
    WRITE("type", type);
    WRITE("b", removeBottom);
    WRITE("t", removeTop);
    WRITE("l", removeLeft);
    WRITE("r", removeRight);
}

static void to_json(json& j, const Config::Visuals& o)
{
    const Config::Visuals dummy = {};
    WRITE("SCOPE", scope);
    WRITE("Party mode", partyMode);
    WRITE("AA lines", antiAimLines);
    WRITE("Hitshitado", hitMarkerColor);
    WRITE("PostEnabled", PostEnabled);
    WRITE("World exposure", worldExposure);
    WRITE("Model ambient", modelAmbient);
    WRITE("Bloom scale", bloomScale);
    WRITE("Trail", playerTrailColor);
    WRITE("Molotov Polygon", molotovPolygon);
    WRITE("World", world);
    WRITE("Props", props);
    WRITE("Sky", sky);
    WRITE("ViewmodelScope", viewmodelInScope);
    WRITE("Disable post-processing", disablePostProcessing);
    WRITE("Inverse ragdoll gravity", inverseRagdollGravity);
    WRITE("Smoke Color", smokeColor);
    WRITE("Molotov Color", molotovColor);
    WRITE("No fog", noFog);
    WRITE("Fog controller", fog);
    WRITE("Fog options", fogOptions);
    WRITE("No 3d sky", no3dSky);
    WRITE("No aim punch", noAimPunch);
    WRITE("No view punch", noViewPunch);
    WRITE("No view bob", noViewBob);
    WRITE("No hands", noHands);
    WRITE("No sleeves", noSleeves);
    WRITE("No weapons", noWeapons);
    WRITE("No smoke", noSmoke);
    WRITE("Wireframe smoke", wireframeSmoke);
    WRITE("No molotov", noMolotov);
    WRITE("Wireframe molotov", wireframeMolotov);
    WRITE("Console Color", console);
    WRITE("No blur", noBlur);
    WRITE("No scope overlay", noScopeOverlay);
    WRITE("No grass", noGrass);
    //WRITE("No shadows", noShadows);
    WRITE("Shadows changer", shadowsChanger);
    WRITE("Motion Blur", motionBlur);
    WRITE("Full bright", fullBright);
    WRITE("Zoom", zoom);
    WRITE("Zoom key", zoomKey);
    WRITE("Thirdperson", thirdperson.enable);
    WRITE("Thirdperson key", thirdperson.key);
    WRITE("Thirdperson distance", thirdperson.distance);
    WRITE("On Nade", thirdperson.disableOnGrenade);
    WRITE("Transparency in scope", thirdperson.thirdpersonTransparency);
    WRITE("While dead", thirdperson.whileDead);
    WRITE("Freecam", freeCam);
    WRITE("Freecam key", freeCamKey);
    WRITE("Freecam speed", freeCamSpeed);
    WRITE("Keep FOV", keepFov);
    WRITE("FOV", fov);
    WRITE("Far Z", farZ);
    WRITE("Flash reduction", flashReduction);
    WRITE("Skybox", skybox);
    WRITE("Custom skybox", customSkybox);
    WRITE("Deagle spinner", deagleSpinner);
    WRITE("Screen effect", screenEffect);
    WRITE("Hit effect", hitEffect);
    WRITE("Hit effect time", hitEffectTime);
    WRITE("Hit marker", hitMarker);
    WRITE("Hit marker time", hitMarkerTime);
    WRITE("Playermodel T", playerModelT);
    WRITE("Playermodel CT", playerModelCT);
    if (o.playerModel[0])
        j["Custom Playermodel"] = o.playerModel;
    WRITE("Disable jiggle bones", disableJiggleBones);
    WRITE("Bullet Tracers", bulletTracers);
    WRITE("SPRITE", bulletSprite);
    WRITE("TYPE", bulletEffects);
    WRITE("WIDTH", bulletWidth);
    WRITE("AMP", amplitude);
    WRITE("EFFECTS", bulletEffects);
    WRITE("Bullet Impacts", bulletImpacts);
    WRITE("Bullet Impacts time", bulletImpactsTime);
    WRITE("Molotov Hull", molotovHull);
    WRITE("Smoke Hull", smokeHull);
    WRITE("Viewmodel", viewModel);
    WRITE("Spread circle", spreadCircle);
    WRITE("Asus walls", asusWalls);
    WRITE("Asus props", asusProps);
    WRITE("Smoke timer", smokeTimer);
    WRITE("Smoke timer BG", smokeTimerBG);
    WRITE("Smoke timer TIMER", smokeTimerTimer);
    WRITE("Smoke timer TEXT", smokeTimerText);
    WRITE("Molotov timer", molotovTimer);
    WRITE("Molotov timer BG", molotovTimerBG);
    WRITE("Molotov timer TIMER", molotovTimerTimer);
    WRITE("Molotov timer TEXT", molotovTimerText);
    WRITE("Footstep beamsE", footstepBeamsE);
    WRITE("Footstep beamsA", footstepBeamsA);
    WRITE("Footstep beams radiusE", footstepBeamRadiusE);
    WRITE("Footstep beams radiusA", footstepBeamRadiusA);
    WRITE("Footstep beams thicknessE", footstepBeamThicknessE);
    WRITE("Footstep beams thicknessA", footstepBeamThicknessA);
    WRITE("Taser range", TaserRange);
    WRITE("Knife range", KnifeRange);
    WRITE("Hitbox on Hit", onHitHitbox);
}

static void to_json(json& j, const ImVec4& o)
{
    j[0] = o.x;
    j[1] = o.y;
    j[2] = o.z;
    j[3] = o.w;
}

static void to_json(json& j, const sticker_setting& o)
{
    const sticker_setting dummy;

    WRITE("Kit", kit);
    WRITE("Wear", wear);
    WRITE("Scale", scale);
    WRITE("Rotation", rotation);
}

static void to_json(json& j, const item_setting& o)
{
    const item_setting dummy;

    WRITE("Enabled", enabled);
    WRITE("Definition index", itemId);
    WRITE("Quality", quality);
    WRITE("Paint Kit", paintKit);
    WRITE("Definition override", definition_override_index);
    WRITE("Seed", seed);
    WRITE("StatTrak", stat_trak);
    WRITE("Wear", wear);
    if (o.custom_name[0])
        j["Custom name"] = o.custom_name;
    WRITE("Stickers", stickers);
}

static void to_json(json& j, const Config::NewESP::Box& o, const Config::NewESP::Box& dummy)
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("TYPE", type);
}

static void to_json(json& j, const Config::NewESP::Shared& o, const Config::NewESP::Shared& dummy)
{
    WRITE("ENABLED", enable);
    WRITE("NAME", name);
    WRITE("BOX", box);
    WRITE("SNAPLINE", snapline);
}

static void to_json(json& j, const Config::NewESP::Weapon& o, const Config::NewESP::Weapon& dummy)
{
    to_json(j, static_cast<const Config::NewESP::Shared&>(o), dummy);
    WRITE("AMMO", ammo);
    WRITE("ICON", icon);
}

static void to_json(json& j, const Config::NewESP::HealthBar& o, const Config::NewESP::HealthBar& dummy)
{
    WRITE("ENABLED", enabled);
    WRITE("OUTLINE", outline);
    WRITE("BACKGROUND", background);
    WRITE("STYLE", style);
    WRITE("TEXT", text);
    WRITE("SOLID", solid);
    WRITE("BOTTOM", bottom);
    WRITE("TOP", top);
}

static void to_json(json& j, const Config::NewESP::AmmoBar& o, const Config::NewESP::AmmoBar& dummy)
{
    WRITE("ENABLED", enabled);
    WRITE("OUTLINE", outline);
    WRITE("BACKGROUND", background);
    WRITE("STYLE", style);
    WRITE("TEXT", text);
    WRITE("SOLID", solid);
    WRITE("LEFT", left);
    WRITE("RIGHT", right);
}

static void to_json(json& j, const Config::NewESP::Player& o, const Config::NewESP::Player& dummy)
{
    WRITE("ENABLED", enable);
    WRITE("BOX", box);
    WRITE("SKELETON", skeleton);
    WRITE("SNAPLINE", snapline);
    WRITE("NAME", name);
    WRITE("WEAPON", weapon);
    WRITE("WEAPONI", weaponIcon);
    WRITE("FLAGS", flags);
    WRITE("HEALTH", health);
    WRITE("AMMO", ammo);
    WRITE("DORMANT", dormant);
    WRITE("DORMANT TIME", dormantTime);
    WRITE("SA", showArmor);
    WRITE("SB", showBomb);
    WRITE("SK", showKit);
    WRITE("SP", showPin);
    WRITE("SR", showReload);
    WRITE("SS", showScoped);
    WRITE("SF", showFD);
    WRITE("SM", showMoney);
    WRITE("SFF", showFlashed);
}

static void to_json(json& j, const Config::NewESP::Projectiles& o, const Config::NewESP::Projectiles& dummy)
{
    to_json(j, static_cast<const Config::NewESP::Shared&>(o), dummy);
    WRITE("TRAILS", trails);
}

static void to_json(json& j, const Config::NewESP& o)
{
    const Config::NewESP dummy;
    WRITE("ENEMY", enemy);
    WRITE("ALLIES", allies);
    WRITE("WEAPONS", weapons);
    WRITE("PROJECTILES", projectiles);
}
#pragma endregion

void removeEmptyObjects(json& j) noexcept
{
    for (auto it = j.begin(); it != j.end();) {
        auto& val = it.value();
        if (val.is_object() || val.is_array())
            removeEmptyObjects(val);
        if (val.empty() && !j.is_array())
            it = j.erase(it);
        else
            ++it;
    }
}

void Config::save(size_t id) const noexcept
{
    createConfigDir();

    if (std::ofstream out{ path / (const char8_t*)configs[id].c_str() }; out.good()) {
        json j;

        to_json(j["Legitbot Key"], legitbotKey, KeyBind::NONE);
        j["Recoil control system"] = recoilControlSystem;
        to_json(j["Force baim"], forceBaim, KeyBind::NONE);
        j["LegitbotGlobal"] = lgb;
        j["LegitbotConfig"] = legitbob;
        j["RagebotGlobal"] = ragebot;
        j["RagebotConfig"] = rageBot;
        j["SEX"] = condAA;
        to_json(j["Ragebot Key"], ragebotKey, KeyBind::NONE);
        to_json(j["Min damage override Key"], minDamageOverrideKey, KeyBind::NONE);
        to_json(j["Freestand key"], freestandKey, KeyBind::NONE);
        to_json(j["Triggerbot Key"], triggerbotKey, KeyBind::NONE);
        to_json(j["Flip Flick"], flipFlick, KeyBind::NONE);
        to_json(j["Manual forward Key"], manualForward, KeyBind::NONE);
        to_json(j["Manual backward Key"], manualBackward, KeyBind::NONE);
        to_json(j["Manual right Key"], manualRight, KeyBind::NONE);
        to_json(j["Manual left Key"], manualLeft, KeyBind::NONE);
        to_json(j["Fake flick key"], fakeFlickOnKey, KeyBind::NONE);
        to_json(j["Invert"], invert, KeyBind::NONE);
        to_json(j["HCOK"], hitchanceOverride, KeyBind::NONE);
        j["Dlights"] = dlightConfig;
        j["Rage Anti aim"] = rageAntiAim;
        j["Fakelag"] = fakelag;
        j["Menu"] = menu;
        j["Tickbase"] = tickbase;
        j["Backtrack"] = backtrack;

        j["Glow"]["Items"] = glow;
        j["Glow"]["Players"] = playerGlow;

        j["Chams"] = chams;
        j["PENIS?"] = esp;
        j["Visuals"] = visuals;
        j["Misc"] = misc;
        j["Skin changer"] = skinChanger;

        removeEmptyObjects(j);
        out << std::setw(2) << j;
    }
}

void Config::add(std::string name) noexcept
{
    if (&name && std::find(configs.cbegin(), configs.cend(), name) == configs.cend()) {
        configs.emplace_back(name);
        save(configs.size() - 1);
    }
}

void Config::remove(size_t id) noexcept
{
    std::error_code ec;
    std::filesystem::remove(path / (const char8_t*)configs[id].c_str(), ec);
    configs.erase(configs.cbegin() + id);
}

void Config::rename(size_t item, const char* newName) noexcept
{
    std::error_code ec;
    std::filesystem::rename(path / (const char8_t*)configs[item].c_str(), path / (const char8_t*)newName, ec);
    configs[item] = newName;
}

void Config::reset() noexcept
{
    legitbob = { };
    lgb = { };
    recoilControlSystem = { };
    ragebot = { };
    rageBot = { };
    condAA = { };
    menu = { };
    dlightConfig = { };
    rageAntiAim = { };
    fakelag = { };
    tickbase = { };
    esp = { };
    backtrack = { };
    Glow::resetConfig();
    chams = { };
    visuals = { };
    skinChanger = { };
    misc = { };
    menu = { };
}

void Config::listConfigs() noexcept
{
    configs.clear();
    std::error_code ec;
    std::transform(std::filesystem::directory_iterator{ path, ec },
        std::filesystem::directory_iterator{ },
        std::back_inserter(configs),
        [](const auto& entry) 
        { 
            return std::string{ (const char*)entry.path().filename().u8string().c_str() 
        }; });
}

void Config::createConfigDir() const noexcept
{
    std::error_code ec; std::filesystem::create_directory(path, ec);
}

void Config::openConfigDir() const noexcept
{
    createConfigDir();
    ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void Config::scheduleFontLoad(const std::string& name) noexcept
{
    scheduledFonts.push_back(name);
}

static auto getFontData(const std::string& fontName) noexcept
{
    HFONT font = CreateFontA(0, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, fontName.c_str());

    std::unique_ptr<std::byte[]> data;
    DWORD dataSize = GDI_ERROR;

    if (font) {
        HDC hdc = CreateCompatibleDC(nullptr);

        if (hdc) {
            SelectObject(hdc, font);
            dataSize = GetFontData(hdc, 0, 0, nullptr, 0);

            if (dataSize != GDI_ERROR) {
                data = std::make_unique<std::byte[]>(dataSize);
                dataSize = GetFontData(hdc, 0, 0, data.get(), dataSize);

                if (dataSize == GDI_ERROR)
                    data.reset();
            }
            DeleteDC(hdc);
        }
        DeleteObject(font);
    }
    return std::make_pair(std::move(data), dataSize);
}

bool Config::loadScheduledFonts() noexcept
{
    bool result = false;

    for (const auto& fontName : scheduledFonts) {
        if (fontName == "Default") {
            if (fonts.find("Default") == fonts.cend()) {
                ImFontConfig cfg;
                cfg.OversampleH = cfg.OversampleV = 1;
                cfg.PixelSnapH = true;
                cfg.RasterizerMultiply = 1.7f;

                Font newFont;

                cfg.SizePixels = 13.0f;
                newFont.big = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                cfg.SizePixels = 10.0f;
                newFont.medium = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                cfg.SizePixels = 8.0f;
                newFont.tiny = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                fonts.emplace(fontName, newFont);
                result = true;
            }
            continue;
        }

        const auto [fontData, fontDataSize] = getFontData(fontName);
        if (fontDataSize == GDI_ERROR)
            continue;

        if (fonts.find(fontName) == fonts.cend()) {
            const auto ranges = Helpers::getFontGlyphRanges();
            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;
            cfg.RasterizerMultiply = 1.7f;

            Font newFont;
            newFont.tiny = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 8.0f, &cfg, ranges);
            newFont.medium = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 10.0f, &cfg, ranges);
            newFont.big = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 13.0f, &cfg, ranges);
            fonts.emplace(fontName, newFont);
            result = true;
        }
    }
    scheduledFonts.clear();
    return result;
}
