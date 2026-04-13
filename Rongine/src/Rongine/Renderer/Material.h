#pragma once
#include "Rongine/Core/Core.h"
#include "Rongine/Renderer/Shader.h"
#include "Rongine/Renderer/Texture.h"
#include "Rongine/Renderer/UniformBuffer.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace Rongine {

	struct MaterialData
	{
		glm::vec3 Albedo = { 1.0f, 1.0f, 1.0f };
		float Roughness = 0.5f;

		float Metallic = 0.0f;
		float Emission = 0.0f;
		float _pad0 = 0.0f;
		float _pad1 = 0.0f;
	};

	class Material
	{
	public:
		Material(const Ref<Shader>& shader);
		~Material() = default;

		void bind(uint32_t uboBindingPoint = 2) const;

		void setAlbedo(const glm::vec3& albedo);
		void setRoughness(float roughness);
		void setMetallic(float metallic);
		void setEmission(float emission);

		void setTexture(const std::string& slot, const Ref<Texture2D>& texture);
		Ref<Texture2D> getTexture(const std::string& slot) const;

		Ref<Shader> getShader() const { return m_shader; }
		const MaterialData& getData() const { return m_data; }

		static Ref<Material> create(const Ref<Shader>& shader);

	private:
		void uploadData() const;

		Ref<Shader> m_shader;
		MaterialData m_data;
		mutable Ref<UniformBuffer> m_ubo;
		mutable bool m_dirty = true;

		std::unordered_map<std::string, Ref<Texture2D>> m_textures;
	};

	class MaterialInstance
	{
	public:
		MaterialInstance(const Ref<Material>& baseMaterial);
		~MaterialInstance() = default;

		void bind(uint32_t uboBindingPoint = 2) const;

		void setAlbedo(const glm::vec3& albedo);
		void setRoughness(float roughness);
		void setMetallic(float metallic);

		void setTexture(const std::string& slot, const Ref<Texture2D>& texture);

		Ref<Material> getBaseMaterial() const { return m_baseMaterial; }

		static Ref<MaterialInstance> create(const Ref<Material>& baseMaterial);

	private:
		void uploadData() const;

		Ref<Material> m_baseMaterial;
		MaterialData m_overrideData;
		mutable Ref<UniformBuffer> m_ubo;
		mutable bool m_dirty = true;

		std::unordered_map<std::string, Ref<Texture2D>> m_textureOverrides;
	};

}
