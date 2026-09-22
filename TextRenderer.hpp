#pragma once

#include "GL.hpp"

#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

#include <string>
#include <unordered_map>
#include <vector>

struct TextRenderer {
    TextRenderer(std::string const &font_path, uint32_t pixel_size);
	~TextRenderer();
	TextRenderer(TextRenderer const &) = delete;
	TextRenderer &operator=(TextRenderer const &) = delete;

    // Function to actually draw the text
    float draw_text(std::string const& text, glm::vec2 topLeft, float maxWidth, glm::u8vec4 color, glm::uvec2 const& drawSize, float scale = 1.0f) const;

    float measure_height(std::string const& text, float maxWidth, float scale = 1.0f) const;

    // Font metrics
	float ascender = 0.0f; //distance from the top of a line to its baseline
	float line_height = 0.0f; //distance between baselines
	float space_size = 0.0f; //width of a space

private:
    // Glyph by HarfBuzz
    struct Glyph {
        uint32_t index;
        glm::vec2 offset;
    };

    // Permanent Glpyh loaded by FreeType
    struct LoadedGlyph {
        glm::ivec2 pos = glm::ivec2(0);
        glm::ivec2 size = glm::ivec2(0);
        glm::ivec2 bearing = glm::ivec2(0);
    };

    // What we send to GPU
    struct Vertex {
        glm::vec2 pos;
        glm::vec2 coord;
        glm::u8vec4 color;
    };

    // Shape a single word and append it to out
    float shape_word(char const* str, int len, std::vector<Glyph> &out) const;

    LoadedGlyph const &get_glyph(uint32_t index) const;

    float layout(std::string const &text, glm::vec2 topLeft, float maxWidth, glm::u8vec4 color,
                 glm::uvec2 const &drawSize, std::vector<Vertex> *verts, float scale) const;

    FT_Library ft_library;
    FT_Face ft_face;
    hb_font_t *hb_font;
    hb_buffer_t *hb_buffer;

    static constexpr int AtlasSize = 1024;
    static constexpr int Padding = 2;
    GLuint atlas_tex = 0;
    mutable std::unordered_map<uint32_t, LoadedGlyph> glyph_cache;
    mutable glm::ivec2 atlas_cursor = glm::ivec2(Padding); //Where next glyph goes
    mutable int atlas_row_height = 0;

    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    mutable std::vector<Vertex> vertex_scratch;
};