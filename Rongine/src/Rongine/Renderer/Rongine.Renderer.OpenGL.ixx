module;
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#include <initializer_list>
#include <cstring>
#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "stb_image.h"

#include "Rongine/Core/RongineMacros.h"

export module Rongine.Renderer:OpenGL;

import :Interfaces;

import Rongine.Core;
import Rongine.RendererData;

// =============================================================
// 非导出：OpenGL 后端辅助函数（原 impl.cpp 内容）
// =============================================================
namespace Rongine {

	static GLenum shaderTypeFormString(const std::string& type)
	{
		if (type == "vertex")
		{
			return GL_VERTEX_SHADER;
		}
		if (type == "fragment")
		{
			return GL_FRAGMENT_SHADER;
		}

		RONG_CORE_ASSERT(false, "Unknown shader type!");
		return 0;
	}

	static GLenum shaderDateTypetoOpenGLBaseType(const Rongine::ShaderDataType& type)
	{
		switch (type)
		{
		case ShaderDataType::Float:
		case ShaderDataType::Float2:
		case ShaderDataType::Float3:
		case ShaderDataType::Float4:
		case ShaderDataType::Mat3:
		case ShaderDataType::Mat4: return GL_FLOAT;
		case ShaderDataType::Int:
		case ShaderDataType::Int2:
		case ShaderDataType::Int3:
		case ShaderDataType::Int4: return GL_INT;
		case ShaderDataType::Bool: return GL_BOOL;
		}
		RONG_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	// === 辅助工具函数：判断是否是深度格式 ===
	static bool isDepthFormat(FramebufferTextureFormat format)
	{
		switch (format)
		{
			case FramebufferTextureFormat::DEPTH24STENCIL8: return true;
		}
		return false;
	}

	// === 辅助工具函数：根据枚举创建纹理 (DSA) ===
	static void createTextures(bool multisampled, uint32_t* outID, uint32_t count)
	{
		glCreateTextures(GL_TEXTURE_2D, count, outID);
		// 这里的 multisampled 暂时没用，为了以后抗锯齿预留
	}

	static void bindTexture(bool multisampled, uint32_t id)
	{
		glBindTexture(GL_TEXTURE_2D, id);
	}

	// === 辅助工具函数：配置纹理参数 ===
	static void attachColorTexture(uint32_t id, int samples, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, int index)
	{
		// 1. 分配显存
		glTextureStorage2D(id, 1, internalFormat, width, height);

		// 2. 设置过滤参数
		// 注意：整数纹理 (RED_INTEGER) 必须使用 NEAREST，不能线性插值！
		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// 3. 绑定到 FBO
		// glNamedFramebufferTexture 是 OpenGL 4.5 DSA
		// 如果我们把 FBO ID 传进去，还需要加上 FBO 参数，这里为了通用性
		// 我们在 invalidate 外部统一做 glNamedFramebufferTexture
	}

	static GLenum OpenGLUsage(ShaderStorageBufferUsage usage)
	{
		switch (usage)
		{
		case ShaderStorageBufferUsage::StaticDraw:  return GL_STATIC_DRAW;
		case ShaderStorageBufferUsage::DynamicDraw: return GL_DYNAMIC_DRAW;
		}
		RONG_CORE_ASSERT(false, "Unknown usage!");
		return GL_DYNAMIC_DRAW;
	}

	static GLenum ToGLBlendFactor(BlendFactor factor)
	{
		switch (factor)
		{
		case BlendFactor::Zero:                return GL_ZERO;
		case BlendFactor::One:                 return GL_ONE;
		case BlendFactor::SrcAlpha:            return GL_SRC_ALPHA;
		case BlendFactor::OneMinusSrcAlpha:    return GL_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DstAlpha:            return GL_DST_ALPHA;
		case BlendFactor::OneMinusDstAlpha:    return GL_ONE_MINUS_DST_ALPHA;
		}
		return GL_ONE;
	}

	static GLenum ToGLDepthFunc(DepthFunc func)
	{
		switch (func)
		{
		case DepthFunc::Less:         return GL_LESS;
		case DepthFunc::LessEqual:    return GL_LEQUAL;
		case DepthFunc::Equal:        return GL_EQUAL;
		case DepthFunc::Greater:      return GL_GREATER;
		case DepthFunc::GreaterEqual: return GL_GEQUAL;
		case DepthFunc::Always:       return GL_ALWAYS;
		case DepthFunc::Never:        return GL_NEVER;
		}
		return GL_LESS;
	}

} // namespace Rongine

export namespace Rongine {
	class GraphicsContext {
	public:
		virtual void init()=0;
		virtual void swapBuffers()=0;
	};


	class OpenGLVertexBuffer:public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(uint32_t size)
			:m_size(size)
		{
			glCreateBuffers(1, &m_rendererID);
			glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
			glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
		}

		OpenGLVertexBuffer(float* vertex,uint32_t size)
			:m_size(size)
		{
			glCreateBuffers(1, &m_rendererID);
			glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
			glBufferData(GL_ARRAY_BUFFER, size, vertex, GL_STATIC_DRAW);
		}

		virtual ~OpenGLVertexBuffer()
		{
			glDeleteBuffers(1,&m_rendererID);
		}

		virtual void bind() const override
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
		}

		virtual void unbind() const override
		{
			glBindBuffer(GL_ARRAY_BUFFER,0);
		}

		virtual void setData(const void* data, uint32_t size) override
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
			glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
		}

		virtual const BufferLayout& getLayout()const override { return m_layout; }
		virtual void setLayout(const BufferLayout& layout) override { m_layout = layout; }

		virtual uint32_t getSize() const override { return m_size; }
	private:
		uint32_t m_rendererID;
		BufferLayout m_layout;
		uint32_t m_size;
	};

	class OpenGLIndexBuffer :public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(uint32_t count)
			:m_count(count)
		{
			glCreateBuffers(1, &m_rendererID);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);
		}

		OpenGLIndexBuffer(uint32_t* indices, uint32_t count)
			:m_count(count)
		{
			glCreateBuffers(1, &m_rendererID);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, count*sizeof(uint32_t), indices, GL_STATIC_DRAW);
		}

		virtual ~OpenGLIndexBuffer()
		{
			glDeleteBuffers(1, &m_rendererID);
		}

		inline uint32_t getCount() const override{ return m_count; }

		virtual void bind() const override
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
		}

		virtual void unbind() const override
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	private:
		uint32_t m_rendererID;
		uint32_t m_count;
	};




	class OpenGLShader:public Shader
	{
	public:
		OpenGLShader(const std::string& filepath)
		{
			std::string source = readFile(filepath);
			auto shaderSources = preProcess(source);
			compile(shaderSources);

			//从文件路径截取文件名
			auto lastSlash = filepath.find_last_of("/\\");
			lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
			auto lastDot = filepath.rfind(".");
			size_t count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
			m_name = filepath.substr(lastSlash, count);
		}

		OpenGLShader(const std::string& name,const std::string& vertexSrc, const std::string& fragmentSrc)
			:m_name(name)
		{
			std::unordered_map<GLenum, std::string> shaderSources;
			shaderSources[GL_VERTEX_SHADER] = vertexSrc;
			shaderSources[GL_FRAGMENT_SHADER] = fragmentSrc;
			compile(shaderSources);
		}

		virtual ~OpenGLShader() override
		{
			glDeleteProgram(m_rendererID);
		}

		virtual void bind() const override
		{
			glUseProgram(m_rendererID);
		}

		virtual void unbind() const override
		{
			glUseProgram(0);
		}

		virtual const std::string& getName() const override { return m_name; }

		virtual void setInt(const std::string& name, int value) override
		{
			uploadUniformInt(name, value);
		}

		virtual void setFloat(const std::string& name, float value) override
		{
			uploadUniformFloat(name, value);
		}

		virtual void setFloat3(const std::string& name, const glm::vec3& value) override
		{
			uploadUniformFloat3(name, value);
		}

		virtual void setFloat4(const std::string& name, const glm::vec4& value) override
		{
			uploadUniformFloat4(name, value);
		}

		virtual void setMat4(const std::string& name, const glm::mat4& value) override
		{
			uploadUniformMat4(name, value);
		}

		virtual void setIntArray(const std::string& name, int* value, uint32_t count) override
		{
			uploadUniformIntArray(name, value, count);
		}


		void uploadUniformInt(const std::string& name, int value)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			glUniform1i(location, value);
		}

		void uploadUniformFloat(const std::string& name, float value)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			glUniform1f(location, value);
		}

		void uploadUniformFloat2(const std::string& name, const glm::vec2& value)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			glUniform2f(location, value.x,value.y);
		}

		void uploadUniformFloat3(const std::string& name, const glm::vec3& value)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			glUniform3f(location, value.x, value.y,value.z);
		}

		void uploadUniformFloat4(const std::string& name, const glm::vec4& value)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			glUniform4f(location, value.x, value.y, value.z,value.w);
		}

		void uploadUniformMat3(const std::string& name, const glm::mat3& matrix)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
		}

		void uploadUniformMat4(const std::string& name, const glm::mat4& matrix)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
		}

		void uploadUniformIntArray(const std::string& name, int* values, uint32_t count)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			glUniform1iv(location, count,values);
		}

	private:
		std::string readFile(const std::string& filepath)
		{
			std::string result;

			std::fstream in;
			in.open(filepath, std::fstream::in| std::fstream::binary);
			if (in)
			{
				in.seekg(0, std::fstream::end);
				result.resize(in.tellg());
				in.seekg(0, std::fstream::beg);
				in.read(&result[0], result.size());
				in.close();
			}
			else
			{
				RONG_CORE_ASSERT(false, "Could not open file '{0}'", filepath);
			}

			return result;
		}

		std::unordered_map<uint32_t, std::string> preProcess(const std::string& source)
		{
			std::unordered_map<GLenum, std::string> shaderSources;

			const char* typeToken = "#type";
			size_t typeTokenLength = strlen(typeToken);
			size_t pos = source.find(typeToken, 0);

			while (pos != std::string::npos)
			{
				size_t eol = source.find_first_of("\r\n", pos);
				RONG_CORE_ASSERT(eol != std::string::npos, "Syntax error");

				size_t begin = pos + typeTokenLength + 1;
				std::string type = source.substr(begin, eol-begin);

				size_t nextLinePos = source.find_first_not_of("\r\n", eol);
				pos = source.find(typeToken, nextLinePos);

				shaderSources[shaderTypeFormString(type)] = source.substr(nextLinePos,(pos!=std::string::npos?pos:source.size())-nextLinePos);
			}

			return shaderSources;
		}

		void compile(const std::unordered_map<uint32_t, std::string>& shaderSources)
		{
			GLuint program = glCreateProgram();

			RONG_CORE_ASSERT(shaderSources.size() <= 2, "We only support 2 shaders for now");
			std::array<GLenum,2> glShaderIDs;
			int glShaderIDIndex = 0;

			for (auto& kv : shaderSources)
			{
				GLenum type = kv.first;
				std::string source = kv.second;

				GLuint shader = glCreateShader(type);

				glShaderIDs[glShaderIDIndex++] = shader;

				const GLchar* sourceCStr = source.c_str();
				glShaderSource(shader, 1, &sourceCStr,0);

				glCompileShader(shader);

				GLint isCompiled = 0;
				glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
				if (isCompiled == GL_FALSE)
				{
					GLint maxLength = 0;
					glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

					std::vector<GLchar> infoLog(maxLength);
					glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

					glDeleteShader(shader);

					RONG_CORE_ERROR("{0}", infoLog.data());
					RONG_CORE_ASSERT(false, "Shader compilation failure!");
					break;
				}

				glAttachShader(program, shader);
			}

			m_rendererID = program;

			glLinkProgram(program);
			GLint isLinked = 0;
			glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
			if (isLinked == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

				// The maxLength includes the NULL character
				std::vector<GLchar> infoLog(maxLength);
				glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

				// We don't need the program anymore.
				glDeleteProgram(program);

				for (auto id : glShaderIDs)
					glDeleteShader(id);

				RONG_CORE_ERROR("{0}", infoLog.data());
				RONG_CORE_ASSERT(false, "Shader link failure!");
				return;
			}

			for (auto& id : glShaderIDs)
				glDetachShader(program, id);
		}

	private:
		uint32_t m_rendererID;
		std::string m_name;
	};




	class OpenGLTexture2D:public Texture2D
	{
	public:
		OpenGLTexture2D(uint32_t width,uint32_t height)
			:m_width(width),m_height(height)
		{
			m_internalFormat = GL_RGBA8;
			m_dataFormat = GL_RGBA;

			glCreateTextures(GL_TEXTURE_2D, 1, &m_rendererID);
			glTextureStorage2D(m_rendererID, 1, m_internalFormat, m_width, m_height);

			glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTextureParameterf(m_rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

		OpenGLTexture2D(const std::string& path)
			:m_path(path)
		{
			stbi_set_flip_vertically_on_load(1);

			int width, height, channels;
			stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels,0);
			RONG_CORE_ASSERT(data, "Failed to load image!");

			if (channels == 3)
			{
				m_internalFormat = GL_RGB8;
				m_dataFormat = GL_RGB;
			}
			else if (channels == 4)
			{
				m_internalFormat = GL_RGBA8;
				m_dataFormat = GL_RGBA;
			}

			m_width = width;
			m_height = height;

			glCreateTextures(GL_TEXTURE_2D, 1, &m_rendererID);
			glTextureStorage2D(m_rendererID, 1, m_internalFormat, m_width, m_height);

			glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTextureParameterf(m_rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTextureSubImage2D(m_rendererID, 0, 0, 0, m_width, m_height, m_dataFormat, GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);
		}

		OpenGLTexture2D(const TextureSpecification& specification)
			: m_specification(specification), m_width(specification.Width), m_height(specification.Height)
		{
			m_internalFormat = GL_RGBA8;
			m_dataFormat = GL_RGBA;

			if (specification.Format == ImageFormat::RGBA32F)
			{
				m_internalFormat = GL_RGBA32F;
				m_dataFormat = GL_RGBA;
			}

			glCreateTextures(GL_TEXTURE_2D, 1, &m_rendererID);
			glTextureStorage2D(m_rendererID, 1, m_internalFormat, m_width, m_height);

			// 设置过滤器
			glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

		virtual ~OpenGLTexture2D() override
		{
			glDeleteTextures(1, &m_rendererID);
		}

		virtual uint32_t getWidth() const override { return m_width; }
		virtual uint32_t getHeight() const override { return m_height; }

		virtual uint32_t getRendererID() const override { return m_rendererID; }

		virtual void bind(uint32_t slot=0) override
		{
			glBindTextureUnit(slot, m_rendererID);
		}

		virtual void setData(void* data, uint32_t size) override
		{
			uint32_t bpp = m_dataFormat == GL_RGBA ? 4 : 3;
			RONG_CORE_ASSERT(m_width * m_height * bpp == size, "Data must be entire texture!");
			glTextureSubImage2D(m_rendererID, 0, 0, 0, m_width, m_height, m_dataFormat, GL_UNSIGNED_BYTE, data);
		}

		virtual bool operator==(const Texture& other) const override {
			return m_rendererID == ((OpenGLTexture2D&)other).m_rendererID;
		}
	private:
		TextureSpecification m_specification;

		std::string m_path;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_rendererID;

		uint32_t m_internalFormat, m_dataFormat;
	};




	class OpenGLVertexArray:public VertexArray
	{
	public:
		OpenGLVertexArray()
		{
			glCreateVertexArrays(1, &m_rendererID);
		}

		virtual ~OpenGLVertexArray()
		{
			glDeleteVertexArrays(1, &m_rendererID);
		}

		virtual void bind() const override
		{
			glBindVertexArray(m_rendererID);
		}

		virtual void unbind() const override
		{
			glBindVertexArray(0);
		}

		virtual void addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override
		{
			glBindVertexArray(m_rendererID);
			vertexBuffer->bind();

			const auto& layout = vertexBuffer->getLayout();
			uint32_t index = 0;
			for (const auto& element : layout)
			{
				switch (element.type)
				{
				case ShaderDataType::Float:
				case ShaderDataType::Float2:
				case ShaderDataType::Float3:
				case ShaderDataType::Float4:
				case ShaderDataType::Mat3:
				case ShaderDataType::Mat4:
				{
					glEnableVertexAttribArray(index);
					glVertexAttribPointer(
						index,
						element.getComponentCount(),
						shaderDateTypetoOpenGLBaseType(element.type),
						element.normalized ? GL_TRUE : GL_FALSE,
						layout.getStride(),
						(const void*)element.offset
					);
					index++;
					break;
				}
				case ShaderDataType::Int:
				case ShaderDataType::Int2:
				case ShaderDataType::Int3:
				case ShaderDataType::Int4:
				case ShaderDataType::Bool:
				{
					glEnableVertexAttribArray(index);
					// 核心修复：针对整数类型，必须使用 glVertexAttribIPointer
					// 注意：这个函数没有 normalized 参数
					glVertexAttribIPointer(
						index,
						element.getComponentCount(),
						shaderDateTypetoOpenGLBaseType(element.type),
						layout.getStride(),
						(const void*)element.offset
					);
					index++;
					break;
				}
				}
			}

			m_vertexBuffers.push_back(vertexBuffer);
		}

		virtual void setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override
		{
			glBindVertexArray(m_rendererID);
			indexBuffer->bind();

			m_indexBuffer = indexBuffer;
		}

		virtual const std::vector<Ref<VertexBuffer>>& getVertexBuffers()const override { return m_vertexBuffers; }
		virtual const Ref<IndexBuffer>& getIndexBuffer() const override { return m_indexBuffer; }

	private:
		uint32_t m_rendererID;
		std::vector<Ref<VertexBuffer>> m_vertexBuffers;
		Ref<IndexBuffer> m_indexBuffer;
	};




	class OpenGLFramebuffer:public Framebuffer
	{
	public:
		OpenGLFramebuffer(const FramebufferSpecification& spec)
			: m_specification(spec)
		{
			// 1. 分离颜色附件和深度附件的配置
			for (auto spec : m_specification.Attachments.Attachments)
			{
				if (!isDepthFormat(spec.TextureFormat))
					m_colorAttachmentSpecs.emplace_back(spec);
				else
					m_depthAttachmentSpec = spec;
			}

			invalidate();
		}

		virtual ~OpenGLFramebuffer()
		{
			glDeleteFramebuffers(1, &m_rendererID);
			glDeleteTextures(m_colorAttachments.size(), m_colorAttachments.data());
			glDeleteTextures(1, &m_depthAttachment);
		}

		void invalidate()
		{
			if (m_rendererID)
			{
				glDeleteFramebuffers(1, &m_rendererID);
				glDeleteTextures(m_colorAttachments.size(), m_colorAttachments.data());
				glDeleteTextures(1, &m_depthAttachment);

				m_colorAttachments.clear();
				m_depthAttachment = 0;
			}

			// 创建 FBO
			glCreateFramebuffers(1, &m_rendererID);

			bool multisample = m_specification.samples > 1;

			// --- 1. 处理颜色附件 ---
			if (m_colorAttachmentSpecs.size())
			{
				// 调整 vector 大小并创建 Texture IDs
				m_colorAttachments.resize(m_colorAttachmentSpecs.size());
				createTextures(multisample, m_colorAttachments.data(), m_colorAttachments.size());

				for (size_t i = 0; i < m_colorAttachments.size(); i++)
				{
					// 根据格式进行不同的显存分配
					switch (m_colorAttachmentSpecs[i].TextureFormat)
					{
					case FramebufferTextureFormat::RGBA8:
						attachColorTexture(m_colorAttachments[i], m_specification.samples, GL_RGBA8, GL_RGBA, m_specification.width, m_specification.height, i);
						break;
					case FramebufferTextureFormat::RED_INTEGER:
						// 注意：R32I 是 32位整数，Format 是 RED_INTEGER
						// 重要：对于整数纹理，必须覆盖过滤参数为 Nearest，否则无法读取
						glTextureStorage2D(m_colorAttachments[i], 1, GL_R32I, m_specification.width, m_specification.height);
						glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
						glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						break;
					case FramebufferTextureFormat::RG_INTEGER:
						// 使用 GL_RG32I (两个32位整数)
						glTextureStorage2D(m_colorAttachments[i], 1, GL_RG32I, m_specification.width, m_specification.height);
						// 整数纹理必须用最近邻插值
						glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
						glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						break;
					case FramebufferTextureFormat::RGBA_INTEGER:
						// 使用 GL_RGBA32I (4个32位整数：R, G, B, A)
						glTextureStorage2D(m_colorAttachments[i], 1, GL_RGBA32I, m_specification.width, m_specification.height);

						// 整数纹理必须最近邻插值，否则无法读取
						glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
						glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						break;
					}

					// 将纹理挂载到 FBO 的 COLOR_ATTACHMENT[i]
					glNamedFramebufferTexture(m_rendererID, GL_COLOR_ATTACHMENT0 + i, m_colorAttachments[i], 0);
				}
			}

			// --- 2. 处理深度附件 ---
			if (m_depthAttachmentSpec.TextureFormat != FramebufferTextureFormat::None)
			{
				createTextures(multisample, &m_depthAttachment, 1);
				glTextureStorage2D(m_depthAttachment, 1, GL_DEPTH24_STENCIL8, m_specification.width, m_specification.height);
				glNamedFramebufferTexture(m_rendererID, GL_DEPTH_STENCIL_ATTACHMENT, m_depthAttachment, 0);
			}

			// --- 3. 告诉 OpenGL 我们要画到哪些附件上 (DrawBuffers) ---
			if (m_colorAttachments.size() > 1)
			{
				RONG_CORE_ASSERT(m_colorAttachments.size() <= 4, "Rongine only supports 4 color attachments!");
				GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

				// DSA 版本的 DrawBuffers
				glNamedFramebufferDrawBuffers(m_rendererID, m_colorAttachments.size(), buffers);
			}
			else if (m_colorAttachments.empty())
			{
				// 只有深度的情况
				glNamedFramebufferDrawBuffer(m_rendererID, GL_NONE);
			}

			// 检查完整性
			if (glCheckNamedFramebufferStatus(m_rendererID, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				RONG_CORE_ERROR("Framebuffer is incomplete!");
			}
		}

		virtual void bind() override
		{
			glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
			glViewport(0, 0, m_specification.width, m_specification.height);
		}

		virtual void unbind() override
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		virtual uint32_t getColorAttachmentRendererID(uint32_t index=0) const override
		{
			RONG_CORE_ASSERT(index < m_colorAttachments.size(), "Index out of bounds!");
			return m_colorAttachments[index];
		}

		virtual const FramebufferSpecification& getSpecification() const override { return m_specification; }

		virtual void resize(uint32_t width, uint32_t height) override
		{
			if (width == 0 || height == 0 || width > 8192 || height > 8192) return;
			m_specification.width = width;
			m_specification.height = height;
			invalidate();
		}

		// 读取像素 ID
		virtual int readPixel(uint32_t attachmentIndex, int x, int y) override
		{
			RONG_CORE_ASSERT(attachmentIndex < m_colorAttachments.size(),"attachmentIndex > m_colorAttachments.size()");

			// 这里的 ReadBuffer 设置为我们要读取的那个 attachment
			glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

			int pixelData;
			// 读取 x, y 处 1x1 大小的像素，格式为 RED_INTEGER, 类型为 INT
			glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);

			return pixelData;
		}

		virtual std::pair<int, int> readPixelRG(uint32_t attachmentIndex, int x, int y) override
		{
			RONG_CORE_ASSERT(attachmentIndex < m_colorAttachments.size(), "attachmentIndex > m_colorAttachments.size()");
			glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

			int pixelData[2]; // 存 EntityID, FaceID
			glReadPixels(x, y, 1, 1, GL_RG_INTEGER, GL_INT, pixelData);

			return { pixelData[0], pixelData[1] };
		}

		virtual glm::ivec4 readPixelID(uint32_t attachmentIndex, int x, int y) override
		{
			glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

			int pixelData[4] = { -1, -1, -1, -1 }; // 默认值
			glReadPixels(x, y, 1, 1, GL_RGBA_INTEGER, GL_INT, &pixelData);

			return { pixelData[0], pixelData[1], pixelData[2], pixelData[3] };
		}

		// 清空附件
		virtual void clearAttachment(uint32_t attachmentIndex, int value) override
		{
			RONG_CORE_ASSERT(attachmentIndex < m_colorAttachments.size(), "Index out of bounds!");

			auto& spec = m_colorAttachmentSpecs[attachmentIndex];
			int clearValue[4] = { value, value, value, value };

			// 针对 RGBA8 (颜色层) 和 整数层 分别处理
			if (spec.TextureFormat == FramebufferTextureFormat::RGBA8)
			{
				// 如果是清空背景颜色层，value 通常被当做颜色值（需要注意这里逻辑是否符合你预期）
				// 建议增加一个专门清空颜色的 clearColorAttachment(vec4)
				float clearColor[4] = { (float)value, (float)value, (float)value, (float)value };
				glClearTexImage(m_colorAttachments[attachmentIndex], 0, GL_RGBA, GL_FLOAT, clearColor);
			}
			else
			{
				GLenum format = GL_NONE;
				switch (spec.TextureFormat)
				{
				case FramebufferTextureFormat::RED_INTEGER:  format = GL_RED_INTEGER; break;
				case FramebufferTextureFormat::RG_INTEGER:   format = GL_RG_INTEGER;  break;
				case FramebufferTextureFormat::RGBA_INTEGER: format = GL_RGBA_INTEGER; break;
				}
				glClearTexImage(m_colorAttachments[attachmentIndex], 0, format, GL_INT, clearValue);
			}
		}

	private:
		uint32_t m_rendererID;
		FramebufferSpecification m_specification;

		std::vector<uint32_t> m_colorAttachments;
		uint32_t  m_depthAttachment;

		std::vector<FramebufferTextureSpecification> m_colorAttachmentSpecs;
		FramebufferTextureSpecification m_depthAttachmentSpec = FramebufferTextureFormat::None;
	};




	class OpenGLUniformBuffer : public UniformBuffer
	{
	public:
		OpenGLUniformBuffer(uint32_t size, uint32_t bindingPoint)
		{
			glCreateBuffers(1, &m_rendererID);
			glNamedBufferData(m_rendererID, size, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_rendererID);
		}

		virtual ~OpenGLUniformBuffer()
		{
			glDeleteBuffers(1, &m_rendererID);
		}

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) override
		{
			glNamedBufferSubData(m_rendererID, offset, size, data);
		}

		virtual void bind(uint32_t bindingPoint) const override
		{
			glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_rendererID);
		}

	private:
		uint32_t m_rendererID = 0;
	};




	class OpenGLShaderStorageBuffer : public ShaderStorageBuffer
	{
	public:
		OpenGLShaderStorageBuffer(uint32_t size, ShaderStorageBufferUsage usage)
			: m_size(size), m_usage(usage)
		{
			glCreateBuffers(1, &m_rendererID);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_rendererID);
			glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, OpenGLUsage(usage));
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}

		virtual ~OpenGLShaderStorageBuffer()
		{
			glDeleteBuffers(1, &m_rendererID);
		}

		virtual void bind(uint32_t bindingPoint) const override
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_rendererID);
		}

		virtual void unbind() const override
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) override
		{
			if (size + offset > m_size)
			{
				RONG_CORE_ERROR("ShaderStorageBuffer overflow! Trying to set {0} bytes at offset {1}, but capacity is {2}", size, offset, m_size);
				return;
			}

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_rendererID);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}

		virtual uint32_t getSize() const override { return m_size; }

		virtual void resize(uint32_t size) override
		{
			m_size = size;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_rendererID);

			glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, OpenGLUsage(m_usage));
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}

	private:
		uint32_t m_rendererID;
		uint32_t m_size;
		ShaderStorageBufferUsage m_usage;
	};




	class OpenGLPipelineState : public PipelineState
	{
	public:
		OpenGLPipelineState(const PipelineStateDesc& desc)
			: m_desc(desc)
		{
		}

		virtual ~OpenGLPipelineState() = default;

		virtual void bind() const override
		{
			// Blend
			if (m_desc.Blend.Enabled)
			{
				glEnable(GL_BLEND);
				glBlendFunc(ToGLBlendFactor(m_desc.Blend.SrcFactor),
				            ToGLBlendFactor(m_desc.Blend.DstFactor));
			}
			else
			{
				glDisable(GL_BLEND);
			}

			// Depth
			if (m_desc.Depth.TestEnabled)
			{
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(ToGLDepthFunc(m_desc.Depth.CompareFunc));
			}
			else
			{
				glDisable(GL_DEPTH_TEST);
			}
			glDepthMask(m_desc.Depth.WriteEnabled ? GL_TRUE : GL_FALSE);

			// Rasterizer - Culling
			if (m_desc.Rasterizer.Culling != CullFace::None)
			{
				glEnable(GL_CULL_FACE);
				switch (m_desc.Rasterizer.Culling)
				{
				case CullFace::Front:        glCullFace(GL_FRONT); break;
				case CullFace::Back:         glCullFace(GL_BACK); break;
				case CullFace::FrontAndBack: glCullFace(GL_FRONT_AND_BACK); break;
				default: break;
				}
			}
			else
			{
				glDisable(GL_CULL_FACE);
			}

			// Rasterizer - Fill mode
			switch (m_desc.Rasterizer.FillMode)
			{
			case PolygonMode::Fill:  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
			case PolygonMode::Line:  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
			case PolygonMode::Point: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
			}

			// Rasterizer - Line width
			glLineWidth(m_desc.Rasterizer.LineWidth);

			// Rasterizer - Polygon offset
			if (m_desc.Rasterizer.PolygonOffsetEnabled)
			{
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(m_desc.Rasterizer.PolygonOffsetFactor,
				                m_desc.Rasterizer.PolygonOffsetUnits);
			}
			else
			{
				glDisable(GL_POLYGON_OFFSET_FILL);
			}

			// Shader
			if (m_desc.Shader)
				m_desc.Shader->bind();
		}

		virtual void unbind() const override
		{
			if (m_desc.Shader)
				m_desc.Shader->unbind();

			// 恢复默认状态
			glDisable(GL_POLYGON_OFFSET_FILL);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDepthMask(GL_TRUE);
			glLineWidth(1.0f);
		}

		virtual const PipelineStateDesc& getDesc() const override { return m_desc; }

	private:
		PipelineStateDesc m_desc;
	};




	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void init() override
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glEnable(GL_DEPTH_TEST);
		}

		virtual void setColor(const glm::vec4& color) override
		{
			glClearColor(color.r, color.g, color.b, color.a);
		}

		virtual void setViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override
		{
			glViewport(x, y, width, height);
		}

		virtual void clear() override
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

		virtual void drawIndexed(const Ref<VertexArray>& vertexArray, uint32_t count = 0) override
		{
			glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
		}

		virtual void drawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override
		{
			glDrawArrays(GL_LINES, 0, vertexCount);
		}

		virtual void drawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount) override
		{
			glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr, instanceCount);
		}

		virtual void drawArrays(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override
		{
			glDrawArrays(GL_TRIANGLES, 0, vertexCount);
		}

		virtual void setDepthTest(bool enabled) override
		{
			if (enabled)
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);
		}

		virtual void setDepthWrite(bool enabled) override
		{
			glDepthMask(enabled ? GL_TRUE : GL_FALSE);
		}

		virtual void setBlend(bool enabled) override
		{
			if (enabled)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else
			{
				glDisable(GL_BLEND);
			}
		}

		virtual void setCullFace(bool enabled, bool backFace = true) override
		{
			if (enabled)
			{
				glEnable(GL_CULL_FACE);
				glCullFace(backFace ? GL_BACK : GL_FRONT);
			}
			else
			{
				glDisable(GL_CULL_FACE);
			}
		}

		virtual void setWireframe(bool enabled) override
		{
			glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
		}
	};




	class OpenGLComputeShader : public ComputeShader
	{
	public:
		OpenGLComputeShader(const std::string& filepath)
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

		virtual ~OpenGLComputeShader()
		{
			glDeleteProgram(m_rendererID);
		}

		virtual void bind() const override
		{
			glUseProgram(m_rendererID);
		}

		virtual void unbind() const override
		{
			glUseProgram(0);
		}

		virtual void dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ) override
		{
			// 1. 启动计算
			glDispatchCompute(groupX, groupY, groupZ);

			// 2. 内存屏障：确保计算着色器写入图像/SSBO完成后，其他操作才能读取
			// GL_ALL_BARRIER_BITS 比较保险，也可以根据具体用途使用 GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

		virtual void setInt(const std::string& name, int value) override { uploadUniformInt(name, value); }
		virtual void setFloat(const std::string& name, float value) override { uploadUniformFloat(name, value); }
		virtual void setFloat2(const std::string& name, const glm::vec2& value) override { uploadUniformFloat2(name, value); }
		virtual void setFloat3(const std::string& name, const glm::vec3& value) override { uploadUniformFloat3(name, value); }
		virtual void setFloat4(const std::string& name, const glm::vec4& value) override { uploadUniformFloat4(name, value); }
		virtual void setMat4(const std::string& name, const glm::mat4& value) override { uploadUniformMat4(name, value); }
		virtual void setIntArray(const std::string& name, int* value, uint32_t count) override { uploadUniformIntArray(name, value, count); }

		virtual const std::string& getName() const override { return m_name; }

		void uploadUniformInt(const std::string& name, int value)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			if (location != -1) glUniform1i(location, value);
		}

		void uploadUniformFloat(const std::string& name, float value)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			if (location != -1) glUniform1f(location, value);
		}

		void uploadUniformFloat2(const std::string& name, const glm::vec2& value)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			if (location != -1) glUniform2f(location, value.x, value.y);
		}

		void uploadUniformFloat3(const std::string& name, const glm::vec3& value)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			if (location != -1) glUniform3f(location, value.x, value.y, value.z);
		}

		void uploadUniformFloat4(const std::string& name, const glm::vec4& value)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			if (location != -1) glUniform4f(location, value.x, value.y, value.z, value.w);
		}

		void uploadUniformMat4(const std::string& name, const glm::mat4& matrix)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			if (location != -1) glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
		}

		void uploadUniformIntArray(const std::string& name, int* values, uint32_t count)
		{
			GLint location = glGetUniformLocation(m_rendererID, name.c_str());
			if (location != -1) glUniform1iv(location, count, values);
		}

	private:
		std::string readFile(const std::string& filepath)
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

		void compile(const std::string& source)
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

	private:
		uint32_t m_rendererID;
		std::string m_name;
	};


	class OpenGLContext:public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle)
			:m_windowHandle(windowHandle)
		{
			RONG_CORE_ASSERT(m_windowHandle, "window handle is null");
		}

		virtual void init() override {
			glfwMakeContextCurrent(m_windowHandle);
			int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

			RONG_CORE_ASSERT(status,"Failed to initialize Glad!");

			RONG_CORE_INFO("OpenGL Info:");
			RONG_CORE_INFO("  Vendor: {0}", (const char*)glGetString(GL_VENDOR));
			RONG_CORE_INFO("  Renderer: {0}",(const char*) glGetString(GL_RENDERER));
			RONG_CORE_INFO("  Version: {0}",(const char*)glGetString(GL_VERSION));
		}

		virtual void swapBuffers() override {
			glfwSwapBuffers(m_windowHandle);
		}
	private:
		GLFWwindow* m_windowHandle;
	};


// =============================================================
// 工厂函数：接口类的 create() 实现（依赖 OpenGL 具体类，
// 故在 :OpenGL 分区中类外定义，声明位于 :Interfaces 的接口类上）
// =============================================================

// ----------------------VertexBuffer-----------------------------
	Ref<VertexBuffer> VertexBuffer::create(uint32_t size)
	{

		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLVertexBuffer>(size);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::create(float* vertex, uint32_t size)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLVertexBuffer>(vertex, size);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}

	////////////////////////IndexBuffer/////////////////////////////

	Ref<IndexBuffer> IndexBuffer::create(uint32_t count)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLIndexBuffer>(count);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::create(uint32_t* indices, uint32_t count)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLIndexBuffer>(indices, count);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	} 

// ----------------------Shader-----------------------------
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
			return CreateRef<OpenGLShader>(filepath);
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
			return CreateRef<OpenGLShader>(name,vertexSrc,fragmentSrc);
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}

// ----------------------Texture2D-----------------------------
	Ref<Texture2D> Texture2D::create(uint32_t width, uint32_t height)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLTexture2D>(width,height);
		}
		}
		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
	Ref<Texture2D> Rongine::Texture2D::create(const std::string& path)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLTexture2D>(path);
		}
		}
		RONG_CORE_ASSERT(false,"Unknown RendererAPI!");
		return nullptr;
	}


	Ref<Texture2D> Texture2D::create(const TextureSpecification& specification)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLTexture2D>(specification);
		}
		}
		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

// ----------------------VertexArray-----------------------------
	Ref<VertexArray> Rongine::VertexArray::create()
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
			return CreateRef<OpenGLVertexArray>();
		}
		}
		RONG_CORE_ASSERT(false, "UnKnown RendererAPI!");
		return nullptr;
	}

// ----------------------Framebuffer-----------------------------
	Ref<Framebuffer> Framebuffer::create(const FramebufferSpecification& spec)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None: {
			RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL: {
			return CreateRef<OpenGLFramebuffer>(spec);
		}
		}
		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

// ----------------------UniformBuffer-----------------------------
	Ref<UniformBuffer> UniformBuffer::create(uint32_t size, uint32_t bindingPoint)
	{
		switch (RendererAPI::getAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLUniformBuffer>(size, bindingPoint);
		}
		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

// ----------------------ShaderStorageBuffer-----------------------------
	Ref<ShaderStorageBuffer> ShaderStorageBuffer::create(uint32_t size, ShaderStorageBufferUsage usage)
	{
		switch (Renderer::getAPI())
		{
		case RendererAPI::API::None:    RONG_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShaderStorageBuffer>(size, usage);
		}

		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

// ----------------------PipelineState-----------------------------
	Ref<PipelineState> PipelineState::create(const PipelineStateDesc& desc)
	{
		switch (RendererAPI::getAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLPipelineState>(desc);
		}
		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

// 后端静态成员：渲染命令的 API 实例
	RendererAPI* RenderCommand::s_rendererAPI = new OpenGLRendererAPI;

// Renderer::submit 依赖 OpenGLShader 的下行转换，同样在本分区定义
	void Renderer::submit(const Ref<Shader>& shader,const Ref<VertexArray>& vertexArray,const glm::mat4& transform)
	{
		shader->bind();
		std::dynamic_pointer_cast<OpenGLShader>(shader)->uploadUniformMat4("u_ViewProjection", s_sceneData->viewProjectionMatrix);
		std::dynamic_pointer_cast<OpenGLShader>(shader)->uploadUniformMat4("u_Transform", transform);

		vertexArray->bind();
		RenderCommand::drawIndexed(vertexArray);
	}

// ----------------------ComputeShader-----------------------------
	Ref<ComputeShader> ComputeShader::create(const std::string& filepath)
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
			return CreateRef<OpenGLComputeShader>(filepath);
		}
		}
		RONG_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}


}
