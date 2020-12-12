#pragma once
#include "../../Dependencies/Inlcudes/glm/mat4x4.hpp"
#define WORLD_UP 0.0f, 1.0f, 0.0f
#define FRONT 0.0f, 0.0f, -1.0f
#define START 0.0f, 0.0f, 1.0
#define PITCH	0.0f
#define YAW		-90.0f
#define SENSITIVITY	0.1f
#define SPEED 10.0f
#define FOV 45.0f

enum camera_compass
{
	FORWARD,
	BACK,
	RIGHT,
	LEFT,
	UP,
	DOWN
};
class camera
{
public:
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 world_up;
	float pitch;
	float yaw;
	float fov;
	float mouse_sensitivity;
	float speed;
	bool grounded = false;
	camera(glm::vec3 position = glm::vec3(START),
		float mouse_sensitivty = SENSITIVITY,
		float speed = SPEED,
		glm::vec3 up = glm::vec3(WORLD_UP),
		glm::vec3 front = glm::vec3(FRONT),
		float fov = FOV,
		float pitch = PITCH,
		float yaw = YAW);
	glm::mat4 getViewMatrix();
	void processKeyboard(camera_compass direction, float delta);
	void processMouse(float xoffset, float yoffset, bool constrain = true);
	void processScroll(float offset);
private:
	void updateAxes();
};