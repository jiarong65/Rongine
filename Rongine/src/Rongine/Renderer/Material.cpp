#include "Rongpch.h"
#include "Material.h"

namespace Rongine {

	// ============================================================
	//  Material
	// ============================================================

	Material::Material(const Ref<Shader>& shader)
		: m_shader(shader)
	{
		m_ubo = UniformBuffer::create(sizeof(MaterialData), 2);
	}

	void Material::bind(uint32_t uboBindingPoint) const
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

	void Material::setAlbedo(const glm::vec3& albedo) { m_data.Albedo = albedo; m_dirty = true; }
	void Material::setRoughness(float roughness) { m_data.Roughness = roughness; m_dirty = true; }
	void Material::setMetallic(float metallic) { m_data.Metallic = metallic; m_dirty = true; }
	void Material::setEmission(float emission) { m_data.Emission = emission; m_dirty = true; }

	void Material::setTexture(const std::string& slot, const Ref<Texture2D>& texture)
	{
		m_textures[slot] = texture;
	}

	Ref<Texture2D> Material::getTexture(const std::string& slot) const
	{
		auto it = m_textures.find(slot);
		return (it != m_textures.end()) ? it->second : nullptr;
	}

	void Material::uploadData() const
	{
		m_ubo->setData(&m_data, sizeof(MaterialData));
	}

	Ref<Material> Material::create(const Ref<Shader>& shader)
	{
		return CreateRef<Material>(shader);
	}

	// ============================================================
	//  MaterialInstance
	// ============================================================

	MaterialInstance::MaterialInstance(const Ref<Material>& baseMaterial)
		: m_baseMaterial(baseMaterial), m_overrideData(baseMaterial->getData())
	{
		m_ubo = UniformBuffer::create(sizeof(MaterialData), 2);
	}

	void MaterialInstance::bind(uint32_t uboBindingPoint) const
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

	void MaterialInstance::setAlbedo(const glm::vec3& albedo) { m_overrideData.Albedo = albedo; m_dirty = true; }
	void MaterialInstance::setRoughness(float roughness) { m_overrideData.Roughness = roughness; m_dirty = true; }
	void MaterialInstance::setMetallic(float metallic) { m_overrideData.Metallic = metallic; m_dirty = true; }

	void MaterialInstance::setTexture(const std::string& slot, const Ref<Texture2D>& texture)
	{
		m_textureOverrides[slot] = texture;
	}

	void MaterialInstance::uploadData() const
	{
		m_ubo->setData(&m_overrideData, sizeof(MaterialData));
	}

	Ref<MaterialInstance> MaterialInstance::create(const Ref<Material>& baseMaterial)
	{
		return CreateRef<MaterialInstance>(baseMaterial);
	}

}
