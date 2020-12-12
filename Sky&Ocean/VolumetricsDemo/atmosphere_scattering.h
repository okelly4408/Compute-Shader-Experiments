#pragma once
#define TRANSMITTANCE_UNIT	20
#define IRRADIANCE_UNIT		21
#define INSCATTER_UNIT		22
#define DELTA_E_UNIT		23
#define DELTA_R_UNIT		24
#define DELTA_M_UNIT		25
#define DELTA_J_UNIT		26
#define ATMOSPHERE_UNIT		27
#include "../../Dependencies/Inlcudes/glm/mat4x4.hpp"

const float Rg = 6360.0;
const float Rt = 6420.0;
const float RL = 6421.0;

const int TRANSMITTANCE_WIDTH = 256;
const int TRANSMITTANCE_HEIGHT = 64;

const int SKY_W = 64;
const int SKY_H = 16;

const int RES_R = 32;
const int RES_MU = 128;
const int RES_MU_S = 32;
const int RES_NU = 8;

#include "../../Dependencies/Inlcudes/GL/glew.h"
#include "shader_program.h"

class atmospheric_scattering
{
public:
	atmospheric_scattering(float width, float height);
	void render_skymap(glm::vec3 sun_dir, float time);
	GLuint transmittanceTexture;
	GLuint irradianceTexture;
	GLuint inscatterTexture;
	GLuint atmosphereTexture;
private:
	void load_programs();
	void init_textures();
	void init_fbo(GLuint *framebuffer, GLuint *unit, GLuint *handle);
	void precompute();
	static void draw_quad(GLuint *vao);
	float screen_width;
	float screen_height;
	GLuint deltaETexture;
	GLuint deltaSRTexture;
	GLuint deltaSMTexture;
	GLuint deltaJTexture;
	GLuint screen_quad;
	GLuint fbo;
	shader_program transmittance_compute;
	shader_program irradiance_compute;
	shader_program inscatter_compute;
	shader_program copy_irradiance_compute;
	shader_program copy_inscatter_compute;
	shader_program j_compute;
	shader_program irradiancen_compute;
	shader_program inscattern_compute;
	shader_program copy_inscattern_compute;
	shader_program atmosphere_program;
	shader_program reflectance_program;
};