#pragma once
#include <algorithm>
#include <cmath>
#include "../../Dependencies/Inlcudes/GL/glew.h"

#include "shader_program.h"
#include "../../Dependencies/Inlcudes/glm/vec3.hpp"
#include "../../Dependencies/Inlcudes/glm/mat4x4.hpp"
#include "atmosphere_scattering.h"

#ifndef M_PI
#define M_PI	3.14159265
#endif


#define SPECTRUM_1_2_UNIT	10
#define SPECTRUM_3_4_UNIT	11
#define BUTTERFLY_UNIT		12
#define SLOPE_VARIANCE_UNIT	13
#define FFT_A_UNIT			14
#define FFT_B_UNIT			15
#define TEST_LAYER_UNIT		16
class ocean
{
public:
	ocean(int screen_width, int screen_height);
	void render(glm::mat4 projection, glm::mat4 view, glm::vec3 camera_position, float t, atmospheric_scattering scattering, glm::vec3 sun_dir);
	float grid_size = 8.0;
	bool choppy = true;
	const int N_SLOPE_VARIANCE = 10;
	float GRID1_SIZE = 893.0, GRID2_SIZE = 101.0, GRID3_SIZE = 21.0, GRID4_SIZE = 11.0;
	float WIND = 20.0;
	float OMEGA = 2.0;
	float A = 2.0;
	const float cm = 0.23f, km = 370.0f;
	int screen_width, screen_height;
	shader_program init, variances, fftx, ffty, render_program;
private:
	void load_programs();
	void generate_mesh(float width, float height);
	int bit_reverse(int i, int n);
	void compute_weight(int n, int k, float &wr, float &wi);
	float *compute_butterfly_texture();
	void simulate_fft_waves(float t);
	float get_slope_variance(float kx, float ky, float *spectrum_sample);
	void compute_slope_variance_tex();
	long lrandom(long *seed);
	float frandom(long *seed);
	float grandom(float mean, float stdd, long *seed);
	void get_spectrum_sample(int i, int j, float legnth_scale, float kmin, float *result);
	void generate_waves_spectrum();
	float sqr(float x);
	float omega(float k);
	float spectrum(float kx, float ky, bool omnnispectrum);
	const int PASSES = 8;
	const int FFT_SIZE = 1 << PASSES;
	float *spectrum_1_2_data, *spectrum_3_4_data;
	GLuint grid_vbo, grid_ibo, grid_vao;
	int grid_vbo_size = 0;
	GLuint spec_1_2_handle, spec_3_4_handle, butterfly_handle, slope_var_handle, fft_a_handle, fft_b_handle;
	void draw_quad(GLuint *vao);
};

