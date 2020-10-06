#include "Dialogue.hpp"
#include "ColorTextureProgram.hpp"

#include "gl_errors.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

//All DrawLines instances share a vertex array object and vertex buffer, initialized at load time:

//n.b. declared static so they don't conflict with similarly named global variables elsewhere:
static GLuint vertex_buffer = 0;
static GLuint vertex_buffer_for_color_texture_program = 0;
static std::string dictionary("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,'.!?*() ");
static std::map<int, DrawDialogue::Character> glyph_id_to_tex_id;
static FT_Library library;
static FT_Face face;
float scale = 0.09f * 0.03f;

static Load< void > setup_buffers(LoadTagDefault, [](){
	{
		// Credits for Zizhuo Lin, xiaoqiao, and https://learnopengl.com/In-Practice/Text-Rendering for the starter code!!!
		FT_Init_FreeType(&library);
		FT_New_Face(library, "dist/Baloo-Regular.ttf", 0, &face);
		hb_buffer_t *buf = hb_buffer_create();
		hb_font_t *font = hb_ft_font_create(face, NULL);
		hb_shape(font, buf, NULL, 0);
		FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	    FT_Set_Char_Size(face, 0, 1000, 0, 0);
	    FT_Set_Pixel_Sizes(face, 0, 50);
	    
	    // set up dictionary textures
	    hb_buffer_add_utf8(buf, dictionary.data(), -1, 0, -1);
	    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
	    hb_buffer_set_language(buf, hb_language_from_string("en", -1));
	    unsigned int glyph_count = 0;
	    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
	    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	    for (int i = 0; i < glyph_count; ++i) {
	        auto glyphid = glyph_info[i].codepoint;
			if (FT_Load_Char(face, glyphid, FT_LOAD_DEFAULT)) continue;
			if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) continue;
			FT_GlyphSlot slot = face->glyph;
			int r = slot->bitmap.rows;
			int w = slot->bitmap.width;
			unsigned char* glyph_buffer = slot->bitmap.buffer;
			GLuint tex = 0;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, r, 0, GL_RED, GL_UNSIGNED_BYTE, glyph_buffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		    glBindTexture(GL_TEXTURE_2D, 0);
		    DrawDialogue::Character c = {
				tex,
				glm::ivec2(w, r),
				glm::ivec2(slot->bitmap_left, slot->bitmap_top),
			};
			glyph_id_to_tex_id.insert(std::pair<int, DrawDialogue::Character>(glyphid, c));
	    }

	    hb_buffer_destroy(buf);
      	hb_font_destroy(font);
	   //  FT_Done_Face(face);
  		// FT_Done_FreeType(library);
	}

	{ //set up vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.
	}

	{ //vertex array mapping buffer for color_program:
		//ask OpenGL to fill vertex_buffer_for_color_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program->Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(DrawDialogue::Vertex), //stride
			(GLbyte *)0 + offsetof(DrawDialogue::Vertex, Position) //offset
		);
		glEnableVertexAttribArray(color_texture_program->Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program->Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(DrawDialogue::Vertex), //stride
			(GLbyte *)0 + offsetof(DrawDialogue::Vertex, Color) //offset
		);
		glEnableVertexAttribArray(color_texture_program->Color_vec4);

		glVertexAttribPointer(
			color_texture_program->TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(DrawDialogue::Vertex), //stride
			(GLbyte *)0 + offsetof(DrawDialogue::Vertex, TexCoord) //offset
		);
		glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);
	}

	GL_ERRORS(); //PARANOIA: make sure nothing strange happened during setup
});


DrawDialogue::DrawDialogue(glm::mat4 const &world_to_clip_) : world_to_clip(world_to_clip_) {
}

void DrawDialogue::draw_text(std::string const &text, glm::vec3 const &anchor_in, glm::u8vec4 const &color) {
	// FT_Library library;
	// std::cout << text << "\n";
	FT_Init_FreeType(&library);
	// FT_Face face;
	FT_New_Face(library, "dist/Baloo-Regular.ttf", 0, &face);
	hb_buffer_t *buf = hb_buffer_create();
	hb_font_t *font = hb_ft_font_create(face, NULL);
	hb_buffer_add_utf8(buf, text.data(), -1, 0, -1);
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buf, hb_language_from_string("en", -1));
    unsigned int glyph_count = 0;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    glm::vec3 start = anchor_in;
    glyph_to_be_drawn.resize(0);
    for (int i = 0; i < glyph_count; ++i) {
        auto glyphid = glyph_info[i].codepoint;
        if (glyph_id_to_tex_id.find(glyphid) != glyph_id_to_tex_id.end()) {
        	Character cur = glyph_id_to_tex_id[glyphid];
        	// printf("%d\n", glyphid);
        	int r = cur.size.y;
			int w = cur.size.x;
			int offset_left = cur.bearing.x;
			int offset_top = cur.bearing.y;

	        float x_step = w * scale;
	        float y_step = r * scale;

			glm::vec3 upper_left = start + glm::vec3(scale * offset_left, -scale * (r - offset_top), 0.0f);
			glm::vec3 upper_right = upper_left + glm::vec3(x_step, 0.0f, 0.0f);
			glm::vec3 bottom_left = upper_left + glm::vec3(0.0f, y_step, 0.0f);
			glm::vec3 bottom_right = upper_left + glm::vec3(x_step, y_step, 0.0f);
			
			glm::vec2 upper_left_tex = glm::vec2(0.0f, 0.0f);
			glm::vec2 upper_right_tex = upper_left_tex + glm::vec2(1.0f, 0.0f);
			glm::vec2 bottom_left_tex = upper_left_tex + glm::vec2(0.0f, 1.0f);
			glm::vec2 bottom_right_tex = upper_left_tex + glm::vec2(1.0f, 1.0f);
			if (r && w) {
				attribs.emplace_back(upper_left, color, bottom_left_tex);
				attribs.emplace_back(bottom_left, color, upper_left_tex);
				attribs.emplace_back(bottom_right, color, upper_right_tex);

				attribs.emplace_back(upper_right, color, bottom_right_tex);
				attribs.emplace_back(upper_left, color, bottom_left_tex);
				attribs.emplace_back(bottom_right, color, upper_right_tex);

				start += glm::vec3(x_step, 0.0f, 0.0f);
				glyph_to_be_drawn.push_back(cur.tex);
			} else {
				start += glm::vec3(20.0f * scale, 0.0f, 0.0f);
				if (start.x - anchor_in.x >= scale * 20.0f * 50.0f) {
					start.x = anchor_in.x;
					start += glm::vec3(0.0f, -scale * 30.0f, 0.0f);
				}
			}
        }

    }
    hb_buffer_destroy(buf);
  	hb_font_destroy(font);
 //    FT_Done_Face(face);
	// FT_Done_FreeType(library);
}

DrawDialogue::~DrawDialogue() {
	if (attribs.empty()) return;

	//based on DrawSprites.cpp :

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, attribs.size() * sizeof(attribs[0]), attribs.data(), GL_STREAM_DRAW); //upload attribs array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_program as current program:
	glUseProgram(color_texture_program->program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(world_to_clip));

	//use the mapping vertex_buffer_for_color_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);
	
	int start = 0;
	for (auto iter = glyph_to_be_drawn.begin(); iter != glyph_to_be_drawn.end(); iter++) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, *iter);
		// printf("%u\n", *iter);
		//run the OpenGL pipeline:
		glDrawArrays(GL_TRIANGLES, start, 6);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		start += 6;
	}

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
}


