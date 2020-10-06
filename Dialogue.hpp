#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <map>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

struct DrawDialogue {
	//Start drawing; will remember world_to_clip matrix:
	DrawDialogue(glm::mat4 const &world_to_clip);

	//draw wireframe text, start at anchor, move in x direction, mat gives x and y directions for text drawing:
	// (default character box is 1 unit high)
	void draw_text(std::string const &text, glm::vec3 const &anchor_in, glm::u8vec4 const &color = glm::u8vec4(0xff));

	//Finish drawing (push attribs to GPU):
	~DrawDialogue();

	glm::mat4 world_to_clip;
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) : Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};

	// referenced https://learnopengl.com/In-Practice/Text-Rendering
	struct Character {
		unsigned int tex;
		glm::ivec2 size;
		glm::ivec2 bearing;
	};

	std::vector<Vertex> attribs;
	std::vector<unsigned int> glyph_to_be_drawn;
};
