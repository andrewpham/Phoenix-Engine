#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace phoenix
{
	class Shader
	{
	public:
		unsigned int _program;

		Shader(const char*);
		Shader(const char*, const char*);
		Shader(const char*, const char*, const char*);

		inline void use() const
		{
			glUseProgram(_program);
		}

		inline void setBool(const std::string& name, bool v0) const
		{
			glUniform1i(glGetUniformLocation(_program, name.c_str()), static_cast<int>(v0));
		}
		inline void setInt(const std::string& name, int v0) const
		{
			glUniform1i(glGetUniformLocation(_program, name.c_str()), v0);
		}
		inline void setFloat(const std::string& name, float v0) const
		{
			glUniform1f(glGetUniformLocation(_program, name.c_str()), v0);
		}
		inline void setVec2(const std::string& name, const glm::vec2& value) const
		{
			glUniform2fv(glGetUniformLocation(_program, name.c_str()), 1, &value[0]);
		}
		inline void setVec2(const std::string& name, float v0, float v1) const
		{
			glUniform2f(glGetUniformLocation(_program, name.c_str()), v0, v1);
		}
		inline void setIVec2(const std::string& name, const glm::ivec2& value) const
		{
			glUniform2iv(glGetUniformLocation(_program, name.c_str()), 1, &value[0]);
		}
		inline void setIVec2(const std::string& name, int v0, int v1) const
		{
			glUniform2i(glGetUniformLocation(_program, name.c_str()), v0, v1);
		}
		inline void setVec3(const std::string& name, const glm::vec3& value) const
		{
			glUniform3fv(glGetUniformLocation(_program, name.c_str()), 1, &value[0]);
		}
		inline void setVec3(const std::string& name, float v0, float v1, float v2) const
		{
			glUniform3f(glGetUniformLocation(_program, name.c_str()), v0, v1, v2);
		}
		inline void setVec4(const std::string& name, const glm::vec4& value) const
		{
			glUniform4fv(glGetUniformLocation(_program, name.c_str()), 1, &value[0]);
		}
		inline void setVec4(const std::string& name, float v0, float v1, float v2, float v3) const
		{
			glUniform4f(glGetUniformLocation(_program, name.c_str()), v0, v1, v2, v3);
		}
		inline void setMat2(const std::string& name, const glm::mat2& value) const
		{
			glUniformMatrix2fv(glGetUniformLocation(_program, name.c_str()), 1, GL_FALSE, &value[0][0]);
		}
		inline void setMat3(const std::string& name, const glm::mat3& value) const
		{
			glUniformMatrix3fv(glGetUniformLocation(_program, name.c_str()), 1, GL_FALSE, &value[0][0]);
		}
		inline void setMat4(const std::string& name, const glm::mat4& value) const
		{
			glUniformMatrix4fv(glGetUniformLocation(_program, name.c_str()), 1, GL_FALSE, &value[0][0]);
		}

	private:
		static const unsigned int _MSG_LEN = 1024;

		void checkCompileErrors(unsigned int, const std::string);
	};
}