#include "Rongpch.h"
#include "OpenGLComputeShader.h"
#include "Rongine/Core/Log.h"
#include <fstream>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace Rongine {

	OpenGLComputeShader::OpenGLComputeShader(const std::string& filepath)
	{
		std::string source = readFile(filepath);
		compile(source);

		// 从文件路径提取文件名作为 Shader 名称
		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind(".");
		size_t count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_name = filepath.substr(lastSlash, count);
	}

	OpenGLComputeShader::~OpenGLComputeShader()
	{
		glDeleteProgram(m_rendererID);
	}

	void OpenGLComputeShader::bind() const
	{
		glUseProgram(m_rendererID);
	}

	void OpenGLComputeShader::unbind() const
	{
		glUseProgram(0);
	}

	void OpenGLComputeShader::dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
	{
		// 1. 启动计算
		glDispatchCompute(groupX, groupY, groupZ);

		// 2. 内存屏障：确保计算着色器写入图像/SSBO完成后，其他操作才能读取
		// GL_ALL_BARRIER_BITS 比较保险，也可以根据具体用途使用 GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	void OpenGLComputeShader::setInt(const std::string& name, int value) { uploadUniformInt(name, value); }
	void OpenGLComputeShader::setFloat(const std::string& name, float value) { uploadUniformFloat(name, value); }
	void OpenGLComputeShader::setFloat2(const std::string& name, const glm::vec2& value) { uploadUniformFloat2(name, value); }
	void OpenGLComputeShader::setFloat3(const std::string& name, const glm::vec3& value) { uploadUniformFloat3(name, value); }
	void OpenGLComputeShader::setFloat4(const std::string& name, const glm::vec4& value) { uploadUniformFloat4(name, value); }
	void OpenGLComputeShader::setMat4(const std::string& name, const glm::mat4& value) { uploadUniformMat4(name, value); }
	void OpenGLComputeShader::setIntArray(const std::string& name, int* value, uint32_t count) { uploadUniformIntArray(name, value, count); }

	void OpenGLComputeShader::uploadUniformInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(m_rendererID, name.c_str());
		if (location != -1) glUniform1i(location, value);
	}
	void OpenGLComputeShader::uploadUniformFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(m_rendererID, name.c_str());
		if (location != -1) glUniform1f(location, value);
	}
	void OpenGLComputeShader::uploadUniformFloat2(const std::string& name, const glm::vec2& value)
	{
		GLint location = glGetUniformLocation(m_rendererID, name.c_str());
		if (location != -1) glUniform2f(location, value.x, value.y);
	}
	void OpenGLComputeShader::uploadUniformFloat3(const std::string& name, const glm::vec3& value)
	{
		GLint location = glGetUniformLocation(m_rendererID, name.c_str());
		if (location != -1) glUniform3f(location, value.x, value.y, value.z);
	}
	void OpenGLComputeShader::uploadUniformFloat4(const std::string& name, const glm::vec4& value)
	{
		GLint location = glGetUniformLocation(m_rendererID, name.c_str());
		if (location != -1) glUniform4f(location, value.x, value.y, value.z, value.w);
	}
	void OpenGLComputeShader::uploadUniformMat4(const std::string& name, const glm::mat4& matrix)
	{
		GLint location = glGetUniformLocation(m_rendererID, name.c_str());
		if (location != -1) glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}
	void OpenGLComputeShader::uploadUniformIntArray(const std::string& name, int* values, uint32_t count)
	{
		GLint location = glGetUniformLocation(m_rendererID, name.c_str());
		if (location != -1) glUniform1iv(location, count, values);
	}

	std::string OpenGLComputeShader::readFile(const std::string& filepath)
	{
		std::string result;
		std::fstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&result[0], result.size());
			in.close();
		}
		else
		{
			RONG_CORE_ERROR("Could not open shader file '{0}'", filepath);
		}
		return result;
	}

	void OpenGLComputeShader::compile(const std::string& source)
	{
		// 1. 创建 Program
		GLuint program = glCreateProgram();

		// 2. 创建 Compute Shader
		GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
		const GLchar* sourceCStr = source.c_str();
		glShaderSource(shader, 1, &sourceCStr, 0);
		glCompileShader(shader);

		// 3. 检查编译错误
		GLint isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

			glDeleteShader(shader);
			glDeleteProgram(program); 

			RONG_CORE_ERROR("Compute Shader Compilation Failed: {0}", infoLog.data());
			RONG_CORE_ASSERT(false, "Compute Shader compilation failure!");
			return;
		}

		// 4. 链接 Program
		glAttachShader(program, shader);
		glLinkProgram(program);

		// 5. 检查链接错误
		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

			glDeleteProgram(program);
			glDeleteShader(shader);

			RONG_CORE_ERROR("Compute Shader Link Failed: {0}", infoLog.data());
			RONG_CORE_ASSERT(false, "Compute Shader link failure!");
			return;
		}

		// 6. 清理 Shader 对象
		glDetachShader(program, shader);
		glDeleteShader(shader);

		m_rendererID = program;
	}
}