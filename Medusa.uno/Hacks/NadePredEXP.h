#pragma once
#include "../JsonForward.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../imguiCustom.h"
#include "../SDK/ITexture.h"
struct ViewSetup;
namespace NadePrediction {
	void run() noexcept;
	void draw(ImDrawList*, bool nadeView = false) noexcept;
	void renderView(ViewSetup*) noexcept;
	void drawNadeView() noexcept;
	void drawGUI() noexcept;
	json toJson() noexcept;
	void fromJson(const json&) noexcept;
	void reset() noexcept;
};
