#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "Story.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//camera:
	Scene::Camera *camera = nullptr;
	std::unordered_map<std::string, Scene::Camera*> cameraNames;

	void set_camera(std::string const& name);

	std::string curNode;
	std::unordered_set<std::string> flags;
	std::vector<Story::Choice const*> visible_choices;

	void go_to_node(std::string const &name);
	void take_choice(uint32_t index);
	void hide_mesh(std::string const &name);

};
