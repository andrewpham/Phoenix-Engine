#include <engine/shader.h>
#include <engine/strings.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace phoenix
{
	Shader::Shader(const char* cShaderFilename)
	{
		std::string cShaderStr;
		std::ifstream cShaderFileStream;
		cShaderFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			cShaderFileStream.open(cShaderFilename);

			std::stringstream cShaderStrStream;
			cShaderStrStream << cShaderFileStream.rdbuf();

			cShaderFileStream.close();

			cShaderStr = cShaderStrStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cerr << FILE_STREAM_OPEN_ERROR;
		}

		const char* cShaderCode = cShaderStr.c_str();

		unsigned int cShader;

		cShader = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(cShader, 1, &cShaderCode, nullptr);
		glCompileShader(cShader);
		checkCompileErrors(cShader, "compute");

		_program = glCreateProgram();
		glAttachShader(_program, cShader);
		glLinkProgram(_program);
		checkCompileErrors(_program, "program");

		glDeleteShader(cShader);
	}

	Shader::Shader(const char* vShaderFilename, const char* fShaderFilename)
	{
		std::string vShaderStr;
		std::string fShaderStr;
		std::ifstream vShaderFileStream;
		std::ifstream fShaderFileStream;
		vShaderFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			vShaderFileStream.open(vShaderFilename);
			fShaderFileStream.open(fShaderFilename);

			std::stringstream vShaderStrStream, fShaderStrStream;
			vShaderStrStream << vShaderFileStream.rdbuf();
			fShaderStrStream << fShaderFileStream.rdbuf();

			vShaderFileStream.close();
			fShaderFileStream.close();

			vShaderStr = vShaderStrStream.str();
			fShaderStr = fShaderStrStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cerr << FILE_STREAM_OPEN_ERROR;
		}

		const char* vShaderCode = vShaderStr.c_str();
		const char* fShaderCode = fShaderStr.c_str();

		unsigned int vShader, fShader;

		vShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vShader, 1, &vShaderCode, nullptr);
		glCompileShader(vShader);
		checkCompileErrors(vShader, "vertex");

		fShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fShader, 1, &fShaderCode, nullptr);
		glCompileShader(fShader);
		checkCompileErrors(fShader, "fragment");

		_program = glCreateProgram();
		glAttachShader(_program, vShader);
		glAttachShader(_program, fShader);
		glLinkProgram(_program);
		checkCompileErrors(_program, "program");

		glDeleteShader(vShader);
		glDeleteShader(fShader);
	}

	Shader::Shader(const char* vShaderFilename, const char* gShaderFilename, const char* fShaderFilename)
	{
		std::string vShaderStr;
		std::string gShaderStr;
		std::string fShaderStr;
		std::ifstream vShaderFileStream;
		std::ifstream gShaderFileStream;
		std::ifstream fShaderFileStream;
		vShaderFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		gShaderFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			vShaderFileStream.open(vShaderFilename);
			gShaderFileStream.open(gShaderFilename);
			fShaderFileStream.open(fShaderFilename);

			std::stringstream vShaderStrStream, gShaderStrStream, fShaderStrStream;
			vShaderStrStream << vShaderFileStream.rdbuf();
			gShaderStrStream << gShaderFileStream.rdbuf();
			fShaderStrStream << fShaderFileStream.rdbuf();

			vShaderFileStream.close();
			gShaderFileStream.close();
			fShaderFileStream.close();

			vShaderStr = vShaderStrStream.str();
			gShaderStr = gShaderStrStream.str();
			fShaderStr = fShaderStrStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cerr << FILE_STREAM_OPEN_ERROR;
		}

		const char* vShaderCode = vShaderStr.c_str();
		const char* gShaderCode = gShaderStr.c_str();
		const char* fShaderCode = fShaderStr.c_str();

		unsigned int vShader, gShader, fShader;

		vShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vShader, 1, &vShaderCode, nullptr);
		glCompileShader(vShader);
		checkCompileErrors(vShader, "vertex");

		gShader = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(gShader, 1, &gShaderCode, nullptr);
		glCompileShader(gShader);
		checkCompileErrors(gShader, "geometry");

		fShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fShader, 1, &fShaderCode, nullptr);
		glCompileShader(fShader);
		checkCompileErrors(fShader, "fragment");

		_program = glCreateProgram();
		glAttachShader(_program, vShader);
		glAttachShader(_program, gShader);
		glAttachShader(_program, fShader);
		glLinkProgram(_program);
		checkCompileErrors(_program, "program");

		glDeleteShader(vShader);
		glDeleteShader(gShader);
		glDeleteShader(fShader);
	}

	void Shader::checkCompileErrors(unsigned int shaderID, const std::string shaderType)
	{
		int success;
		char log[_MSG_LEN];

		if (shaderType != "program")
		{
			glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shaderID, _MSG_LEN, nullptr, log);
				std::cerr << "Error compiling " << shaderType << " shader!\n" << log << "\n";
			}
		}
		else
		{
			glGetProgramiv(shaderID, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shaderID, _MSG_LEN, nullptr, log);
				std::cerr << "Error linking shaders!\n" << log << "\n";
			}
		}
	}
}