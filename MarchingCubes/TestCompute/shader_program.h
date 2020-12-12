
#include <string>
#include <GL/glew.h>
#include <fstream>
#include <sstream>

const GLenum SHADER_TYPES[5] = {
	GL_VERTEX_SHADER,
	GL_FRAGMENT_SHADER,
	GL_GEOMETRY_SHADER,
	GL_TESS_CONTROL_SHADER,
	GL_TESS_EVALUATION_SHADER
};
class shader_program
{
	public:
		GLuint program_id;
		std::string file_paths[5];
		int files;
		shader_program()
		{
			files = 0;
			program_id = 0;
		}
		shader_program(std::string compute_path);
		shader_program(std::string vertex_path, std::string fragment_path);
		shader_program::shader_program(std::string vertex_path, std::string fragment_path, std::string geometry_path);
		shader_program(std::string vertex_path, std::string fragment_path, std::string geometry_path, std::string tess_control_path, std::string tess_eval_path);
		void reload_shaders();
	private:
		static void shader_program::create_shader(GLuint *shader, std::string file_name, GLenum type);
};