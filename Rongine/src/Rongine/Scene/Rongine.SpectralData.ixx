module;
#include <string>
#include <vector>
#include <glm/glm.hpp>

export module Rongine.SpectralData;

export import Rongine.SceneData;

export namespace Rongine {

	struct SpectralPreset
	{
		std::string Name;

		SpectralMaterialComponent::MaterialType Type = SpectralMaterialComponent::MaterialType::Diffuse;

		std::vector<float> Slot0; // Reflectance / n / Transmission
		std::vector<float> Slot1; // Unused / k / IOR

		glm::vec3 PreviewColor;   // UI preview
	};
}
