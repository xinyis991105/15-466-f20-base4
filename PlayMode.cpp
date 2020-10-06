#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"
#include "ColorProgram.hpp"

#include "DrawLines.hpp"
#include "Dialogue.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"

#include <iostream>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

#include <random>

// Based on what I had for game3 and added the dialogue component!
GLuint hexapod_meshes_for_lit_color_texture_program = 0;
std::ifstream chars_stream;
std::ifstream ranges_stream;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("shabby.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("shabby.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > project_whatev(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Project-whatev.wav"));
});

Load< Sound::Sample > wrong_ball(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("wrong_ball.wav"));
});

Load< Sound::Sample > there_you_go(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("there_you_go.wav"));
});

// referenced my asset pipeline code from game1
Load<void> dialogue_reading(LoadTagDefault, []() {
	chars_stream.open(data_path("../chars.asset"));
	ranges_stream.open(data_path("../ranges.asset"));
	return;
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	std::vector<char> chars;
	std::vector<std::pair<int, int>> ranges;
	read_chunk(chars_stream, "char", &chars);
	read_chunk(ranges_stream, "rang", &ranges);
	lines.resize(ranges.size());
	for (int i = 0; i < ranges.size(); i++) {
		int start = ranges[i].first;
		int end = ranges[i].second;
		std::string s("");
		for (int j = start; j < end; j++) {
			s += chars[j];
		}
		lines[i].dialogue_text = s;
		lines[i].which = i >= 31 ? 1 : 0;
		std::string tmp = s;
		int upuntil = 0;
		while (tmp.find(" ") != std::string::npos) {
			upuntil += tmp.find(" ") + 1;
			tmp = tmp.substr(tmp.find(" ") + 1);
		}
		lines[i].dialogue_text = lines[i].dialogue_text.substr(0, upuntil - 1);
		lines[i].next = stoi(tmp);
	}

	std::mt19937 mt;
	if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
	collectibles.resize(8);
	play_which.resize(8);
	shrink.resize(8);
	auto rand = mt();
	for (int i=0; i < 8; i++) {
		play_which[i] = rand >> i & 1;
		shrink[i] = -1.0f;
	}
	
	for (auto &transform : scene.transforms) {
		if (transform.name == "Cube") cube = &transform;
		else if (transform.name == "Knot") knot = &transform;
		else if (transform.name == "Collectible1") collectibles[0] = &transform;
		else if (transform.name == "Collectible2") collectibles[1] = &transform;
		else if (transform.name == "Collectible3") collectibles[2] = &transform;
		else if (transform.name == "Collectible4") collectibles[3] = &transform;
		else if (transform.name == "Collectible5") collectibles[4] = &transform;
		else if (transform.name == "Collectible6") collectibles[5] = &transform;
		else if (transform.name == "Collectible7") collectibles[6] = &transform;
		else if (transform.name == "Collectible8") collectibles[7] = &transform;
	}

	cube_base_rotation = cube->rotation;
	knot_base_rotation = knot->rotation;
	cube_base_position = cube->position;
	knot_base_position = knot->position;
	collectible_base_scale = collectibles[0]->scale;
	cube->scale = glm::vec3(0.7f, 0.7f, 0.7f);

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	
	camera = &scene.cameras.front();
	camera_base_position = camera->transform->position;
	camera_base_rotation = camera->transform->rotation;
	camera_rel_cube = camera_base_position - cube_base_position;

	//start music loop playing:
	// (note: position will be over-ridden in update())
	bgm = Sound::loop_3D(*project_whatev, 0.2f, cube_base_position, 10.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_SPACE) {
			if (!dialogue_mode) {
				front.downs += 1;
				front.pressed = true;
			} else if (choice_index != 1 || !at_choice){
				if (cur_lines != 42 || (cur_lines == 42 && play_last_sound)) {
					next = lines[cur_lines].next;
				} else {
					play_last_sound = true;
				}
			} else {
				first_choice_left = !first_choice_left;
			}
			if (over && until_restart == 0.0f) {
				reset = true;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_l) {
			listening = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_p) {
			pinching = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_c) {
			collecting = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RETURN) {
			if (choice_index == 1 && at_choice) {
				cur_lines = first_choice_left ? 32 : 33;
				next = lines[cur_lines].next;
				at_choice = false;
			}
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_SPACE) {
			front.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
	  	if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
	  		if (!dialogue_mode) {
				camera_rotation_angle += -evt.motion.xrel;
				angle = camera_rotation_angle / float(window_size.x) * float(M_PI);
			}
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	swing += elapsed / 5.0f;
	swing -= std::floor(swing);

	cube->rotation = cube_base_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(swing * 2.0f * float(M_PI))),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

	float knot_swing = swing;

	knot->rotation = knot_base_rotation * glm::angleAxis(
		glm::radians(25.0f * std::sin(knot_swing * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	for (int i=0; i < 8; i++) {
		if (shrink[i] > 0.0) {
			shrink[i] = shrink[i] - 0.03f;
			if (shrink[i] < 0.0f) {
				shrink[i] = 0.0f;
			}
			collectibles[i]->scale = glm::vec3(shrink[i]);
		}
	}

	if (play_last_sound && !pls_already) {
		if (first_choice_left) {
			Sound::play_3D(*wrong_ball, 1.0f, cube->position, 10.0f);
		} else {
			Sound::play_3D(*there_you_go, 1.0f, cube->position, 10.0f);
		}
		pls_already = true;
	}

	if (over) {
		until_restart -= elapsed;
		until_restart = std::max(0.0f, until_restart);
	}

	if (reset && over) {
		reset = false;
		over = false;
		pls_already = false;
		play_last_sound = false;
		cur_lines = 0;
		next = -2;
		cube->position = cube_base_position;
		knot->position = knot_base_position;
		cube->scale = glm::vec3(0.7f, 0.7f, 0.7f);
		knot->scale = glm::vec3(1.0f, 1.0f, 1.0f);
		camera->transform->position = camera_base_position;
		camera->transform->rotation = camera_base_rotation;
		listening = false;
		pinching = false;
		collecting = false;
		shrinking = false;
		camera_rotation_angle = 0.0f;
		choice_index = 0;
		at_choice = false;
		first_choice_left = true;
		dialogue_mode = true;
		until_restart = 3.0f;
		points = 0;
		std::mt19937 mt;
		auto rand = mt();
		for (int i=0; i < 8; i++) {
			play_which[i] = rand >> i & 1;
			shrink[i] = -1.0f;
			collectibles[i]->scale = collectible_base_scale;
		}
		bgm = Sound::loop_3D(*project_whatev, 0.2f, cube_base_position, 10.0f);
	}

	if (!dialogue_mode) {
		if (points == 8) {
			dialogue_mode = true;
			at_choice = false;
			cur_lines = 22;
			bgm->stop(5.0f);
		}

		if (!over) {
			float speed = 0.007f;
			glm::vec3 dir = cube->position - camera->transform->position;
			dir.z = 0.0f;
			if (up.pressed && !down.pressed) {
				cube->position.z += 0.01f;
				knot->position.z += 0.01f;
				camera->transform->position.z += 0.01f;
			}
			if (!up.pressed && down.pressed) {
				cube->position.z -= 0.01f;
				knot->position.z -= 0.01f;
				camera->transform->position.z -= 0.01f;
			} 
			if (front.pressed) {
				cube->position += speed * dir;
				knot->position += speed * dir;
				camera->transform->position += speed * dir;
			}

			cube->position.x = std::min(6.7f, cube->position.x);
			cube->position.x = std::max(-6.7f, cube->position.x);
			cube->position.y = std::min(6.7f, cube->position.y);
			cube->position.y = std::max(-6.7f, cube->position.y);
			cube->position.z = std::max(-0.5f, cube->position.z);
			cube->position.z = std::min(5.5f, cube->position.z);
			
			knot->position.x = std::min(6.7f, knot->position.x);
			knot->position.x = std::max(-6.7f, knot->position.x);
			knot->position.y = std::min(6.7f, knot->position.y);
			knot->position.y = std::max(-6.7f, knot->position.y);
			knot->position.z = std::max(-0.5f, knot->position.z);
			knot->position.z = std::min(5.5f, knot->position.z);

			glm::vec3 tmp = glm::vec3(
				camera_rel_cube.x * cos(angle) - camera_rel_cube.y * sin(angle),
				camera_rel_cube.x * sin(angle) + camera_rel_cube.y * cos(angle),
				camera_rel_cube.z
			);
			camera->transform->position = cube->position + tmp;
			camera->transform->rotation = glm::normalize(
				camera_base_rotation
				* glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f))
			);
		}

		if (listening) {
			for (int i=0; i < 8; i++) {
				float dx = cube->position.x - collectibles[i]->position.x;
				float dy = cube->position.y - collectibles[i]->position.y;
				float dz = cube->position.z - collectibles[i]->position.z;
				if (dx * dx + dy * dy + dz * dz <= 2.0f && shrink[i] == -1.0f) {
					if (play_which[i]) {
						Sound::play_3D(*there_you_go, 0.2f, collectibles[i]->position, 10.0f);
					} else {
						Sound::play_3D(*wrong_ball, 1.0f, collectibles[i]->position, 10.0f);
					}
					break;
				}
			}
			listening = false;
		}

		if (pinching && !collecting) {
			for (int i=0; i < 8; i++) {
				float dx = cube->position.x - collectibles[i]->position.x;
				float dy = cube->position.y - collectibles[i]->position.y;
				float dz = cube->position.z - collectibles[i]->position.z;
				if (dx * dx + dy * dy + dz * dz <= 2.0f && shrink[i] == -1.0f) {
					if (shrink[i] == 0.0f) continue;
					if (play_which[i]) {
						bgm->stop(5.0f);
						dialogue_mode = true;
						cur_lines = 19;
						shrinking = true;
						at_choice = false;
					} else {
						// shrink
						if (shrink[i] != 0.0f) {
							shrink[i] = 1.5f;
							points += 1;
							bgm->set_volume(std::min(0.2f * (points + 1), 0.6f), 1.0f);
						}
					}
					break;
				}
			}
			pinching = false;
		}

		if (collecting && !pinching) {
			for (int i=0; i < 8; i++) {
				float dx = cube->position.x - collectibles[i]->position.x;
				float dy = cube->position.y - collectibles[i]->position.y;
				float dz = cube->position.z - collectibles[i]->position.z;
				if (dx * dx + dy * dy + dz * dz <= 2.0f && shrink[i] == -1.0f) {
					if (play_which[i]) {
						if (shrink[i] != 0.0f) {
							shrink[i] = 1.5f;
							points += 1;
							bgm->set_volume(std::min(0.2f * (points + 1), 0.6f), 1.0f);
						}
					} else {
						// shrink
						bgm->stop(5.0f);
						dialogue_mode = true;
						shrinking = true;
						at_choice = false;
						cur_lines = 19;
					}
					break;
				}
			}
			collecting = false;
		}

		//reset button press counters:
		front.downs = 0;
		up.downs = 0;
		down.downs = 0;
	} else {
		// hardcoded dialogue flow specific to this game
		if (shrinking) {
			cube->scale -= glm::vec3(0.001f);
			if (cube->scale.x < 0.0f) {
				cube->scale = glm::vec3(0.0f);
				shrinking = false;
			}
		}
		switch (next) {
			case -1:
				over = true;
				break;
			case 0:
				at_choice = true;
				choice_index++;
				if (choice_index == 2) {
					dialogue_mode = false;
				}
				next = -2;
				break;
			case -2:
				break;
			default:
				cur_lines = next - 1;
		}
	}
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

	{ //use DrawLines to overlay some text:
		if (dialogue_mode) {
			glDisable(GL_DEPTH_TEST);
			float aspect = float(drawable_size.x) / float(drawable_size.y);
			DrawDialogue dialogue(glm::mat4(
				1.0f / aspect, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			));
			DrawDialogue dialogue_sec(glm::mat4(
				1.0f / aspect, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			));

			constexpr float H = 0.09f;
			glm::u8vec4 text_color = lines[cur_lines].which || (choice_index == 1 && at_choice) ? glm::u8vec4(0x1, 0x34, 0x1, 0xff) : glm::u8vec4(0x34, 0x1, 0x1, 0xff);
			glm::vec3 position = lines[cur_lines].which ?
				glm::vec3(-aspect + 6.0f * H, -1.0 + 4.0f * H, 0.0f) : 
				glm::vec3(-aspect + 6.0f * H, -1.0 + 14.0f * H, 0.0f);
			if (!over) {
				if (choice_index != 1 || !at_choice) {
					dialogue.draw_text(lines[cur_lines].dialogue_text, position, text_color);
					if (cur_lines == 42 && !play_last_sound) {
						dialogue_sec.draw_text("(Press Space to listen)", glm::vec3(-aspect + 6.0f * H, -1.0f + 1.0f * H, 0.0f), text_color);
					}
				} else {
					glm::u8vec4 text_on = glm::u8vec4(0xfe, 0xfe, 0x1, 0xff);
					if (first_choice_left) {
						dialogue.draw_text(lines[32].dialogue_text, glm::vec3(-aspect + 4.0f * H, -1.0f + 4.0f * H, 0.0f), text_on);
						dialogue_sec.draw_text(lines[33].dialogue_text, glm::vec3(-aspect + 30.0f * H, -1.0f + 4.0f * H, 0.0f), text_color);
					} else {
						dialogue.draw_text(lines[32].dialogue_text, glm::vec3(-aspect + 4.0f * H, -1.0f + 4.0f * H, 0.0f), text_color);
						dialogue_sec.draw_text(lines[33].dialogue_text, glm::vec3(-aspect + 30.0f * H, -1.0f + 4.0f * H, 0.0f), text_on);
					}
				}
			} else if (until_restart == 0.0f) {
				dialogue.draw_text("Press Space to restart", glm::vec3(-aspect + 6.0f * H, -1.0 + 10.0f * H, 0.0f), text_color);
			}
		}
	}

	GL_ERRORS();
}
