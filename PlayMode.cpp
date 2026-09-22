#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "TextRenderer.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <iterator>

// Using Kings Font designed by Robert Leuschke.
// Downloaded from Google Fonts and are licensed under the Open Font License
// https://fonts.google.com/specimen/Kings?preview.script=Latn


GLuint cell_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > cell_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("Cell.pnct"));
	cell_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > cell_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("Cell.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = cell_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = cell_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load<TextRenderer> text_renderer(LoadTagDefault, []() -> TextRenderer const* {
	return new TextRenderer(data_path("Kings-Regular.ttf"), 48);
});

Load<Story> story(LoadTagDefault, []() -> Story const* {
	return new Story(make_story());
});

PlayMode::PlayMode() : scene(*cell_scene) {
	// Get all the cameras in the scene
	for (Scene::Camera &cam : scene.cameras) {
		cameraNames[cam.transform->name] = &cam;
	}
	if (cameraNames.empty()) throw std::runtime_error("No Cameras Found");

	camera = &scene.cameras.front();
	go_to_node(story->start);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN && !evt.key.repeat) {
		if (evt.key.key >= SDLK_1 && evt.key.key <= SDLK_9) {
			take_choice(uint32_t(evt.key.key - SDLK_1));
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {


}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{
		float scale = float(drawable_size.y) / 1080.0f;
		float margin = 40.0f * scale;
		float maxWidth = float(drawable_size.x) * 0.6f;

		auto draw_outlined = [&](std::string const &text, glm::vec2 pos, glm::u8vec4 color) {
			glm::u8vec4 outline = glm::u8vec4(0x00, 0x00, 0x00, 0xff);
			float x = std::max(1.0f, 2.0f * scale);
			glm::vec2 offsets[4] = {
				glm::vec2(-x, 0.0f), glm::vec2(x, 0.0f),
				glm::vec2(0.0f, -x), glm::vec2(0.0f, x),
			};
			for (glm::vec2 const &offset : offsets) {
				text_renderer->draw_text(text, pos + offset, maxWidth, outline, drawable_size, scale);
			}
			return text_renderer->draw_text(text, pos, maxWidth, color, drawable_size, scale);
		};

		glm::vec2 at = glm::vec2(margin, margin);

		Story::Node const &node = story->nodes.at(curNode);

		at.y += draw_outlined(node.text, at, glm::u8vec4(0xff, 0xf0, 0xd0, 0xff));
		at.y += 20.0f * scale;

		for (size_t i = 0; i < visible_choices.size(); ++i) {
			std::string line = std::to_string(i + 1) + ") " + visible_choices[i]->text;
			at.y += draw_outlined(line, at, glm::u8vec4(0xe8, 0xb8, 0x60, 0xff));
		}

		//endings have no choices:
		if (node.choices.empty()) {
			draw_outlined("The End", at, glm::u8vec4(0xe8, 0xb8, 0x60, 0xff));
		}
	}
	GL_ERRORS();
}

void PlayMode::set_camera(std::string const &name) {
	auto cam = cameraNames.find(name);
	if (cam == cameraNames.end()) {
		throw std::runtime_error("No camera named '" + name + "' in the scene.");
	}
	camera = cam->second;
}

void PlayMode::go_to_node(std::string const &name) {
	auto found = story->nodes.find(name);
	if (found == story->nodes.end()) {
		throw std::runtime_error("Story has no node named '" + name + "'.");
	}
	curNode = name;
	Story::Node const &node = found->second;

	if (!node.camera.empty()) {
		set_camera(node.camera);
	}

	//Decide which choices to show based on flags:
	visible_choices.clear();
	for (Story::Choice const &choice : node.choices) {
		bool show = true;
		for (std::string const &flag : choice.need) {
			if (!flags.count(flag)) show = false;
		}
		for (std::string const &flag : choice.need_not) {
			if (flags.count(flag)) show = false;
		}
		if (show) visible_choices.emplace_back(&choice);
	}
}

void PlayMode::take_choice(uint32_t index) {
	Story::Choice const &choice = *visible_choices[index];

	for (std::string const &flag : choice.set_flags) {
		flags.insert(flag);
	}
	for (std::string const &mesh : choice.hide_meshes) {
		hide_mesh(mesh);
	}
	go_to_node(choice.go_to);
}

void PlayMode::hide_mesh(std::string const &name) {
	scene.drawables.remove_if([&](Scene::Drawable const &drawable) {
		return drawable.transform->name == name;
	});
}