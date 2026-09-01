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
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <execution>
#include <limits>
#include <numeric>
#include <utility>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <glad/glad.h>
#include <TopoDS_Edge.hxx>

#include "Rongine/Core/RongineMacros.h"

export module Rongine.Renderer:Interfaces;

import Rongine.Core;
import Rongine.Log;
import Rongine.LayerStack;
import Rongine.Events;
import Rongine.RendererData;
import Rongine.RendererCameras;
import Rongine.RenderThread;
import Rongine.Scene;
import Rongine.SceneData;
import Rongine.Commands;
import Rongine.BVH;

export namespace Rongine {

	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() {};

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual void setData(const void* data, uint32_t size) = 0;
		virtual const BufferLayout& getLayout() const = 0;
		virtual void setLayout(const BufferLayout& layout) = 0;

		virtual uint32_t getSize() const = 0;

		static Ref<VertexBuffer> create(uint32_t size);
		static Ref<VertexBuffer> create(float* vertex, uint32_t size);
	};

	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() {};

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual uint32_t getCount() const = 0;

		static Ref<IndexBuffer> create(uint32_t count);
		static Ref<IndexBuffer> create(uint32_t* indices, uint32_t count);
	};

	class Shader
	{
	public:
		virtual ~Shader() {}

		virtual void bind() const=0;
		virtual void unbind() const=0;

		virtual void setInt(const std::string& name, int value) = 0;
		virtual void setFloat(const std::string& name, float value) = 0;
		virtual void setFloat3(const std::string& name, const glm::vec3& value) = 0;
		virtual void setFloat4(const std::string& name, const glm::vec4& value) = 0;
		virtual void setMat4(const std::string& name, const glm::mat4& value) = 0;
		virtual void setIntArray(const std::string& name, int* value,uint32_t count) = 0;

		virtual const std::string& getName()const =0;

		static Ref<Shader> create(const std::string& filepath);
		static Ref<Shader> create(const std::string& name,const std::string& vertexSrc, const std::string& fragmentSrc);
	};

	class ShaderLibray
	{
	public:
		void add(const std::string& name, const Ref<Shader>& shader)
		{
			RONG_CORE_ASSERT(!exists(name), "Shader already exists!");
			m_shaders[name] = shader;
		}

		void add(const Ref<Shader>& shader)
		{
			const std::string& name = shader->getName();
			add(name, shader);
		}

		Ref<Shader> load(const std::string& filepath)
		{
			Ref<Shader> shader = Shader::create(filepath);
			auto& name = shader->getName();
			add(shader);
			return shader;
		}

		Ref<Shader> load(const std::string& name, const std::string& filepath)
		{
			Ref<Shader> shader = Shader::create(filepath);
			add(name, shader);
			return shader;
		}

		Ref<Shader> get(const std::string& name)
		{
			RONG_CORE_ASSERT(exists(name), "Shader is not exists!");
			return m_shaders[name];
		}

		bool exists(const std::string& name)
		{
			return m_shaders.find(name) != m_shaders.end();
		}

	private:
		std::unordered_map<std::string, Ref<Shader>> m_shaders;
	};

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t getWidth() const = 0;
		virtual uint32_t getHeight() const = 0;
		virtual uint32_t getRendererID() const = 0;

		virtual void bind(uint32_t slot = 0) = 0;

		virtual void setData(void* data, uint32_t size) = 0;

		virtual bool operator==(const Texture& texture) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> create(uint32_t width, uint32_t height);
		static Ref<Texture2D> create(const std::string& path);

		static Ref<Texture2D> create(const TextureSpecification& specification);
	};

	class VertexArray
	{
	public:
		virtual ~VertexArray() {}

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual void addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
		virtual void setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) = 0;

		virtual const std::vector<Ref<VertexBuffer>>& getVertexBuffers()const = 0;
		virtual const Ref<IndexBuffer>& getIndexBuffer() const = 0;

		static Ref<VertexArray> create();
	};

	// 网格组件：实体的渲染数据（原来定义在 Rongine.Components，现归属渲染器模块；
	// 实体系统通过 Rongine.Components 的 export import Rongine.Renderer 继续可见）
	struct MeshComponent
	{
		Ref<VertexArray> VA;
		AABB BoundingBox;

		Ref<VertexArray> EdgeVA;

		std::vector<LineVertex> LocalLines;
		std::vector<CubeVertex> LocalVertices;
		std::vector<uint32_t> LocalIndices;

		std::map<int, TopoDS_Edge> m_IDToEdgeMap;

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
		MeshComponent(const Ref<VertexArray>& va) : VA(va) {}
		MeshComponent(const Ref<VertexArray>& va, const std::vector<CubeVertex>& verts)
			: VA(va), LocalVertices(verts) {
		}
		MeshComponent(const Ref<VertexArray>& va, const std::vector<CubeVertex>& verts, const std::vector<uint32_t> indices)
			: VA(va), LocalVertices(verts), LocalIndices(indices) {
		}
	};

	class RendererAPI
	{
	public:
		enum class API {
			None = 0, OpenGL = 1
		};
	public:
		virtual ~RendererAPI() = default;

		virtual void init() = 0;
		virtual void setColor(const glm::vec4& color) = 0;
		virtual void setViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void clear() = 0;

		virtual void drawIndexed(const Ref<VertexArray>& vertexArray, uint32_t count = 0) = 0;
		virtual void drawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) = 0;

		// 现代渲染命令
		virtual void drawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount) = 0;
		virtual void drawArrays(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) = 0;

		// GPU 状态管理
		virtual void setDepthTest(bool enabled) = 0;
		virtual void setDepthWrite(bool enabled) = 0;
		virtual void setBlend(bool enabled) = 0;
		virtual void setCullFace(bool enabled, bool backFace = true) = 0;
		virtual void setWireframe(bool enabled) = 0;

		inline static API getAPI() { return s_api; };

	private:
		static API s_api;
	};

	class RenderCommand
	{
	public:
		inline static void init()
		{
			s_rendererAPI->init();
		}

		inline static void setColor(const glm::vec4& color)
		{
			s_rendererAPI->setColor(color);
		}

		inline static void setViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_rendererAPI->setViewPort(x, y, width, height);
		}

		inline static void clear()
		{
			s_rendererAPI->clear();
		}

		inline static void drawIndexed(const Ref<VertexArray>& vertexArray)
		{
			s_rendererAPI->drawIndexed(vertexArray);
		}

		inline static void drawIndexed(const Ref<VertexArray>& vertexArray, uint32_t count)
		{
			s_rendererAPI->drawIndexed(vertexArray, count);
		}

		inline static void drawLines(const Ref<VertexArray>& vertexArray, uint32_t count)
		{
			s_rendererAPI->drawLines(vertexArray, count);
		}

		inline static void drawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount)
		{
			s_rendererAPI->drawIndexedInstanced(vertexArray, indexCount, instanceCount);
		}

		inline static void drawArrays(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
		{
			s_rendererAPI->drawArrays(vertexArray, vertexCount);
		}

		inline static void setDepthTest(bool enabled)
		{
			s_rendererAPI->setDepthTest(enabled);
		}

		inline static void setDepthWrite(bool enabled)
		{
			s_rendererAPI->setDepthWrite(enabled);
		}

		inline static void setBlend(bool enabled)
		{
			s_rendererAPI->setBlend(enabled);
		}

		inline static void setCullFace(bool enabled, bool backFace = true)
		{
			s_rendererAPI->setCullFace(enabled, backFace);
		}

		inline static void setWireframe(bool enabled)
		{
			s_rendererAPI->setWireframe(enabled);
		}

	private:
		static RendererAPI* s_rendererAPI;
	};

	class UniformBuffer
	{
	public:
		virtual ~UniformBuffer() = default;

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void bind(uint32_t bindingPoint) const = 0;

		static Ref<UniformBuffer> create(uint32_t size, uint32_t bindingPoint);
	};

	class ShaderStorageBuffer
	{
	public:
		virtual ~ShaderStorageBuffer() = default;

		virtual void bind(uint32_t bindingPoint) const = 0;
		virtual void unbind() const = 0;

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		virtual uint32_t getSize() const = 0;

		virtual void resize(uint32_t size) = 0;

		static Ref<ShaderStorageBuffer> create(uint32_t size, ShaderStorageBufferUsage usage = ShaderStorageBufferUsage::DynamicDraw);
	};

	struct PipelineStateDesc
	{
		Ref<Shader> Shader;
		BlendState Blend;
		DepthState Depth;
		RasterState Rasterizer;
	};

	class PipelineState
	{
	public:
		virtual ~PipelineState() = default;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual const PipelineStateDesc& getDesc() const = 0;

		static Ref<PipelineState> create(const PipelineStateDesc& desc);
	};

	class Framebuffer
	{
	public:
		virtual void bind() = 0;
		virtual void unbind() = 0;

		virtual const FramebufferSpecification& getSpecification() const = 0;

		virtual void resize(uint32_t width, uint32_t height) = 0;

		virtual int readPixel(uint32_t attachmentIndex, int x, int y) = 0;
		virtual std::pair<int, int> readPixelRG(uint32_t attachmentIndex, int x, int y) = 0;
		virtual glm::ivec4 readPixelID(uint32_t attachmentIndex, int x, int y) = 0;
		virtual void clearAttachment(uint32_t attachmentIndex, int value) = 0;
		virtual uint32_t getColorAttachmentRendererID(uint32_t index = 0) const = 0;

		static Ref<Framebuffer> create(const FramebufferSpecification& spec);
	};

	struct RenderPassSpec
	{
		std::string Name = "Unnamed Pass";
		Ref<Framebuffer> TargetFramebuffer = nullptr; // nullptr = 默认帧缓冲
		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		bool ClearColorBuffer = true;
		bool ClearDepthBuffer = true;
		int ClearAttachmentIndex = -1;    // 需要额外清除的附件索引 (-1 = 不清除)
		int ClearAttachmentValue = -1;    // 清除附件的填充值
	};

	class RenderPass
	{
	public:
		using ExecuteFn = std::function<void()>;

		RenderPass(const RenderPassSpec& spec)
			: m_spec(spec)
		{
		}

		~RenderPass() = default;

		void begin()
		{
			if (m_spec.TargetFramebuffer)
				m_spec.TargetFramebuffer->bind();

			if (m_spec.ClearColorBuffer)
				RenderCommand::setColor(m_spec.ClearColor);

			if (m_spec.ClearColorBuffer || m_spec.ClearDepthBuffer)
				RenderCommand::clear();

			if (m_spec.ClearAttachmentIndex >= 0 && m_spec.TargetFramebuffer)
				m_spec.TargetFramebuffer->clearAttachment(m_spec.ClearAttachmentIndex, m_spec.ClearAttachmentValue);
		}

		void end()
		{
			if (m_spec.TargetFramebuffer)
				m_spec.TargetFramebuffer->unbind();
		}

		const std::string& getName() const { return m_spec.Name; }
		const RenderPassSpec& getSpec() const { return m_spec; }
		Ref<Framebuffer> getTargetFramebuffer() const { return m_spec.TargetFramebuffer; }

	private:
		RenderPassSpec m_spec;
	};

	class RenderGraph
	{
	public:
		using PassExecuteFn = std::function<void(RenderPass&)>;

		struct PassNode
		{
			Ref<RenderPass> Pass;
			PassExecuteFn Execute;
			bool Enabled = true;
		};

		RenderGraph() = default;
		~RenderGraph() = default;

		void addPass(const RenderPassSpec& spec, PassExecuteFn executeFn)
		{
			PassNode node;
			node.Pass = CreateRef<RenderPass>(spec);
			node.Execute = std::move(executeFn);
			node.Enabled = true;
			m_passes.push_back(std::move(node));
		}

		void setPassEnabled(const std::string& name, bool enabled)
		{
			for (auto& node : m_passes)
			{
				if (node.Pass->getName() == name)
				{
					node.Enabled = enabled;
					return;
				}
			}
			RONG_CORE_WARN("RenderGraph: Pass '{0}' not found", name);
		}

		void execute()
		{
			for (auto& node : m_passes)
			{
				if (!node.Enabled)
					continue;

				node.Pass->begin();
				node.Execute(*node.Pass);
				node.Pass->end();
			}
		}

		void clear()
		{
			m_passes.clear();
		}

		const std::vector<PassNode>& getPasses() const { return m_passes; }

	private:
		std::vector<PassNode> m_passes;
	};

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

	class Material
	{
	public:
		Material(const Ref<Shader>& shader)
			: m_shader(shader)
		{
			m_ubo = UniformBuffer::create(sizeof(MaterialData), 2);
		}

		~Material() = default;

		void bind(uint32_t uboBindingPoint = 2) const
		{
			if (m_dirty)
			{
				uploadData();
				m_dirty = false;
			}

			if (m_shader)
				m_shader->bind();

			m_ubo->bind(uboBindingPoint);

			uint32_t slot = 0;
			for (auto& [name, texture] : m_textures)
			{
				if (texture)
					texture->bind(slot++);
			}
		}

		void setAlbedo(const glm::vec3& albedo) { m_data.Albedo = albedo; m_dirty = true; }
		void setRoughness(float roughness) { m_data.Roughness = roughness; m_dirty = true; }
		void setMetallic(float metallic) { m_data.Metallic = metallic; m_dirty = true; }
		void setEmission(float emission) { m_data.Emission = emission; m_dirty = true; }

		void setTexture(const std::string& slot, const Ref<Texture2D>& texture)
		{
			m_textures[slot] = texture;
		}

		Ref<Texture2D> getTexture(const std::string& slot) const
		{
			auto it = m_textures.find(slot);
			return (it != m_textures.end()) ? it->second : nullptr;
		}

		Ref<Shader> getShader() const { return m_shader; }
		const MaterialData& getData() const { return m_data; }

		static Ref<Material> create(const Ref<Shader>& shader)
		{
			return CreateRef<Material>(shader);
		}

	private:
		void uploadData() const
		{
			m_ubo->setData(&m_data, sizeof(MaterialData));
		}

		Ref<Shader> m_shader;
		MaterialData m_data;
		mutable Ref<UniformBuffer> m_ubo;
		mutable bool m_dirty = true;

		std::unordered_map<std::string, Ref<Texture2D>> m_textures;
	};

	class MaterialInstance
	{
	public:
		MaterialInstance(const Ref<Material>& baseMaterial)
			: m_baseMaterial(baseMaterial), m_overrideData(baseMaterial->getData())
		{
			m_ubo = UniformBuffer::create(sizeof(MaterialData), 2);
		}

		~MaterialInstance() = default;

		void bind(uint32_t uboBindingPoint = 2) const
		{
			if (m_dirty)
			{
				uploadData();
				m_dirty = false;
			}

			auto shader = m_baseMaterial->getShader();
			if (shader)
				shader->bind();

			m_ubo->bind(uboBindingPoint);

			uint32_t slot = 0;
			for (auto& [name, texture] : m_textureOverrides)
			{
				if (texture)
					texture->bind(slot++);
			}
		}

		void setAlbedo(const glm::vec3& albedo) { m_overrideData.Albedo = albedo; m_dirty = true; }
		void setRoughness(float roughness) { m_overrideData.Roughness = roughness; m_dirty = true; }
		void setMetallic(float metallic) { m_overrideData.Metallic = metallic; m_dirty = true; }

		void setTexture(const std::string& slot, const Ref<Texture2D>& texture)
		{
			m_textureOverrides[slot] = texture;
		}

		Ref<Material> getBaseMaterial() const { return m_baseMaterial; }

		static Ref<MaterialInstance> create(const Ref<Material>& baseMaterial)
		{
			return CreateRef<MaterialInstance>(baseMaterial);
		}

	private:
		void uploadData() const
		{
			m_ubo->setData(&m_overrideData, sizeof(MaterialData));
		}

		Ref<Material> m_baseMaterial;
		MaterialData m_overrideData;
		mutable Ref<UniformBuffer> m_ubo;
		mutable bool m_dirty = true;

		std::unordered_map<std::string, Ref<Texture2D>> m_textureOverrides;
	};

	class Renderer
	{
	public:
		static void init();

		static void beginScene(const OrthographicCamera& camera);
		static void endScene();

		static void onWindowResize(uint32_t width, uint32_t height);

		static void submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

		inline static RendererAPI::API getAPI() { return RendererAPI::getAPI(); }
	private:
		struct SceneData 
		{
			glm::mat4 viewProjectionMatrix;
		};

		static SceneData* s_sceneData;
	};

	class Renderer2D
	{
	public:
		static void init();
		static void shutdown();

		static void beginScene(const OrthographicCamera& camera);
		static void beginScene(const PerspectiveCamera& camera);
		static void endScene();

		static void flush();

		static void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);

		static void drawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void drawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);

		static void drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		struct Statistics
		{
			uint32_t DrawCalls;
			uint32_t QuadCounts;

			uint32_t getTotalVertexCounts() { return QuadCounts * 4; }
			uint32_t getTotalIndexCounts() { return QuadCounts * 6; }
		};

		static Statistics getStatistics();
		static void resetStatistics();
	private:
		static void flushAndReset();
	};

	class Renderer3D
	{
	public:
		static void init();
		static void shutdown();

		static void setSelection(int entityID, int faceID);

		static void setHover(int entityID, int faceID, int edgeID);
		static int getHoveredEntityID();

		static void setSpectralRendering(bool enable);
		static bool isSpectralRendering();

		static void beginScene(const PerspectiveCamera& camera);
		static void endScene();
		static void flush();

		static void beginLines(const PerspectiveCamera& camera);
		static void endLines();

		// --- 基础绘制 ---
		static void drawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color);
		static void drawGrid(const glm::mat4& transform, float size, int steps);

		static void drawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color);
		static void drawCube(const glm::vec3& position, const glm::vec3& size, const Ref<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

		// --- 旋转绘制  ---
		static void drawRotatedCube(const glm::vec3& position, const glm::vec3& size, float rotation, const glm::vec3& axis, const glm::vec4& color);
		static void drawRotatedCube(const glm::vec3& position, const glm::vec3& size, float rotation, const glm::vec3& axis, const Ref<Texture2D>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void drawModel(const Ref<VertexArray>& va, const glm::mat4& transform = glm::mat4(1.0f),int entityID=-1, const MaterialComponent* material=nullptr);
		static void drawModel(Entity& en, const glm::mat4& transform = glm::mat4(1.0f), int entityID = -1);
		static void drawEdges(const Ref<VertexArray>& va, const glm::mat4& transform, const glm::vec4& color, int entityID, int selectedEdgeID = -1);

		// --- 统计信息 ---
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t CubeCount = 0;
			uint32_t GetTotalVertexCount() { return CubeCount * 24; } // 24 vertices per cube
			uint32_t GetTotalIndexCount() { return CubeCount * 36; }  // 36 indices per cube
		};

		// cs光追
		static void SetSpectralRange(float start, float end);
		static void UploadSceneDataToGPU(Scene* scene);
		static void RenderComputeFrame(const PerspectiveCamera& camera,float time,bool resetAccumulation=false);
		static Ref<Texture2D> GetComputeOutputTexture();
		static void ResizeComputeOutput(uint32_t width, uint32_t height);

		static void BuildAccelerationStructures(Scene* scene);

		static void setAccelType(const AccelType& acceltype);
		static AccelType getAccelType();

		static int getBVHNodeCount();
		static int getOctreeNodeCount();

		static Statistics getStatistics();
		static void resetStatistics();

	private:
		static void flushAndReset();
	};

	class SpectralRenderer
	{
	public:
		SpectralRenderer();
		~SpectralRenderer() = default;

		void OnResize(uint32_t width, uint32_t height);

		void Render(Scene& scene, const PerspectiveCamera& camera);

		uint32_t GetFinalTextureID() const { return m_FinalTexture->getRendererID(); }

	private:
		// 一个简单的光线结构体
		struct Ray
		{
			glm::vec3 Origin;
			glm::vec3 Direction;
		};
		struct HitPayload
		{
			float HitDistance = -1.0f; // < 0 表示没打中
			glm::vec3 WorldPosition;
			glm::vec3 WorldNormal;
			int EntityID = -1;

			glm::vec3 Albedo = { 0.8f, 0.8f, 0.8f };
			float Roughness = 0.5f;
			float Metallic = 0.0f;
		};

		//渲染每一个像素
		glm::vec4 PerPixel(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const PerspectiveCamera& camera, Scene& scene);

		HitPayload TraceRay(const Ray& ray, Scene& scene);

		//光线-三角形求交辅助函数
		bool RayTriangleIntersect(const Ray& ray,
			const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
			float& t, glm::vec3& normal);

	private:
		Ref<Texture2D> m_FinalTexture;
		std::vector<uint32_t> m_ImageData; // CPU 端的像素 Buffer (RGBA8)

		uint32_t m_Width = 0, m_Height = 0;
	};

} // export namespace Rongine
// =============================================================
// 实现（非导出）：Renderer2D / Renderer3D / Renderer 类外定义与私有数据
// =============================================================
namespace Rongine {

	// 辅助工具：把 float (0.0-1.0) 转成 RGBA8 (0-255) 并打包成 uint32
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f);
		uint8_t g = (uint8_t)(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f);
		uint8_t b = (uint8_t)(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f);
		uint8_t a = (uint8_t)(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f);
		return (a << 24) | (b << 16) | (g << 8) | r;
	}
} // namespace Rongine

namespace Rongine {

// =============================================================
// 非导出：渲染器私有类型与辅助函数（原 impl.cpp 内容）
// =============================================================

// =============================================================
// 非导出：Renderer2D 批渲染数据
// =============================================================

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;
	};

	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 10000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32;

		Ref<VertexArray> QuadVertexArray;
		Ref<IndexBuffer> QuadIndexBuffer;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> TextureShader;
		Ref<Texture2D> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotsIndex = 1;//0=whiteTexture;

		glm::vec4 QuadVertexPositions[4];

		Renderer2D::Statistics Stats;
	};
	

	static Renderer2DData s_data ;

	void Renderer2D::init()
	{
		s_data.QuadVertexArray = VertexArray::create();

		s_data.QuadVertexBuffer = VertexBuffer::create(s_data.MaxVertices*sizeof(QuadVertex));

		s_data.QuadVertexBufferBase = new QuadVertex[s_data.MaxVertices];

		BufferLayout layout = {
			{ShaderDataType::Float3,"a_Position"},
			{ShaderDataType::Float4,"a_Color"},
			{ShaderDataType::Float2,"a_TexCoord"},
			{ShaderDataType::Float,"a_TexIndex"},
			{ShaderDataType::Float,"a_TilingFactor"}
		};
		s_data.QuadVertexBuffer->setLayout(layout);
		s_data.QuadVertexArray->addVertexBuffer(s_data.QuadVertexBuffer);


		uint32_t squareIndices[6] = { 0,1,2,2,3,0 };

		uint32_t* quadIndices = new uint32_t[s_data.MaxIndices];
		uint32_t offset = 0;

		for (uint32_t i = 0; i < s_data.MaxIndices; i+=6)
		{
			quadIndices[i] = offset;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		s_data.QuadIndexBuffer = IndexBuffer::create(quadIndices, s_data.MaxIndices);
		s_data.QuadVertexArray->setIndexBuffer(s_data.QuadIndexBuffer);

		delete[] quadIndices;

		s_data.TextureShader = Shader::create("assets/shaders/Texture.glsl");

		s_data.TextureShader->bind();
		s_data.TextureShader->setInt("u_Texture", 0);

		s_data.WhiteTexture = Texture2D::create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_data.WhiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[s_data.MaxTextureSlots];
		for (int i = 0; i < s_data.MaxTextureSlots; i++)
			samplers[i] = i;

		s_data.TextureShader->setIntArray("u_Textures", samplers,s_data.MaxTextureSlots);
		s_data.TextureShader->bind();

		s_data.TextureSlots[0] = s_data.WhiteTexture;

		s_data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_data.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		s_data.QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
		s_data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };
	}

	void Renderer2D::shutdown()
	{
		delete[] s_data.QuadVertexBufferBase;
	}

	void Renderer2D::beginScene(const OrthographicCamera& camera)
	{
		s_data.TextureShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

		s_data.QuadIndexCount = 0;
		s_data.QuadVertexBufferPtr = s_data.QuadVertexBufferBase;
		s_data.TextureSlotsIndex = 1;
	}

	void Renderer2D::beginScene(const PerspectiveCamera& camera)
	{
		s_data.TextureShader->bind();
		s_data.TextureShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

		s_data.QuadIndexCount = 0;
		s_data.QuadVertexBufferPtr = s_data.QuadVertexBufferBase;
		s_data.TextureSlotsIndex = 1;
	}

	void Renderer2D::endScene()
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_data.QuadVertexBufferPtr - (uint8_t*)s_data.QuadVertexBufferBase);
		s_data.QuadVertexBuffer->setData(s_data.QuadVertexBufferBase, dataSize);

		flush();
	}

	void Renderer2D::flush()
	{
		s_data.QuadVertexArray->bind();
		s_data.TextureShader->bind();

		for (uint32_t i = 0; i < s_data.TextureSlotsIndex; i++)
			s_data.TextureSlots[i]->bind(i);
		RenderCommand::drawIndexed(s_data.QuadVertexArray, s_data.QuadIndexCount);

		s_data.Stats.DrawCalls++;
	}


	void Renderer2D::flushAndReset()
	{
		Renderer2D::endScene();

		s_data.QuadIndexCount = 0;
		s_data.QuadVertexBufferPtr = s_data.QuadVertexBufferBase;
		s_data.TextureSlotsIndex = 1;
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		drawQuad(glm::vec3(position, 0.0f), size, color);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		if (s_data.QuadIndexCount >= Renderer2DData::MaxIndices)
			flushAndReset();

		const float texIndex = 0.0f;
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
			glm::scale(glm::mat4(1.0f), {size.x,size.y,1.0f});

		s_data.QuadVertexBufferPtr->Position = transform* s_data.QuadVertexPositions[0];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 2：右下
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[1];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 3：右上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[2];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 4：左上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[3];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		s_data.QuadIndexCount += 6;

		s_data.Stats.QuadCounts++;

		//s_data.TextureShader->bind();
		//s_data.TextureShader->setFloat4("u_Color", color);
		//	  
		//s_data.WhiteTexture->bind();

		//glm::mat4 transform = glm::translate(glm::mat4(1.0f),position)* glm::scale(glm::mat4(1.0f), glm::vec3(size.x,size.y,1.0f));
		//s_data.TextureShader->setMat4("u_Transform", transform);

		//s_data.QuadVertexArray->bind();
		//RenderCommand::drawIndexed(s_data.QuadVertexArray);
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4 & tintColor )
	{
		drawQuad(glm::vec3(position, 1.0f), size, texture,tilingFactor,tintColor);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4 & tintColor )
	{
		if(s_data.QuadIndexCount>=Renderer2DData::MaxIndices)
			flushAndReset();

		float texIndex = 0.0f;
		for (uint32_t i = 0; i < s_data.TextureSlotsIndex; i++)
		{
			if (*texture.get() == *s_data.TextureSlots[i].get()) {
				texIndex = (float)i;
				break;
			}
		}

		if (texIndex == 0.0f)
		{
			texIndex = (float)s_data.TextureSlotsIndex;
			s_data.TextureSlots[s_data.TextureSlotsIndex] = texture;
			s_data.TextureSlotsIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
			glm::scale(glm::mat4(1.0f), { size.x,size.y,1.0f });

		s_data.QuadVertexBufferPtr->Position = transform* s_data.QuadVertexPositions[0];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 2：右下
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[1];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 3：右上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[2];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 4：左上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[3];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		s_data.QuadIndexCount += 6;

		s_data.Stats.QuadCounts++;
	}

	void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		drawRotatedQuad(glm::vec3(position, 1.0f), size, rotation, color);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		if (s_data.QuadIndexCount >= Renderer2DData::MaxIndices)
			flushAndReset();

		const float texIndex = 0.0f;
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
			glm::rotate(glm::mat4(1.0f), glm::radians(rotation), {0.0f,0.0f,1.0f})*
			glm::scale(glm::mat4(1.0f), { size.x,size.y,1.0f });

		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[0];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 2：右下
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[1];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 3：右上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[2];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 4：左上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[3];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		s_data.QuadIndexCount += 6;

		s_data.Stats.QuadCounts++;
	}

	void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		drawRotatedQuad(glm::vec3(position, 1.0f), size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		if (s_data.QuadIndexCount >= Renderer2DData::MaxIndices)
			flushAndReset();

		float texIndex = 0.0f;
		for (uint32_t i = 0; i < s_data.TextureSlotsIndex; i++)
		{
			if (*texture.get() == *s_data.TextureSlots[i].get()) {
				texIndex = (float)i;
				break;
			}
		}

		if (texIndex == 0.0f)
		{
			texIndex = (float)s_data.TextureSlotsIndex;
			s_data.TextureSlots[s_data.TextureSlotsIndex] = texture;
			s_data.TextureSlotsIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
			glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f,0.0f,1.0f }) *
			glm::scale(glm::mat4(1.0f), { size.x,size.y,1.0f });

		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[0];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 2：右下
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[1];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 3：右上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[2];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 4：左上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[3];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		s_data.QuadIndexCount += 6;

		s_data.Stats.QuadCounts++;
	}

	Renderer2D::Statistics Renderer2D::getStatistics()
	{
		return s_data.Stats;
	}

	void Renderer2D::resetStatistics()
	{
		memset(&s_data.Stats, 0, sizeof(Statistics));
	}

// =============================================================
// 非导出：Renderer3D 批渲染/光追数据
// =============================================================
	struct Renderer3DData
	{
		static const uint32_t MaxCubes = 10000;
		static const uint32_t MaxVertices = MaxCubes * 24;
		static const uint32_t MaxIndices = MaxCubes * 36;
		static const uint32_t MaxTextureSlots = 32;
		static const uint32_t MaxLines = 10000;
		static const uint32_t MaxLineVertices = MaxLines * 2;

		int SelectedEntityID;
		int SelectedFaceID;

		int HoveredEntityID = -1;
		int HoveredFaceID = -1;
		int HoveredEdgeID = -1;

		Ref<VertexArray> CubeVA;
		Ref<VertexBuffer> CubeVB;
		Ref<Shader> TextureShader;
		Ref<Shader> LineShader;
		Ref<Texture2D> WhiteTexture;

		Ref<VertexArray> BatchLineVA;
		Ref<VertexBuffer> BatchLineVB;
		Ref<Shader> BatchLineShader;

		uint32_t CubeIndexCount = 0;
		CubeVertex* CubeVertexBufferBase = nullptr;
		CubeVertex* CubeVertexBufferPtr = nullptr;

		uint32_t BatchLineVertexCount = 0;
		BatchLineVertex* BatchLineVertexBufferBase = nullptr;
		BatchLineVertex* BatchLineVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;

		glm::vec4 CubeVertexPositions[24];
		glm::vec3 CubeVertexNormals[24];

		Renderer3D::Statistics Stats;

		glm::mat4 ViewProjection;

		// Camera UBO (binding = 0)
		struct CameraData
		{
			glm::mat4 ViewProjection;
			glm::vec3 ViewPos;
			float _pad0;
		};
		CameraData CameraUBOData;
		Ref<UniformBuffer> CameraUBO;

		// SSBO
		Ref<ShaderStorageBuffer> VerticesSSBO;
		Ref<ShaderStorageBuffer> TrianglesSSBO;
		Ref<ShaderStorageBuffer> MaterialsSSBO;

		// 临时缓存，避免每帧都重新分配 vector 内存
		std::vector<GPUVertex> HostVertices;
		std::vector<TriangleData> HostTriangles;
		std::vector<GPUMaterial> HostMaterials;

		Ref<Texture2D> ComputeOutputTexture; // 画布
		Ref<Texture2D> AccumulationTexture;  // 累加 
		Ref<ComputeShader> RaytracingShader; // 画笔
		Ref<ComputeShader> SpectralShader;   //光谱画笔

		uint32_t FrameIndex = 1;             // 帧数
		bool UseSpectralRendering = false;   // 光谱光追开关

		//光谱曲线
		std::vector<float> HostSpectralCurves;
		Ref<ShaderStorageBuffer> SpectralCurvesSSBO;

		float SpectralStart = 380.0f;
		float SpectralEnd = 780.0f;

		//加速结构
		AccelType CurrentAccelType = AccelType::None;

		// CPU 端缓存
		std::vector<GPUBVHNode> BVHNodes;
		std::vector<GPUOctreeNode> OctreeNodes;
		std::vector<uint32_t> SortedTriangleIndices;

		// GPU 端 SSBO
		Ref<ShaderStorageBuffer> BVHStorageBuffer;    // Binding 6
		Ref<ShaderStorageBuffer> OctreeStorageBuffer; // Binding 7
		Ref<ShaderStorageBuffer> IndexMapBuffer;      // Binding 8 (用于间接寻址)

		uint32_t BVHNodeCount = 0;
	};

	static Renderer3DData s_Data;

	void Renderer3D::init()
	{
		s_Data.CubeVA = VertexArray::create();

		s_Data.CubeVB = VertexBuffer::create(s_Data.MaxVertices * sizeof(CubeVertex));
		s_Data.CubeVB->setLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float,  "a_TexIndex" },
			{ ShaderDataType::Float,  "a_TilingFactor" },
			{ ShaderDataType::Int,    "a_FaceID" }
			});
		s_Data.CubeVA->addVertexBuffer(s_Data.CubeVB);

		s_Data.CubeVertexBufferBase = new CubeVertex[s_Data.MaxVertices];

		uint32_t* cubeIndices = new uint32_t[s_Data.MaxIndices];
		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
		{
			cubeIndices[i + 0] = offset + 0;
			cubeIndices[i + 1] = offset + 1;
			cubeIndices[i + 2] = offset + 2;

			cubeIndices[i + 3] = offset + 2;
			cubeIndices[i + 4] = offset + 3;
			cubeIndices[i + 5] = offset + 0;

			offset += 4;
		}

		Ref<IndexBuffer> cubeIB = IndexBuffer::create(cubeIndices, s_Data.MaxIndices);
		s_Data.CubeVA->setIndexBuffer(cubeIB);
		delete[] cubeIndices;

		s_Data.WhiteTexture = Texture2D::create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[s_Data.MaxTextureSlots];
		for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++) samplers[i] = i;

		s_Data.TextureShader = Shader::create("assets/shaders/Texture.glsl");
		s_Data.TextureShader->bind();
		s_Data.TextureShader->setIntArray("u_Textures", samplers, s_Data.MaxTextureSlots);

		s_Data.LineShader = Shader::create("assets/shaders/Line.glsl");

		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		// 初始化 Model 矩阵为单位矩阵，防止第一帧 Batch 渲染出错
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));

		// --- 初始化 24 个顶点的标准位置 ---
		// Front Face
		s_Data.CubeVertexPositions[0] = { -0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[1] = { 0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[2] = { 0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[3] = { -0.5f,  0.5f,  0.5f, 1.0f };

		// Right Face
		s_Data.CubeVertexPositions[4] = { 0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[5] = { 0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[6] = { 0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[7] = { 0.5f,  0.5f,  0.5f, 1.0f };

		// Back Face
		s_Data.CubeVertexPositions[8] = { 0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[9] = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[10] = { -0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[11] = { 0.5f,  0.5f, -0.5f, 1.0f };

		// Left Face
		s_Data.CubeVertexPositions[12] = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[13] = { -0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[14] = { -0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[15] = { -0.5f,  0.5f, -0.5f, 1.0f };

		// Top Face
		s_Data.CubeVertexPositions[16] = { -0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[17] = { 0.5f,  0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[18] = { 0.5f,  0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[19] = { -0.5f,  0.5f, -0.5f, 1.0f };

		// Bottom Face
		s_Data.CubeVertexPositions[20] = { -0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[21] = { 0.5f, -0.5f, -0.5f, 1.0f };
		s_Data.CubeVertexPositions[22] = { 0.5f, -0.5f,  0.5f, 1.0f };
		s_Data.CubeVertexPositions[23] = { -0.5f, -0.5f,  0.5f, 1.0f };

		// --- 初始化法线 ---
		for (int i = 0; i < 4; i++) s_Data.CubeVertexNormals[i] = { 0.0f, 0.0f, 1.0f }; // Front
		for (int i = 4; i < 8; i++) s_Data.CubeVertexNormals[i] = { 1.0f, 0.0f, 0.0f }; // Right
		for (int i = 8; i < 12; i++) s_Data.CubeVertexNormals[i] = { 0.0f, 0.0f, -1.0f };// Back
		for (int i = 12; i < 16; i++) s_Data.CubeVertexNormals[i] = { -1.0f, 0.0f, 0.0f };// Left
		for (int i = 16; i < 20; i++) s_Data.CubeVertexNormals[i] = { 0.0f, 1.0f, 0.0f }; // Top
		for (int i = 20; i < 24; i++) s_Data.CubeVertexNormals[i] = { 0.0f, -1.0f, 0.0f };// Bottom


		//线框
		s_Data.BatchLineVA = VertexArray::create();

		s_Data.BatchLineVB = VertexBuffer::create(s_Data.MaxLineVertices * sizeof(BatchLineVertex));
		s_Data.BatchLineVB->setLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" }
			});
		s_Data.BatchLineVA->addVertexBuffer(s_Data.BatchLineVB);

		s_Data.BatchLineVertexBufferBase = new BatchLineVertex[s_Data.MaxLineVertices];

		s_Data.BatchLineShader = Shader::create("assets/shaders/BatchLine.glsl");


		// 初始化 Camera UBO (binding point = 0)
		s_Data.CameraUBO = UniformBuffer::create(sizeof(Renderer3DData::CameraData), 0);

		// 初始化光线追踪资源
		// 使用新的 Spec 创建高精度纹理
		TextureSpecification spec;
		spec.Width = 1280;
		spec.Height = 720;
		spec.Format = ImageFormat::RGBA32F; 
		s_Data.ComputeOutputTexture = Texture2D::create(spec);

		s_Data.RaytracingShader = ComputeShader::create("assets/shaders/Raytrace.glsl");
		s_Data.SpectralShader = ComputeShader::create("assets/shaders/SpectralRaytrace.glsl");
	}

	void Renderer3D::shutdown()
	{
		delete[] s_Data.CubeVertexBufferBase;
	}

	void Renderer3D::setSelection(int entityID, int faceID)
	{
		s_Data.SelectedEntityID = entityID;
		s_Data.SelectedFaceID = faceID;
	}

	void Renderer3D::setHover(int entityID, int faceID, int edgeID)
	{
		s_Data.HoveredEntityID = entityID;
		s_Data.HoveredFaceID = faceID;
		s_Data.HoveredEdgeID = edgeID;
	}

	int Renderer3D::getHoveredEntityID()
	{
		return s_Data.HoveredEntityID;
	}

	void Renderer3D::setSpectralRendering(bool enable) { s_Data.UseSpectralRendering = enable; }

	bool Renderer3D::isSpectralRendering() { return s_Data.UseSpectralRendering; }

	void Renderer3D::beginScene(const PerspectiveCamera& camera)
	{
		s_Data.TextureShader->bind();

		// 通过 UBO 上传相机数据 (所有 shader 共享, binding = 0)
		s_Data.ViewProjection = camera.getViewProjectionMatrix();
		s_Data.CameraUBOData.ViewProjection = s_Data.ViewProjection;
		s_Data.CameraUBOData.ViewPos = camera.getPosition();
		s_Data.CameraUBO->setData(&s_Data.CameraUBOData, sizeof(Renderer3DData::CameraData));

		// 保持兼容：继续用 uniform 设置 (逐步迁移到 UBO 后可移除)
		s_Data.TextureShader->setMat4("u_ViewProjection", s_Data.ViewProjection);
		s_Data.TextureShader->setFloat3("u_ViewPos", camera.getPosition());
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));
		s_Data.TextureShader->setInt("u_EntityID", -1);

		s_Data.CubeIndexCount = 0;
		s_Data.CubeVertexBufferPtr = s_Data.CubeVertexBufferBase;
		s_Data.TextureSlotIndex = 1;
	}

	void Renderer3D::endScene()
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CubeVertexBufferPtr - (uint8_t*)s_Data.CubeVertexBufferBase);
		s_Data.CubeVB->setData(s_Data.CubeVertexBufferBase, dataSize);
		flush();
	}

	void Renderer3D::flush()
	{
		if (s_Data.CubeIndexCount == 0) return;

		// Batch 渲染时，顶点已经在 CPU 变换过了，所以 GPU 的 u_Model 必须是 Identity
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));
		s_Data.TextureShader->setInt("u_EntityID", -1);
		s_Data.TextureShader->setInt("u_SelectedEntityID", -1);
		s_Data.TextureShader->setInt("u_HoveredEntityID", -1);
		s_Data.TextureShader->setFloat3("u_Albedo", glm::vec3(1.0f));
		s_Data.TextureShader->setFloat("u_Roughness", 0.5f);
		s_Data.TextureShader->setFloat("u_Metallic", 0.0f);

		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			s_Data.TextureSlots[i]->bind(i);

		s_Data.CubeVA->bind();
		RenderCommand::drawIndexed(s_Data.CubeVA, s_Data.CubeIndexCount);
		s_Data.Stats.DrawCalls++;
	}

	void Renderer3D::flushAndReset()
	{
		endScene();
		s_Data.CubeIndexCount = 0;
		s_Data.CubeVertexBufferPtr = s_Data.CubeVertexBufferBase;
		s_Data.TextureSlotIndex = 1;
	}

	void Renderer3D::beginLines(const PerspectiveCamera& camera)
	{
		s_Data.BatchLineShader->bind();
		s_Data.BatchLineShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

		RenderCommand::setDepthTest(false);

		s_Data.BatchLineVertexCount = 0;
		s_Data.BatchLineVertexBufferPtr = s_Data.BatchLineVertexBufferBase;
	}

	void Renderer3D::endLines()
	{
		if (s_Data.BatchLineVertexCount > 0)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.BatchLineVertexBufferPtr - (uint8_t*)s_Data.BatchLineVertexBufferBase);
			s_Data.BatchLineVB->setData(s_Data.BatchLineVertexBufferBase, dataSize);

			s_Data.BatchLineVA->bind();
			RenderCommand::drawLines(s_Data.BatchLineVA, s_Data.BatchLineVertexCount);

			s_Data.Stats.DrawCalls++;
		}

		RenderCommand::setDepthTest(true);
	}

	void Renderer3D::drawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
	{
		if (s_Data.BatchLineVertexCount >= Renderer3DData::MaxLineVertices)
			return;

		s_Data.BatchLineVertexBufferPtr->Position = p0;
		s_Data.BatchLineVertexBufferPtr->Color = color;
		s_Data.BatchLineVertexBufferPtr++;

		s_Data.BatchLineVertexBufferPtr->Position = p1;
		s_Data.BatchLineVertexBufferPtr->Color = color;
		s_Data.BatchLineVertexBufferPtr++;

		s_Data.BatchLineVertexCount += 2;
	}

	void Renderer3D::drawGrid(const glm::mat4& transform, float size, int steps)
	{
		float stepSize = size / steps;

		glm::vec4 greyColor = { 0.6f, 0.6f, 0.6f, 0.5f };
		glm::vec4 redColor = { 0.8f, 0.2f, 0.2f, 1.0f };
		glm::vec4 greenColor = { 0.2f, 0.8f, 0.2f, 1.0f };

		// 1. 批量计算并写入灰色网格线
		for (int i = -steps; i <= steps; i++)
		{
			if (i == 0) continue;

			float pos = i * stepSize;

			// 本地坐标点 (延伸到负方向)
			glm::vec4 p1_local = { pos, -size, 0.0f, 1.0f }; // 竖线
			glm::vec4 p2_local = { pos,  size, 0.0f, 1.0f };
			glm::vec4 p3_local = { -size, pos, 0.0f, 1.0f }; // 横线
			glm::vec4 p4_local = { size, pos, 0.0f, 1.0f };

			drawLine(glm::vec3(transform * p1_local), glm::vec3(transform * p2_local), greyColor);
			drawLine(glm::vec3(transform * p3_local), glm::vec3(transform * p4_local), greyColor);
		}

		// 2. 写入十字坐标轴 (贯穿全屏)
		// X轴 (红): 从左到右 (-size 到 size)
		glm::vec3 xStart = glm::vec3(transform * glm::vec4(-size, 0.0f, 0.0f, 1.0f));
		glm::vec3 xEnd = glm::vec3(transform * glm::vec4(size, 0.0f, 0.0f, 1.0f));

		// Y轴 (绿): 从下到上 (-size 到 size)
		glm::vec3 yStart = glm::vec3(transform * glm::vec4(0.0f, -size, 0.0f, 1.0f));
		glm::vec3 yEnd = glm::vec3(transform * glm::vec4(0.0f, size, 0.0f, 1.0f));

		drawLine(xStart, xEnd, redColor);
		drawLine(yStart, yEnd, greenColor);
	}

	// --- 基础绘制 (Axis Aligned) ---
	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color)
	{
		drawCube(position, size, s_Data.WhiteTexture, color);
	}

	void Renderer3D::drawCube(const glm::vec3& position, const glm::vec3& size, const Ref<Texture2D>& texture, const glm::vec4& tintColor)
	{
		// 复用旋转绘制，旋转为 0 即可
		drawRotatedCube(position, size, 0.0f, { 0.0f, 1.0f, 0.0f }, texture, tintColor);
	}

	// --- 旋转绘制 ---
	void Renderer3D::drawRotatedCube(const glm::vec3& position, const glm::vec3& size, float rotation, const glm::vec3& axis, const glm::vec4& color)
	{
		drawRotatedCube(position, size, rotation, axis, s_Data.WhiteTexture, color);
	}

	void Renderer3D::drawRotatedCube(const glm::vec3& position, const glm::vec3& size, float rotation, const glm::vec3& axis, const Ref<Texture2D>& texture, const glm::vec4& tintColor)
	{
		if (s_Data.CubeIndexCount >= Renderer3DData::MaxIndices)
			flushAndReset();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= Renderer3DData::MaxTextureSlots)
				flushAndReset();

			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		// 计算变换矩阵 (包含旋转)
		glm::mat4 rotationMat;
		if (rotation != 0.0f)
			rotationMat = glm::rotate(glm::mat4(1.0f), rotation, axis);
		else
			rotationMat = glm::mat4(1.0f);

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* rotationMat
			* glm::scale(glm::mat4(1.0f), size);

		// 计算法线矩阵 (只旋转)
		glm::mat3 normalMatrix = glm::mat3(rotationMat);

		// 填充 24 个顶点
		for (int i = 0; i < 24; i++)
		{
			s_Data.CubeVertexBufferPtr->Position = transform * s_Data.CubeVertexPositions[i];
			s_Data.CubeVertexBufferPtr->Normal = normalMatrix * s_Data.CubeVertexNormals[i]; // 旋转法线
			s_Data.CubeVertexBufferPtr->Color = tintColor;
			s_Data.CubeVertexBufferPtr->FaceID = -1;

			switch (i % 4)
			{
			case 0: s_Data.CubeVertexBufferPtr->TexCoord = { 0.0f, 0.0f }; break;
			case 1: s_Data.CubeVertexBufferPtr->TexCoord = { 1.0f, 0.0f }; break;
			case 2: s_Data.CubeVertexBufferPtr->TexCoord = { 1.0f, 1.0f }; break;
			case 3: s_Data.CubeVertexBufferPtr->TexCoord = { 0.0f, 1.0f }; break;
			}

			s_Data.CubeVertexBufferPtr->TexIndex = textureIndex;
			s_Data.CubeVertexBufferPtr->TilingFactor = 1.0f;
			s_Data.CubeVertexBufferPtr++;
		}

		s_Data.CubeIndexCount += 36;
		s_Data.Stats.CubeCount++;
	}

	// ===========================================
	//  Mesh Rendering
	// ===========================================
	void Renderer3D::drawModel(const Ref<VertexArray>& va, const glm::mat4& transform,int entityID, const MaterialComponent* material)
	{
		// 1. 如果批处理里有方块没画，先画掉 (flush 会使用 Identity Model 矩阵)
		//flush();

		s_Data.TextureShader->bind();

		// 2. 分别设置矩阵
		// u_Model: 物体的变换 (Shader 用它算 v_Position 和 v_Normal)
		s_Data.TextureShader->setMat4("u_Model", transform);

		// u_ViewProjection: 相机的 VP (保持 beginScene 设的值，不需要乘 transform)
		s_Data.TextureShader->setMat4("u_ViewProjection", s_Data.ViewProjection);

		s_Data.WhiteTexture->bind(0);

		s_Data.TextureShader->setInt("u_EntityID", entityID);

		s_Data.TextureShader->setInt("u_SelectedEntityID", s_Data.SelectedEntityID);
		s_Data.TextureShader->setInt("u_SelectedFaceID", s_Data.SelectedFaceID);

		s_Data.TextureShader->setInt("u_HoveredEntityID", s_Data.HoveredEntityID);
		s_Data.TextureShader->setInt("u_HoveredFaceID", s_Data.HoveredFaceID);

		if (material)
		{
			s_Data.TextureShader->setFloat3("u_Albedo", material->Albedo);
			s_Data.TextureShader->setFloat("u_Roughness", material->Roughness);
			s_Data.TextureShader->setFloat("u_Metallic", material->Metallic);
		}
		else
		{
			s_Data.TextureShader->setFloat3("u_Albedo", glm::vec3(1.0f));
			s_Data.TextureShader->setFloat("u_Roughness", 0.5f);
			s_Data.TextureShader->setFloat("u_Metallic", 0.0f);
		}

		va->bind();

		// 3. 显式传递 Index Count！
		uint32_t count = va->getIndexBuffer()->getCount();
		RenderCommand::drawIndexed(va, count);

		s_Data.Stats.DrawCalls++;

		// 4. 画完后，恢复 u_Model 为单位矩阵
		// 否则之后如果再调用 drawCube，方块会飞到错误的地方
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));
	}

	void Renderer3D::drawModel(Entity& en, const glm::mat4& transform,int entityID)
	{
		s_Data.TextureShader->bind();

		s_Data.TextureShader->setMat4("u_Model", transform);
		s_Data.TextureShader->setMat4("u_ViewProjection", s_Data.ViewProjection);

		s_Data.WhiteTexture->bind(0);

		if (en.HasComponent<MaterialComponent>())
		{
			const auto& mat = en.GetComponent<MaterialComponent>();

			s_Data.TextureShader->setFloat3("u_Albedo", mat.Albedo);
			s_Data.TextureShader->setFloat("u_Roughness", mat.Roughness);
			s_Data.TextureShader->setFloat("u_Metallic", mat.Metallic);
		}

		s_Data.TextureShader->setInt("u_EntityID", entityID);

		s_Data.TextureShader->setInt("u_SelectedEntityID", s_Data.SelectedEntityID);
		s_Data.TextureShader->setInt("u_SelectedFaceID", s_Data.SelectedFaceID);

		s_Data.TextureShader->setInt("u_HoveredEntityID", s_Data.HoveredEntityID);
		s_Data.TextureShader->setInt("u_HoveredFaceID", s_Data.HoveredFaceID);


		Ref<VertexArray> va = en.GetComponent<MeshComponent>().VA;
		va->bind();

		// 3. 显式传递 Index Count！
		uint32_t count = va->getIndexBuffer()->getCount();
		RenderCommand::drawIndexed(va, count);

		s_Data.Stats.DrawCalls++;

		//清理显存
		s_Data.TextureShader->setMat4("u_Model", glm::mat4(1.0f));

		s_Data.TextureShader->setFloat3("u_Albedo", glm::vec3(1.0f));
		s_Data.TextureShader->setFloat("u_Roughness", 0.5f);
		s_Data.TextureShader->setFloat("u_Metallic", 0.0f);
	}

	void Renderer3D::drawEdges(const Ref<VertexArray>& va, const glm::mat4& transform, const glm::vec4& color, int entityID, int selectedEdgeID)
	{
		if (!va) return;

		// 绑定线框 Shader
		s_Data.LineShader->bind();

		s_Data.LineShader->setMat4("u_ViewProjection", s_Data.ViewProjection);
		s_Data.LineShader->setMat4("u_Transform", transform);
		s_Data.LineShader->setFloat4("u_Color", color);

		s_Data.LineShader->setInt("u_EntityID", entityID);
		s_Data.LineShader->setInt("u_SelectedEdgeID", selectedEdgeID);

		s_Data.LineShader->setInt("u_HoveredEntityID", s_Data.HoveredEntityID);
		s_Data.LineShader->setInt("u_HoveredEdgeID", s_Data.HoveredEdgeID);

		va->bind();

		// 绘制线条
		// 获取第一个 VertexBuffer
		auto& vb = va->getVertexBuffers()[0];

		// 通过 Size / Stride 计算顶点数量
		// count = 总字节数 / 单个顶点的步长
		uint32_t vertexCount = vb->getSize() / vb->getLayout().getStride();
		RenderCommand::drawLines(va, vertexCount);
	}


	void Renderer3D::SetSpectralRange(float start, float end)
	{
		if (s_Data.SpectralStart != start || s_Data.SpectralEnd != end)
		{
			s_Data.SpectralStart = start;
			s_Data.SpectralEnd = end;

			s_Data.FrameIndex = 1;
		}
	}

	void Renderer3D::UploadSceneDataToGPU(Scene* scene)
	{
		// 1. 清空所有 Host 缓存
		s_Data.HostVertices.clear();
		s_Data.HostTriangles.clear();
		s_Data.HostMaterials.clear();
		s_Data.HostSpectralCurves.clear(); // [新增] 清空光谱数据

		auto view = scene->getRegistry().view<TransformComponent, MeshComponent>();

		for (auto entityHandle : view)
		{
			auto [tc, mesh] = view.get<TransformComponent, MeshComponent>(entityHandle);

			if (mesh.LocalVertices.empty() || mesh.LocalIndices.empty())
				continue;

			// ==================== 材质处理逻辑 ====================
			// 默认值初始化
			GPUMaterial gpuMat;
			// 初始化默认值
			gpuMat.AlbedoRoughness = { 0.8f, 0.8f, 0.8f, 0.5f }; // RGB, Roughness
			gpuMat.Metallic = 0.0f;
			gpuMat.Emission = 0.0f;
			gpuMat.SpectralIndex0 = -1; // -1 表示无效
			gpuMat.SpectralIndex1 = -1;
			gpuMat.Type = 0; // 默认 Diffuse
			gpuMat._pad1 = 0; gpuMat._pad2 = 0; gpuMat._pad3 = 0;

			Entity entity = { entityHandle, scene };

			// --- 1. 读取基础物理属性 (作为 Fallback 或混合参数) ---
			if (entity.HasComponent<MaterialComponent>())
			{
				auto& mat = entity.GetComponent<MaterialComponent>();
				gpuMat.AlbedoRoughness = { mat.Albedo.r, mat.Albedo.g, mat.Albedo.b, mat.Roughness };
				gpuMat.Metallic = mat.Metallic;
			}

			// --- 2. 读取光谱数据 ---
			if (entity.HasComponent<SpectralMaterialComponent>())
			{
				auto& specComp = entity.GetComponent<SpectralMaterialComponent>();

				// 设置材质类型 (0=Diffuse, 1=Conductor, 2=Dielectric)
				gpuMat.Type = (int)specComp.Type;

				// 辅助 Lambda: 上传一条曲线并返回索引
				auto UploadCurve = [&](const std::vector<float>& curveData) -> int {
					if (curveData.empty()) return -1;

					// 32点对齐
					int startIndex = (int)(s_Data.HostSpectralCurves.size() / 32);
					std::vector<float> aligned = curveData;
					if (aligned.size() < 32) aligned.resize(32, 0.0f);
					if (aligned.size() > 32) aligned.resize(32);

					s_Data.HostSpectralCurves.insert(s_Data.HostSpectralCurves.end(), aligned.begin(), aligned.end());
					return startIndex;
					};

				// 根据类型上传 Slot0 和 Slot1
				// Slot0: Reflectance / n / Transmission
				gpuMat.SpectralIndex0 = UploadCurve(specComp.SpectrumSlot0);

				// Slot1: k / IOR (只有金属和玻璃需要)
				if (specComp.Type != SpectralMaterialComponent::MaterialType::Diffuse) {
					gpuMat.SpectralIndex1 = UploadCurve(specComp.SpectrumSlot1);
				}
			}

			// 存入 HostMaterials
			uint32_t currentMatIndex = (uint32_t)s_Data.HostMaterials.size();
			s_Data.HostMaterials.push_back(gpuMat);			
			// ============================================================

			glm::mat4 transform = tc.GetTransform();
			glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(transform)));

			uint32_t vertexOffset = (uint32_t)s_Data.HostVertices.size();

			// --- 转换并合并顶点 ---
			for (const auto& v : mesh.LocalVertices)
			{
				GPUVertex gpuV;
				gpuV.Position = glm::vec3(transform * glm::vec4(v.Position, 1.0f));
				gpuV.Normal = glm::normalize(normalMatrix * v.Normal);
				gpuV.TexCoord = v.TexCoord;
				gpuV._pad1 = 0.0f; gpuV._pad2 = 0.0f; gpuV._pad3 = { 0.0f, 0.0f };

				s_Data.HostVertices.push_back(gpuV);
			}

			// --- 合并索引 ---
			for (size_t i = 0; i < mesh.LocalIndices.size(); i += 3)
			{
				if (i + 2 >= mesh.LocalIndices.size()) break;

				TriangleData tri;
				tri.v0 = mesh.LocalIndices[i + 0] + vertexOffset;
				tri.v1 = mesh.LocalIndices[i + 1] + vertexOffset;
				tri.v2 = mesh.LocalIndices[i + 2] + vertexOffset;
				tri.MaterialID = currentMatIndex;

				s_Data.HostTriangles.push_back(tri);
			}
		}

		// ==================== 上传 SSBO 数据 ====================

		if (s_Data.HostVertices.empty()) return;

		size_t vertSize = s_Data.HostVertices.size() * sizeof(GPUVertex);
		size_t triSize = s_Data.HostTriangles.size() * sizeof(TriangleData);
		size_t matSize = s_Data.HostMaterials.size() * sizeof(GPUMaterial);

		// 1. Vertices SSBO
		if (!s_Data.VerticesSSBO) s_Data.VerticesSSBO = ShaderStorageBuffer::create((uint32_t)vertSize, ShaderStorageBufferUsage::DynamicDraw);
		else s_Data.VerticesSSBO->resize((uint32_t)vertSize);
		s_Data.VerticesSSBO->bind(1);
		s_Data.VerticesSSBO->setData(s_Data.HostVertices.data(), (uint32_t)vertSize);

		// 2. Triangles SSBO
		if (!s_Data.TrianglesSSBO) s_Data.TrianglesSSBO = ShaderStorageBuffer::create((uint32_t)triSize, ShaderStorageBufferUsage::DynamicDraw);
		else s_Data.TrianglesSSBO->resize((uint32_t)triSize);
		s_Data.TrianglesSSBO->bind(2);
		s_Data.TrianglesSSBO->setData(s_Data.HostTriangles.data(), (uint32_t)triSize);

		// 3. Materials SSBO
		if (!s_Data.MaterialsSSBO) s_Data.MaterialsSSBO = ShaderStorageBuffer::create((uint32_t)matSize, ShaderStorageBufferUsage::DynamicDraw);
		else s_Data.MaterialsSSBO->resize((uint32_t)matSize);
		s_Data.MaterialsSSBO->bind(3);
		s_Data.MaterialsSSBO->setData(s_Data.HostMaterials.data(), (uint32_t)matSize);

		// 4.  Spectral Curves SSBO (Binding = 5)
		if (!s_Data.HostSpectralCurves.empty())
		{
			size_t curveSize = s_Data.HostSpectralCurves.size() * sizeof(float);

			if (!s_Data.SpectralCurvesSSBO)
				s_Data.SpectralCurvesSSBO = ShaderStorageBuffer::create((uint32_t)curveSize, ShaderStorageBufferUsage::DynamicDraw);
			else
				s_Data.SpectralCurvesSSBO->resize((uint32_t)curveSize);

			s_Data.SpectralCurvesSSBO->bind(5); //避免冲突
			s_Data.SpectralCurvesSSBO->setData(s_Data.HostSpectralCurves.data(), (uint32_t)curveSize);
		}
	}

	void Renderer3D::RenderComputeFrame(const PerspectiveCamera& camera, float time, bool resetAccumulation)
	{
		// 1. 帧数管理
		if (resetAccumulation)
			s_Data.FrameIndex = 1;
		else
			s_Data.FrameIndex++;

		Ref<ComputeShader> shader;
		if (s_Data.UseSpectralRendering)
			shader = s_Data.SpectralShader;
		else
			shader = s_Data.RaytracingShader;

		auto& outputTexture = s_Data.ComputeOutputTexture;
		auto& accumulationTexture = s_Data.AccumulationTexture;

		if (!shader || !outputTexture || !accumulationTexture) return;

		shader->bind();

		// 2. 绑定图像单元
		// Binding 0: 输出显示
		glBindImageTexture(0, outputTexture->getRendererID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		// Binding 4: 累积缓冲区
		glBindImageTexture(4, accumulationTexture->getRendererID(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glm::mat4 invProj = glm::inverse(camera.getProjectionMatrix());
		glm::mat4 invView = glm::inverse(camera.getViewMatrix());

		// 3. 设置 Uniforms
		shader->setFloat("u_Time", time);
		shader->setMat4("u_InverseProjection", invProj);
		shader->setMat4("u_InverseView", invView);
		shader->setFloat3("u_CameraPos", camera.getPosition());
		shader->setInt("u_FrameIndex", s_Data.FrameIndex);

		shader->setFloat("u_LambdaMin", s_Data.SpectralStart);
		shader->setFloat("u_LambdaMax", s_Data.SpectralEnd);

		// 4. 绑定 SSBO
		if (s_Data.VerticesSSBO) s_Data.VerticesSSBO->bind(1);
		if (s_Data.TrianglesSSBO) s_Data.TrianglesSSBO->bind(2);
		if (s_Data.MaterialsSSBO) s_Data.MaterialsSSBO->bind(3);
		if (s_Data.SpectralCurvesSSBO) s_Data.SpectralCurvesSSBO->bind(5);

		// 5. 发射计算
		uint32_t width = outputTexture->getWidth();
		uint32_t height = outputTexture->getHeight();
		uint32_t groupX = (width + 8 - 1) / 8;
		uint32_t groupY = (height + 8 - 1) / 8;

		shader->dispatch(groupX, groupY, 1);
		shader->unbind();
	}

	Ref<Texture2D> Renderer3D::GetComputeOutputTexture()
	{
		return s_Data.ComputeOutputTexture;
	}

	void Renderer3D::ResizeComputeOutput(uint32_t width, uint32_t height)
	{
		// 1. 安全检查：如果尺寸没变，或者尺寸无效，直接返回
		if (s_Data.ComputeOutputTexture &&
			s_Data.ComputeOutputTexture->getWidth() == width &&
			s_Data.ComputeOutputTexture->getHeight() == height)
		{
			return;
		}

		if (width == 0 || height == 0) return;

		// 2. 重新创建高精度纹理
		TextureSpecification spec;
		spec.Width = width;
		spec.Height = height;
		spec.Format = ImageFormat::RGBA32F; // 必须保持 RGBA32F
		s_Data.ComputeOutputTexture = Texture2D::create(spec);

		// 创建累积纹理
		spec.Width = width;
		spec.Height = height;
		spec.Format = ImageFormat::RGBA32F;
		s_Data.AccumulationTexture = Texture2D::create(spec);

		s_Data.FrameIndex = 1;
	}

	void Renderer3D::BuildAccelerationStructures(Scene* scene)
	{
		// 1. 如果当前没有启用 BVH，直接返回，节省性能
		if (s_Data.CurrentAccelType != AccelType::BVH)
			return;

		// 计时开始 (用于性能分析)
		auto start = std::chrono::high_resolution_clock::now();

		// 2. 收集场景中所有的三角形
		// 注意：这里的遍历顺序必须与 UploadSceneDataToGPU 中上传 Triangles 的顺序严格一致！
		// 否则索引就会错乱，BVH 会指向错误的三角形。

		std::vector<BVHTriangle> worldTriangles;
		uint32_t globalTriIndex = 0; // 这是三角形在 GPU Triangles Buffer (Binding 2) 中的原始索引

		// 获取所有带 Mesh 和 Transform 的实体
		auto view = scene->getAllEntitiesWith<TransformComponent, MeshComponent>();

		for (auto entity : view)
		{
			auto [tc, mesh] = view.get<TransformComponent, MeshComponent>(entity);

			// 跳过空 Mesh
			if (mesh.LocalVertices.empty() || mesh.LocalIndices.empty())
				continue;

			glm::mat4 transform = tc.GetTransform();

			// 遍历当前 Mesh 的所有三角形
			// 假设是 Indexed Draw，步长为 3
			for (size_t i = 0; i < mesh.LocalIndices.size(); i += 3)
			{
				// 获取顶点索引
				uint32_t idx0 = mesh.LocalIndices[i];
				uint32_t idx1 = mesh.LocalIndices[i + 1];
				uint32_t idx2 = mesh.LocalIndices[i + 2];

				// 变换到世界坐标 (World Space)
				// BVH 必须基于世界坐标构建，因为光线是在世界空间漫游的
				glm::vec4 v0 = transform * glm::vec4(mesh.LocalVertices[idx0].Position, 1.0f);
				glm::vec4 v1 = transform * glm::vec4(mesh.LocalVertices[idx1].Position, 1.0f);
				glm::vec4 v2 = transform * glm::vec4(mesh.LocalVertices[idx2].Position, 1.0f);

				BVHTriangle tri;
				tri.V0 = glm::vec3(v0);
				tri.V1 = glm::vec3(v1);
				tri.V2 = glm::vec3(v2);

				// 计算质心 (用于构建时的空间划分)
				tri.Centroid = (tri.V0 + tri.V1 + tri.V2) / 3.0f;

				// 记录它在 GPU 三角形大数组里的原始 ID
				tri.Index = globalTriIndex++;

				worldTriangles.push_back(tri);
			}
		}

		// 如果没有三角形，就不构建了
		if (worldTriangles.empty()) return;

		// 3. 执行 BVH 构建 (CPU高计算量操作)
		BVHBuilder builder(worldTriangles);

		// 4. 获取构建结果
		const auto& nodes = builder.GetNodes();
		const auto& sortedIndices = builder.GetSortedIndices();

		s_Data.BVHNodeCount = (uint32_t)nodes.size();

		// 5. 上传节点数据 (Binding 6)
		uint32_t nodeBufferSize = (uint32_t)nodes.size() * sizeof(GPUBVHNode);

		if (!s_Data.BVHStorageBuffer || s_Data.BVHStorageBuffer->getSize() < nodeBufferSize)
		{
			s_Data.BVHStorageBuffer = ShaderStorageBuffer::create(nodeBufferSize, ShaderStorageBufferUsage::StaticDraw);
			s_Data.BVHStorageBuffer->bind(6);
		}
		s_Data.BVHStorageBuffer->setData(nodes.data(), nodeBufferSize);

		// 6. 上传索引映射表 (Binding 8)
		// Shader 遍历到叶子节点时，拿到的是 sortedIndices 里的索引，
		// 需要通过 sortedIndices[i] 查找到原始的 TriangleID
		uint32_t indexBufferSize = (uint32_t)sortedIndices.size() * sizeof(uint32_t);

		if (!s_Data.IndexMapBuffer || s_Data.IndexMapBuffer->getSize() < indexBufferSize)
		{
			s_Data.IndexMapBuffer = ShaderStorageBuffer::create(indexBufferSize, ShaderStorageBufferUsage::StaticDraw);
			s_Data.IndexMapBuffer->bind(8);
		}
		s_Data.IndexMapBuffer->setData(sortedIndices.data(), indexBufferSize);

		// 性能统计日志
		auto end = std::chrono::high_resolution_clock::now();
		float duration = std::chrono::duration<float, std::milli>(end - start).count();
		RONG_CORE_INFO("BVH Rebuilt: {0} Triangles, {1} Nodes in {2}ms", worldTriangles.size(), nodes.size(), duration);
	}

	void Renderer3D::setAccelType(const AccelType& acceltype)
	{
		s_Data.CurrentAccelType = acceltype;
	}

	AccelType Renderer3D::getAccelType()
	{
		return s_Data.CurrentAccelType;
	}

	int Renderer3D::getBVHNodeCount()
	{
		return s_Data.BVHNodeCount;
	}

	int Renderer3D::getOctreeNodeCount()
	{
		return s_Data.OctreeNodes.size();
	}

	Renderer3D::Statistics Renderer3D::getStatistics() { return s_Data.Stats; }
	void Renderer3D::resetStatistics() { memset(&s_Data.Stats, 0, sizeof(Statistics)); }

// =============================================================
// Renderer （初始化与场景接口，submit 在 :OpenGL 分区定义）
// =============================================================
	void Renderer::init()
	{
		RenderCommand::init();
		Renderer2D::init();
		Renderer3D::init();
	}

	void Renderer::beginScene(const OrthographicCamera& camera)
	{
		s_sceneData->viewProjectionMatrix = camera.getViewProjectionMatrix();
	}

	void Renderer::endScene()
	{

	}

	void Renderer::onWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::setViewPort(0, 0, width, height);
	}

// =============================================================
// SpectralRenderer（CPU 逐像素光线追踪）
// =============================================================
	SpectralRenderer::SpectralRenderer()
	{
	}

	void SpectralRenderer::OnResize(uint32_t width, uint32_t height)
	{
		if (m_Width == width && m_Height == height) return;

		m_Width = width;
		m_Height = height;

		// 重新分配内存
		m_ImageData.resize(m_Width * m_Height);

		// 创建或重建 GPU 纹理
		if (m_FinalTexture) m_FinalTexture.reset(); // 释放旧的
		m_FinalTexture = Texture2D::create(m_Width, m_Height);
	}

	void SpectralRenderer::Render( Scene& scene, const PerspectiveCamera& camera)
	{
		if (m_Width == 0 || m_Height == 0) return;

		// 创建一个行索引的集合: 0, 1, 2, ..., height-1
		std::vector<uint32_t> verticalIter(m_Height);
		std::iota(verticalIter.begin(), verticalIter.end(), 0);

		// 使用 std::execution::par 让所有 CPU 核心一起跑
		std::for_each(std::execution::par, verticalIter.begin(), verticalIter.end(),
			[this, &camera,&scene](uint32_t y)
			{
				for (uint32_t x = 0; x < m_Width; x++)
				{
					glm::vec4 color = PerPixel(x, y, m_Width, m_Height, camera,scene);

					// 写入 Buffer
					m_ImageData[x + y * m_Width] = ConvertToRGBA(color);
				}
			});

		// 把 CPU 数据上传到 GPU
		m_FinalTexture->setData(m_ImageData.data(), m_ImageData.size() * sizeof(uint32_t));
	}

	// 计算像素
	glm::vec4 SpectralRenderer::PerPixel(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const PerspectiveCamera& camera, Scene& scene)
	{
		Ray ray;
		ray.Origin = camera.getPosition();

		glm::vec2 coord = { (float)x / (float)width, (float)y / (float)height };
		coord = coord * 2.0f - 1.0f;

		glm::vec4 target = camera.getInverseProjectionMatrix() * glm::vec4(coord.x, coord.y, 1.0f, 1.0f);
		glm::vec3 rayDir = glm::vec3(camera.getInverseViewMatrix() * glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0));

		ray.Direction = glm::normalize(rayDir);

		HitPayload payload = TraceRay(ray, scene);

		if (payload.HitDistance < 0.0f)
		{
			return glm::vec4(0.1f, 0.1f, 0.1f, 1.0f); // 背景色
		}

		// ===========================================================
		// 简单的光照计算 (Simple Lighting)
		// ===========================================================

		// 1. 定义光源 (比如从右上方射下来的白光)
		glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f)); // 光的方向 (指向光源的反方向)
		glm::vec3 lightColor = { 1.0f, 1.0f, 1.0f };

		// 2. 准备向量
		glm::vec3 normal = glm::normalize(payload.WorldNormal);
		glm::vec3 viewDir = glm::normalize(camera.getPosition() - payload.WorldPosition);

		// --- A. 漫反射 (Diffuse) ---
		// 塑料和金属都有漫反射，但金属的漫反射通常很弱（甚至为0，全黑）
		// N dot L
		float diff = glm::max(glm::dot(normal, -lightDir), 0.0f);
		glm::vec3 diffuse = diff * payload.Albedo;

		// --- B. 高光 (Specular) ---
		// 使用简单的反射向量算法
		glm::vec3 reflectDir = glm::reflect(lightDir, normal);

		// 粗糙度转光泽度 (Roughness -> Shininess)
		// 粗糙度 0 -> 非常亮 (指数大), 粗糙度 1 -> 非常散 (指数小)
		float shininess = (1.0f - payload.Roughness) * 256.0f;

		float spec = std::pow(glm::max(glm::dot(viewDir, reflectDir), 0.0f), shininess);
		glm::vec3 specular = spec * lightColor;

		// --- C. 材质混合 (Mix) ---
		glm::vec3 finalColor;

		if (payload.Metallic > 0.5f)
		{
			// 金属模式：
			// 漫反射几乎没有 (变黑)，原本的 Albedo 变成了高光的颜色 (有色高光)
			// 简单的金属近似：Albedo 作用于 Specular
			glm::vec3 kS = payload.Albedo;
			glm::vec3 kD = glm::vec3(0.0f); // 金属几乎没漫反射

			finalColor = kD * diff + specular * kS;
		}
		else
		{
			// 塑料/非金属模式：
			// 高光通常是白色的，Albedo 作用于漫反射
			float specularIntensity = 0.5f; // 塑料的高光强度通常固定
			finalColor = diffuse + specular * specularIntensity;
		}

		// --- D. 环境光 (Ambient) ---
		// 稍微加一点底色，防止背光面全黑
		glm::vec3 ambient = payload.Albedo * 0.1f;
		finalColor += ambient;

		// 简单的 Gamma 矫正 
		 finalColor = glm::pow(finalColor, glm::vec3(1.0f / 2.2f));

		return glm::vec4(finalColor, 1.0f);
	}

	// Möller–Trumbore ray-triangle intersection algorithm
	bool SpectralRenderer::RayTriangleIntersect(const Ray& ray,
		const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
		float& t, glm::vec3& normal)
	{
		const float EPSILON = 0.0000001f;
		glm::vec3 edge1 = v1 - v0;
		glm::vec3 edge2 = v2 - v0;
		glm::vec3 h = glm::cross(ray.Direction, edge2);
		float a = glm::dot(edge1, h);

		if (a > -EPSILON && a < EPSILON) return false; // 光线平行于三角形

		float f = 1.0f / a;
		glm::vec3 s = ray.Origin - v0;
		float u = f * glm::dot(s, h);
		if (u < 0.0f || u > 1.0f) return false;

		glm::vec3 q = glm::cross(s, edge1);
		float v = f * glm::dot(ray.Direction, q);
		if (v < 0.0f || u + v > 1.0f) return false;

		float currentT = f * glm::dot(edge2, q);
		if (currentT > EPSILON) // 只有 t > 0 才算击中（前方）
		{
			t = currentT;
			// 简单的面法线 (Flat Shading)
			// 如果要光滑着色，需要用 u,v 插值顶点法线
			normal = glm::normalize(glm::cross(edge1, edge2));
			return true;
		}
		return false;
	}

	SpectralRenderer::HitPayload SpectralRenderer::TraceRay(const Ray& ray, Scene& scene)
	{
		HitPayload payload;
		payload.HitDistance = std::numeric_limits<float>::max(); // 初始化为无穷远

		// 1. 获取场景中所有带 MeshComponent 的物体
		auto view = scene.getAllEntitiesWith<TransformComponent, MeshComponent>();

		// 2. 遍历所有物体 (Brute Force，以后用 BVH 加速)
		for (auto entityHandle : view)
		{
			auto [tc, mesh] = view.get<TransformComponent, MeshComponent>(entityHandle);

			// 跳过没有顶点数据的
			if (mesh.LocalVertices.empty()) continue;

			// 获取模型矩阵 (Model Matrix)
			glm::mat4 transform = tc.GetTransform();

			// 提取法线矩阵 (用于变换法线，防止非均匀缩放导致法线错误)
			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

			// 3. 遍历物体 meshes 里的所有三角形
			// LocalVertices 是 CubeVertex 结构体
			size_t numIndices = mesh.LocalIndices.size();
			for (size_t i = 0; i < numIndices; i += 3)
			{
				// 1. 从索引数组拿顶点下标
				uint32_t idx0 = mesh.LocalIndices[i];
				uint32_t idx1 = mesh.LocalIndices[i + 1];
				uint32_t idx2 = mesh.LocalIndices[i + 2];

				// 2. 用下标去顶点数组拿数据
				glm::vec3 localV0 = mesh.LocalVertices[idx0].Position;
				glm::vec3 localV1 = mesh.LocalVertices[idx1].Position;
				glm::vec3 localV2 = mesh.LocalVertices[idx2].Position;

				// 3. 变换到世界坐标
				glm::vec3 worldV0 = glm::vec3(transform * glm::vec4(localV0, 1.0f));
				glm::vec3 worldV1 = glm::vec3(transform * glm::vec4(localV1, 1.0f));
				glm::vec3 worldV2 = glm::vec3(transform * glm::vec4(localV2, 1.0f));

				float t = 0.0f;
				glm::vec3 n;

				if (RayTriangleIntersect(ray, worldV0, worldV1, worldV2, t, n))
				{
					if (t < payload.HitDistance)
					{
						payload.HitDistance = t;
						payload.EntityID = (int)(uint32_t)entityHandle;
						payload.WorldPosition = ray.Origin + ray.Direction * t;
						payload.WorldNormal = n;
					}

					Entity entity = { entityHandle, &scene };
					if (entity.HasComponent<MaterialComponent>())
					{
						const auto& material = entity.GetComponent<MaterialComponent>();
						payload.Albedo = material.Albedo;
						payload.Roughness = material.Roughness;
						payload.Metallic = material.Metallic;
					}
					else
					{
						// 默认材质 (灰色塑料)
						payload.Albedo = { 0.8f, 0.8f, 0.8f };
						payload.Roughness = 0.5f;
						payload.Metallic = 0.0f;
					}
				}
			}
		}

		// 如果没打中任何东西，距离重置为 -1
		if (payload.HitDistance == std::numeric_limits<float>::max())
			payload.HitDistance = -1.0f;

		return payload;
	}

RendererAPI::API RendererAPI::s_api = RendererAPI::API::OpenGL;

Renderer::SceneData* Renderer::s_sceneData = new SceneData;


} // namespace Rongine
