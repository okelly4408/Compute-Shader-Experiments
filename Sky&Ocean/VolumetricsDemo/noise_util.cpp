#include "noise_util.h"

void noise_util::perlin_noise(GLuint *handle, GLuint unit, float width, float height, float depth)
{
	
	glGenTextures(1, handle);
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_3D, *handle);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, width, height, depth);
	glBindTexture(GL_TEXTURE_3D, 0);
	glUseProgram(noise_program.program_id);
	glActiveTexture(GL_TEXTURE0 + PERMUTATION_TEXTURE_UNIT);
	glBindTexture(GL_TEXTURE_2D, permutationTexture);
	glUniform1i(glGetUniformLocation(noise_program.program_id, "permutation_texture"), PERMUTATION_TEXTURE_UNIT);
	glActiveTexture(GL_TEXTURE0 + GRADIENT_TEXTURE_UNIT);
	glBindTexture(GL_TEXTURE_2D, gradientTexture);
	glUniform1i(glGetUniformLocation(noise_program.program_id, "gradient_texture"), GRADIENT_TEXTURE_UNIT);
	glBindImageTexture(1, *handle, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);
	glDispatchCompute(width / 8, height / 8, depth / 8);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);
}
void noise_util::create_noise_map(GLuint *handle, GLuint unit, float width, float height, float depth)
{
	//srand(DEFAULT_SEED);

	if (true)
	{
		glGenTextures(1, handle);
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_3D, *handle);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, width, height, depth);
		glBindTexture(GL_TEXTURE_3D, 0);
	}
	srand(0);
	shader_program np(".Noise/noise3D_main.comp");
	glUseProgram(np.program_id);
	glBindImageTexture(1, *handle, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);
	glActiveTexture(GL_TEXTURE0 + GRADIENT_TEXTURE_UNIT);
	glBindTexture(GL_TEXTURE_2D, gradientTexture);
	GLuint uniform_location = glGetUniformLocation(np.program_id, "gradient_texture");
	glUniform1i(uniform_location, GRADIENT_TEXTURE_UNIT);
	glActiveTexture(GL_TEXTURE0 + PERMUTATION_TEXTURE_UNIT);
	glBindTexture(GL_TEXTURE_2D, permutationTexture);
	uniform_location = glGetUniformLocation(np.program_id, "permutation_texture");
	glUniform1i(uniform_location, PERMUTATION_TEXTURE_UNIT);
	glDispatchCompute(width / 8, height / 8, depth / 8);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);
}
void noise_util::create_noise_luts()
{
	glm::vec4 *data = new glm::vec4[256 * 256];
	int permutation[256];
	for (int i = 0; i < 256; i++)
	{
		permutation[i] = -1;
	}
	for (int i = 0; i < 256; i++)
	{
		bool a = true;
		while (a)
		{
			int iP = rand() % 256;
			if (permutation[iP] == -1)
			{
				permutation[iP] = i;
				a = false;
			}
		}
	}
	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 256; j++)
		{
			int A = permutation[i] + j;
			int AA = permutation[A % 256];
			int AB = permutation[(A % 256) + 1];
			int B = permutation[((i + 1) % 256)] + j;
			int BA = permutation[B % 256];
			int BB = permutation[(B + 1) % 256];
			glm::vec4 pixel((float)AA / 255.0, (float)AB / 255.0, (float)BA / 255.0f, (float)BB / 255.0f);
			data[(256 * i) + j] = pixel;
		}
	}
	glActiveTexture(GL_TEXTURE0 + PERMUTATION_TEXTURE_UNIT);
	glGenTextures(1, &permutationTexture);
	glBindTexture(GL_TEXTURE_2D, permutationTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, 256, 256);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA, GL_FLOAT, data);
	delete[] data;
	glm::vec4 *pixel_data = new glm::vec4[256];
	for (int i = 0; i < 256; i++)
	{
		int idx = permutation[i] % 16;
		glm::vec4 pixel(gradients[idx].x, gradients[idx].y, gradients[idx].z, 1.0);
		pixel_data[i] = pixel;
	}
	glActiveTexture(GL_TEXTURE0 + GRADIENT_TEXTURE_UNIT);
	glGenTextures(1, &gradientTexture);
	glBindTexture(GL_TEXTURE_2D, gradientTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, 256, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA, GL_FLOAT, pixel_data);
	delete[] pixel_data;
	glBindTexture(GL_TEXTURE_2D, 0);
}
