#include "atmosphere_scattering.h"
#include "../../Dependencies/Inlcudes/glm/vec3.hpp"
#include "../../Dependencies/Inlcudes/glm/detail/func_matrix.inl"
#include "../../Dependencies/Inlcudes/glm/gtc/type_ptr.hpp"

atmospheric_scattering::atmospheric_scattering(float width, float height)
{
	this->screen_width = width;
	this->screen_height = height;
	GLuint atmosphere_unit;
	init_fbo(&fbo, &atmosphere_unit, &atmosphereTexture);
	load_programs();
	init_textures();
	precompute();
	draw_quad(&screen_quad);
}
void atmospheric_scattering::render_skymap(glm::vec3 sun_dir, float time)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glUseProgram(reflectance_program.program_id);
	glActiveTexture(GL_TEXTURE0 + TRANSMITTANCE_UNIT);
	glBindTexture(GL_TEXTURE_2D, transmittanceTexture);
	glUniform1i(glGetUniformLocation(reflectance_program.program_id, "transmittance_texture"), TRANSMITTANCE_UNIT);
	glActiveTexture(GL_TEXTURE0 + IRRADIANCE_UNIT);
	glBindTexture(GL_TEXTURE_2D, irradianceTexture);
	glUniform1i(glGetUniformLocation(reflectance_program.program_id, "irradiance_texture"), IRRADIANCE_UNIT);
	glActiveTexture(GL_TEXTURE0 + INSCATTER_UNIT);
	glBindTexture(GL_TEXTURE_3D, inscatterTexture);
	glUniform1i(glGetUniformLocation(reflectance_program.program_id, "inscatter_texture"), INSCATTER_UNIT);
	glUniform1f(glGetUniformLocation(reflectance_program.program_id, "time"), time);
	glUniform3f(glGetUniformLocation(reflectance_program.program_id, "sun_dir"), sun_dir.x, sun_dir.y, sun_dir.z);
	glBindVertexArray(screen_quad);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void atmospheric_scattering::load_programs()
{
	transmittance_compute = shader_program(".AtmosphericScattering/transmittance_compute.glsl");
	irradiance_compute = shader_program(".AtmosphericScattering/irradiance_compute.glsl");
	inscatter_compute = shader_program(".AtmosphericScattering/inscatter_compute.glsl");
	j_compute = shader_program(".AtmosphericScattering/inscatter_s_compute.glsl");
	irradiancen_compute = shader_program(".AtmosphericScattering/irradiance_n_compute.glsl");
	inscattern_compute = shader_program(".AtmosphericScattering/inscatter_n_compute.glsl");
	copy_irradiance_compute = shader_program(".AtmosphericScattering/copy_irradiance_compute.glsl");
	copy_inscattern_compute = shader_program(".AtmosphericScattering/copy_inscatter_n_compute.glsl");
	copy_inscatter_compute = shader_program(".AtmosphericScattering/copy_inscatter_compute.glsl");
	atmosphere_program = shader_program(".AtmosphericScattering/atmosphere_v.glsl", ".AtmosphericScattering/atmosphere_f.glsl");
	reflectance_program = shader_program(".AtmosphericScattering/reflection_map_v.glsl", ".AtmosphericScattering/reflection_map_f.glsl");
}
void atmospheric_scattering::init_fbo(GLuint *framebuffer, GLuint *unit, GLuint *handle)
{
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLuint screen_handle, screen_unit;
	screen_unit = ATMOSPHERE_UNIT;
	glActiveTexture(GL_TEXTURE0 + screen_unit);
	glGenTextures(1, &screen_handle);
	glBindTexture(GL_TEXTURE_2D, screen_handle);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, screen_width, screen_height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, screen_handle, 0);
	GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, draw_buffers);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	*framebuffer = fbo;
	*unit = screen_unit;
	*handle = screen_handle;
}
void  atmospheric_scattering::init_textures()
{
	glActiveTexture(GL_TEXTURE0 + TRANSMITTANCE_UNIT);
	glGenTextures(1, &transmittanceTexture);
	glBindTexture(GL_TEXTURE_2D, transmittanceTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, TRANSMITTANCE_WIDTH, TRANSMITTANCE_HEIGHT);

	glActiveTexture(GL_TEXTURE0 + IRRADIANCE_UNIT);
	glGenTextures(1, &irradianceTexture);
	glBindTexture(GL_TEXTURE_2D, irradianceTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, SKY_W, SKY_H);

	glActiveTexture(GL_TEXTURE0 + INSCATTER_UNIT);
	glGenTextures(1, &inscatterTexture);
	glBindTexture(GL_TEXTURE_3D, inscatterTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexStorage3D(GL_TEXTURE_3D,1, GL_RGBA16F, RES_MU_S * RES_NU, RES_MU, RES_R);

	glActiveTexture(GL_TEXTURE0 + DELTA_E_UNIT);
	glGenTextures(1, &deltaETexture);
	glBindTexture(GL_TEXTURE_2D, deltaETexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, SKY_W, SKY_H);

	glActiveTexture(GL_TEXTURE0 + DELTA_R_UNIT);
	glGenTextures(1, &deltaSRTexture);
	glBindTexture(GL_TEXTURE_3D, deltaSRTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, RES_MU_S * RES_NU, RES_MU, RES_R);

	glActiveTexture(GL_TEXTURE0 + DELTA_M_UNIT);
	glGenTextures(1, &deltaSMTexture);
	glBindTexture(GL_TEXTURE_3D, deltaSMTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, RES_MU_S * RES_NU, RES_MU, RES_R);

	glActiveTexture(GL_TEXTURE0 + DELTA_J_UNIT);
	glGenTextures(1, &deltaJTexture);
	glBindTexture(GL_TEXTURE_3D, deltaJTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA16F, RES_MU_S * RES_NU, RES_MU, RES_R);
}
void  atmospheric_scattering::precompute()
{
	glUseProgram(transmittance_compute.program_id);
	glBindImageTexture(0, transmittanceTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(TRANSMITTANCE_WIDTH / 32, TRANSMITTANCE_HEIGHT / 8, 1);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

	glUseProgram(irradiance_compute.program_id);
	glActiveTexture(GL_TEXTURE0 + TRANSMITTANCE_UNIT);
	glBindTexture(GL_TEXTURE_2D, transmittanceTexture);
	glUniform1i(glGetUniformLocation(irradiance_compute.program_id, "transmittance_tex"), TRANSMITTANCE_UNIT);
	glBindImageTexture(0, deltaETexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(SKY_W / 8, SKY_H / 2, 1);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glUseProgram(inscatter_compute.program_id);
	glActiveTexture(GL_TEXTURE0 + TRANSMITTANCE_UNIT);
	glBindTexture(GL_TEXTURE_2D, transmittanceTexture);
	glUniform1i(glGetUniformLocation(inscatter_compute.program_id, "transmittance_tex"), TRANSMITTANCE_UNIT);
	glBindImageTexture(0, deltaSRTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindImageTexture(1, deltaSMTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(16, 8, 8);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	glUseProgram(copy_irradiance_compute.program_id);
	glUniform1f(glGetUniformLocation(copy_irradiance_compute.program_id, "k"), 0.0);
	glUniform1i(glGetUniformLocation(copy_irradiance_compute.program_id, "add"), 0);
	glBindImageTexture(0, irradianceTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
	glBindImageTexture(1, deltaETexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	glDispatchCompute(8, 8, 1);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glUseProgram(copy_inscatter_compute.program_id);
	glBindImageTexture(0, inscatterTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindImageTexture(1, deltaSRTexture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
	glBindImageTexture(2, deltaSMTexture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
	glDispatchCompute(16, 8, 8);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	for (int order = 2; order <= 4; ++order)
	{
		glUseProgram(j_compute.program_id);
		glActiveTexture(GL_TEXTURE0 + TRANSMITTANCE_UNIT);
		glBindTexture(GL_TEXTURE_2D, transmittanceTexture);
		glUniform1i(glGetUniformLocation(j_compute.program_id, "transmittance_tex"), TRANSMITTANCE_UNIT);
		glActiveTexture(GL_TEXTURE0 + DELTA_E_UNIT);
		glBindTexture(GL_TEXTURE_2D, deltaETexture);
		glUniform1i(glGetUniformLocation(j_compute.program_id, "delta_e_tex"), DELTA_E_UNIT);
		glBindImageTexture(0, deltaJTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glActiveTexture(GL_TEXTURE0 + DELTA_R_UNIT);
		glBindTexture(GL_TEXTURE_3D, deltaSRTexture);
		glUniform1i(glGetUniformLocation(j_compute.program_id, "delta_sr_sampler"), DELTA_R_UNIT);
		glActiveTexture(GL_TEXTURE0 + DELTA_M_UNIT);
		glBindTexture(GL_TEXTURE_3D, deltaSMTexture);
		glUniform1i(glGetUniformLocation(j_compute.program_id, "delta_sm_sampler"), DELTA_M_UNIT);
		glUniform1f(glGetUniformLocation(j_compute.program_id, "first"), order == 2 ? 1.0 : 0.0);
		glDispatchCompute(16, 8, 8);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(irradiancen_compute.program_id);
		glBindImageTexture(0, deltaETexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glUniform1f(glGetUniformLocation(irradiancen_compute.program_id, "first"), order == 2 ? 1.0 : 0.0);
		glActiveTexture(GL_TEXTURE0 + TRANSMITTANCE_UNIT);
		glBindTexture(GL_TEXTURE_2D, transmittanceTexture);
		glUniform1i(glGetUniformLocation(irradiancen_compute.program_id, "transmittance_tex"), TRANSMITTANCE_UNIT);
		glActiveTexture(GL_TEXTURE0 + DELTA_R_UNIT);
		glBindTexture(GL_TEXTURE_3D, deltaSRTexture);
		glUniform1i(glGetUniformLocation(irradiancen_compute.program_id, "delta_sr_sampler"), DELTA_R_UNIT);
		glActiveTexture(GL_TEXTURE0 + DELTA_M_UNIT);
		glBindTexture(GL_TEXTURE_3D, deltaSMTexture);
		glUniform1i(glGetUniformLocation(irradiancen_compute.program_id, "delta_sm_sampler"), DELTA_M_UNIT);
		glDispatchCompute(8, 8, 1);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(inscattern_compute.program_id);
		glBindImageTexture(0, deltaSRTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glActiveTexture(GL_TEXTURE0 + DELTA_J_UNIT);
		glBindTexture(GL_TEXTURE_3D, deltaJTexture);
		glUniform1i(glGetUniformLocation(inscattern_compute.program_id, "delta_j_sampler"), DELTA_J_UNIT);
		glUniform1f(glGetUniformLocation(inscattern_compute.program_id, "first"), order == 2 ? 1.0 : 0.0);
		glActiveTexture(GL_TEXTURE0 + TRANSMITTANCE_UNIT);
		glBindTexture(GL_TEXTURE_2D, transmittanceTexture);
		glUniform1i(glGetUniformLocation(inscattern_compute.program_id, "transmittance_tex"), TRANSMITTANCE_UNIT);
		glDispatchCompute(16, 8, 8);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(copy_irradiance_compute.program_id);
		glBindImageTexture(0, irradianceTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
		glBindImageTexture(1, deltaETexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
		glUniform1i(glGetUniformLocation(copy_irradiance_compute.program_id, "add"), 1);
		glUniform1f(glGetUniformLocation(copy_irradiance_compute.program_id, "k"), 1.0);
		glDispatchCompute(8, 8, 1);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(copy_inscattern_compute.program_id);
		glBindImageTexture(0, inscatterTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
		glBindImageTexture(1, deltaSRTexture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
		glDispatchCompute(16, 8, 8);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
}
void atmospheric_scattering::draw_quad(GLuint *vao)
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
	GLuint quad_vao;
	glGenVertexArrays(1, &quad_vao);
	glBindVertexArray(quad_vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, false, 4 * sizeof(float), (GLvoid *)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	*vao = quad_vao;
}