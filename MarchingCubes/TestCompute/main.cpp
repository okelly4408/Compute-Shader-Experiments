#include "main.h"
#include "../../Dependencies/Inlcudes/GL/glew.h"

GLuint tth, ttu, noise_map_handle, noise_map_unit;
int main()
{
	create_window(WIDTH, HEIGHT);
	init_glew();
#ifdef DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(debugMessage, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif
	GLuint quad_vao;
	create_quad(&quad_vao);
	shader_program draw_program("vert.glsl", "frag.glsl", "geom.glsl");
	print_error(5);
	
	create_marching_texture(&tth, &ttu);
	march_cubes(&count, &vao, &tth, &ttu);
	
	
	
	GLuint fbo, screen_handle, screen_unit;
	init_fbo(&fbo, &screen_unit, &screen_handle);
	float zdex = 0.0f;
	int frame = 0;
	shader_program quad_program("quad_vert.glsl", "quad_frag.glsl");
	print_memory_info();
	char *version = (char*)malloc(sizeof(char) * 64);
	version = (char *)glGetString(GL_VERSION);
	std::string str_version(version);
	glActiveTexture(GL_TEXTURE0 + GRASS_TEXTURE_UNIT);
	GLuint grass_texture_handle;
	glGenTextures(1, &grass_texture_handle);
	glBindTexture(GL_TEXTURE_2D, grass_texture_handle);
	GLint grass_tex_width, grass_tex_height;
	unsigned char *grass_tex_data = SOIL_load_image("grass-texture.png", &grass_tex_width, &grass_tex_height, 0, SOIL_LOAD_RGBA);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, grass_tex_width, grass_tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, grass_tex_data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	GLint rock_tex_width, rock_tex_height;
	unsigned char *rock_tex_data = SOIL_load_image("tiles.png", &rock_tex_width, &rock_tex_height, 0, SOIL_LOAD_RGBA);
	glActiveTexture(GL_TEXTURE0 + ROCK_TEXTURE_UNIT);
	GLuint rock_texture_handle;
	glGenTextures(1, &rock_texture_handle);
	glBindTexture(GL_TEXTURE_2D, rock_texture_handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rock_tex_width, rock_tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rock_tex_data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	while(!glfwWindowShouldClose(window))
	{
		float current_frame = glfwGetTime();
		delta_time = current_frame - last_frame;
		std::ostringstream oss;
		oss << delta_time;
		std::string str_dt(oss.str());
		std::string title = "OpenGL v. " + str_version + "; fps: " + std::to_string(1.0 / std::stof(str_dt));
		glfwSetWindowTitle(window, title.c_str());
		last_frame = current_frame;
		glfwPollEvents();
		getInput();
		glClearColor(0.4f, 0.6f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		if (wire)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUseProgram(draw_program.program_id);
		
		GLuint location;
		location = glGetUniformLocation(draw_program.program_id, "projection");
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(getProjectionMatrix()));
		location = glGetUniformLocation(draw_program.program_id, "view");
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(c.getViewMatrix()));

		

		glActiveTexture(GL_TEXTURE0 + ROCK_TEXTURE_UNIT);
		glBindTexture(GL_TEXTURE_2D, rock_texture_handle);
		location = glGetUniformLocation(draw_program.program_id, "rock_texture");
		glUniform1i(location, ROCK_TEXTURE_UNIT);
		glActiveTexture(GL_TEXTURE0 + GRASS_TEXTURE_UNIT);
		glBindTexture(GL_TEXTURE_2D, grass_texture_handle);
		location = glGetUniformLocation(draw_program.program_id, "grass_texture");
		glUniform1i(location, GRASS_TEXTURE_UNIT);
		draw_scene_texture(fbo, vao, count);
		

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUseProgram(quad_program.program_id);
		glActiveTexture(GL_TEXTURE0 + POST_SCREEN_TEXTURE_UNIT);
		glBindTexture(GL_TEXTURE_2D, screen_handle);
		location = glGetUniformLocation(quad_program.program_id, "screen_texture");
		glUniform1i(location, screen_unit);

		glActiveTexture(GL_TEXTURE0 + PERMUTATION_TEXTURE_UNIT);
		glBindTexture(GL_TEXTURE_2D, rh);
		location = glGetUniformLocation(quad_program.program_id, "perm_texture");
		glUniform1i(location, PERMUTATION_TEXTURE_UNIT);

		glActiveTexture(GL_TEXTURE0 + NOISE_MAP_TEXTURE_UNIT);
		glBindTexture(GL_TEXTURE_3D, noise_map_handle);
		location = glGetUniformLocation(quad_program.program_id, "noise_map");
		glUniform1i(location, NOISE_MAP_TEXTURE_UNIT);

		glActiveTexture(GL_TEXTURE0 + GRADIENT_TEXTURE_UNIT);
		glBindTexture(GL_TEXTURE_2D, gh);
		location = glGetUniformLocation(quad_program.program_id, "grad_texture");
		glUniform1i(location, GRADIENT_TEXTURE_UNIT);

		location = glGetUniformLocation(quad_program.program_id, "right");
		vec3 right, up, forward, pos;
		right = c.right;
		up = c.up;
		forward = c.front;
		pos = c.position;
		glUniform3f(location, right.x, right.y, right.z);
		location = glGetUniformLocation(quad_program.program_id, "up");
		glUniform3f(location, up.x, up.y, up.z);
		location = glGetUniformLocation(quad_program.program_id, "forward");
		glUniform3f(location, forward.x, forward.y, forward.z);
		location = glGetUniformLocation(quad_program.program_id, "pos");
		glUniform3f(location, pos.x, pos.y, pos.z);
		location = glGetUniformLocation(quad_program.program_id, "perspective");
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(getProjectionMatrix()));
		glBindVertexArray(quad_vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glUseProgram(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glfwSwapBuffers(window);
		frame++;
		if (frame % 5 == 0)
			zdex += (1.0f / 128.0f);
	}
	glfwTerminate();
	return 0;
}
void print_memory_info()
{
	int work_grp_cnt[3];

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

	printf("max global (total) work group size x:%i y:%i z:%i\n",
		work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

	int work_grp_size[3];

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);

	printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
		work_grp_size[0], work_grp_size[1], work_grp_size[2]);
	int work_grp_inv;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	printf("max local work group invocations %i\n", work_grp_inv);
	int max_3d_texture_size;
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_3d_texture_size);
	printf("max 3d texture size: %i\n", max_3d_texture_size);
}
void init_fbo(GLuint *framebuffer, GLuint *unit, GLuint *handle)
{
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLuint screen_handle, screen_unit;
	screen_unit = POST_SCREEN_TEXTURE_UNIT;
	glActiveTexture(GL_TEXTURE0 + screen_unit);
	glGenTextures(1, &screen_handle);
	glBindTexture(GL_TEXTURE_2D, screen_handle);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, WIDTH, HEIGHT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	GLuint rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WIDTH, HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, screen_handle, 0);
	GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, draw_buffers);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "PROBLEM!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	*framebuffer = fbo;
	*unit = screen_unit;
	*handle = screen_handle;
}
void create_noise_map(GLuint noise_program, GLuint *unit, GLuint *handle, unsigned int size, unsigned int seed)
{
	srand(seed);
	create_noise_luts(&rh, &gh);
	if (first_render)
	{
		GLuint nh;
		glGenTextures(1, &nh);
		glActiveTexture(GL_TEXTURE0 + NOISE_MAP_TEXTURE_UNIT);
		glBindTexture(GL_TEXTURE_3D, nh);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, size, size, size);
		glBindTexture(GL_TEXTURE_3D, 0); 
		*unit = NOISE_MAP_TEXTURE_UNIT;
		*handle = nh;
		first_render = false;
	}
	std::cout << "SEED: " << seed << std::endl;
	glUseProgram(noise_program);
	glBindImageTexture(1, *handle, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);
	print_error(1023);
	glActiveTexture(GL_TEXTURE0 + GRADIENT_TEXTURE_UNIT);
	glBindTexture(GL_TEXTURE_2D, gh);
	print_error(1022);
	GLuint uniform_location = glGetUniformLocation(noise_program, "gradient_texture");
	glUniform1i(uniform_location, GRADIENT_TEXTURE_UNIT);
	print_error(1021);
	glActiveTexture(GL_TEXTURE0 + PERMUTATION_TEXTURE_UNIT);
	print_error(102);
	glBindTexture(GL_TEXTURE_2D, rh);
	uniform_location = glGetUniformLocation(noise_program, "permutation_texture");
	glUniform1i(uniform_location, PERMUTATION_TEXTURE_UNIT);
	print_error(103);
	glDispatchCompute(size / 8, size / 8, size / 8);
	//glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}
void create_noise_luts(GLuint *permutationTexture, GLuint *gradientTexture)
{
	vec4 *data = new vec4[256 * 256];
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
			vec4 pixel((float)AA / 255.0, (float)AB / 255.0, (float)BA / 255.0f, (float)BB / 255.0f);
			data[(256 * i) + j] = pixel;
		}
	}
	glActiveTexture(GL_TEXTURE0 + PERMUTATION_TEXTURE_UNIT);
	glGenTextures(1, permutationTexture);
	glBindTexture(GL_TEXTURE_2D, *permutationTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	print_error(1028);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, 256, 256);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA, GL_FLOAT, data);
	delete[] data;
	vec4 *pixel_data = new vec4[256];
	for (int i = 0; i < 256; i++)
	{
		int idx = permutation[i] % 16;
		vec4 pixel(gradients[idx].x, gradients[idx].y, gradients[idx].z, 1.0);
		pixel_data[i] = pixel;
	}
	glActiveTexture(GL_TEXTURE0 + GRADIENT_TEXTURE_UNIT);
	glGenTextures(1, gradientTexture);
	glBindTexture(GL_TEXTURE_2D, *gradientTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	print_error(1029);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, 256, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA, GL_FLOAT, pixel_data);
	delete[] pixel_data;
	glBindTexture(GL_TEXTURE_2D, 0);
}
void draw_scene_texture(GLuint fbo, GLuint vao, GLuint count)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClearColor(0.4f, 0.6f, 0.6f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, count * 3);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindVertexArray(0);
}
void init_glew()
{
	glewExperimental = GL_TRUE;
	if (glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		glfwTerminate();
	}
}
void create_window(int width, int height)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	window = glfwCreateWindow(width, height, "openGL", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void create_quad(GLuint *v)
{
	vec3 vertices[]{
		vec3(-1.0f, -1.0f, 0.0f),
		vec3(-1.0f, 1.0f, 0.0f),
		vec3(1.0f, 1.0f, 0.0f),
		vec3(1.0f, -1.0f, 0.0f)
	};
	unsigned int indices[]{
		0, 1, 2,
		2, 3, 0

	};
	GLuint vbo, vao, ibo;
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ibo);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(GL_FLOAT), (GLvoid *)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	*v = vao;
}
GLuint mcvbo, mctcbo, mcnbo;
void march_cubes(GLuint *triangle_count, GLuint *vao, GLuint *march_handle, GLuint *march_unit)
{
	glDeleteVertexArrays(1, vao);

	//create buffers for cube vertex storage
	int num_cubes = 128 * 128 * 128;
	glDeleteBuffers(1, &mcvbo);
	glDeleteBuffers(1, &mctcbo);
	glDeleteBuffers(1, &mcnbo);
	glGenBuffers(1, &mcvbo);
	glGenBuffers(1, &mctcbo);
	glGenBuffers(1, &mcnbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mcvbo);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * 8 * num_cubes, nullptr, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
	print_error(100);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mctcbo);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(glm::uint), nullptr, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
	print_error(101);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mcnbo);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * 8 * num_cubes, nullptr, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
	print_error(1022);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	noise_prog = shader_program("noise_3D_compute.glsl");
	create_noise_map(noise_prog.program_id, &noise_map_unit, &noise_map_handle, 256, time(0));
	comp_prog = shader_program("marching_cubes_compute.glsl");
	glUseProgram(comp_prog.program_id);
	glBindImageTexture(1, *march_handle, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mcvbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mctcbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mcnbo);
	print_error(102);
	GLuint uniform_location; /*= glGetUniformLocation(draw_program, "destination")*/
	//glBindImageTexture(uniform_location, tex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glActiveTexture(GL_TEXTURE0 + NOISE_MAP_TEXTURE_UNIT);
	glBindTexture(GL_TEXTURE_3D, noise_map_handle);
	print_error(8008);
	uniform_location = glGetUniformLocation(comp_prog.program_id, "noise_map");
	glUniform1i(uniform_location, NOISE_MAP_TEXTURE_UNIT);
	print_error(8009);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glDispatchCompute(32, 32, 32);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mctcbo);
	//print_error(109);
	glm::uint tri_count = ((glm::uint *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::uint), GL_MAP_READ_BIT))[0];
	print_error(110);
	std::cout << "Triangles created: " << tri_count << std::endl;
	GLuint va;
	glGenVertexArrays(1, &va);
	glBindVertexArray(va);
	print_error(100);
	glBindBuffer(GL_ARRAY_BUFFER, mcvbo);
	print_error(1020);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mcnbo);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)0);
	glEnableVertexAttribArray(1);
	print_error(106);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	print_error(107);
	glBindVertexArray(0);
	print_error(108);
	*vao = va;
	*triangle_count = tri_count;
}
glm::mat4 getProjectionMatrix()
{
	return glm::perspective(45.0f, (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
}

void create_marching_texture(GLuint *handle, GLuint *unit)
{
	*unit = MARCHING_CUBES_TEXTURE_UNIT;
	float pixel_data[256][16];
	
	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			pixel_data[i][j] = (float)tri_table[i][j];
		}
	}
	print_error(1302);
	glActiveTexture(GL_TEXTURE0 + *unit);
	print_error(2302);
	glGenTextures(1, handle);
	print_error(3302);
	glBindTexture(GL_TEXTURE_2D, *handle);
	print_error(4302);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	print_error(5302);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	print_error(6302);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	print_error(7302);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	print_error(8302);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R16F, 16, 256);
	print_error(9302);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 256, GL_RED, GL_FLOAT, pixel_data);
	print_error(10302);
	glBindTexture(GL_TEXTURE_2D, 0);
	print_error(303);
}
inline void print_error(int n)
{
	int error = glGetError();
	if (error != 0)
	{
		std::cout << "#" << n << " ERROR: " << error << std::endl;
	}
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		comp_prog.reload_shaders();
		noise_prog.reload_shaders();
	}
	if (key == GLFW_KEY_M && action == GLFW_PRESS)
	{
		if (wire)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			wire = false;
		}
		else
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			wire = true;
		}
	}
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		//draw_texture(&count, &vao, &handle, &unit, comp_prog);
		march_cubes(&count, &vao, &tth, &ttu);
	}
	if (key >-0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (first_mouse)
	{
		m_last_x = xpos;
		m_last_y = ypos;
		first_mouse = false;
	}
	double xoff = xpos - m_last_x;
	double yoff = ypos - m_last_y;

	m_last_x = xpos;
	m_last_y = ypos;
	c.processMouse(xoff, yoff);
}
void getInput()
{
	if (keys[GLFW_KEY_W])
		c.processKeyboard(FORWARD, delta_time);
	if (keys[GLFW_KEY_S])
		c.processKeyboard(BACK, delta_time);
	if (keys[GLFW_KEY_D])
		c.processKeyboard(RIGHT, delta_time);
	if (keys[GLFW_KEY_A])
		c.processKeyboard(LEFT, delta_time);
	if (keys[GLFW_KEY_Q])
		c.processKeyboard(UP, delta_time);
	if (keys[GLFW_KEY_Z])
		c.processKeyboard(DOWN, delta_time);
}
void APIENTRY debugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	switch (id)
	{
	case 131169:
	case 131185:
		return;
	}
	printf("Message: %s\n", message);
	printf("Source: ");
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		printf("API");
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		printf("Window System");
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		printf("Shader Compiler");
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		printf("Third Party");
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		printf("Application");
		break;
	case GL_DEBUG_SOURCE_OTHER:
		printf("Other");
		break;
	}
	printf("\n");
	printf("Type: ");
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		printf("Error");
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		printf("Deprecated Behaviour");
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		printf("Undefined Behaviour");
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		printf("Portability");
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		printf("Performance");
		break;
	case GL_DEBUG_TYPE_MARKER:
		printf("Marker");
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		printf("Push Group");
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		printf("Pop Group");
		break;
	case GL_DEBUG_TYPE_OTHER:
		printf("Other");
		break;
	}
	printf("\n");
	printf("ID: %d\n", id);
	printf("Severity: ");
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		printf("High");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		printf("Medium");
		break;
	case GL_DEBUG_SEVERITY_LOW:
		printf("Low");
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		printf("Notification");
		break;
	}
	printf("\n\n");
}