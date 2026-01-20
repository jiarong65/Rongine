#pragma once
#include "Rongine/Renderer/ComputeShader.h"
#include <glad/glad.h>

namespace Rongine {

	class OpenGLComputeShader : public ComputeShader
	{
	public:
		OpenGLComputeShader(const std::string& filepath);
		virtual ~OpenGLComputeShader();

		virtual void bind() const override;
		virtual void unbind() const override;

		virtual void dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ) override;

		virtual void setInt(const std::string& name, int value) override;
		virtual void setFloat(const std::string& name, float value) override;
		virtual void setFloat2(const std::string& name, const glm::vec2& value) override;
		virtual void setFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void setFloat4(const std::string& name, const glm::vec4& value) override;
		virtual void setMat4(const std::string& name, const glm::mat4& value) override;
		virtual void setIntArray(const std::string& name, int* value, uint32_t count) override;

		virtual const std::string& getName() const override { return m_name; }

		void uploadUniformInt(const std::string& name, int value);
		void uploadUniformFloat(const std::string& name, float value);
		void uploadUniformFloat2(const std::string& name, const glm::vec2& value);
		void uploadUniformFloat3(const std::string& name, const glm::vec3& value);
		void uploadUniformFloat4(const std::string& name, const glm::vec4& value);
		void uploadUniformMat4(const std::string& name, const glm::mat4& matrix);
		void uploadUniformIntArray(const std::string& name, int* values, uint32_t count);

	private:
		std::string readFile(const std::string& filepath);
		void compile(const std::string& source);

	private:
		uint32_t m_rendererID;
		std::string m_name;
	};
}