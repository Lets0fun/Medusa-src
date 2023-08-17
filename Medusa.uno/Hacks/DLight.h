#pragma once
#include "../SDK/Vector.h"
#include "../SDK/VirtualMethod.h"

struct ColorRGBExp32
{
	std::byte r, g, b;
	signed char exponent;
};

namespace Dlights
{
	void setupLights() noexcept;
}

struct dlight_t
{
	int		flags;
	Vector	origin;
	float	radius;
	ColorRGBExp32 color;
	float	die;
	float	decay;
	float	minlight;
	int		key;
	int		style;
	Vector	m_Direction;
	float	m_InnerAngle;
	float	m_OuterAngle;
	float GetRadius() const
	{
		return radius;
	}
	float GetRadiusSquared() const
	{
		return radius * radius;
	}
	float IsRadiusGreaterThanZero() const
	{
		return radius > 0.0f;
	}
};

class Cock
{
public:

	VIRTUAL_METHOD(dlight_t*, CL_AllocDlight, 4, (int key), (this, key));
	VIRTUAL_METHOD(dlight_t*, CL_AllocElight, 5, (int key), (this, key));
	VIRTUAL_METHOD(dlight_t*, GetElightByKey, 8, (int key), (this, key));
};