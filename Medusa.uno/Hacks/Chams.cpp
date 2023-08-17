#include <cstring>
#include <functional>

#include "../Config.h"
#include "../Helpers.h"
#include "../Hooks.h"
#include "../Interfaces.h"

#include "Animations.h"
#include "Backtrack.h"
#include "Chams.h"

#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/StudioRender.h"
#include "../SDK/KeyValues.h"
#include "../xor.h"
#include "../includes.hpp"
#include "../SDK/RenderView.h"
#include "../SDK/Input.h"
UserCmd* cmdsex;
int goofy1;
void Sex::GetCmd(UserCmd* cmd)
{
    cmdsex = cmd;
    goofy1 = get_moving_flag(cmd);
}

auto health(Entity* entity)
{
    return entity->health();
}
static Material* normal;
static Material* flat;
static Material* animated;
static Material* animated2;
static Material* glass;
static Material* crystal;
static Material* chrome;
static Material* plastic;
static Material* glow;
static Material* fractal;
static Material* pearlescent;
static Material* metallic;
static Material* snowflakes;
static Material* tiedye;

static constexpr auto dispatchMaterial(int id) noexcept
{
    switch (id) {
    default:
    case 0: return normal;
    case 1: return flat;
    case 2: return animated;
    case 3: return animated2;
    case 4: return glass;
    case 5: return chrome;
    case 6: return crystal;
    case 7: return plastic;
    case 8: return glow;
    case 9: return fractal;
    case 10: return pearlescent;
    case 11: return metallic;
    case 12: return snowflakes;
    case 13
    : return tiedye;
    }
}

static void initializeMaterials() noexcept
{
    normal = interfaces->materialSystem->createMaterial("normal", KeyValues::fromString("VertexLitGeneric", c_xor("$nofog 1")));
    flat = interfaces->materialSystem->createMaterial("flat", KeyValues::fromString("UnlitGeneric", c_xor("$nofog 1")));
    chrome = interfaces->materialSystem->createMaterial("chrome", KeyValues::fromString("VertexLitGeneric", "$envmap env_cubemap"));
    glow = interfaces->materialSystem->createMaterial("glow", KeyValues::fromString("VertexLitGeneric", "$additive 1 $nofog 1 $envmap models/effects/cube_white $envmapfresnel 1 $alpha .8"));
    pearlescent = interfaces->materialSystem->createMaterial("pearlescent", KeyValues::fromString("VertexLitGeneric", "$basetexture vgui/white_additive $ambientonly 1 $phong 1 $pearlescent 3 $basemapalphaphongmask 1"));
    metallic = interfaces->materialSystem->createMaterial("metallic", KeyValues::fromString("VertexLitGeneric", "$basetexture white $ignorez 0 $envmap env_cubemap $normalmapalphaenvmapmask 1 $envmapcontrast 1 $nofog 1 $model 1 $nocull 0 $selfillum 1 $halfambert 1 $znearer 0 $flat 1"));
    fractal = interfaces->materialSystem->createMaterial("fractal", KeyValues::fromString("VertexLitGeneric", c_xor("$nocull 1 $nofog 1 $bumpmap models/weapons/customization/materials/origamil_camo")));                                                      /*$envmap env_cubemap $rimlight 1 $rimlightexponent 9999999 $rimlightboost 0 $selfillum 1 $phong 1 $basemapalphaphongmask 1 $phongboost 0*/
    tiedye = interfaces->materialSystem->createMaterial("tiedye", KeyValues::fromString("VertexLitGeneric", c_xor("$basetexture models/weapons/customization/paints/anodized_multi/smoke $nofog 1 $noambient 1 $ignorez 1 $nocull 1 $rimlight 1 $rimlightexponent 9999999 $rimlightboost 0 $selfillum 1 $phong 1 $basemapalphaphongmask 1 $phongboost 0 Proxies { TextureScroll { textureScrollVar $basetexturetransform textureScrollRate 0.20 textureScrollAngle 45 } TextureScroll { textureScrollVar $bumptransform textureScrollRate 0.20 textureScrollAngle 45} }")));
    {
        const auto kv = KeyValues::fromString("UnlitGeneric", c_xor("$envmap editor/cube_vertigo $envmapcontrast 1 $basetexture dev/zone_warning proxies { texturescroll { texturescrollvar $basetexturetransform texturescrollrate 0.6 texturescrollangle 90 } }"));
        kv->setString("$envmaptint", "[.0 .0 .0]");
        animated = interfaces->materialSystem->createMaterial("animated", kv);
    }

    {
        const auto kv = KeyValues::fromString("UnlitGeneric", c_xor("$basetexture sprites/light_glow04 $nodecal 1 $model 1 $additive 1 $nocull 1 $wireframe 0 $AnimatedTexture 1 proxies { texturescroll { texturescrollvar $basetexturetransform texturescrollrate .90 texturescrollangle 90 } }"));
        animated2 = interfaces->materialSystem->createMaterial(c_xor("animated2"), kv);
    }

    {
        const auto kv = KeyValues::fromString("VertexLitGeneric", c_xor("$baseTexture detail/dt_metal1 $additive 1 $envmap editor/cube_vertigo"));
        //  kv->setString("$color", "[.05 .05 .05]");
        glass = interfaces->materialSystem->createMaterial("glass", kv);
    }

    {
        const auto kv = KeyValues::fromString("VertexLitGeneric", c_xor("$baseTexture black $bumpmap effects/flat_normal $translucent 1 $envmap models/effects/crystal_cube_vertigo_hdr $envmapfresnel 0 $phong 1 $phongexponent 16 $phongboost 2"));
        kv->setString("$phongtint", "[.2 .35 .6]");
        crystal = interfaces->materialSystem->createMaterial("crystal", kv);
    }

    {
        const auto kv = KeyValues::fromString("VertexLitGeneric", c_xor("$baseTexture black $bumpmap models/inventory_items/trophy_majors/matte_metal_normal $additive 1 $envmap editor/cube_vertigo $envmapfresnel 1 $normalmapalphaenvmapmask 1 $phong 1 $phongboost 20 $phongexponent 3000 $phongdisablehalflambert 1"));
        //kv->setString("$phongfresnelranges", "[.1 .4 1]");
        //kv->setString("$phongtint", "[.8 .9 1]");
        plastic = interfaces->materialSystem->createMaterial("plastic", kv);
    }

    {
        const auto kv = KeyValues::fromString("UnlitGeneric", c_xor("$basetexture dev/snowfield $additive 1 $envmap editor/cube_vertigo $envmapfresnel 1 $alpha 1 proxies { texturescroll { texturescrollvar $basetexturetransform texturescrollrate 0.12 texturescrollangle 253 } }"));
        kv->setString("$envmaptint", "[0 0 0]");
        kv->setString("$envmapfresnelminmaxexp", "[.00000 .0 .0]");
        snowflakes = interfaces->materialSystem->createMaterial(c_xor("snowflakes"), kv);
    }

}

bool Chams::render(void* ctx, void* state, const ModelRenderInfo& info, matrix3x4* customBoneToWorld) noexcept
{
    static bool materialsInitialized = false;
    if (!materialsInitialized) {
        initializeMaterials();
        materialsInitialized = true;
    }

    appliedChams = false;
    this->ctx = ctx;
    this->state = state;
    this->info = &info;
    this->customBoneToWorld = customBoneToWorld;

    if (std::string_view{ info.model->name }.starts_with("models/weapons/w_"))
    {
        if (std::strstr(info.model->name + 17, "sleeve"))
            applyChams(config->chams["Local player"].materials);
        else if (std::strstr(info.model->name + 17, "arms"))
            applyChams(config->chams["Local player"].materials);
        else
            applyChams(config->chams["Attachements"].materials);
    }
    else if (std::string_view{ info.model->name }.starts_with("models/weapons/v_"))
    {
        if (std::strstr(info.model->name + 17, "sleeve"))
            renderSleeves();
        else if (std::strstr(info.model->name + 17, "arms"))
            renderHands();
        else if (!std::strstr(info.model->name + 17, "tablet")
            && !std::strstr(info.model->name + 17, "parachute")
            && !std::strstr(info.model->name + 17, "fists"))
            renderWeapons();
    }
    else
    {
        const auto entity = interfaces->entityList->getEntity(info.entityIndex);
        if (entity && !entity->isDormant())
        {
            if (entity->isPlayer())
                renderPlayer(entity);

            if (entity->getClientClass()->classId == ClassId::CSRagdoll)
                applyChams(config->chams["Ragdolls"].materials);
        }
    }

    return appliedChams;
}
bool localPlayerssss{ false };
void Chams::renderPlayer(Entity* player) noexcept
{
    if (!localPlayer)
        return;

    if (player == localPlayer.get()) {
        if (localPlayer->isScoped() && memory->input->isCameraInThirdPerson)
            interfaces->renderView->setBlend((100.f - config->visuals.thirdperson.thirdpersonTransparency) / 100.f);
        applyChams(config->chams["Local player"].materials, health(player));
        renderDesync(health(player));
        renderFakelag(health(player));
        localPlayerssss = true;
    }
    else if (localPlayer->isOtherEnemy(player)) {
        //applyChams(config->chams[c_xor("Enemies")].materials, health(player));
        if (config->backtrack.enabled)
        {
            const auto records = Animations::getBacktrackRecords(player->index());
            if (config->backtrack.allticks && (records && !records->empty()))
            {
                for (int x = 0; x < records->size(); x++) {
                    if (records && records->size() && Backtrack::valid(records->front().simulationTime)) {
                        //if (!appliedChams)
                            //hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customBoneToWorld);
                        applyChams(config->chams[c_xor("Backtrack")].materials, health(player), records->at(x).matrix);
                        interfaces->studioRender->forcedMaterialOverride(nullptr);
                    }
                }
            }
            else
            if (records && !records->empty())
            {
                int lastTick = -1;

                for (int i = static_cast<int>(records->size() - 1U); i >= 0; i--)
                {
                    if (Backtrack::valid(records->at(i).simulationTime) && records->at(i).origin != player->origin())
                    {
                        lastTick = i;
                        break;
                    }
                }

                if (lastTick != -1)
                {
                    //if (!appliedChams)
                        //hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customBoneToWorld);
                    applyChams(config->chams[c_xor("Backtrack")].materials, health(player), records->at(lastTick).matrix);
                    interfaces->studioRender->forcedMaterialOverride(nullptr);
                }
            }
            applyChams(config->chams[c_xor("Enemies")].materials, health(player));
        }
        else
            applyChams(config->chams[c_xor("Enemies")].materials, health(player));
    }
    else {
        applyChams(config->chams[c_xor("Allies")].materials, health(player));
    }
}

void Chams::renderFakelag(int health) noexcept
{
    if (!config->rageAntiAim[static_cast<int>(goofy1)].desync && !(config->fakelag.limit > 1))
        return;

    if (!localPlayer->isAlive())
        return;

    if (localPlayer->velocity().length2D() < 2.5f)
        return;

    if (Animations::gotFakelagMatrix())
    {
        auto fakelagMatrix = Animations::getFakelagMatrix();
        if (!appliedChams)
            hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customBoneToWorld);
        applyChams(config->chams["Fake lag"].materials, health, fakelagMatrix.data());
        interfaces->studioRender->forcedMaterialOverride(nullptr);
    }
}

void Chams::renderDesync(int health) noexcept
{
    if (Animations::gotFakeMatrix())
    {
        auto fakeMatrix = Animations::getFakeMatrix();
        for (auto& i : fakeMatrix)
        {
            i[0][3] += info->origin.x;
            i[1][3] += info->origin.y;
            i[2][3] += info->origin.z;
        }
        if (!appliedChams)
            hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customBoneToWorld);
        applyChams(config->chams["Desync"].materials, health, fakeMatrix.data());
        interfaces->studioRender->forcedMaterialOverride(nullptr);
        for (auto& i : fakeMatrix)
        {
            i[0][3] -= info->origin.x;
            i[1][3] -= info->origin.y;
            i[2][3] -= info->origin.z;
        }
    }
}

void Chams::renderWeapons() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    auto w = localPlayer->getActiveWeapon();
    if (localPlayer->isScoped() && w && (w->itemDefinitionIndex2() == WeaponId::Aug || w->itemDefinitionIndex2() == WeaponId::Sg553))
        return;

    applyChams(config->chams["Weapons"].materials, localPlayer->health());
}

void Chams::renderAttachements() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    applyChams(config->chams["Attachements"].materials, localPlayer->health());
}

void Chams::renderHands() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    applyChams(config->chams["Hands"].materials, localPlayer->health());
}

void Chams::renderSleeves() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    applyChams(config->chams["Sleeves"].materials, localPlayer->health());
}

void Chams::renderAttachments(void* ctx, void* state, const ModelRenderInfo& info, matrix3x4* customBoneToWorld) noexcept
{
    static bool materialsInitialized = false;
    if (!materialsInitialized) {
        initializeMaterials();
        materialsInitialized = true;
    }

    appliedChams = false;
    this->ctx = ctx;
    this->state = state;
    this->info = &info;
    this->customBoneToWorld = customBoneToWorld;

    if (!localPlayer || !localPlayer->isAlive())
        return;
    if (std::string_view{ info.model->name }.starts_with("models/weapons/w_"))
    {
        if (std::strstr(info.model->name + 17, "sleeve"))
            renderSleeves();
        else if (std::strstr(info.model->name + 17, "arms"))
            renderHands();
        else if (!std::strstr(info.model->name + 17, "tablet")
            && !std::strstr(info.model->name + 17, "parachute")
            && !std::strstr(info.model->name + 17, "fists"))
            renderWeapons();
    }
    applyChams(config->chams["Attachments"].materials, localPlayer->health());
}

void Chams::applyChams(const std::array<Config::Chams::Material, 7>& chams, int health, const matrix3x4* customMatrix) noexcept
{
    for (const auto& cham : chams) {
        if (!cham.enabled || !cham.ignorez)
            continue;

        const auto material = dispatchMaterial(cham.material);
        if (!material)
            continue;

        float r, g, b;
        if (cham.healthBased && health) {
            Helpers::healthColor(std::clamp(health / 100.0f, 0.0f, 1.0f), r, g, b);
        }
        else if (cham.rainbow) {
            std::tie(r, g, b) = rainbowColor(cham.rainbowSpeed);
        }
        else {
            r = cham.color[0];
            g = cham.color[1];
            b = cham.color[2];
        }

        if (material == glow || material == chrome || material == plastic || material == glass || material == crystal)
            material->findVar("$envmaptint")->setVectorValue(r, g, b);
        else
            material->colorModulate(r, g, b);

        const auto pulse = cham.color[3] * (cham.blinking ? std::sin(memory->globalVars->currenttime * 5) * 0.5f + 0.5f : 1.0f);

        if (material == glow)
            material->findVar("$envmapfresnelminmaxexp")->setVecComponentValue(9.0f * (1.2f - pulse), 2);
        else
            material->alphaModulate(pulse);

        material->setMaterialVarFlag(MaterialVarFlag::IGNOREZ, true);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, cham.wireframe);
        interfaces->studioRender->forcedMaterialOverride(material);
        hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customMatrix ? customMatrix : customBoneToWorld);
        interfaces->studioRender->forcedMaterialOverride(nullptr);
    }

    for (const auto& cham : chams) {
        if (!cham.enabled || cham.ignorez)
            continue;

        const auto material = dispatchMaterial(cham.material);
        if (!material)
            continue;

        float r, g, b;
        if (cham.healthBased && health) {
            Helpers::healthColor(std::clamp(health / 100.0f, 0.0f, 1.0f), r, g, b);
        }
        else if (cham.rainbow) {
            std::tie(r, g, b) = rainbowColor(cham.rainbowSpeed);
        }
        else {
            r = cham.color[0];
            g = cham.color[1];
            b = cham.color[2];
        }

        if (material == glow || material == chrome || material == plastic || material == glass || material == crystal)
            material->findVar("$envmaptint")->setVectorValue(r, g, b);
        else
            material->colorModulate(r, g, b);

        const auto pulse = cham.color[3] * (cham.blinking ? std::sin(memory->globalVars->currenttime * 5) * 0.5f + 0.5f : 1.0f);
        if (material == glow)
            material->findVar("$envmapfresnelminmaxexp")->setVecComponentValue(9.0f * (1.2f - pulse), 2);
        else
            material->alphaModulate(pulse);

        if (cham.cover && !appliedChams)
            hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customMatrix ? customMatrix : customBoneToWorld);

        material->setMaterialVarFlag(MaterialVarFlag::IGNOREZ, false);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, cham.wireframe);
        interfaces->studioRender->forcedMaterialOverride(material);
        hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customMatrix ? customMatrix : customBoneToWorld);
        appliedChams = true;
    }
}
