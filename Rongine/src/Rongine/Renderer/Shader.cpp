#include "Rongpch.h"
#include "Shader.h"


#include "Rongine/Core/Log.h"
#include "Rongine/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"

namespace Rongine {
	Ref<Shader> Shader::create(const std::string& filepath)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None:
		{
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL:
		{
			return std::make_shared<OpenGLShader>(filepath);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}
	Ref<Shader> Shader::create(const std::string& name,const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None:
		{
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL:
		{
			return std::make_shared<OpenGLShader>(name,vertexSrc,fragmentSrc);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}

	void ShaderLibray::add(const std::string& name, const Ref<Shader>& shader)
	{
		RONG_CORE_ASSERT(!exists(name), "Shader already exists!");
		m_shaders[name] = shader;
	}

	void ShaderLibray::add(const Ref<Shader>& shader)
	{
		const std::string& name = shader->getName();
		add(name, shader);
	}

	Ref<Shader> ShaderLibray::load(const std::string& filepath)
	{
		Ref<Shader> shader=Shader::create(filepath);
		auto& name = shader->getName();
		add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibray::load(const std::string& name, const std::string& filepath)
	{
		Ref<Shader> shader = Shader::create(filepath);
		add(name, shader);
		return shader;
	}

	Ref<Shader> ShaderLibray::get(const std::string& name)
	{
		RONG_CORE_ASSERT(exists(name), "Shader is not exists!");
		return m_shaders[name];
	}

	bool ShaderLibray::exists(const std::string& name)
	{
		return m_shaders.find(name) != m_shaders.end();
	}

}

