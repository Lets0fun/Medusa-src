#include "ESP.h"
#include "../font.h"
#include "../SDK/Input.h"
static bool worldToScreen(const Vector& in, Vector& out) noexcept
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

static void renderSnaplines(const PlayerData& player, ColorToggle config) noexcept
{
    if (!config.enabled)
        return;

    if (player.dormant)
        return;

    Vector position;
    if (!worldToScreen(player.origin, position))
        return;

    if (config.rainbow && !player.dormant)
        interfaces->surface->setDrawColor(rainbowColor(config.rainbowSpeed));
    else if (!player.dormant)
        interfaces->surface->setDrawColor(config.color[0] * 255, config.color[1] * 255, config.color[2] * 255, player.dormant ? config.color[3] * 70.f : config.color[3] * 255);

    const auto [width, height] = interfaces->surface->getScreenSize();
    interfaces->surface->drawLine(width / 2, height, static_cast<int>(position.x), static_cast<int>(position.y));
}

struct BoundingBox {
    float x0, y0;
    float x1, y1;
    Vector vertices[8];

    BoundingBox(const BaseData& entity) noexcept
    {
        const auto [width, height] = interfaces->surface->getScreenSize();

        x0 = static_cast<float>(width * 2);
        y0 = static_cast<float>(height * 2);
        x1 = -x0;
        y1 = -y0;

        const auto& mins = entity.obbMins;
        const auto& maxs = entity.obbMaxs;

        for (int i = 0; i < 8; ++i) {
            const Vector point{ i & 1 ? maxs.x : mins.x,
                                i & 2 ? maxs.y : mins.y,
                                i & 4 ? maxs.z : mins.z };

            if (!worldToScreen(point.transform(entity.coordinateFrame), vertices[i])) {
                valid = false;
                return;
            }
            x0 = min(x0, vertices[i].x);
            y0 = min(y0, vertices[i].y);
            x1 = max(x1, vertices[i].x);
            y1 = max(y1, vertices[i].y);
        }
        valid = true;
    }

    operator bool() noexcept
    {
        return valid;
    }
private:
    bool valid;
};

static void renderBox(const PlayerData& player, const BoundingBox& bbox, Config::NewESP::Player& config) noexcept
{
    if (config.box.enabled) {
        if (config.box.rainbow)
            interfaces->surface->setDrawColor(rainbowColor(config.box.rainbowSpeed));
        else
            interfaces->surface->setDrawColor(config.box.color[0] * 255, config.box.color[1] * 255, config.box.color[2] * 255, config.box.color[3] * 255 * player.fadingAlpha());
        switch (config.box.type) {
        case 0:
            interfaces->surface->drawOutlinedRect(bbox.x0, bbox.y0, bbox.x1, bbox.y1);
            interfaces->surface->setDrawColor(0,0,0, config.box.color[3] * 255 * player.fadingAlpha());

            interfaces->surface->drawOutlinedRect(bbox.x0 + 1, bbox.y0 + 1, bbox.x1 - 1, bbox.y1 - 1);
            interfaces->surface->drawOutlinedRect(bbox.x0 - 1, bbox.y0 - 1, bbox.x1 + 1, bbox.y1 + 1);
            
            break;
        case 1:
            interfaces->surface->drawLine(bbox.x0, bbox.y0, bbox.x0, bbox.y0 + (bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y0, bbox.x0 + (bbox.x1 - bbox.x0) / 4, bbox.y0);
            interfaces->surface->drawLine(bbox.x1, bbox.y0, bbox.x1 - (bbox.x1 - bbox.x0) / 4, bbox.y0);
            interfaces->surface->drawLine(bbox.x1, bbox.y0, bbox.x1, bbox.y0 + (bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y1, bbox.x0, bbox.y1 - (bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y1, bbox.x0 + (bbox.x1 - bbox.x0) / 4, bbox.y1);
            interfaces->surface->drawLine(bbox.x1, bbox.y1, bbox.x1 - (bbox.x1 - bbox.x0) / 4, bbox.y1);
            interfaces->surface->drawLine(bbox.x1, bbox.y1, bbox.x1, bbox.y1 - (bbox.y1 - bbox.y0) / 4);

            {
                interfaces->surface->setDrawColor(0, 0, 0, player.dormant ? config.box.color[3] * 127.5f : config.box.color[3] * 255);

                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y0 - 1, bbox.x0 - 1, bbox.y0 + (bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y0 - 1, bbox.x0 + (bbox.x1 - bbox.x0) / 4, bbox.y0 - 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y0 - 1, bbox.x1 - (bbox.x1 - bbox.x0) / 4, bbox.y0 - 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y0 - 1, bbox.x1 + 1, bbox.y0 + (bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y1 + 1, bbox.x0 - 1, bbox.y1 - (bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y1 + 1, bbox.x0 + (bbox.x1 - bbox.x0) / 4, bbox.y1 + 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y1 + 1, bbox.x1 - (bbox.x1 - bbox.x0) / 4, bbox.y1 + 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y1 + 1, bbox.x1 + 1, bbox.y1 - (bbox.y1 - bbox.y0) / 4);


                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y0 + 1, bbox.x0 + 1, bbox.y0 + (bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y0 + 1, bbox.x0 + (bbox.x1 - bbox.x0) / 4, bbox.y0 + 1);


                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y0 + 1, bbox.x1 - (bbox.x1 - bbox.x0) / 4, (bbox.y0 + 1));
                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y0 + 1, bbox.x1 - 1, bbox.y0 + (bbox.y1 - bbox.y0) / 4);

                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y1 - 1, bbox.x0 + 1, (bbox.y1) - (bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y1 - 1, bbox.x0 + (bbox.x1 - bbox.x0) / 4, bbox.y1 - 1);

                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y1 - 1, bbox.x1 - (bbox.x1 - bbox.x0) / 4, bbox.y1 - 1);
                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y1 - 1, (bbox.x1 - 1), (bbox.y1 - 1) - (bbox.y1 - bbox.y0) / 4);

                interfaces->surface->drawLine(bbox.x0 - 1, (bbox.y1 - bbox.y0) / 4 + bbox.y0, bbox.x0 + 1, (bbox.y1 - bbox.y0) / 4 + bbox.y0);
                interfaces->surface->drawLine(bbox.x1 + 1, (bbox.y1 - bbox.y0) / 4 + bbox.y0, bbox.x1 - 1, (bbox.y1 - bbox.y0) / 4 + bbox.y0);
                interfaces->surface->drawLine(bbox.x0 - 1, (bbox.y1 - bbox.y0) * 3 / 4 + bbox.y0, bbox.x0 + 1, (bbox.y1 - bbox.y0) * 3 / 4 + bbox.y0);
                interfaces->surface->drawLine(bbox.x1 + 1, (bbox.y1 - bbox.y0) * 3 / 4 + bbox.y0, bbox.x1 - 1, (bbox.y1 - bbox.y0) * 3 / 4 + bbox.y0);
                interfaces->surface->drawLine((bbox.x1 - bbox.x0) / 4 + bbox.x0, bbox.y0 + 1, (bbox.x1 - bbox.x0) / 4 + bbox.x0, bbox.y0 - 1);
                interfaces->surface->drawLine((bbox.x1 - bbox.x0) / 4 + bbox.x0, bbox.y1 + 1, (bbox.x1 - bbox.x0) / 4 + bbox.x0, bbox.y1 - 1);
                interfaces->surface->drawLine((bbox.x1 - bbox.x0) * 3 / 4 + bbox.x0, bbox.y0 + 1, (bbox.x1 - bbox.x0) * 3 / 4 + bbox.x0, bbox.y0 - 1);
                interfaces->surface->drawLine((bbox.x1 - bbox.x0) * 3 / 4 + bbox.x0, bbox.y1 + 1, (bbox.x1 - bbox.x0) * 3 / 4 + bbox.x0, bbox.y1 - 1);
            }
            break;
        }
    }
}

static void renderBox(const BoundingBox& bbox, Config::NewESP::Box& config) noexcept
{
    if (config.enabled) {
        if (config.rainbow)
            interfaces->surface->setDrawColor(rainbowColor(config.rainbowSpeed));
        else
            interfaces->surface->setDrawColor(config.color[0] * 255, config.color[1] * 255, config.color[2] * 255, config.color[3] * 255);
        switch (config.type) {
        case 0:
            interfaces->surface->drawOutlinedRect(bbox.x0, bbox.y0, bbox.x1, bbox.y1);
            interfaces->surface->setDrawColor(0, 0, 0, config.color[3] * 255);

            interfaces->surface->drawOutlinedRect(bbox.x0 + 1, bbox.y0 + 1, bbox.x1 - 1, bbox.y1 - 1);
            interfaces->surface->drawOutlinedRect(bbox.x0 - 1, bbox.y0 - 1, bbox.x1 + 1, bbox.y1 + 1);

            break;
        case 1:
            interfaces->surface->drawLine(bbox.x0, bbox.y0, bbox.x0, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y0, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0);
            interfaces->surface->drawLine(bbox.x1, bbox.y0, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0);
            interfaces->surface->drawLine(bbox.x1, bbox.y0, bbox.x1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y1, bbox.x0, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);
            interfaces->surface->drawLine(bbox.x0, bbox.y1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1);
            interfaces->surface->drawLine(bbox.x1, bbox.y1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1);
            interfaces->surface->drawLine(bbox.x1, bbox.y1, bbox.x1, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);

            {
                interfaces->surface->setDrawColor(0, 0, 0, config.color[3] * 255);

                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y0 - 1, bbox.x0 - 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y0 - 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0 - 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y0 - 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0 - 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y0 - 1, bbox.x1 + 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y1 + 1, bbox.x0 - 1, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 - 1, bbox.y1 + 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 + 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y1 + 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 + 1);
                interfaces->surface->drawLine(bbox.x1 + 1, bbox.y1 + 1, bbox.x1 + 1, bbox.y1 - fabsf(bbox.y1 - bbox.y0) / 4);


                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y0 + 1, bbox.x0 + 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 + 2, bbox.y0 + 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y0 + 1);


                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y0 + 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, (bbox.y0 + 1));
                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y0 + 1, bbox.x1 - 1, bbox.y0 + fabsf(bbox.y1 - bbox.y0) / 4);

                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y1 - 1, bbox.x0 + 1, (bbox.y1) - fabsf(bbox.y1 - bbox.y0) / 4);
                interfaces->surface->drawLine(bbox.x0 + 1, bbox.y1 - 1, bbox.x0 + fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 - 1);

                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y1 - 1, bbox.x1 - fabsf(bbox.x1 - bbox.x0) / 4, bbox.y1 - 1);
                interfaces->surface->drawLine(bbox.x1 - 1, bbox.y1 - 2, (bbox.x1 - 1), (bbox.y1 - 1) - fabsf(bbox.y1 - bbox.y0) / 4);

                interfaces->surface->drawLine(bbox.x0 - 1, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0, bbox.x0 + 2, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0);
                interfaces->surface->drawLine(bbox.x1 + 1, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0, bbox.x1 - 2, fabsf((bbox.y1 - bbox.y0) / 4) + bbox.y0);
                interfaces->surface->drawLine(bbox.x0 - 1, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0, bbox.x0 + 2, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0);
                interfaces->surface->drawLine(bbox.x1 + 1, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0, bbox.x1 - 2, fabsf((bbox.y1 - bbox.y0) * 3 / 4) + bbox.y0);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y0 + 1, fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y0 - 2);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y1 + 1, fabsf((bbox.x1 - bbox.x0) / 4) + bbox.x0, bbox.y1 - 2);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y0 + 1, fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y0 - 2);
                interfaces->surface->drawLine(fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y1 + 1, fabsf((bbox.x1 - bbox.x0) * 3 / 4) + bbox.x0, bbox.y1 - 2);
            }
            break;
        }
    }
}

static void renderName(const PlayerData& player, const BoundingBox& bbox, Config::NewESP::Player& config)
{
    if (config.name.enabled) {
        auto name = std::wstring(player.name.begin(), player.name.end());
        const auto [width, height] { interfaces->surface->getTextSize(hooks->nameEsp, name.c_str()) };
        interfaces->surface->setTextFont(hooks->nameEsp);
        if (config.name.rainbow)
            interfaces->surface->setTextColor(rainbowColor(config.name.rainbowSpeed));
        else
            interfaces->surface->setTextColor(config.name.color[0] * 255, config.name.color[1] * 255, config.name.color[2] * 255, config.name.color[3] * 255 * player.fadingAlpha());
        interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y0 - height);
        interfaces->surface->printText(name);
    }
}

static void renderName(const BoundingBox& bbox, ColorToggle& cfg, std::string name1, unsigned long font, bool bottom, int offset = 0)
{
    if (cfg.enabled) {
        auto name = std::wstring(name1.begin(), name1.end());
        const auto [width, height] { interfaces->surface->getTextSize(font, name.c_str()) };
        interfaces->surface->setTextFont(font);
        if (cfg.rainbow)
            interfaces->surface->setTextColor(rainbowColor(cfg.rainbowSpeed));
        else
            interfaces->surface->setTextColor(cfg.color[0] * 255, cfg.color[1] * 255, cfg.color[2] * 255, cfg.color[3] * 255);
        if (!bottom)
            interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y0 - height - offset);
        else
            interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y1 + height + offset);
        interfaces->surface->printText(name);
    }
}

static void renderWeapon(const PlayerData& player, const BoundingBox& bbox, Config::NewESP::Player& config)
{
    int offset = 0;
    if (config.weapon.enabled) {
        auto name = std::wstring(player.activeWeapon.begin(), player.activeWeapon.end());
        const auto [width, height] { interfaces->surface->getTextSize(hooks->smallFonts, name.c_str()) };
        interfaces->surface->setTextFont(hooks->smallFonts);
        if (config.weapon.rainbow)
            interfaces->surface->setTextColor(rainbowColor(config.weapon.rainbowSpeed));
        else
            interfaces->surface->setTextColor(config.weapon.color[0] * 255, config.weapon.color[1] * 255, config.weapon.color[2] * 255, config.weapon.color[3] * 255 * player.fadingAlpha());
        interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y1 + 1 + (config.ammo.enabled && !player.isBomb && !player.isGrenade && !player.isKnife && !player.activeWeapon.empty() ? 6 : 0));
        interfaces->surface->printText(name);
    }
    if (config.weaponIcon.enabled) {
        auto name = std::wstring(player.activeWeaponIcon.begin(), player.activeWeaponIcon.end());

        const auto [width, height] { interfaces->surface->getTextSize(hooks->weaponIcon, name.c_str()) };
        interfaces->surface->setTextFont(hooks->weaponIcon);
        if (config.weaponIcon.rainbow && !player.dormant)
            interfaces->surface->setTextColor(rainbowColor(config.weaponIcon.rainbowSpeed));
        else
            interfaces->surface->setTextColor(config.weaponIcon.color[0] * 255, config.weaponIcon.color[1] * 255, config.weaponIcon.color[2] * 255, config.weaponIcon.color[3] * 255 * player.fadingAlpha());
        interfaces->surface->setTextPosition((bbox.x0 + bbox.x1 - width) / 2, bbox.y1 + 1 + (config.ammo.enabled && !player.isBomb && !player.isGrenade && !player.isKnife && !player.activeWeapon.empty() ? 6 : 0) + (config.weapon.enabled ? 10 : 0));
        interfaces->surface->printText(name);
    }
}

static void healthBar(const PlayerData& player, const BoundingBox& bbox, Config::NewESP::Player& config)
{
    if (config.health.enabled)
    {
        float drawPositionLeft = bbox.x0 - 3;
        float drawPositionRight = bbox.x1 + 8;
        float drawPositionBottom = 3.5f;
        float drawPositionBottomEh = 6.5f;
        float drawPositionX = bbox.x0 - 5;
        int type = config.health.style;
        int alpha = config.health.solid.color[3];
        if (config.health.background)
        {
            interfaces->surface->setDrawColor(0, 0, 0, 180 * player.fadingAlpha());
            interfaces->surface->drawFilledRect(drawPositionLeft - 3, bbox.y0, drawPositionLeft + 1, bbox.y1);
        }
        if (config.health.outline)
        {
            interfaces->surface->setDrawColor(0, 0, 0, 255 * player.fadingAlpha());
            interfaces->surface->drawOutlinedRect(drawPositionLeft - 3, bbox.y0 - 1, drawPositionLeft + 1, bbox.y1 + 1);
        }
        if (config.health.text.enabled && player.health != 100)
        {
            if (!config.health.text.rainbow)
                interfaces->surface->setTextColor(config.health.text.color[0] * 255, config.health.text.color[1] * 255, config.health.text.color[2] * 255, config.health.text.color[3] * 255 * player.fadingAlpha());
            else
                interfaces->surface->setTextColor(rainbowColor(config.health.text.rainbowSpeed));
            interfaces->surface->setTextFont(hooks->smallFonts);
            interfaces->surface->setTextPosition(drawPositionLeft - 4 - interfaces->surface->getTextSize(hooks->smallFonts, std::to_wstring(player.health).c_str()).first, bbox.y0 + abs(bbox.y1 - bbox.y0) * (100 - player.health) / 100 - interfaces->surface->getTextSize(hooks->smallFonts, std::to_wstring(player.health).c_str()).second / 2);
            interfaces->surface->printText(std::to_wstring(player.health));
        }
        if (type == 0)
        {
            if (config.health.solid.rainbow)
                interfaces->surface->setDrawColor(rainbowColor(config.health.solid.rainbowSpeed));
            else
                interfaces->surface->setDrawColor(config.health.solid.color[0] * 255, config.health.solid.color[1] * 255, config.health.solid.color[2] * 255, config.health.solid.color[3] * 255 * player.fadingAlpha());
            interfaces->surface->drawFilledRect(drawPositionLeft - 2, bbox.y0 + abs(bbox.y1 - bbox.y0) * (100 - player.health) / 100, drawPositionLeft, bbox.y1);
        }
        else if (type == 1)
        {
            Render::gradient1(drawPositionLeft - 2, bbox.y0 + abs(bbox.y1 - bbox.y0) * (100 - player.health) / 100, drawPositionLeft, bbox.y1, Color(Helpers::calculateColor(config.health.top, config.health.top.color[3] * player.fadingAlpha())), Color(Helpers::calculateColor(config.health.bottom, config.health.bottom.color[3] * player.fadingAlpha())), Render::GRADIENT_VERTICAL);
        }
        else if (type == 2)
        {
            Color color;
            if (player.health <= 25)
                color = Color(250, 57, 80, (int)(255 * player.fadingAlpha()));
            if (player.health <= 50 && player.health > 25)
                color = Color(209, 110, 80, (int)(255 * player.fadingAlpha()));
            if (player.health <= 70 && player.health > 50)
                color = Color(163, 169, 80, (int)(255 * player.fadingAlpha()));
            if (player.health <= 100 && player.health > 70)
                color = Color(120, 225, 80, (int)(255 * player.fadingAlpha()));
            interfaces->surface->setDrawColor(color.r(), color.g(), color.b(), color.a());
            interfaces->surface->drawFilledRect(drawPositionLeft - 2, bbox.y0 + abs(bbox.y1 - bbox.y0) * (100 - player.health) / 100, drawPositionLeft, bbox.y1);
        }
        //drawPositionLeft -= 5;
    }
}

static void ammoBar(const PlayerData& player, const BoundingBox& bbox, Config::NewESP::Player& config)
{
    if (player.isBomb || player.isGrenade || player.isKnife || player.activeWeapon.empty())
        return;

    if (config.ammo.enabled)
    {
        float drawPositionLeft = bbox.x0 - 3;
        float drawPositionRight = bbox.x1 + 8;
        float drawPositionBottom = 3.f;
        float drawPositionBottomEh = 5.f;
        float drawPositionX = bbox.x0 - 5;
        int type = config.ammo.style;
        int alpha = config.ammo.solid.color[3];
        if (config.ammo.background)
        {
            interfaces->surface->setDrawColor(0, 0, 0, 180 * player.fadingAlpha());
            interfaces->surface->drawFilledRect(bbox.x0 - 1, bbox.y1 + drawPositionBottom - 1, bbox.x1 + 1, bbox.y1 + drawPositionBottomEh + 1);
        }
        if (config.ammo.outline)
        {
            interfaces->surface->setDrawColor(0, 0, 0, 255 * player.fadingAlpha());
            interfaces->surface->drawOutlinedRect(bbox.x0 - 1, bbox.y1 + drawPositionBottom - 1, bbox.x1 + 1, bbox.y1 + drawPositionBottomEh + 1);
        }
        if (config.ammo.text.enabled && player.clip2 != player.maxClip)
        {
            if (!config.ammo.text.rainbow)
                interfaces->surface->setTextColor(config.ammo.text.color[0] * 255, config.ammo.text.color[1] * 255, config.ammo.text.color[2] * 255, config.ammo.text.color[3] * 255 * player.fadingAlpha());
            else
                interfaces->surface->setTextColor(rainbowColor(config.ammo.text.rainbowSpeed));
            interfaces->surface->setTextFont(hooks->smallFonts);
            interfaces->surface->setTextPosition(bbox.x1 - abs(bbox.x1 - bbox.x0) * (player.maxClip - player.clip2) / player.maxClip + 1, bbox.y1 + drawPositionBottomEh - interfaces->surface->getTextSize(hooks->smallFonts, std::to_wstring(player.clip2).c_str()).second / 2);
            interfaces->surface->printText(std::to_wstring(player.clip2));
        }
        if (type == 0)
        {
            if (config.ammo.solid.rainbow)
                interfaces->surface->setDrawColor(rainbowColor(config.ammo.solid.rainbowSpeed));
            else
                interfaces->surface->setDrawColor(config.ammo.solid.color[0] * 255, config.ammo.solid.color[1] * 255, config.ammo.solid.color[2] * 255, config.ammo.solid.color[3] * 255 * player.fadingAlpha());
            interfaces->surface->drawFilledRect(bbox.x0, bbox.y1 + drawPositionBottom, bbox.x1 - abs(bbox.x1 - bbox.x0) * (player.maxClip - player.clip2) / player.maxClip, bbox.y1 + drawPositionBottomEh);
        }
        if (type == 1)
        {
            Render::gradient1(bbox.x0, bbox.y1 + drawPositionBottom, bbox.x1 - abs(bbox.x1 - bbox.x0) * (player.maxClip - player.clip2) / player.maxClip, bbox.y1 + drawPositionBottomEh, Color(Helpers::calculateColor(config.ammo.left, config.ammo.left.color[3] * player.fadingAlpha())), Color(Helpers::calculateColor(config.ammo.right, config.ammo.right.color[3] * player.fadingAlpha())), Render::GRADIENT_HORIZONTAL);
        }

        //drawPositionLeft -= 5;
    }
}

static void drawPlayerSkeleton(const ColorToggle& config, const std::vector<std::pair<Vector, Vector>>& bones, bool dormant) noexcept
{
    if (!config.enabled)
        return;

    if (dormant)
        return;

    const auto color = Helpers::calculateColor(config);

    std::vector<std::pair<ImVec2, ImVec2>> points, shadowPoints;

    for (const auto& [bone, parent] : bones) {
        ImVec2 bonePoint;
        if (!Helpers::worldToScreen(bone, bonePoint))
            continue;

        ImVec2 parentPoint;
        if (!Helpers::worldToScreen(parent, parentPoint))
            continue;

        points.emplace_back(bonePoint, parentPoint);
    }

    for (const auto& [bonePoint, parentPoint] : points)
    {
        if (!config.rainbow)
            interfaces->surface->setDrawColor(config.color[0] * 255, config.color[1] * 255, config.color[2] * 255, config.color[3] * 255);
        else
            interfaces->surface->setDrawColor(rainbowColor(config.rainbowSpeed));
        interfaces->surface->drawLine(bonePoint.x, bonePoint.y, parentPoint.x, parentPoint.y);
    }
}

static void playerFlags(const PlayerData& player,const BoundingBox& bbox, Config::NewESP::Player& config)
{
    if (config.flags.enabled)
    {
        int offset = 0;
        if (config.showArmor && player.armor > 0)
        {
            std::wstring name;
            if (player.hasHelmet)
                name = L"HK";
            else
                name = L"K";
            const auto [tWidth, tHeight] = interfaces->surface->getTextSize(hooks->smallFonts, name.c_str());
            interfaces->surface->setTextFont(hooks->smallFonts);    
            interfaces->surface->setTextColor(config.flags.color[0] * 255, config.flags.color[1] * 255, config.flags.color[2] * 255, config.flags.color[3] * 255 * player.fadingAlpha());
            interfaces->surface->setTextPosition(bbox.x1 + 3, bbox.y0 + (9 * offset));
            interfaces->surface->printText(name.c_str());
            offset += 1;
        }
        if (config.showKit && player.hasDefuser)
        {
            std::wstring name = L"KIT";
            const auto [tWidth, tHeight] = interfaces->surface->getTextSize(hooks->smallFonts, name.c_str());
            interfaces->surface->setTextFont(hooks->smallFonts);
            interfaces->surface->setTextColor(config.flags.color[0] * 255, config.flags.color[1] * 255, config.flags.color[2] * 255, config.flags.color[3] * 255 * player.fadingAlpha());
            interfaces->surface->setTextPosition(bbox.x1 + 3, bbox.y0 + (9 * offset));
            interfaces->surface->printText(name.c_str());
            offset += 1;
        }
        /*if (config.showPin && player.pinPulled)
        {
            std::wstring name = L"PIN";
            const auto [tWidth, tHeight] = interfaces->surface->getTextSize(hooks->smallFonts, name.c_str());
            interfaces->surface->setTextFont(hooks->smallFonts);
            interfaces->surface->setTextColor(config.flags.color[0] * 255, config.flags.color[1] * 255, config.flags.color[2] * 255, config.flags.color[3] * 255 * player.fadingAlpha());
            interfaces->surface->setTextPosition(bbox.x1 + 3, bbox.y0 + (9 * offset));
            interfaces->surface->printText(name.c_str());
            offset += 1;
        }*/
        if (config.showBomb && player.hasBomb)
        {
            std::wstring name = L"C4";
            const auto [tWidth, tHeight] = interfaces->surface->getTextSize(hooks->smallFonts, name.c_str());
            interfaces->surface->setTextFont(hooks->smallFonts);
            interfaces->surface->setTextColor(config.flags.color[0] * 255, config.flags.color[1] * 255, config.flags.color[2] * 255, config.flags.color[3] * 255 * player.fadingAlpha());
            interfaces->surface->setTextPosition(bbox.x1 + 3, bbox.y0 + (9 * offset));
            interfaces->surface->printText(name.c_str());
            offset += 1;
        }
        if (config.showFD && player.isFakeDucking)
        {
            std::wstring name = L"FD";
            const auto [tWidth, tHeight] = interfaces->surface->getTextSize(hooks->smallFonts, name.c_str());
            interfaces->surface->setTextFont(hooks->smallFonts);
            interfaces->surface->setTextColor(config.flags.color[0] * 255, config.flags.color[1] * 255, config.flags.color[2] * 255, config.flags.color[3] * 255 * player.fadingAlpha());
            interfaces->surface->setTextPosition(bbox.x1 + 3, bbox.y0 + (9 * offset));
            interfaces->surface->printText(name.c_str());
            offset += 1;
        }
        if (config.showScoped && player.isScoped)
        {
            std::wstring name = L"SCOPED";
            const auto [tWidth, tHeight] = interfaces->surface->getTextSize(hooks->smallFonts, name.c_str());
            interfaces->surface->setTextFont(hooks->smallFonts);
            interfaces->surface->setTextColor(config.flags.color[0] * 255, config.flags.color[1] * 255, config.flags.color[2] * 255, config.flags.color[3] * 255 * player.fadingAlpha());
            interfaces->surface->setTextPosition(bbox.x1 + 3, bbox.y0 + (9 * offset));
            interfaces->surface->printText(name.c_str());
            offset += 1;
        }
        if (config.showFlashed && player.flashDuration > 0)
        {
            std::wstring name = L"FLASHED";
            const auto [tWidth, tHeight] = interfaces->surface->getTextSize(hooks->smallFonts, name.c_str());
            interfaces->surface->setTextFont(hooks->smallFonts);
            interfaces->surface->setTextColor(config.flags.color[0] * 255, config.flags.color[1] * 255, config.flags.color[2] * 255, config.flags.color[3] * 255 * player.fadingAlpha());
            interfaces->surface->setTextPosition(bbox.x1 + 3, bbox.y0 + (9 * offset));
            interfaces->surface->printText(name.c_str());
            offset += 1;
        }
        if (config.showMoney)
        {
            std::wstring name = L"$" + std::to_wstring(player.money);
            const auto [tWidth, tHeight] = interfaces->surface->getTextSize(hooks->smallFonts, name.c_str());
            interfaces->surface->setTextFont(hooks->smallFonts);
            interfaces->surface->setTextColor(config.flags.color[0] * 255, config.flags.color[1] * 255, config.flags.color[2] * 255, config.flags.color[3] * 255 * player.fadingAlpha());
            interfaces->surface->setTextPosition(bbox.x1 + 3, bbox.y0 + (9 * offset));
            interfaces->surface->printText(name.c_str());
            offset += 1;
        }
    }
}

static void playerESP(const PlayerData& player, Config::NewESP::Player& config)
{
    BoundingBox bbox{ player };
    if (!bbox)
        return;

    renderSnaplines(player, config.snapline);
    renderBox(player, bbox, config);
    playerFlags(player, bbox, config);
    renderName(player, bbox, config);
    renderWeapon(player, bbox, config);
    ammoBar(player, bbox, config);
    drawPlayerSkeleton(config.skeleton, player.bones, player.dormant);
    healthBar(player, bbox, config);
}

static void localPlayerESP(Config::NewESP::Player& config)
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!config.enable)
        return;

    BoundingBox bbox{ localPlayer.get() };
    if (!bbox)
        return;

    const auto model = localPlayer->getModel();
    if (!model)
        return;

    const auto studioModel = interfaces->modelInfo->getStudioModel(model);
    if (!studioModel)
        return;

    if (!localPlayer->getBoneCache().memory)
        return;

    std::vector<std::pair<Vector, Vector>> bones;
    matrix3x4 boneMatrices[MAXSTUDIOBONES];
    memcpy(boneMatrices, localPlayer->getBoneCache().memory, std::clamp(localPlayer->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
    bones.clear();
    bones.reserve(20);

    for (int i = 0; i < studioModel->numBones; ++i) {
        const auto bone = studioModel->getBone(i);

        if (!bone || bone->parent == -1 || !(bone->flags & BONE_USED_BY_HITBOX))
            continue;

        bones.emplace_back(boneMatrices[i].origin(), boneMatrices[bone->parent].origin());
    }
    drawPlayerSkeleton(config.skeleton, bones, false);
    renderBox(bbox, config.box);
    renderName(bbox, config.name, localPlayer->getPlayerName(), hooks->nameEsp, false);
}

static void weaponESP(const WeaponData& weapon, Config::NewESP::Weapon& config)
{
    BoundingBox bbox{ weapon };
    if (!bbox)
        return;

    renderBox(bbox, config.box);
    int offset = 0;
    renderName(bbox, config.name, std::string(weapon.name), hooks->smallFonts, true, offset);
    renderName(bbox, config.icon, std::string(weapon.icon), hooks->weaponIcon, true, offset + (config.name.enabled ? 10 : 0));
    if (config.ammo.enabled)
    {
        if (weapon.maxClip == 0)
        {
            offset = 0;
            return;
        }
        else
        {
            interfaces->surface->setDrawColor(0, 0, 0, config.ammo.color[3] * 180);
            interfaces->surface->drawFilledRect(bbox.x0, bbox.y1 + 3, bbox.x1, bbox.y1 + 6);
            interfaces->surface->setDrawColor(0, 0, 0, config.ammo.color[3] * 255);
            interfaces->surface->drawOutlinedRect(bbox.x0 - 1, bbox.y1 + 2, bbox.x1 + 1, bbox.y1 + 7);
            if (!config.ammo.rainbow)
                interfaces->surface->setDrawColor(config.ammo.color[0] * 255, config.ammo.color[1] * 255, config.ammo.color[2] * 255, config.ammo.color[3] * 255);
            else
                interfaces->surface->setDrawColor(rainbowColor(config.ammo.rainbowSpeed));
            interfaces->surface->drawFilledRect(bbox.x0, bbox.y1 + 3, bbox.x1 - abs(bbox.x1 - bbox.x0) * (weapon.maxClip - weapon.clip) / weapon.maxClip, bbox.y1 + 6);
            offset = 7;
        }
    }
}

static void renderEntityBox(const BaseData& entity, const char* name, Config::NewESP::Shared& config)
{
    BoundingBox bbox{ entity };
    if (!bbox)
        return;

    renderBox(bbox, config.box);
    renderName(bbox, config.name, name, hooks->smallFonts, false);
}

static void renderProjectileESP(const ProjectileData& entity, const char* name, const char* icon, Config::NewESP::Projectiles& config)
{
    BoundingBox bbox{ entity };
    if (!bbox)
        return;
    if (!entity.exploded)
    {
        renderBox(bbox, config.box);
        renderName(bbox, config.name, name, hooks->smallFonts, false);
    }

}

void ESP::render() noexcept
{
    if (memory->input->isCameraInThirdPerson)
        localPlayerESP(config->esp.local);
    for (const auto& projectiles : GameData::projectiles()) {
        if (config->esp.projectiles.enable)
        {
            renderProjectileESP(projectiles, projectiles.name, projectiles.icons, config->esp.projectiles);
        }
    }
    for (const auto& weapon : GameData::weapons()){
        if (config->esp.weapons.enable)
            weaponESP(weapon, config->esp.weapons);
    }
    for (const auto& player : GameData::players()) {
        if (!player.alive || !player.inViewFrustum)
            continue;

        auto& playerConfig = player.enemy ? config->esp.enemy : config->esp.allies;
        if (!playerConfig.dormant && player.dormant)
            continue;

        if (!playerConfig.enable)
            continue;

        if (player.fadingAlpha() == 0)
            continue;

        playerESP(player, playerConfig);
    }
}