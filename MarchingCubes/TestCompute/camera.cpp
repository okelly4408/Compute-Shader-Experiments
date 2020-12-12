#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "camera.h"

camera::camera(glm::vec3 position, float mouse_sensitivity, float speed, glm::vec3 up, glm::vec3 front, float fov, float pitch, float yaw)
{
	this->position = position;
	this->mouse_sensitivity = mouse_sensitivity;
	this->speed = speed;
	this->world_up = up;
	this->front = front;
	this->fov = fov;
	this->pitch = pitch;
	this->yaw = yaw;
}
glm::mat4 camera::getViewMatrix()
{
	return glm::lookAt(this->position, this->position + this->front, this->up);
}
void camera::processKeyboard(camera_compass direction, float delta)
{
	float velocity = delta * this->speed;
	switch(direction)
	{
	case FORWARD:
		this->position += this->front * velocity;
		break;
	case BACK:
		this->position -= this->front * velocity;
		break;
	case RIGHT:
		this->position += this->right * velocity;
		break;
	case LEFT:
		this->position -= this->right * velocity;
		break;
	case UP:
		this->position += this->up * velocity;
		break;
	case DOWN:
		this->position -= this->up * velocity;
		break;
	}
}
void camera::processMouse(float xoffset, float yoffset, bool constrain)
{
	xoffset *= this->mouse_sensitivity;
	yoffset *= this->mouse_sensitivity;

	this->yaw += xoffset;
	this->pitch += -yoffset;

	if (constrain)
	{
		if (this->pitch > 89.0f)
			this->pitch = 89.0f;
		if (this->pitch < -89.0f)
			this->pitch = -89.0f;
	}
	this->updateAxes();
}
void camera::processScroll(float offset)
{
	if (this->fov >= 1.0f && this->fov <= 45.0f)
		this->fov -= offset;
	if (this->fov <= 1.0f)
		this->fov = 1.0f;
	if (this->fov >= 45.0f)
		this->fov = 45.0f;
}
void camera::updateAxes()
{
	glm::vec3 front;
	front.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
	front.y = sin(glm::radians(this->pitch));
	front.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
	this->front = glm::normalize(front);
	this->right = glm::normalize(glm::cross(this->front, this->world_up));
	this->up = glm::normalize(glm::cross(this->right, this->front));
}


