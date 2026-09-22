#include "TextRenderer.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

#include <hb-ft.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

//Structure of FreeType/HarfBuzz setup + shaping based on:
//  https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
//Glyph rasterization based on the FreeType tutorial:
//  https://www.freetype.org/freetype2/docs/tutorial/step1.html

TextRenderer::TextRenderer(std::string const &font_path, uint32_t pixelSize) {
    // Load the font
    if (FT_Error err = FT_Init_FreeType(&ft_library)) {
        throw std::runtime_error("FT_Init_FreeType failed: " + std::to_string(err));
    }
    if (FT_Error err = FT_New_Face(ft_library, font_path.c_str(), 0, &ft_face)) {
        throw std::runtime_error("FT_New_Face failed: " + std::to_string(err));
    }
    if (FT_Error err = FT_Set_Pixel_Sizes(ft_face, 0, pixelSize)) {
        throw std::runtime_error("FT_Set_Pixel_Size failed: " + std::to_string(err));
    }

    ascender = ft_face->size->metrics.ascender / 64.0f;
    line_height = ft_face->size->metrics.height / 64.0f;

    hb_font = hb_ft_font_create(ft_face, nullptr);
    hb_buffer = hb_buffer_create();

    { // measure a space
        std::vector<Glyph> scratch;
        space_size = shape_word(" ", 1, scratch);
    }

    glGenTextures(1, &atlas_tex);
    glBindTexture(GL_TEXTURE_2D, atlas_tex);
    {
        std::vector<uint8_t> zeros(AtlasSize * AtlasSize, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, AtlasSize, AtlasSize, 0, GL_RED, GL_UNSIGNED_BYTE, zeros.data());
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    program = gl_compile_program(
		//vertex shader
		"#version 330\n"
		"in vec2 Position;\n"
		"in vec2 TexCoord;\n"
		"in vec4 Color;\n"
		"out vec2 texCoord;\n"
		"out vec4 color;\n"
		"void main() {\n"
		"	gl_Position = vec4(Position, 0.0, 1.0);\n"
		"	texCoord = TexCoord;\n"
		"	color = Color;\n"
		"}\n"
	,
		//fragment shader
		"#version 330\n"
		"uniform sampler2D TEX;\n"
		"in vec2 texCoord;\n"
		"in vec4 color;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	float coverage = texture(TEX, texCoord).r;\n"
		"	fragColor = vec4(color.rgb, color.a * coverage);\n"
		"}\n"
	);

    GLint Position_vec2 = glGetAttribLocation(program, "Position");
	GLint TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");
	GLint Color_vec4 = glGetAttribLocation(program, "Color");

	glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, "TEX"), 0);
	glUseProgram(0);

    glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glVertexAttribPointer(Position_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLbyte *)0 + offsetof(Vertex, pos));
	glEnableVertexAttribArray(Position_vec2);
	glVertexAttribPointer(TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLbyte *)0 + offsetof(Vertex, coord));
	glEnableVertexAttribArray(TexCoord_vec2);
	glVertexAttribPointer(Color_vec4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLbyte *)0 + offsetof(Vertex, color));
	glEnableVertexAttribArray(Color_vec4);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	GL_ERRORS();
}

TextRenderer::~TextRenderer() {
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(program);
	glDeleteTextures(1, &atlas_tex);

	hb_buffer_destroy(hb_buffer);
	hb_font_destroy(hb_font);
	FT_Done_Face(ft_face);
	FT_Done_FreeType(ft_library);
}

float TextRenderer::shape_word(char const* str, int len, std::vector<Glyph> &out) const {
    hb_buffer_clear_contents(hb_buffer);
	hb_buffer_add_utf8(hb_buffer, str, len, 0, len);
	hb_buffer_guess_segment_properties(hb_buffer);
	hb_shape(hb_font, hb_buffer, nullptr, 0);

	unsigned int count = 0;
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, &count);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, &count);

	float pen_x = 0.0f, pen_y = 0.0f;
	for (unsigned int i = 0; i < count; ++i) {
		Glyph g;
		g.index = info[i].codepoint;
		g.offset.x = pen_x + pos[i].x_offset / 64.0f;
		g.offset.y = -(pen_y + pos[i].y_offset / 64.0f);
		out.emplace_back(g);

		pen_x += pos[i].x_advance / 64.0f;
		pen_y += pos[i].y_advance / 64.0f;
	}
	return pen_x;
}


TextRenderer::LoadedGlyph const &TextRenderer::get_glyph(uint32_t index) const {
    auto found = glyph_cache.find(index);
	if (found != glyph_cache.end()) return found->second;

	LoadedGlyph loaded;

	if (FT_Error err = FT_Load_Glyph(ft_face, index, FT_LOAD_RENDER)) {
		return glyph_cache.emplace(index, loaded).first->second;
	}

	FT_GlyphSlot slot = ft_face->glyph;
	FT_Bitmap const &bitmap = slot->bitmap;
	loaded.size = glm::ivec2(bitmap.width, bitmap.rows);
	loaded.bearing = glm::ivec2(slot->bitmap_left, slot->bitmap_top);

	//blank glyphs have no bitmap
	if (loaded.size.x == 0 || loaded.size.y == 0) {
		return glyph_cache.emplace(index, loaded).first->second;
	}

	if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
		loaded.size = glm::ivec2(0);
		return glyph_cache.emplace(index, loaded).first->second;
	}

    if (atlas_cursor.x + loaded.size.x + Padding > AtlasSize) {
		//move to the next row:
		atlas_cursor.x = Padding;
		atlas_cursor.y += atlas_row_height + Padding;
		atlas_row_height = 0;
	}
	if (atlas_cursor.y + loaded.size.y + Padding > AtlasSize) {
		std::cerr << "WARNING: glyph atlas is full; glyph " << index << " will not be drawn." << std::endl;
		loaded.size = glm::ivec2(0);
		return glyph_cache.emplace(index, loaded).first->second;
	}
	loaded.pos = atlas_cursor;
	atlas_cursor.x += loaded.size.x + Padding;
	atlas_row_height = std::max(atlas_row_height, loaded.size.y);

	std::vector< uint8_t > pixels(loaded.size.x * loaded.size.y);
	for (int row = 0; row < loaded.size.y; ++row) {
		uint8_t const *src = bitmap.buffer + row * bitmap.pitch;
		std::copy(src, src + loaded.size.x, pixels.begin() + row * loaded.size.x);
	}

    glBindTexture(GL_TEXTURE_2D, atlas_tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		loaded.pos.x, loaded.pos.y, loaded.size.x, loaded.size.y,
		GL_RED, GL_UNSIGNED_BYTE, pixels.data());
	glBindTexture(GL_TEXTURE_2D, 0);

	return glyph_cache.emplace(index, loaded).first->second;
}


float TextRenderer::layout(std::string const &text, glm::vec2 topLeft, float maxWidth, glm::u8vec4 color,
                           glm::uvec2 const &drawSize, std::vector<Vertex> *verts) const {

    topLeft = glm::round(topLeft);

	float pen_x = 0.0f;
	float baseline = ascender;
	bool line_empty = true;

	auto new_line = [&]() {
		pen_x = 0.0f;
		baseline += line_height;
		line_empty = true;
	};

	glm::vec2 to_clip = glm::vec2(2.0f / drawSize.x, -2.0f / drawSize.y);
	auto clip = [&](float x, float y) {
		return glm::vec2(x * to_clip.x - 1.0f, y * to_clip.y + 1.0f);
	};

	std::vector<Glyph> word;

	size_t at = 0;
	while (at < text.size()) {
		char c = text[at];
		if (c == '\n') {
			new_line();
			++at;
			continue;
		}
		if (c == ' ') {
			if (!line_empty) pen_x += space_size;
			++at;
			continue;
		}

		size_t end = text.find_first_of(" \n", at);
		if (end == std::string::npos) end = text.size();

		word.clear();
		float word_width = shape_word(text.data() + at, int(end - at), word);

		if (maxWidth > 0.0f && !line_empty && pen_x + word_width > maxWidth) {
			new_line();
		}

		if (verts) {
			for (Glyph const &g : word) {
				LoadedGlyph const &cg = get_glyph(g.index);
				if (cg.size.x == 0 || cg.size.y == 0) continue;

				float x0 = topLeft.x + std::round(pen_x + g.offset.x) + cg.bearing.x;
				float y0 = topLeft.y + std::round(baseline + g.offset.y) - cg.bearing.y;
				float x1 = x0 + cg.size.x;
				float y1 = y0 + cg.size.y;

				float u0 = cg.pos.x / float(AtlasSize);
				float v0 = cg.pos.y / float(AtlasSize);
				float u1 = (cg.pos.x + cg.size.x) / float(AtlasSize);
				float v1 = (cg.pos.y + cg.size.y) / float(AtlasSize);

				Vertex tl{clip(x0, y0), glm::vec2(u0, v0), color};
				Vertex tr{clip(x1, y0), glm::vec2(u1, v0), color};
				Vertex bl{clip(x0, y1), glm::vec2(u0, v1), color};
				Vertex br{clip(x1, y1), glm::vec2(u1, v1), color};

				verts->insert(verts->end(), {tl, bl, br, tl, br, tr});
			}
		}
		pen_x += word_width;
		line_empty = false;
		at = end;
	}

	//total height = number of lines * line height:
	return baseline - ascender + line_height;
}


float TextRenderer::draw_text(std::string const& text, glm::vec2 topLeft, float maxWidth, glm::u8vec4 color, glm::uvec2 const& drawSize) const {
    vertex_scratch.clear();
    float height = layout(text, topLeft, maxWidth, color, drawSize, &vertex_scratch);
    if (vertex_scratch.empty()) return height;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_scratch.size() * sizeof(Vertex), vertex_scratch.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, atlas_tex);
	glBindVertexArray(vao);

	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertex_scratch.size()));

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
	glDisable(GL_BLEND);

	GL_ERRORS();
	return height;
}

float TextRenderer::measure_height(std::string const& text, float maxWidth) const {
    return layout(text, glm::vec2(0.0f), maxWidth, glm::u8vec4(0), glm::uvec2(1), nullptr);
}