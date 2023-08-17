#include "includes.hpp"
#include "render.hpp"

void Render::drawText(unsigned long font, Color color, ImVec2 pos, std::string string)
{
    interfaces->surface->setTextColor(color.r(), color.g(), color.b(), color.a());
    interfaces->surface->setTextFont(font);
    interfaces->surface->setTextPosition((int)pos.x, (int)pos.y);
    interfaces->surface->printText(std::wstring(string.begin(), string.end()));
}

void Render::drawLine(ImVec2 min, ImVec2 max, int r, int g, int b, int a)
{
    interfaces->surface->setDrawColor(r, g, b, a);
    interfaces->surface->drawLine(min.x, min.y, max.x, max.y);
}

static bool wts(const Vector& in, Vector& out) noexcept
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

void Render::triangle(Vector2D point_one, Vector2D point_two, Vector2D point_three, Color color)
{

	color.SetAlpha(static_cast<int>(color.a()));

	Vertex_t verts[3] = {
		Vertex_t(point_one),
		Vertex_t(point_two),
		Vertex_t(point_three)
	};

	auto surface = interfaces->surface;

	static int texture = surface->createNewTextureID(true);
	unsigned char buffer[4] = { 255, 255, 255, 255 };

	surface->drawSetTextureRGBA(texture, buffer, 1, 1);
	surface->setDrawColor(color.r(), color.g(), color.b(), color.a());
	surface->drawSetTexture(texture);

	surface->drawTexturedPolygon(3, verts);
}

void Render::Draw3DCircle(const Vector& origin, float radius, Color color)
{
	auto prevScreenPos = Vector(0.0f, 0.0f, 0.0f);
	auto step = M_PI * 2.0f / 72.0f;

	auto screenPos = Vector(0.0f, 0.0f, 0.0f);
	auto screen = Vector(0.0f, 0.0f, 0.0f);

	if (!wts(origin, screen))
		return;

	for (auto rotation = 0.0f; rotation <= M_PI * 2.0f; rotation += step)
	{
		Vector pos(radius * cos(rotation) + origin.x, radius * sin(rotation) + origin.y, origin.z);

		if (wts(pos, screenPos))
		{
			if (prevScreenPos.notNull() && prevScreenPos != screenPos)
			{
				Render::drawLine(ImVec2{ prevScreenPos.x, prevScreenPos.y }, ImVec2{ screenPos.x, screenPos.y }, color.r(), color.g(), color.b(), color.a());
			}

			prevScreenPos = screenPos;
		}
	}
}


void Render::Draw3DFilledCircleTest(const Vector& origin, float radius, Color color)
{
	static auto prevScreenPos = Vector(0.0f, 0.0f, 0.0f); //-V656 //ZERO
	static auto step = M_PI * 2.0f;

	auto screenPos = Vector(0.0f, 0.0f, 0.0f);
	auto screen = Vector(0.0f, 0.0f, 0.0f);

	if (!wts(origin, screen))
		return;

	std::vector<Vector2D> temppoints;
	std::vector<Vertex_t> vertices;

	for (auto rotation = 0.0f; rotation <= M_PI * 2.0f; rotation += step / 40.0f) //-V1034
	{
		Vector pos(radius * cos(rotation) + origin.x, radius * sin(rotation) + origin.y, origin.z);

		if (wts(pos, screenPos))
		{
			temppoints.push_back(Vector2D(screenPos.x, screenPos.y));
		}
	}

	for (int i = 0; i < temppoints.size(); i++)
	{
		vertices.emplace_back(Vertex_t(temppoints[i]));
	}

	interfaces->surface->setDrawColor(color.r(), color.g(), color.b(), color.a() / 3);
	interfaces->surface->drawTexturedPolygon(vertices.size(), vertices.data(), true);
}

void Render::Draw3DCircleGradient(const Vector& origin, float radius, Color color)
{
	static auto prevScreenPos = Vector(0.0f, 0.0f, 0.0f); //-V656
	static auto step = M_PI * 2.0f / 60.0f;

	auto screenPos = Vector(0.0f, 0.0f, 0.0f);

	//const auto radius_step = radius / 63.f;
	float rad = radius - 1.f;

	for (int i = 1; i < radius; i++) {
		if (rad > 2.f)
			Draw3DFilledCircleTest(origin, round(rad), Color(color.r(), color.g(), color.b(), color.a() / 4));

		rad -= 1.f;
	}
}

void Render::rectFilled(int x, int y, int w, int h, Color color)
{
	interfaces->surface->setDrawColor(color.r(), color.g(), color.b(), color.a());
	interfaces->surface->drawFilledRect(x, y, x + w, y + h);
}

void Render::rect(int x, int y, int w, int h, Color color)
{
	interfaces->surface->setDrawColor(color.r(), color.g(), color.b(), color.a());
	interfaces->surface->drawOutlinedRect(x, y, x + w, y + h);
}

std::pair<int, int> Render::getTextSize(unsigned long font, std::string text)
{
	return interfaces->surface->getTextSize(font, std::wstring(text.begin(), text.end()).c_str());
}
void Render::gradient1(int x, int y, int w, int h, Color first, Color second, float type)
{
	static auto blend = [](const Color& first, const Color& second, float t) -> Color {
		return Color(
			first.r() + t * (second.r() - first.r()),
			first.g() + t * (second.g() - first.g()),
			first.b() + t * (second.b() - first.b()),
			first.a() + t * (second.a() - first.a()));
	};

	if (first.a() == 255 || second.a() == 255) {
		interfaces->surface->setDrawColor(blend(first, second, 0.5f).r(), blend(first, second, 0.5f).g(), blend(first, second, 0.5f).b(), blend(first, second, 0.5f).a());
		interfaces->surface->drawFilledRect(x, y, w, h);
	}

	interfaces->surface->setDrawColor(first.r(), first.g(), first.b(), first.a());
	interfaces->surface->drawFilledRectFade(x, y, w, h, first.a(), 0, type == GRADIENT_HORIZONTAL);

	interfaces->surface->setDrawColor(second.r(), second.g(), second.b(), second.a());
	interfaces->surface->drawFilledRectFade(x, y, w, h, 0, second.a(), type == GRADIENT_HORIZONTAL);
}
void Render::gradient(int x, int y, int w, int h, Color first, Color second, float type)
{
	static auto blend = [](const Color& first, const Color& second, float t) -> Color {
		return Color(
			first.r() + t * (second.r() - first.r()),
			first.g() + t * (second.g() - first.g()),
			first.b() + t * (second.b() - first.b()),
			first.a() + t * (second.a() - first.a()));
	};

	if (first.a() == 255 || second.a() == 255) {
		interfaces->surface->setDrawColor(blend(first, second, 0.5f).r(), blend(first, second, 0.5f).g(), blend(first, second, 0.5f).b(), blend(first, second, 0.5f).a());
		interfaces->surface->drawFilledRect(x, y, x + w, y + h);
	}

	interfaces->surface->setDrawColor(first.r(), first.g(), first.b(), first.a());
	interfaces->surface->drawFilledRectFade(x, y, x + w, y + h, first.a(), 0, type == GRADIENT_HORIZONTAL);

	interfaces->surface->setDrawColor(second.r(), second.g(), second.b(), second.a());
	interfaces->surface->drawFilledRectFade(x, y, x + w, y + h, 0, second.a(), type == GRADIENT_HORIZONTAL);
}

int Render::textWidth(unsigned long font, std::string string)
{
	return interfaces->surface->getTextSize(font, std::wstring(string.begin(), string.end()).c_str()).first;
}

int Render::textHeight(unsigned long font, std::string string)
{
	return interfaces->surface->getTextSize(font, std::wstring(string.begin(), string.end()).c_str()).second;
}
