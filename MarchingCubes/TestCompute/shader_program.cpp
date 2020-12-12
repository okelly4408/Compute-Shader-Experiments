#include <iostream>
#include "../../VolumetricsDemo/VolumetricsDemo/shader_program.h"

shader_program::shader_program(std::string compute_path)
{
	this->files = 1;
	this->file_paths[0] = compute_path;
	GLuint shader;
	create_shader(&shader, compute_path, GL_COMPUTE_SHADER);
	GLuint program = glCreateProgram();
	glAttachShader(program, shader);
	glLinkProgram(program);
	if (!check_program(program))
	{
		print_shader_log(shader);
	}
	glDeleteShader(shader);
	this->program_id = program;
	printf("HELLO!");
}
shader_program::shader_program(std::string vertex_path, std::string fragment_path)
{
	this->files = 2;
	this->file_paths[0] = vertex_path;
	this->file_paths[1] = fragment_path;
	GLuint v_shader, f_shader;
	create_shader(&v_shader, vertex_path, GL_VERTEX_SHADER);
	create_shader(&f_shader, fragment_path, GL_FRAGMENT_SHADER);
	GLuint program = glCreateProgram();
	glAttachShader(program, v_shader);
	glAttachShader(program, f_shader);
	glLinkProgram(program);
	if(!check_program(program))
	{
		print_shader_log(v_shader);
		print_shader_log(f_shader);
	}
	this->program_id = program;
}
shader_program::shader_program(std::string vertex_path, std::string fragment_path, std::string geometry_path)
{
	this->files = 3;
	this->file_paths[0] = vertex_path;
	this->file_paths[1] = fragment_path;
	this->file_paths[2] = geometry_path;
	GLuint v_shader, g_shader, f_shader;
	create_shader(&v_shader, vertex_path, GL_VERTEX_SHADER);
	create_shader(&g_shader, geometry_path, GL_GEOMETRY_SHADER);
	create_shader(&f_shader, fragment_path, GL_FRAGMENT_SHADER);
	GLuint program = glCreateProgram();
	glAttachShader(program, v_shader);
	glAttachShader(program, g_shader);
	glAttachShader(program, f_shader);
	glLinkProgram(program);
	if (!check_program(program))
	{
		print_shader_log(v_shader);
		print_shader_log(f_shader);
		print_shader_log(g_shader);
	}
	this->program_id = program;
}
shader_program::shader_program(std::string vertex_path, std::string fragment_path, std::string geometry_path, std::string tess_control_path, std::string tess_eval_path)
{
	this->files = 5;
	this->file_paths[0] = vertex_path;
	this->file_paths[1] = fragment_path;
	this->file_paths[2] = geometry_path;
	this->file_paths[3] = tess_control_path;
	this->file_paths[4] = tess_eval_path;
	GLuint v_shader, f_shader, g_shader, tc_shader, te_shader;
	create_shader(&v_shader, vertex_path, GL_VERTEX_SHADER);
	create_shader(&f_shader, fragment_path, GL_FRAGMENT_SHADER);
	create_shader(&g_shader, geometry_path, GL_GEOMETRY_SHADER);
	create_shader(&tc_shader, tess_control_path, GL_TESS_CONTROL_SHADER);
	create_shader(&te_shader, tess_eval_path, GL_TESS_EVALUATION_SHADER);
	GLuint program = glCreateProgram();
	glAttachShader(program, v_shader);
	glAttachShader(program, f_shader);
	glAttachShader(program, g_shader);
	glAttachShader(program, tc_shader);
	glAttachShader(program, te_shader);
	glLinkProgram(program);
	if (!check_program(program))
	{
		print_shader_log(v_shader);
		print_shader_log(f_shader);
		print_shader_log(g_shader);
		print_shader_log(tc_shader);
		print_shader_log(te_shader);
	}
	this->program_id = program;
}
void shader_program::reload_shaders()
{
	GLuint program = this->program_id;
	glDeleteProgram(program);
	program = glCreateProgram();
	this->program_id = program;
	if (this->files == 1)
	{
		GLuint shader;
		create_shader(&shader, this->file_paths[0], GL_COMPUTE_SHADER);
		glAttachShader(program, shader);
		glLinkProgram(program);
		glDeleteShader(shader);
		GLint success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{
			char *info_log = (char*)malloc(sizeof(char) * 512);
			glGetProgramInfoLog(program, 512, nullptr, info_log);
			printf("ERROR COMPILING SHADER: %s\n", info_log);
			free(info_log);
		}
	} else
	{
		for (int i = 0; i < this->files; i++)
		{
			GLuint shader;
			create_shader(&shader, this->file_paths[i], SHADER_TYPES[i]);
			glAttachShader(program, shader);
			glDeleteShader(shader);			
		}
		GLint success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		glLinkProgram(program);
		if (!success)
		{
			char *info_log = (char*)malloc(sizeof(char) * 512);
			glGetProgramInfoLog(program, 512, nullptr, info_log);
			printf("ERROR COMPILING SHADER: %s\n", info_log);
			free(info_log);
		}
	}
}
inline void shader_program::create_shader(GLuint *shader, std::string file_name, GLenum type)
{
	std::string shader_text = preprocess(file_name);
	GLint s = glCreateShader(type);
	const GLchar *source = (const GLchar *)shader_text.c_str();
	int length = shader_text.length();
	glShaderSource(s, 1, &source, &length);
	glCompileShader(s);
	check_shader(s, file_name);
	*shader = s;
}

std::string shader_program::preprocess(std::string filepath)
{
	if (filepath.at(0) == '.')
	{
		std::string shader_path = "../Shaders/";
		filepath = filepath.substr(1, filepath.length() - 1);
		filepath = shader_path + filepath;
	}
	std::ifstream basefile(filepath);
	if (!basefile.good())
		std::cout << "ERROR OPENING FILE" << filepath << std::endl;
	std::string line;
	std::string result = "";
	while (std::getline(basefile, line))
	{
		std::size_t hasInclude = line.find("#include");
		if (hasInclude != std::string::npos)
		{
			std::size_t first = line.find("\"");
			std::size_t last = line.find_last_of("\"");
			std::string include_path = line.substr(first + 1, (last - first) - 1);
			std::string include_path_complete = "";
			if (include_path.at(0) == '.')
			{
				std::string shader_path = "../Shaders/";
				include_path_complete = include_path.substr(1, filepath.length() - 1);
				include_path_complete = shader_path + include_path_complete;
			}
			std::ifstream include_file(include_path_complete);
			if (include_file.good())
			{
				include_file.close();
				std::string include_text = preprocess(include_path);
				result += (include_text + "\n");
			}
			else
			{
				include_file.close();
				std::cout << "ERROR PROCESSING " << filepath << ": " << include_path << " NOT FOUND. SKIPPING..." << std::endl;
				continue;
			}
		}
		else
		{
			result += (line + "\n");
		}
	}
	basefile.close();
	return result;
}

void shader_program::check_shader(GLuint shader, std::string name)
{
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		std::cout << "ERROR COMPILING : "<< name << " : " << std::endl;
		print_shader_log(shader);
	}
}

void shader_program::print_shader_log(GLuint shader)
{
	GLint i;
	char *s;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &i);
	if (i > 0)
	{
		s = (GLchar *)malloc(i);
		glGetShaderInfoLog(shader, i, &i, s);
		fprintf(stderr, "COMPILE LOG: '%s'\n", s);
	}
}

int shader_program::check_program(GLuint program)
{
	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	return linked;
}
