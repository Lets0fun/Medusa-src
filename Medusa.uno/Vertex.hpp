#pragma once
#include "includes.hpp"
#include "Vector2D.hpp"
struct Vertex_t {
	//Vertex_t() { };

	Vector2D m_Position;
	Vector2D m_TexCoord;
	void Vertex_t1(const Vector2D pos, const Vector2D& coord = Vector2D(0, 0)) { //-V818
		m_Position = pos;
		m_TexCoord = coord;
	};
	void Init(const Vector2D pos, const Vector2D& coord = Vector2D(0, 0)) {
		m_Position = pos;
		m_TexCoord = coord;
	};
};