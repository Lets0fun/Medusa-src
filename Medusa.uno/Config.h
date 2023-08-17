#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "imgui/imgui.h"

#include "../SkinChanger.h"

#include "ConfigStructs.h"
#include "InputUtil.h"

class Config {
public:
    Config() noexcept;
    void load(size_t, bool incremental) noexcept;
    void load(const char8_t* name, bool incremental) noexcept;
    void save(size_t) const noexcept;
    void add(std::string) noexcept;
    void remove(size_t) noexcept;
    void rename(size_t, const char*) noexcept;
    void reset() noexcept;
    void listConfigs() noexcept;
    void createConfigDir() const noexcept;
    void openConfigDir() const noexcept;

    constexpr auto& getConfigs() noexcept
    {
        return configs;
    }

    struct RagebotGlobal {
        bool enabled{ false };
        bool knifebot{ false };
        bool silent{ false };
        bool friendlyFire{ false };
        bool autoShot{ false };
        int priority{ 0 };        
        bool autoScope{ false };
        float fov{ 0.0f };
    } ragebot;
    struct Ragebot {
        bool enabled{ false };
        bool autoStop{ false };
        int autoStopMod{ 0 };
        bool betweenShots{ false };
        int hitboxes{ 0 };
        int hitChance{ 50 };
        bool hcov{ false };
        bool dmgov{ false };
        int OvrHitChance{ 50 };
        int multiPointHead{ 0 };
        int multiPointBody{ 0 };
        int minDamage{ 1 };
        int minDamageOverride{ 1 };
    };
    std::array<Ragebot, 11> rageBot;
    struct LegitbotGlobal {
        bool enabled{ false };
        bool aimlock{ false };
        bool friendlyFire{ false };
        bool visibleOnly{ true };
        bool scopedOnly{ true };
        bool ignoreFlash{ false };
        bool ignoreSmoke{ false };
        bool killshot{ false };
        bool enableTriggerbot{ false };
        ColorToggleOutline legitbotFov{ 1.0f, 1.0f, 1.0f, 0.25f };
    }lgb;
    struct Legitbob {
        bool override{ false };
        int reactionTime{ 100 };
        float smooth{ 1.0f };
        int hitboxes{ 0 };
        int hitchanceT{ 50 };
        int minDamage{ 1 };
        bool betweenShots{ true };
        float fov{ 0.0f };
    };
    std::array<Legitbob, 40> legitbob;

    struct RecoilControlSystem {
        bool enabled{ false };
        bool silent{ false };
        int shotsFired{ 0 };
        float horizontal{ 1.0f };
        float vertical{ 1.0f };
    };
    std::array<RecoilControlSystem, 40> recoilControlSystem;

    KeyBind ragebotKey{ std::string("Ragebot") };
    KeyBind minDamageOverrideKey{ std::string("Damage override"), KeyMode::Off };
    KeyBind forceBaim{ std::string("Force body aim"), KeyMode::Off };

    struct Fakelag {
        int mode = 0;
        int limit = 1;
    } fakelag;
    struct AntiAimConditions {
        bool global = false;
        bool moving = false;
        bool slowwalk = false;
        bool jumping = false;
        bool cjump = false;
        bool onUse = false;
        bool chrouch = false;
        ColorToggleOutline visualize{ 1.0f , 1.0f , 1.0f , 0.5f };
        float visualizeOffset = 100.f;
        int visualizeType = 0;
        float visualizeSize = 35.f;
        int animBreakers = 0;
    }condAA;
    struct RageAntiAimConfig {
        int pitch = 0; //Off, Down, Zero, Up
        Yaw yawBase = Yaw::off;
        int yawModifier = 0; //Off, Jitter
        int yawAdd = 0; //-180/180
        float spinBase = 0; //-180/180
        int jitterRange = 0;
        int tickDelays = 2;
        int randomRange = 0;
        bool atTargets = false;
        bool fakeFlick{ false };
        int fakeFlickRate = 16;
        bool freestand{ false };
        //desync
        int leftLimit = 60;
        int rightLimit = 60;
        int peekMode = 0; //Off, Peek real, Peek fake, jitter
        int lbyMode = 0; // Normal, Opposite, sway
        bool desync = false;
        struct Roll {
            bool enabled{ false };
            float offset = 0;
            float pitch = 0;
            float add = 0;
            bool exploitPitch{ false };
            float epAmnt{ 0 };
        };
        Roll roll;
    };     
    std::array<RageAntiAimConfig, 6> rageAntiAim;
    bool disableInFreezetime{ false };
    KeyBind hitchanceOverride{ std::string("Hitchance ovr"), KeyMode::Off };
    KeyBind manualForward{ std::string("Manual forward"), KeyMode::Off },
        manualBackward{ std::string("Manual backward"), KeyMode::Off },
        manualRight{ std::string("Manual right"), KeyMode::Off },
        manualLeft{ std::string("Manual left"), KeyMode::Off };
    KeyBind freestandKey{ std::string("Freestand") };
    KeyBind flipFlick{ std::string("Fake flick flipped") };
    KeyBind fakeFlickOnKey{ std::string("Fake flick") };
    KeyBind invert{ std::string("AA invert") };

    struct Tickbase {
        KeyBind doubletap{ std::string("Double Tap"), KeyMode::Off };
        KeyBind hideshots{ std::string("Hide Shots"), KeyMode::Off };
        bool teleport{ false };
    } tickbase;
    KeyBind legitbotKey{ std::string("Legitbot") };

    KeyBind triggerbotKey{ std::string("TriggerBot") };

    struct Backtrack {
        bool enabled = false;
        int timeLimit = 200;
        bool fakeLatency = false;
        int fakeLatencyAmount = 200;
        bool allticks{ false };
    } backtrack;

    struct Chams {
        struct Material : Color4 {
            bool enabled = false;
            bool healthBased = false;
            bool blinking = false;
            bool wireframe = false;
            bool cover = false;
            bool ignorez = false;
            int material = 0;
        };
        std::array<Material, 7> materials;
    };

    std::unordered_map<std::string, Chams> chams;

    struct GlowItem : Color4 {
        bool enabled = false;
        bool healthBased = false;
        int style = 0;
    };

    struct PlayerGlow {
        GlowItem all, visible, occluded;
    };

    std::unordered_map<std::string, PlayerGlow> playerGlow;
    std::unordered_map<std::string, GlowItem> glow;

    struct NewESP
    {
        struct Box : ColorToggle
        {
            int type = 0; //solid, corners
        };
        struct Shared {
            bool enable{ false };
            Box box;
            ColorToggle name;
            ColorToggle icon;
            ColorToggle snapline;
        };
        struct HealthBar
        {
            bool enabled{ false };
            bool outline{ true };
            bool background{ true };
            ColorToggle text;
            Color4 solid;
            Color4 top;
            Color4 bottom;
            int style = 0; //solid, gradient, health based
        };
        struct AmmoBar
        {
            bool enabled{ false };
            bool outline{ true };
            bool background{ true };
            ColorToggle text;
            Color4 solid;
            Color4 left;
            Color4 right;
            int style = 0; //solid, gradient
        };
        struct Player {
            bool enable{ false };
            Box box;
            ColorToggle skeleton;
            ColorToggle name;
            ColorToggle snapline;
            ColorToggle weapon;
            ColorToggle weaponIcon;
            ColorToggle flags;
            HealthBar health;
            AmmoBar ammo;
            bool dormant{ false };
            float dormantTime = 5.f;
            bool showArmor = true;
            bool showBomb = true;
            bool showKit = true;
            bool showFD = true;
            bool showReload = true;
            bool showScoped = true;
            bool showMoney = true;
            bool showFlashed = true;
            bool showPin = true;
        };
        struct Weapon : Shared {
            ColorToggle ammo;
        };
        struct Projectiles : Shared {
            ColorToggle trails;
        };
        Player enemy;
        Player allies;
        Player local;
        Weapon weapons;
        Projectiles projectiles;
    } esp;
    struct Font {
        ImFont* tiny;
        ImFont* medium;
        ImFont* big;
    };

    struct Visuals {
        bool partyMode{ false };
        ColorToggle footstepBeamsE{ 1.f, 1.0f, 1.f, 1.0f };
        int footstepBeamRadiusE = 250;
        int footstepBeamThicknessE = 2;
        ColorToggle footstepBeamsA{ 1.f, 1.0f, 1.f, 1.0f };
        int footstepBeamRadiusA = 250;
        int footstepBeamThicknessA = 2;
        std::string customSkybox;
        ColorToggle KnifeRange;
        ColorToggle TaserRange;
        bool disablePostProcessing{ false };
        bool inverseRagdollGravity{ false };
        bool noFog{ false };
        struct Fog
        {
            float start{ 0 };
            float end{ 0 };
            float density{ 0 };
        } fogOptions;
        ColorToggle3 fog;
        bool no3dSky{ false };
        bool noAimPunch{ false };
        bool noViewPunch{ false };
        bool noViewBob{ false };
        bool noHands{ false };
        bool noSleeves{ false };
        bool noWeapons{ false };
        bool noSmoke{ false };
        bool wireframeSmoke{ false };
        bool noMolotov{ false };
        bool wireframeMolotov{ false };
        bool noBlur{ false };
        bool noScopeOverlay{ false };
        bool noGrass{ false };
        bool noShadows{ false };
        struct ShadowsChanger
        {
            bool enabled{ false };
            int x{ 0 };
            int y{ 0 };
        } shadowsChanger;
        bool fullBright{ false };
        bool zoom{ false };
        KeyBind zoomKey{ std::string("Zoom") };
        bool freeCam{ false };
        KeyBind freeCamKey{ std::string("Freecam") };
        int freeCamSpeed{ 2 };
        bool keepFov{ false };
        int fov{ 0 };
        int farZ{ 0 };
        int flashReduction{ 0 };
        int skybox{ 0 };
        bool deagleSpinner{ false };
        struct MotionBlur
        {
            bool enabled{ false };
            bool forwardEnabled{ false };
            float fallingMin{ 10.0f };
            float fallingMax{ 20.0f };
            float fallingIntensity{ 1.0f };
            float rotationIntensity{ 1.0f };
            float strength{ 1.0f };
        } motionBlur;
        int screenEffect{ 0 };
        int hitEffect{ 0 };
        float hitEffectTime{ 0.6f };
        int hitMarker{ 0 };
        ColorToggle3 hitMarkerColor{ 1, 1, 1 };
        float hitMarkerTime{ 0.6f };
        ColorToggle bulletImpacts{ 0.0f, 0.0f, 1.f, 0.5f };
        float bulletImpactsTime{ 4.f };
        int playerModelT{ 0 };
        int playerModelCT{ 0 };
        char playerModel[256] { };
        bool disableJiggleBones{ false };
        BulletTracers bulletTracers;
        float bulletWidth = 0.55f;
        int bulletSprite = 0;
        int bulletEffects = 0;
        float amplitude = 5.0f;
        ColorToggle molotovHull{ 1.0f, 0.27f, 0.0f, 0.3f };
        ColorToggleOutline smokeHull{ 0.5f, 0.5f, 0.5f, 0.3f };
        ColorToggle molotovColor{ 1.0f, 0.27f, 0.0f, 0.5f };
        ColorToggle smokeColor{ .75f, .75f, .75f, 0.5f };
        struct Viewmodel
        {
            bool enabled { false };
            int fov{ 0 };
            float x { 0.0f };
            float y { 0.0f };
            float z { 0.0f };
            float roll { 0.0f };
        } viewModel;
        ColorToggle spreadCircle { .0f, .0f, .0f, 0.65f };
        ColorToggle3 world{ 0.5f, 0.5f, 0.5f };
        ColorToggle3 props{ 0.75f, 0.75f, 0.75f };
        ColorToggle3 sky;
        int asusWalls = 100;
        int asusProps = 100;
        bool smokeTimer{ false };
        Color4 smokeTimerBG{ 0.0f, 0.0f, 0.0f, 0.5f };
        Color4 smokeTimerTimer{ 1.0f, 1.0f, 1.0f, 1.0f };
        Color4 smokeTimerText{ 1.0f, 1.0f, 1.0f, 1.0f };
        bool molotovTimer{ false };
        Color4 molotovTimerBG{ 0.0f, 0.0f, 0.0f, 0.5f };
        Color4 molotovTimerTimer{ 1.0f, 1.0f, 1.0f, 1.0f };
        Color4 molotovTimerText{ 1.0f, 1.0f, 1.0f, 1.0f };
        //custom
        ColorToggle molotovPolygon{ 1.0f, 0.27f, 0.0f, 0.3f };
        bool viewmodelInScope{ false };
        ColorToggle3 playerTrailColor{ 1.0f, 1.0f, 1.0f };
        bool PostEnabled = false;
        float worldExposure = 0.0f;
        float modelAmbient = 0.0f;
        float bloomScale = 0.0f;
        struct Thirdperson
        {
            bool enable{ false };
            KeyBind key{ std::string("Thirdperson") };
            int distance{ 100 };
            bool animated{ true };
            bool whileDead{ false };
            bool disableOnGrenade{ false };
            float thirdpersonTransparency = 0.f;
        }thirdperson;
        ColorToggle console{ 1.0f, 1.0f, 1.0f, 1.0f };
        struct Scope
        {
            Color4 color{ 1.f, 1.f, 1.f, 1.f };
            bool fade{ true };
            int offset{ 10 };
            int length{ 50 };
            int type{ 0 };
            bool removeTop = false;
            bool removeBottom = false;
            bool removeLeft = false;
            bool removeRight = false;
        }scope;
        struct OnHitHitbox
        {
            bool enabled{ false };
            Color4 color{ 1.f, 0.5f, 0.5f, 1.f };
            float duration = 2.f;
        }onHitHitbox;
        ColorToggle damageMarker;
        struct AALines {
            bool enabled{ false };
            Color3 real{ 1.f, 0.f, 1.f };
            Color3 fake{ 0.f, 1.f, 1.f };
        }antiAimLines;
    } visuals;
    std::array<item_setting, 36> skinChanger;



    struct Misc {
        Misc() { clanTag[0] = '\0'; name[0] = '\0'; menuKey.keyMode = KeyMode::Toggle; }
        Color4 accentColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        std::string test1;
        bool borders{ true };
        bool chatReveavler{ false };
        KeyBind menuKey = KeyBind::INSERT;
        float autoPeekRadius = 15.f;
        bool antiAfkKick{ false };
        bool adBlock{ false };
        bool autoStrafe{ false };
        bool bunnyHop{ false };
        int bunnyhopType = 0;
        int bhHc = 100;
        //customstart
        int autoPeekStyle{ 0 };
        struct EBDetect
        {
            bool chat{ true };
            int sound{ 0 };
            bool effect{ 0 };
            float effectTime{ 1.f };
        }ebdetect;
        struct Indicators 
        {
            int style = 0;
            bool enabled = false;
            int toShow = 0;
            Color4 color;
        }indicators;
        bool IshowManual{ false };
        bool IshowDesync{ false };
        bool IshowRageBot{ false };
        bool IshowLegitBot{ false };
        bool IshowMisc{ false };
        int moonwalk_style = 0;
        int auto_smoothnes = 50;
        bool clantag{ false };
        bool doorSpam{ false };
        KeyBind doorSpamKey{ std::string("Door spam") };
        //customend
        bool fastDuck{ false };
        bool blockBot{ false };
        KeyBind blockBotKey{ std::string("Block Bot") };
        bool edgeJump{ false };
        KeyBind edgeJumpKey{ std::string("Edge Jump") };
        bool miniJump{ false };
        int miniJumpCrouchLock{ 0 };
        KeyBind miniJumpKey{ std::string("Mini Jump") };
        bool jumpBug{ false };
        KeyBind jumpBugKey{ std::string("Jump Bug") };
        bool edgeBug{ false };
        bool mask{ false };
        int edgeBugPredAmnt{ 20 };
        bool advancedDetectionEB{ false };
        int edgeBugLockType = 0;
        float edgeBugLock = 0.0f;
        KeyBind edgeBugKey{ std::string("Edge Bug") };
        bool autoPixelSurf{ false };
        int autoPixelSurfPredAmnt{ 2 };
        KeyBind autoPixelSurfKey{ std::string("Auto Pixel Surf") };
        bool slowwalk{ false };
        int slowwalkAmnt{ 0 };
        KeyBind slowwalkKey{ std::string("Slow Walk") };
        bool fakeduck{ false };
        KeyBind fakeduckKey{ std::string("Fake Duck") };
        ColorToggle autoPeek{ 1.0f, 1.0f, 1.0f, 1.0f };
        KeyBind autoPeekKey{ std::string("Auto Peek") };
        bool autoPistol{ false };
        bool autoReload{ false };
        bool autoAccept{ false };
        bool radarHack{ false };
        bool revealRanks{ false };
        bool revealMoney{ false };
        bool revealSuspect{ false };
        bool revealVotes{ false };
        bool disableModelOcclusion{ false };
        bool nameStealer{ false };
        bool disablePanoramablur{ false };
        bool killMessage{ false };
        bool nadePredict{ false };
        bool fixTabletSignal{ false };
        bool fastStop{ false };
        bool prepareRevolver{ false };
        bool svPureBypass{ true };
        bool inventoryUnlocker{ false };
        KillfeedChanger killfeedChanger;
        PreserveKillfeed preserveKillfeed;
        char clanTag[16];
        char name[16];
        bool noscopeCrosshair{false};
        bool recoilCrosshair{false};
        ColorToggleThickness nadeDamagePredict;
        Color4 nadeTrailPredict{ 1.f, 1.f, 1.f, 1.f };
        Color4 nadeCirclePredict{ 1.f, 1.f, 1.f, 1.f };
        ColorToggle nadeGlowPredict{ 1.f, 1.f, 1.f, 0.5f };
        struct Watermark {
            bool enabled{ true };
            bool background{ true };
            bool showFps{ true };
            bool showPing{ true };
            bool showUsername{ true };
            bool showTime{ true };
            bool showTicks{ true };
            bool showSpotify{ true };
        };
        Watermark wm;
        struct SpectatorList {
            bool enabled = false;
            bool noTitleBar = false;
            bool avatars = false;
            ImVec2 pos;
        };

        SpectatorList spectatorList;

        struct KeyBindList {
            bool enabled = false;
            bool noTitleBar = false;
            ImVec2 pos;
        };

        KeyBindList keybindList;

        struct Logger {
            int modes{ 0 };
            int events{ 0 };
            int position{ 0 };
            int offset{ 120 };
        };

        Logger loggerOptions;

        ColorToggle3 logger;
        bool snowflakes{ false };
        float aspectratio{ 0 };
        ColorToggle3 bombTimer{ 1.0f, 0.55f, 0.0f };
        ColorToggle3 hurtIndicator{ 0.0f, 0.8f, 0.7f };
        KeyBind prepareRevolverKey{ std::string("Prepare revolver") };
        int hitSound{ 0 };
        int quickHealthshotKey{ 0 };
        int killSound{ 0 };
        std::string customKillSound;
        std::string customHitSound;
        std::string customEBsound;
        PurchaseList purchaseList;

        struct Reportbot {
            bool enabled = false;
            bool textAbuse = false;
            bool griefing = false;
            bool wallhack = true;
            bool aimbot = true;
            bool other = true;
            int target = 0;
            int delay = 1;
            int rounds = 1;
        } reportbot;

        struct PlayerList {
            bool enabled = false;
            bool steamID = false;
            bool rank = false;
            bool wins = false;
            bool money = true;
            bool health = true;
            bool armor = false;

            ImVec2 pos;
        };
        int ragdollGravity{ 0 };
        PlayerList playerList;
        struct Offscreen : ColorToggleOutline
        {
            int size = 25;
            int offset = 250;
            int type = 0;
        };
        Offscreen offscreenEnemies;
        Offscreen offscreenAllies;
        AutoBuy autoBuy;

        struct JumpStats {
            bool enabled = false;
            bool showFails = true;
            bool showColorOnFail = false;
            bool simplifyNaming = false;
        } jumpStats;

        struct Velocity {
            bool enabled = false;
            float position{ 0.9f };
            float alpha{ 1.0f };
            ColorToggle color{ 1.0f, 1.0f, 1.0f, 1.0f };
        } velocity;

        struct KeyBoardDisplay {
            bool enabled = false;
            float position{ 0.8f };
            bool showKeyTiles = false;
            Color4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
        } keyBoardDisplay;
    } misc;
    struct LightsConfig
    {
        struct delightColor : private Color3 {
            bool enabled = false;
            float raduis = 150.0f;
            int exponent = 5;
            int style = 0;
            Color3 asColor3;
        };
        delightColor enemy;
        delightColor local;
        delightColor teammate;
    }dlightConfig;

    struct Menu
    {
        Color4 accentColor;
        int windowStyle = 0;
        float transparency = 60.f;
    }menu;

    struct Dev
    {
        int index;
        int value;
        bool enaled;
    }dev;
    bool predTest{ false };
    void scheduleFontLoad(const std::string& name) noexcept;
    bool loadScheduledFonts() noexcept;
    const auto& getSystemFonts() noexcept { return systemFonts; }
    const auto& getFonts() noexcept { return fonts; }
private:
    std::vector<std::string> scheduledFonts{ "Default" };
    std::vector<std::string> systemFonts{ "Default" };
    std::unordered_map<std::string, Font> fonts;
    std::filesystem::path path;
    std::vector<std::string> configs;
};
inline std::unique_ptr<Config> config;
