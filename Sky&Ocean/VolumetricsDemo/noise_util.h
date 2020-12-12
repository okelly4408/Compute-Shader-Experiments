#pragma once
#include "../../Dependencies/Inlcudes/glm/vec3.hpp"
#include "../../Dependencies/Inlcudes/GL/glew.h"
#include "../../Dependencies/Inlcudes/glm/detail/type_vec4.hpp"
#include "shader_program.h"
#define GRADIENT_TEXTURE_UNIT		60
#define PERMUTATION_TEXTURE_UNIT	61

static const glm::vec3 gradients[] = {
	glm::vec3(1, 1, 0),
	glm::vec3(-1, 1, 0),
	glm::vec3(1, -1, 0),
	glm::vec3(-1, -1, 0),
	glm::vec3(1, 0, 1),
	glm::vec3(-1, 0, 1),
	glm::vec3(0, 1, 1),
	glm::vec3(0, -1, 1),
	glm::vec3(0, 1, -1),
	glm::vec3(0, -1, -1),
	glm::vec3(1, 1, 0),
	glm::vec3(0, -1, 1),
	glm::vec3(-1, 1, 0),
	glm::vec3(0, -1, -1)
};
class noise_util {
public:
	noise_util()
	{
		create_noise_luts();
		noise_program = shader_program("C:/Users/Owen/Documents/Visual Studio 2015/Projects/MarchingCubes/TestCompute/TestCompute/noise_3D_compute.glsl");
	}
	void create_noise_map(GLuint *handle, GLuint unit, float width, float height, float depth);
    void perlin_noise(GLuint *handle, GLuint unit, float width, float height, float depth = 1);

private:
	GLuint gradientTexture;
	GLuint permutationTexture;
	shader_program noise_program;
	void create_noise_luts();
};