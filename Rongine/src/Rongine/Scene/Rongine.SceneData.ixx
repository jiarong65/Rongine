module;
#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#include <cstdint>
#include <string>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <gp_Ax3.hxx>

export module Rongine.SceneData;

export namespace Rongine {

	struct SketchLine
	{
		glm::vec3 P0;
		glm::vec3 P1;
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct SketchComponent
	{
		bool IsActive = false;
		gp_Ax3 PlaneLocalSystem; // OCCT local coordinate system
		glm::mat4 SketchMatrix;

		std::vector<SketchLine> Lines;

		SketchComponent() = default;
		SketchComponent(const SketchComponent&) = default;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		int EntityID; // Edge ID, reused as EntityID when uploaded to shader
	};

	struct IDComponent
	{
		uint64_t ID = 0;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
		IDComponent(uint64_t id) : ID(id) {}
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag) : Tag(tag) {}
	};

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f }; // Euler angles (radians)
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation) : Translation(translation) {}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
			return glm::translate(glm::mat4(1.0f), Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	struct MaterialComponent
	{
		glm::vec3 Albedo = { 1.0f, 1.0f, 1.0f }; // RGB
		float Roughness = 0.5f;                  // 0.0 = smooth, 1.0 = rough
		float Metallic = 0.0f;                   // 0.0 = dielectric, 1.0 = metal
	};

	// 方向光（太阳）。Direction 是光的传播方向（从光源指向场景），着色时取反得到 L。
	struct DirectionalLightComponent
	{
		glm::vec3 Direction = glm::normalize(glm::vec3(-1.0f, -1.5f, -0.6f));
		glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
		float Intensity = 2.0f;
		float LightSize = 0.5f;      // 光源的物理尺寸（世界单位），决定 PCSS 半影宽度
		bool CastShadows = true;
	};

	struct SpectralMaterialComponent
	{
		enum class MaterialType {
			Diffuse = 0,    // insulator/non-metal: Slot0=Reflectance, Slot1=unused (or constant IOR)
			Conductor = 1,  // metal:              Slot0=n, Slot1=k
			Dielectric = 2  // glass:              Slot0=Transmission, Slot1=IOR
		};

		std::string Name;
		MaterialType Type = MaterialType::Diffuse;

		std::vector<float> SpectrumSlot0;
		std::vector<float> SpectrumSlot1;

		int GpuBufferIndex0 = -1;
		int GpuBufferIndex1 = -1;

		SpectralMaterialComponent() = default;
		SpectralMaterialComponent(const std::string& name) : Name(name) {}
	};
}
