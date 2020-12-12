#define GLEW_STATIC
#include "../../Dependencies/Inlcudes/GL/glew.h"

#include "../../Dependencies/Inlcudes/GLFW/glfw3.h"

#define GLM_SWIZZLE
#include "../../Dependencies/Inlcudes/glm/glm.hpp"
#include "../../Dependencies/Inlcudes/glm/detail/type_vec3.hpp"
#include "../../Dependencies/Inlcudes/glm/detail/type_vec4.hpp"
#include "../../Dependencies/Inlcudes/glm/gtc/type_ptr.hpp"
#include <cmath>
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include "camera.h"
#include "shader_program.h"
#include "../../Dependencies/Inlcudes/glm/mat4x4.hpp"
#include "../../Dependencies/Inlcudes/glm/gtc/matrix_transform.inl"
#include <iostream>
#include "ocean.h"
#include "atmosphere_scattering.h"
#include "noise_util.h"
//#define DEBUG
#ifdef _WIN32
	#define WIDTH 1024
	#define HEIGHT 768
#elif __APPLE__
	#define WIDTH 2048
	#define HEIGHT 1536
#endif
#define POST_SCREEN_UNIT 30
glm::mat4 getProjectionMatrix();
void init_glew();
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void init_fbo(GLuint *framebuffer, GLuint *unit, GLuint *handle);
void draw_quad(GLuint *vao);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mode);
void get_camera_to_ocean(glm::mat4 *ctoo, glm::mat4 view, glm::vec3 camera_pos);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void draw_cube(GLuint *cube_vao);
void draw_quad(GLuint *vao);
void create_window(int width, int height);
void APIENTRY debugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
void getInput();
bool keys[1024];
bool first_mouse = true;
float delta_time = 0.0f;
float last_frame = 0.0f;
double m_last_x;
double m_last_y;
bool wire = false;
GLFWwindow *window;
camera c(glm::vec3(0, 10, 0));
bool update = true;
float sunTheta = 0.6*M_PI / 2.0 - 0.05;
float sunPhi = 0.0;
glm::vec3 sun_dir = glm::vec3(std::sin(sunTheta) * std::cos(sunPhi), std::sin(sunTheta) * std::sin(sunPhi), std::cos(sunTheta));
bool update_sun = false;

void create_noise_map(GLuint *handle);
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
	glm::mat4 projection_matrix = getProjectionMatrix();
	shader_program cube_program(".General/cube_vert.glsl", ".General/cube_frag.glsl");
	shader_program scene_program(".General/scene_v.glsl", ".General/scene_f.glsl");
	GLuint cube_vao;
	draw_cube(&cube_vao);
	camera ocean_camera = c;
	ocean ocean_test(WIDTH, HEIGHT);
	atmospheric_scattering atmosphere_test(float(WIDTH), float(HEIGHT));
	noise_util noiseutil = noise_util();
	GLuint nh, nu = 31;
	noiseutil.create_noise_map(&nh, nu, 128, 128, 128);
//	create_noise_map(&nh);
	GLuint fbo, screen_unit, screen_handle;
	init_fbo(&fbo, &screen_unit, &screen_handle);
	GLuint scene_quad;
	draw_quad(&scene_quad);
	while (!glfwWindowShouldClose(window))
	{
		float time = glfwGetTime();
		sun_dir = glm::vec3(std::sin(sunTheta) * std::cos(sunPhi), std::cos(sunTheta), std::sin(sunTheta) * std::sin(sunPhi));
		atmosphere_test.render_skymap(sun_dir, time);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glPolygonMode(GL_FRONT_AND_BACK, wire ? GL_LINE : GL_FILL);
		float current_frame = glfwGetTime();
		delta_time = current_frame - last_frame;
		last_frame = current_frame;
		glfwPollEvents();
		getInput();
		glClearColor(0.3, 0.3, 0.6, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		ocean_camera = c;
		//adjust position of camera to avoid edge artefacts
		ocean_camera.position = ocean_camera.position + (ocean_camera.front * glm::vec3(-4.0));
		glm::mat4 view_matrix = c.getViewMatrix();
		glm::mat4 atm_persp = glm::perspective(45.0f, float(WIDTH) / float(HEIGHT), 0.1f, float(1e5));
		glUseProgram(cube_program.program_id);
		glUniformMatrix4fv(glGetUniformLocation(cube_program.program_id, "projection"), 1, GL_FALSE, glm::value_ptr(projection_matrix));
		glUniformMatrix4fv(glGetUniformLocation(cube_program.program_id, "view"), 1, GL_FALSE, glm::value_ptr(view_matrix));
		glBindVertexArray(cube_vao);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glUseProgram(0);
		
		ocean_test.render(projection_matrix, c.getViewMatrix(), ocean_camera.position, glfwGetTime(), atmosphere_test, sun_dir);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.4f, 0.6f, 0.6f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glUseProgram(scene_program.program_id);
		glActiveTexture(GL_TEXTURE0 + POST_SCREEN_UNIT);
		glBindTexture(GL_TEXTURE_2D, screen_handle);
		glUniform1i(glGetUniformLocation(scene_program.program_id, "geometry_texture"), POST_SCREEN_UNIT);
		glm::mat4 i_persp = glm::inverse(projection_matrix);
		glm::mat4 i_view = glm::inverse(view_matrix);
		glUniformMatrix4fv(glGetUniformLocation(scene_program.program_id, "inv_persp"), 1, false, glm::value_ptr(glm::inverse(projection_matrix)));
		glUniformMatrix4fv(glGetUniformLocation(scene_program.program_id, "inv_view"), 1, false, glm::value_ptr(i_view));
		glUniform1f(glGetUniformLocation(scene_program.program_id, "time"), time);
		glUniform3f(glGetUniformLocation(scene_program.program_id, "camera_pos"),0.0, c.position.y, 0.0);
		glUniform3f(glGetUniformLocation(scene_program.program_id, "sun_dir"), sun_dir.x, sun_dir.y, sun_dir.z);
		glUniform2f(glGetUniformLocation(scene_program.program_id, "iResolution"), WIDTH, HEIGHT);
		glActiveTexture(GL_TEXTURE0 + TRANSMITTANCE_UNIT);
		glBindTexture(GL_TEXTURE_2D, atmosphere_test.transmittanceTexture);
		glUniform1i(glGetUniformLocation(scene_program.program_id, "transmittance_texture"), TRANSMITTANCE_UNIT);
		glActiveTexture(GL_TEXTURE0 + ATMOSPHERE_UNIT);
		glBindTexture(GL_TEXTURE_2D, atmosphere_test.atmosphereTexture);
		glUniform1i(glGetUniformLocation(scene_program.program_id, "reflectance_texture"), ATMOSPHERE_UNIT);
		glActiveTexture(GL_TEXTURE0 + INSCATTER_UNIT);
		glBindTexture(GL_TEXTURE_3D, atmosphere_test.inscatterTexture);
		glUniform1i(glGetUniformLocation(scene_program.program_id, "inscatter_texture"), INSCATTER_UNIT);
		glActiveTexture(GL_TEXTURE0 + nu);
		glBindTexture(GL_TEXTURE_3D, nh);
		glUniform1i(glGetUniformLocation(scene_program.program_id, "noise_texture"), nu);
		glBindVertexArray(scene_quad);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		glfwSwapBuffers(window);
	}
	glfwTerminate();
}
void init_fbo(GLuint *framebuffer, GLuint *unit, GLuint *handle)
{
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLuint screen_handle, screen_unit;
	screen_unit = POST_SCREEN_UNIT;
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
void draw_quad(GLuint *vao)
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
void draw_cube(GLuint *cube_vao)
{
	glm::vec4 cubes_vertices[8] = {
		glm::vec4(0.0, 0.0, 0.0, 1.0),
		glm::vec4(0.0, 0.0, 1.0, 1.0),
		glm::vec4(1.0, 0.0, 1.0, 1.0),
		glm::vec4(1.0, 0.0, 0.0, 1.0),
		glm::vec4(0.0, 1.0, 0.0, 1.0),
		glm::vec4(0.0, 1.0, 1.0, 1.0),
		glm::vec4(1.0, 1.0, 1.0, 1.0),
		glm::vec4(1.0, 1.0, 0.0, 1.0)
	};
	glm::uint cube_indices[36] = {
		0, 1, 2,
		2, 3, 0,

		0, 4, 7,
		7, 3, 0,

		3, 7, 6,
		6, 2, 3,

		1, 5, 6,
		6, 2, 1,

		0, 4, 5,
		5, 1, 0,

		4, 5, 6,
		6, 7, 4
	};
	GLuint c_vao, c_vbo, c_ibo;
	glGenVertexArrays(1, &c_vao);
	glBindVertexArray(c_vao);
	glGenBuffers(1, &c_vbo);
	glGenBuffers(1, &c_ibo);
	glBindBuffer(GL_ARRAY_BUFFER, c_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubes_vertices), cubes_vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, c_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, false, 4 * sizeof(GL_FLOAT), (GLvoid *)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	*cube_vao = c_vao;
}

glm::mat4 getProjectionMatrix()
{
	return glm::perspective(45.0f, (float)WIDTH / (float)HEIGHT, 0.1f, float(1e5));
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
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
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
	std::string s = (char*)malloc(8);
	if (first_mouse)
	{
		m_last_x = xpos;
		m_last_y = ypos;
		first_mouse = false;
	}
	double xoff = xpos - m_last_x;
	double yoff = ypos - m_last_y;
	if (keys[GLFW_MOUSE_BUTTON_RIGHT])
	{
		sunPhi += (xpos - m_last_x) / 400.0;
		sunTheta += (ypos - m_last_y) / 400.0;
	} else
	{
		c.processMouse(xoff, yoff);
	}
	m_last_x = xpos;
	m_last_y = ypos;
	
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		keys[button] = true;
	} else
	{
		keys[GLFW_MOUSE_BUTTON_RIGHT] = false;
	}
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
void create_noise_luts(GLuint *permutationTexture, GLuint *gradientTexture);
void create_noise_map(GLuint *handle)
{
	//srand(DEFAULT_SEED);
	GLuint rh, gh;
	create_noise_luts(&rh, &gh);
	if (true)
	{
		GLuint nh;
		glGenTextures(1, &nh);
		glActiveTexture(GL_TEXTURE0 + 31);
		glBindTexture(GL_TEXTURE_3D, nh);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, 128, 128, 128);
		glBindTexture(GL_TEXTURE_3D, 0);
		*handle = nh;
	}
	std::cout << "SEED: " << 0 << std::endl;
	srand(0);
	shader_program noise_program(".Noise/noise3D_main.comp");
	glUseProgram(noise_program.program_id);
	glBindImageTexture(1, *handle, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);
	glActiveTexture(GL_TEXTURE0 + 38);
	glBindTexture(GL_TEXTURE_2D, gh);
	GLuint uniform_location = glGetUniformLocation(noise_program.program_id, "gradient_texture");
	glUniform1i(uniform_location, 38);
	glActiveTexture(GL_TEXTURE0 + 37);
	glBindTexture(GL_TEXTURE_2D, rh);
	uniform_location = glGetUniformLocation(noise_program.program_id, "permutation_texture");
	glUniform1i(uniform_location, 37);
	glDispatchCompute(128 / 8, 128 / 8, 128 / 8);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);
}
void create_noise_luts(GLuint *permutationTexture, GLuint *gradientTexture)
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
	glActiveTexture(GL_TEXTURE0 + 37);
	glGenTextures(1, permutationTexture);
	glBindTexture(GL_TEXTURE_2D, *permutationTexture);
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
	glActiveTexture(GL_TEXTURE0 + 38);
	glGenTextures(1, gradientTexture);
	glBindTexture(GL_TEXTURE_2D, *gradientTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, 256, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA, GL_FLOAT, pixel_data);
	delete[] pixel_data;
	glBindTexture(GL_TEXTURE_2D, 0);
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