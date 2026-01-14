#include "Rongpch.h"
#include "SceneSerializer.h"

#include "Rongine/Scene/Entity.h"
#include "Rongine/Scene/Components.h"

// --- 引入 CAD 相关的头文件 (用于重建几何体) ---
#include "Rongine/CAD/CADModeler.h"
#include "Rongine/CAD/CADMesher.h"
#include "Rongine/CAD/CADImporter.h"
#include <TopoDS_Shape.hxx> 

#include <BRepTools.hxx> // OCCT BRep 读写工具
#include <filesystem>

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace YAML {

	// --- 辅助转换器：让 YAML 支持 glm::vec3 ---
	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	// --- 辅助转换器：让 YAML 支持 glm::vec4 ---
	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};
}

namespace Rongine {

	// --- 辅助输出运算符 ---
	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Context(scene)
	{
	}

	// =============================================================
	// 核心保存逻辑：序列化单个实体
	// =============================================================
	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		// 检查实体有效性
		if (!entity.HasComponent<IDComponent>()) // 假设你有 IDComponent，如果没有请用 uint32_t 强转
		{
			// 如果没有 ID 组件，至少确保它是个有效实体
		}

		out << YAML::BeginMap; // Entity Start

		// 序列化实体 ID (如果你还没有 IDComponent，这里简单转成整数存一下，虽然加载时不一定能复原同一个ID)
		out << YAML::Key << "Entity" << YAML::Value << (uint64_t)(uint32_t)entity;

		// 1. Tag 组件 (名字)
		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent Map

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent Map
		}

		// 2. Transform 组件 (变换)
		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent Map

			auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			out << YAML::EndMap; // TransformComponent Map
		}

		// 3. CAD Geometry 组件 (核心)
		if (entity.HasComponent<CADGeometryComponent>())
		{
			out << YAML::Key << "CADGeometryComponent";
			out << YAML::BeginMap; // CADGeometryComponent Map

			auto& cadComp = entity.GetComponent<CADGeometryComponent>();

			// 保存类型 (转为 int 存储)
			out << YAML::Key << "Type" << YAML::Value << (int)cadComp.Type;

			// 保存参数
			out << YAML::Key << "Params" << YAML::BeginMap;
			out << YAML::Key << "Width" << YAML::Value << cadComp.Params.Width;
			out << YAML::Key << "Height" << YAML::Value << cadComp.Params.Height;
			out << YAML::Key << "Depth" << YAML::Value << cadComp.Params.Depth;
			out << YAML::Key << "Radius" << YAML::Value << cadComp.Params.Radius;
			out << YAML::Key << "LinearDeflection" << YAML::Value << cadComp.LinearDeflection;
			out << YAML::EndMap; // Params Map

			bool needsBRepSave = (cadComp.Type == CADGeometryComponent::GeometryType::Imported);

			std::string brepFileName = "";

			if (needsBRepSave && cadComp.ShapeHandle)
			{
				uint64_t uuid = entity.GetComponent<IDComponent>().ID; 

				// 构造相对路径
				brepFileName = "assets/cache/" + std::to_string(uuid) + ".brep";

				// 确保目录存在
				std::filesystem::create_directories("assets/cache");

				// 调用 OCCT 保存文件
				TopoDS_Shape* shape = (TopoDS_Shape*)cadComp.ShapeHandle;
				if (!BRepTools::Write(*shape, brepFileName.c_str()))
				{
					RONG_CORE_ERROR("Failed to write BRep file: {0}", brepFileName);
				}
			}

			// 将路径写入 YAML
			out << YAML::Key << "BRepPath" << YAML::Value << brepFileName;

			out << YAML::EndMap; // CADGeometryComponent Map
		}

		out << YAML::EndMap; // Entity End
	}

	// =============================================================
	// 保存整个场景
	// =============================================================
	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		m_Context->getRegistry().each([&](auto entityID)
			{
				Entity entity = { entityID, m_Context.get() };
				if (!entity)
					return;

				SerializeEntity(out, entity);
			});

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	// =============================================================
	// 核心加载逻辑：从文件读取并重建场景
	// =============================================================
	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		RONG_CORE_TRACE("Deserializing scene '{0}'", sceneName);

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				// 1. 读取名字并创建实体
				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				Entity deserializedEntity = m_Context->createEntity(name);
				if (deserializedEntity.HasComponent<IDComponent>())
					deserializedEntity.GetComponent<IDComponent>().ID = uuid; // 恢复 UUID

				RONG_CORE_TRACE("Deserialized entity '{0}' (UUID: {1})", name, uuid);

				// 2. 加载 Transform
				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					tc.Translation = transformComponent["Translation"].as<glm::vec3>();
					tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					tc.Scale = transformComponent["Scale"].as<glm::vec3>();
				}

				// 3. 加载 CAD 组件并现场重建 (Rebuild)
				auto cadComponent = entity["CADGeometryComponent"];
				if (cadComponent)
				{
					auto& cadComp = deserializedEntity.AddComponent<CADGeometryComponent>();

					// A. 读取参数
					cadComp.Type = (CADGeometryComponent::GeometryType)cadComponent["Type"].as<int>();
					auto params = cadComponent["Params"];
					cadComp.Params.Width = params["Width"].as<float>();
					cadComp.Params.Height = params["Height"].as<float>();
					cadComp.Params.Depth = params["Depth"].as<float>();
					cadComp.Params.Radius = params["Radius"].as<float>();

					if (params["LinearDeflection"])
						cadComp.LinearDeflection = params["LinearDeflection"].as<float>();
					else
						cadComp.LinearDeflection = 0.1f;

					// 加载 BRep 文件
					void* shapeHandle = nullptr;
					std::string brepPath = "";
					if (cadComponent["BRepPath"])
						brepPath = cadComponent["BRepPath"].as<std::string>();

					// 优先尝试从文件加载 (针对拉伸、布尔运算后的物体)
					bool loadedFromDisk = false;
					if (!brepPath.empty() && std::filesystem::exists(brepPath))
					{
						BRep_Builder builder;
						TopoDS_Shape shape;
						if (BRepTools::Read(shape, brepPath.c_str(), builder))
						{
							shapeHandle = new TopoDS_Shape(shape);
							loadedFromDisk = true;
						}
						else
						{
							RONG_CORE_ERROR("Failed to load BRep file: {0}", brepPath);
						}
					}

					// 如果没从磁盘加载 (说明是纯参数化物体，或者文件丢失)，则尝试参数化重建
					if (!shapeHandle)
					{
						switch (cadComp.Type)
						{
						case CADGeometryComponent::GeometryType::Cube:
							shapeHandle = CADModeler::MakeCube(cadComp.Params.Width, cadComp.Params.Height, cadComp.Params.Depth);
							break;
						case CADGeometryComponent::GeometryType::Sphere:
							shapeHandle = CADModeler::MakeSphere(cadComp.Params.Radius);
							break;
						case CADGeometryComponent::GeometryType::Cylinder:
							shapeHandle = CADModeler::MakeCylinder(cadComp.Params.Radius, cadComp.Params.Height);
							break;
						}
					}

					cadComp.ShapeHandle = shapeHandle;
					// ========================================================================

					// C. 重新生成网格 (Mesh + Edge)
					if (shapeHandle)
					{
						TopoDS_Shape* occShape = (TopoDS_Shape*)shapeHandle;
						std::vector<CubeVertex> verticesData;

						// 1. 生成面网格
						auto va = CADMesher::CreateMeshFromShape(*occShape, verticesData, cadComp.LinearDeflection);

						if (va)
						{
							auto& meshComp = deserializedEntity.AddComponent<MeshComponent>(va, verticesData);

							// 2. 更新包围盒
							meshComp.BoundingBox = CADImporter::CalculateAABB(*occShape);

							std::vector<LineVertex> lineVerts;
							auto edgeVA = CADMesher::CreateEdgeMeshFromShape(
								*occShape,
								lineVerts,
								meshComp.m_IDToEdgeMap, // 恢复 ID 映射表，确保能被选中
								cadComp.LinearDeflection
							);
							meshComp.EdgeVA = edgeVA;
							meshComp.LocalLines = lineVerts;
							// =================================================================
						}
					}
				}
			}
		}

		return true;
	}

}