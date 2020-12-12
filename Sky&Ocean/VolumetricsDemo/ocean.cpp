#include "ocean.h"
#include "../../Dependencies/Inlcudes/glm/detail/func_matrix.inl"
#include "../../Dependencies/Inlcudes/glm/gtc/type_ptr.hpp"
#include <iostream>
//#define BENCH
ocean::ocean(int screen_width, int screen_height)
{
	this->screen_width = screen_width;
	this->screen_height = screen_height;
	this->load_programs();
	glActiveTexture(GL_TEXTURE0 + BUTTERFLY_UNIT);
	glGenTextures(1, &butterfly_handle);
	glBindTexture(GL_TEXTURE_2D, butterfly_handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	float *data = compute_butterfly_texture();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, FFT_SIZE, PASSES, 0, GL_RGBA, GL_FLOAT, data);
	delete[] data;
	float maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
	glActiveTexture(GL_TEXTURE0 + FFT_A_UNIT);
	glGenTextures(1, &fft_a_handle);
	glBindTexture(GL_TEXTURE_2D_ARRAY, fft_a_handle);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, log2(FFT_SIZE) + 1, GL_RGBA16F, FFT_SIZE, FFT_SIZE, 5);
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	glActiveTexture(GL_TEXTURE0 + FFT_B_UNIT);
	glGenTextures(1, &fft_b_handle);
	glBindTexture(GL_TEXTURE_2D_ARRAY, fft_b_handle);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, log2(FFT_SIZE) + 1, GL_RGBA16F, FFT_SIZE, FFT_SIZE, 5);
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	generate_waves_spectrum();
	compute_slope_variance_tex();
	generate_mesh(screen_width, screen_height);
}
void ocean::render(glm::mat4 projection, glm::mat4 view, glm::vec3 camera_position, float t, atmospheric_scattering scattering, glm::vec3 sun_dir)
{
	simulate_fft_waves(t);
	glUseProgram(this->render_program.program_id);
	glActiveTexture(GL_TEXTURE0 + FFT_A_UNIT);
	glBindTexture(GL_TEXTURE_2D_ARRAY, fft_a_handle);
	glUniform1i(glGetUniformLocation(this->render_program.program_id, "wave_sampler"), FFT_A_UNIT);
	glUniformMatrix4fv(glGetUniformLocation(this->render_program.program_id, "screen_to_camera"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));
	glUniformMatrix4fv(glGetUniformLocation(this->render_program.program_id, "camera_to_world"), 1, GL_FALSE, glm::value_ptr(glm::inverse(view)));
	glUniformMatrix4fv(glGetUniformLocation(this->render_program.program_id, "world_to_screen"), 1, GL_FALSE, glm::value_ptr(projection * view));
	glUniform4f(glGetUniformLocation(this->render_program.program_id, "GRID_SIZES"), GRID1_SIZE, GRID2_SIZE, GRID3_SIZE, GRID4_SIZE);
	glUniform2f(glGetUniformLocation(this->render_program.program_id, "gridSize"), grid_size/float(screen_width), grid_size/float(screen_height));
	glUniform3f(glGetUniformLocation(this->render_program.program_id, "ocean_camera_pos"), camera_position.x, camera_position.y, camera_position.z);
	glUniform3f(glGetUniformLocation(this->render_program.program_id, "sun_dir"), sun_dir.x, sun_dir.y, sun_dir.z);
	glActiveTexture(GL_TEXTURE0 + ATMOSPHERE_UNIT);
	glBindTexture(GL_TEXTURE_2D, scattering.atmosphereTexture);
	glUniform1i(glGetUniformLocation(this->render_program.program_id, "reflectance_texture"), ATMOSPHERE_UNIT);
	glActiveTexture(GL_TEXTURE0 + TRANSMITTANCE_UNIT);
	glBindTexture(GL_TEXTURE_2D, scattering.transmittanceTexture);
	glUniform1i(glGetUniformLocation(this->render_program.program_id, "transmittance_texture"), TRANSMITTANCE_UNIT);
	glActiveTexture(GL_TEXTURE0 + IRRADIANCE_UNIT);
	glBindTexture(GL_TEXTURE_2D, scattering.irradianceTexture);
	glUniform1i(glGetUniformLocation(this->render_program.program_id, "irradiance_texture"), IRRADIANCE_UNIT);
	glActiveTexture(GL_TEXTURE0 + SLOPE_VARIANCE_UNIT);
	glBindTexture(GL_TEXTURE_3D, slope_var_handle);
	glUniform1i(glGetUniformLocation(this->render_program.program_id, "slope_variance_sampler"), SLOPE_VARIANCE_UNIT);
	glUniform1f(glGetUniformLocation(this->render_program.program_id, "time"), t);
	glBindVertexArray(grid_vao);
	glDrawElements(GL_TRIANGLES, grid_vbo_size, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
void ocean::load_programs()
{
	this->fftx = shader_program(".WaterShaders/fftx_compute.glsl");
	this->ffty = shader_program(".WaterShaders/ffty_compute.glsl");
	this->init = shader_program(".WaterShaders/init_compute.glsl");
	this->variances = shader_program(".WaterShaders/variances_compute.glsl");
	this->render_program = shader_program(".WaterShaders/ocean_vert.glsl", ".WaterShaders/ocean_frag.glsl");
}
void ocean::draw_quad(GLuint *vao)
{
	glm::vec4 vertices[] = 
	{
		glm::vec4(-1.0, -1.0, 0.0, 0.0),
		glm::vec4(1.0, -1.0, 1.0, 0.0),
		glm::vec4(1.0, 1.0, 1.0, 1.0),
		glm::vec4(-1.0, 1.0, 0.0, 1.0)
	};
	glm::uint indices[] = 
	{
		0, 1, 2,
		2, 3, 0
	};
	GLuint vbo, ibo;
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ibo);
	GLuint ocean_vao;
	glGenVertexArrays(1, &ocean_vao);
	glBindVertexArray(ocean_vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, false, 4 * sizeof(float), (GLvoid *)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	*vao = ocean_vao;
}
void ocean::generate_mesh(float width, float height)
{
	if (grid_vbo_size != 0) {
		glDeleteVertexArrays(1, &grid_vao);
		glDeleteBuffers(1, &grid_vbo);
		glDeleteBuffers(1, &grid_ibo);
	}
	glGenVertexArrays(1, &grid_vao);
	glBindVertexArray(grid_vao);
	glGenBuffers(1, &grid_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
	float horizon = tan(45 / 180.0 * M_PI);
	float s = std::min(1.1f, 0.5f + horizon * 0.5f);

	float vmargin = 0.1;
	float hmargin = 0.1;
	int size = int(ceil(height * (s + vmargin) / grid_size) + 5) * int(ceil(width * (1.0 + 2.0 * hmargin) / grid_size) + 5);
	if (size > 0)
	{
		glm::vec4 *data = new glm::vec4[int(ceil(height * (s + vmargin) / grid_size) + 5) * int(ceil(width * (1.0 + 2.0 * hmargin) / grid_size) + 5)];
		int n = 0;
		int nx = 0;
		for (float j = height * s - 0.1; j > -height * vmargin - grid_size; j -= grid_size) {
			nx = 0;
			for (float i = -width * hmargin; i < width * (1.0 + hmargin) + grid_size; i += grid_size) {
				data[n++] = glm::vec4(-1.0 + 2.0 * i / width, -1.0 + 2.0 * j / height, 0.0, 1.0);
				nx++;
			}
		}
		glBufferData(GL_ARRAY_BUFFER, n * 16, data, GL_STATIC_DRAW);
		delete[] data;
		glGenBuffers(1, &grid_ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid_ibo);
		grid_vbo_size = 0;
		GLuint *indices = new GLuint[6 * int(ceil(height * (s + vmargin) / grid_size) + 4) * int(ceil(width * (1.0 + 2.0 * hmargin) / grid_size) + 4)];
		int nj = 0;
		for (float j = height * s - 0.1; j > -height * vmargin; j -= grid_size) {
			int ni = 0;
			for (float i = -width * hmargin; i < width * (1.0 + hmargin); i += grid_size) {
				indices[grid_vbo_size++] = ni + (nj + 1) * nx;
				indices[grid_vbo_size++] = (ni + 1) + (nj + 1) * nx;
				indices[grid_vbo_size++] = (ni + 1) + nj * nx;
				indices[grid_vbo_size++] = (ni + 1) + nj * nx;
				indices[grid_vbo_size++] = ni + (nj + 1) * nx;
				indices[grid_vbo_size++] = ni + nj * nx;
				ni++;
			}
			nj++;
		}
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, grid_vbo_size * 4, indices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, false, 4 * sizeof(GL_FLOAT), (GLvoid *)0);
		glEnableVertexAttribArray(0);
		delete[] indices;
		glBindVertexArray(0);
	}
	else
	{
		grid_vbo_size = -1;
	}
}
float ocean::sqr(float x)
{
	return x * x;
}
float ocean::omega(float k)
{
	return sqrt(9.81 * k * (1.0 + sqr(k / km))); // Eq 24
}
long ocean::lrandom(long *seed)
{
	*seed = (*seed * 1103515245 + 12345) & 0x7FFFFFFF;
	return *seed;
}
float ocean::frandom(long *seed)
{
	long r = lrandom(seed) >> (31 - 24);
	return r / (float)(1 << 24);
}
float ocean::spectrum(float kx, float ky, bool omnispectrum = false)
{
	float U10 = WIND;
	float Omega = OMEGA;
	// phase speed
	float k = sqrt(kx * kx + ky * ky);
	float c = omega(k) / k;
	float kp = 9.81 * sqr(Omega / U10);
	float cp = omega(kp) / kp;
	float z0 = 3.7e-5 * sqr(U10) / 9.81 * pow(U10 / cp, 0.9f);
	float u_star = 0.41 * U10 / log(10.0 / z0); 
	float Lpm = exp(-5.0 / 4.0 * sqr(kp / k)); 
	float gamma = Omega < 1.0 ? 1.7 : 1.7 + 6.0 * log(Omega);
	float sigma = 0.08 * (1.0 + 4.0 / pow(Omega, 3.0f)); 
	float Gamma = exp(-1.0 / (2.0 * sqr(sigma)) * sqr(sqrt(k / kp) - 1.0));
	float Jp = pow(gamma, Gamma);
	float Fp = Lpm * Jp * exp(-Omega / sqrt(10.0) * (sqrt(k / kp) - 1.0));
	float alphap = 0.006 * sqrt(Omega);
	float Bl = 0.5 * alphap * cp / c * Fp;
	float alpham = 0.01 * (u_star < cm ? 1.0 + log(u_star / cm) : 1.0 + 3.0 * log(u_star / cm));
	float Fm = exp(-0.25 * sqr(k / km - 1.0));
	float Bh = 0.5 * alpham * cm / c * Fm * Lpm;
	if (omnispectrum)
	{
		return A * (Bl + Bh) / (k * sqr(k));
	}
	float a0 = log(2.0) / 4.0; 
	float ap = 4.0; 
	float am = 0.13 * u_star / cm; 
	float Delta = tanh(a0 + ap * pow(c / cp, 2.5f) + am * pow(cm / c, 2.5f));
	float phi = atan2(ky, kx);
	if (kx < 0.0) {
		return 0.0;
	}
	else {
		Bl *= 2.0;
		Bh *= 2.0;
	}
	return A * (Bl + Bh) * (1.0 + Delta * cos(2.0 * phi)) / (2.0 * M_PI * sqr(sqr(k))); // Eq 67
}
void ocean::get_spectrum_sample(int i, int j, float legnth_scale, float kmin, float* result)
{
	static long seed = std::rand();
	float dk = 2.0 * M_PI / legnth_scale;
	float kx = i * dk;
	float ky = j * dk;
	if (abs(kx) < kmin && abs(ky) < kmin) {
		result[0] = 0.0;
		result[1] = 0.0;
	}
	else {
		float S = spectrum(kx, ky);
		float h = sqrt(S / 2.0) * dk;
		float phi = frandom(&seed) * 2.0 * M_PI;
		result[0] = h * cos(phi);
		result[1] = h * sin(phi);
	}
}
void ocean::generate_waves_spectrum()
{
	if (spectrum_1_2_data != NULL) {
		delete[] spectrum_1_2_data;
		delete[] spectrum_3_4_data;
	}
	spectrum_1_2_data = new float[FFT_SIZE * FFT_SIZE * 4];
	spectrum_3_4_data = new float[FFT_SIZE * FFT_SIZE * 4];

	for (int y = 0; y < FFT_SIZE; ++y) {
		for (int x = 0; x < FFT_SIZE; ++x) {
			int offset = 4 * (x + y * FFT_SIZE);
			int i = x >= FFT_SIZE / 2 ? x - FFT_SIZE : x;
			int j = y >= FFT_SIZE / 2 ? y - FFT_SIZE : y;
			get_spectrum_sample(i, j, GRID1_SIZE, M_PI / GRID1_SIZE, spectrum_1_2_data + offset);
			get_spectrum_sample(i, j, GRID2_SIZE, M_PI * FFT_SIZE / GRID1_SIZE, spectrum_1_2_data + offset + 2);
			get_spectrum_sample(i, j, GRID3_SIZE, M_PI * FFT_SIZE / GRID2_SIZE, spectrum_3_4_data + offset);
			get_spectrum_sample(i, j, GRID4_SIZE, M_PI * FFT_SIZE / GRID3_SIZE, spectrum_3_4_data + offset + 2);
		}
	}
	glActiveTexture(GL_TEXTURE0 + SPECTRUM_1_2_UNIT);
	glGenTextures(1, &spec_1_2_handle);
	glBindTexture(GL_TEXTURE_2D, spec_1_2_handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, FFT_SIZE, FFT_SIZE);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FFT_SIZE, FFT_SIZE, GL_RGBA, GL_FLOAT, spectrum_1_2_data);

	glActiveTexture(GL_TEXTURE0 + SPECTRUM_3_4_UNIT);
	glGenTextures(1, &spec_3_4_handle);
	glBindTexture(GL_TEXTURE_2D, spec_3_4_handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, FFT_SIZE, FFT_SIZE);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FFT_SIZE, FFT_SIZE, GL_RGBA, GL_FLOAT, spectrum_3_4_data);
}
float ocean::get_slope_variance(float kx, float ky, float *spectrumSample)
{
	float kSquare = kx * kx + ky * ky;
	float real = spectrumSample[0];
	float img = spectrumSample[1];
	float hSquare = real * real + img * img;
	return kSquare * hSquare * 2.0;
}
void ocean::compute_slope_variance_tex()
{
	float theoreticSlopeVariance = 0.0;
	float k = 5e-3;
	while (k < 1e3) {
		float nextK = k * 1.001;
		theoreticSlopeVariance += k * k * spectrum(k, 0, true) * (nextK - k);
		k = nextK;
	}
	float totalSlopeVariance = 0.0;
	for (int y = 0; y < FFT_SIZE; ++y) {
		for (int x = 0; x < FFT_SIZE; ++x) {
			int offset = 4 * (x + y * FFT_SIZE);
			float i = 2.0 * M_PI * (x >= FFT_SIZE / 2 ? x - FFT_SIZE : x);
			float j = 2.0 * M_PI * (y >= FFT_SIZE / 2 ? y - FFT_SIZE : y);
			totalSlopeVariance += get_slope_variance(i / GRID1_SIZE, j / GRID1_SIZE, spectrum_1_2_data + offset);
			totalSlopeVariance += get_slope_variance(i / GRID2_SIZE, j / GRID2_SIZE, spectrum_1_2_data + offset + 2);
			totalSlopeVariance += get_slope_variance(i / GRID3_SIZE, j / GRID3_SIZE, spectrum_3_4_data + offset);
			totalSlopeVariance += get_slope_variance(i / GRID4_SIZE, j / GRID4_SIZE, spectrum_3_4_data + offset + 2);
		}
	}
	glUseProgram(variances.program_id);
	glUniform4f(glGetUniformLocation(variances.program_id, "GRID_SIZES"), GRID1_SIZE, GRID2_SIZE, GRID3_SIZE, GRID4_SIZE);
	glUniform1f(glGetUniformLocation(variances.program_id, "slope_variance_delta"), 0.5 * (theoreticSlopeVariance - totalSlopeVariance));
	glActiveTexture(GL_TEXTURE0 + SPECTRUM_1_2_UNIT);
	glBindTexture(GL_TEXTURE_2D, spec_1_2_handle);
	glUniform1i(glGetUniformLocation(variances.program_id, "spectrum_1_2_sampler"), SPECTRUM_1_2_UNIT);
	glActiveTexture(GL_TEXTURE0 + SPECTRUM_3_4_UNIT);
	glBindTexture(GL_TEXTURE_2D, spec_3_4_handle);
	glUniform1i(glGetUniformLocation(variances.program_id, "spectrum_3_4_sampler"), SPECTRUM_3_4_UNIT);
	glActiveTexture(GL_TEXTURE0 + SLOPE_VARIANCE_UNIT);
	glUniform1f(glGetUniformLocation(variances.program_id, "N_SLOPE_VARIANCE"), N_SLOPE_VARIANCE);
	glGenTextures(1, &slope_var_handle);
	glBindTexture(GL_TEXTURE_3D, slope_var_handle);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_R16F, N_SLOPE_VARIANCE, N_SLOPE_VARIANCE, N_SLOPE_VARIANCE);
	glBindImageTexture(0, slope_var_handle, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R16F);
	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
int ocean::bit_reverse(int i, int N)
{
	int j = i;
	int M = N;
	int Sum = 0;
	int W = 1;
	M = M / 2;
	while (M != 0) {
		j = (i & M) > M - 1;
		Sum += j * W;
		W *= 2;
		M = M / 2;
	}
	return Sum;
}
void ocean::compute_weight(int N, int k, float &Wr, float &Wi)
{
	Wr = cosl(2.0 * M_PI * k / float(N));
	Wi = sinl(2.0 * M_PI * k / float(N));
}
float *ocean::compute_butterfly_texture()
{
	float *data = new float[FFT_SIZE * PASSES * 4];

	for (int i = 0; i < PASSES; i++) {
		int nBlocks = (int)powf(2.0, float(PASSES - 1 - i));
		int nHInputs = (int)powf(2.0, float(i));
		for (int j = 0; j < nBlocks; j++) {
			for (int k = 0; k < nHInputs; k++) {
				int i1, i2, j1, j2;
				if (i == 0) {
					i1 = j * nHInputs * 2 + k;
					i2 = j * nHInputs * 2 + nHInputs + k;
					j1 = bit_reverse(i1, FFT_SIZE);
					j2 = bit_reverse(i2, FFT_SIZE);
				}
				else {
					i1 = j * nHInputs * 2 + k;
					i2 = j * nHInputs * 2 + nHInputs + k;
					j1 = i1;
					j2 = i2;
				}

				float wr, wi;
				compute_weight(FFT_SIZE, k * nBlocks, wr, wi);

				int offset1 = 4 * (i1 + i * FFT_SIZE);
				data[offset1 + 0] = (j1 + 0.5) / FFT_SIZE;
				data[offset1 + 1] = (j2 + 0.5) / FFT_SIZE;
				data[offset1 + 2] = wr;
				data[offset1 + 3] = wi;

				int offset2 = 4 * (i2 + i * FFT_SIZE);
				data[offset2 + 0] = (j1 + 0.5) / FFT_SIZE;
				data[offset2 + 1] = (j2 + 0.5) / FFT_SIZE;
				data[offset2 + 2] = -wr;
				data[offset2 + 3] = -wi;
			}
		}
	}

	return data;
}
void ocean::simulate_fft_waves(float t)
{
#ifdef BENCH
	GLuint query;
	glGenQueries(1, &query);
	glBeginQuery(GL_TIME_ELAPSED, query);
#endif
	glUseProgram(init.program_id);
	glUniform1f(glGetUniformLocation(init.program_id, "FFT_SIZE"), FFT_SIZE);
		2.0 * M_PI * FFT_SIZE / GRID2_SIZE,
	glUniform4f(glGetUniformLocation(init.program_id, "INVERSE_GRID_SIZES"),
		2.0 * M_PI * FFT_SIZE / GRID1_SIZE,
		2.0 * M_PI * FFT_SIZE / GRID2_SIZE,
		2.0 * M_PI * FFT_SIZE / GRID3_SIZE,
		2.0 * M_PI * FFT_SIZE / GRID4_SIZE);
	glActiveTexture(GL_TEXTURE0 + SPECTRUM_1_2_UNIT);
	glBindTexture(GL_TEXTURE_2D, spec_1_2_handle);
	glUniform1i(glGetUniformLocation(init.program_id, "spectrum_1_2_Sampler"), SPECTRUM_1_2_UNIT);
	glActiveTexture(GL_TEXTURE0 + SPECTRUM_3_4_UNIT);
	glBindTexture(GL_TEXTURE_2D, spec_3_4_handle);
	glUniform1i(glGetUniformLocation(init.program_id, "spectrum_3_4_Sampler"), SPECTRUM_3_4_UNIT);
	glUniform1f(glGetUniformLocation(init.program_id, "t"), t);
	glBindImageTexture(0, fft_a_handle, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(FFT_SIZE / 32, FFT_SIZE / 32, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(fftx.program_id);
	glActiveTexture(GL_TEXTURE0 + BUTTERFLY_UNIT);
	glBindTexture(GL_TEXTURE_2D, butterfly_handle);
	glUniform1i(glGetUniformLocation(fftx.program_id, "butterfly_sampler"), BUTTERFLY_UNIT);
	glBindImageTexture(0, fft_a_handle, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
	glBindImageTexture(1, fft_b_handle, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
	for (int p = 0; p < PASSES; ++p)
	{
		glUniform1f(glGetUniformLocation(fftx.program_id, "pass"), float(p + 0.5) / PASSES);
		glUniform1i(glGetUniformLocation(fftx.program_id, "p"), p);
		glDispatchCompute(FFT_SIZE / 16, FFT_SIZE / 16, 2);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
	glUseProgram(ffty.program_id);
	glActiveTexture(GL_TEXTURE0 + BUTTERFLY_UNIT);
	glBindTexture(GL_TEXTURE_2D, butterfly_handle);
	glUniform1i(glGetUniformLocation(ffty.program_id, "butterfly_sampler"), BUTTERFLY_UNIT);
	glBindImageTexture(0, fft_a_handle, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
	glBindImageTexture(1, fft_b_handle, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
	for (int p = PASSES; p < 2 * PASSES; ++p) {
		glUniform1f(glGetUniformLocation(ffty.program_id, "pass"), float(p - PASSES + 0.5) / PASSES);
		glUniform1i(glGetUniformLocation(ffty.program_id, "p"), p);
		glDispatchCompute(FFT_SIZE / 16, FFT_SIZE / 16, 2);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	glActiveTexture(GL_TEXTURE0 + FFT_A_UNIT);
	glBindTexture(GL_TEXTURE_2D_ARRAY, fft_a_handle);
	glGenerateMipmapEXT(GL_TEXTURE_2D_ARRAY);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
#ifdef BENCH
	glEndQuery(GL_TIME_ELAPSED);
	int done = 0;
	while (!done)
	{
		glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done);
	}
	GLuint64 elapsed_time;
	glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
	std::cout << "Elapsed time: " << elapsed_time / 1000000.0 << " ms" << std::endl;
#endif+
}
