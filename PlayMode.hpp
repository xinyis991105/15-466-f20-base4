#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "Dialogue.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

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
	} front, down, up;

	struct DialogueLine {
		std::string dialogue_text;
		int which;
		int next;
	};

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// 3D asset control
	Scene::Transform *cube = nullptr;
	Scene::Transform *knot = nullptr;
	std::vector<Scene::Transform*> collectibles;
	std::vector<int> play_which;
	std::vector<float> shrink;
	glm::quat cube_base_rotation;
	glm::quat knot_base_rotation;
	glm::quat camera_base_rotation;
	glm::vec3 cube_base_position;
	glm::vec3 knot_base_position;
	glm::vec3 camera_base_position;
	glm::vec3 collectible_base_scale;
	float swing = 0.0f;
	bool listening = false;
	bool pinching = false;
	bool collecting = false;
	float camera_rotation_angle = 0.0f;
	float angle = 0.0f;
	glm::vec3 camera_rel_cube;
	std::shared_ptr< Sound::PlayingSample > bgm;

	// game flow control
	bool reset = false;
	bool shrinking = false;
	bool over = false;
	int points = 0;
	float until_restart = 5.0f;

	// dialogue related
	bool dialogue_mode = true;
	bool at_choice = false;
	bool play_last_sound = false;
	bool pls_already = false;
	int choice_index = 0;
	bool first_choice_left = true;
	std::vector<DialogueLine> lines;
	int cur_lines = 0;
	int next = -2;


	//camera:
	Scene::Camera *camera = nullptr;
};
