#pragma once
#include <string>
#include "color.h"
#include "SDK/Vector.h"
//#include "includes.hpp"

namespace Render
{
	void drawText(unsigned long font, Color color, ImVec2 pos, std::string string);
	void drawLine(ImVec2 min, ImVec2 max, int r, int g, int b, int a);
	void triangle(Vector2D point_one, Vector2D point_two, Vector2D point_three, Color color);
	void Draw3DCircle(const Vector& origin, float radius, Color color);
	void Draw3DFilledCircleTest(const Vector& origin, float radius, Color color);
	void lineButBendy(const Vector, Color color);
	void Draw3DCircleGradient(const Vector& origin, float radius, Color color);
	void rectFilled(int x, int y, int w, int h, Color color);
	void rect(int x, int y, int w, int h, Color color);
	std::pair<int, int> getTextSize(unsigned long font, std::string text);
	void gradient(int x, int y, int w, int h, Color first, Color second, float type);
	void gradient1(int x, int y, int w, int h, Color first, Color second, float type);
	int textWidth(unsigned long font, std::string string);
	int textHeight(unsigned long font, std::string string);
	enum GradientType
	{
		GRADIENT_HORIZONTAL,
		GRADIENT_VERTICAL
	};
}