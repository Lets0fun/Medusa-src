#pragma once

#include <vector>

#include "../Config.h"

class Entity;
struct ModelRenderInfo;
class matrix3x4;
class Material;

namespace Sex {
    void GetCmd(UserCmd* cmd);
}
class Chams {
public:
    bool render(void*, void*, const ModelRenderInfo&, matrix3x4*) noexcept;
private:
    void renderPlayer(Entity* player) noexcept;
    void renderDesync(int health) noexcept;
    void renderWeapons() noexcept;
    void renderAttachements() noexcept;
    void renderHands() noexcept;
    void renderFakelag(int health) noexcept;
    void renderSleeves() noexcept;
    void renderAttachments(void*, void*, const ModelRenderInfo&, matrix3x4*) noexcept;

    bool appliedChams;
    void* ctx;
    void* state;
    const ModelRenderInfo* info;
    matrix3x4* customBoneToWorld;

    void applyChams(const std::array<Config::Chams::Material, 7>& chams, int health = 0, const matrix3x4* customMatrix = nullptr) noexcept;
};
