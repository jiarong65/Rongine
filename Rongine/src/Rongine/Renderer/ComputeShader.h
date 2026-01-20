#pragma once
#include <string>
#include <glm/glm.hpp>
#include "Rongine/Core/Core.h"

namespace Rongine {

	class ComputeShader
	{
	public:
		virtual ~ComputeShader() = default;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		// Compute Shader 特有的调度函数
		virtual void dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = 0;

		// Uniform 设置接口 (保持与 Shader 一致)
		virtual void setInt(const std::string& name, int value) = 0;
		virtual void setFloat(const std::string& name, float value) = 0;
		virtual void setFloat2(const std::string& name, const glm::vec2& value) = 0;
		virtual void setFloat3(const std::string& name, const glm::vec3& value) = 0;
		virtual void setFloat4(const std::string& name, const glm::vec4& value) = 0;
		virtual void setMat4(const std::string& name, const glm::mat4& value) = 0;
		virtual void setIntArray(const std::string& name, int* value, uint32_t count) = 0;

		virtual const std::string& getName() const = 0;

		static Ref<ComputeShader> create(const std::string& filepath);
	};

}