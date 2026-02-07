#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Rongine/Renderer/VertexArray.h" // 包含你的 VertexArray
#include "Rongine/Renderer/RenderTypes.h"

#include <TopoDS_Edge.hxx>
#include <gp_Ax3.hxx> // OCCT 的坐标系类

#include "entt.hpp"

class TopoDS_Shape;

namespace Rongine {

    struct SketchLine
    {
        glm::vec3 P0;
        glm::vec3 P1;
        glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct LineVertex
    {
        glm::vec3 Position;
        int EntityID; // 存 EdgeID，复用 EntityID 这个名字传入 Shader
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

    struct SketchComponent
    {
        bool IsActive = false;
        gp_Ax3 PlaneLocalSystem; // OCCT 的局部坐标系 (原点, 法线, X轴)
        glm::mat4 SketchMatrix;  // 转好的 GLM 矩阵，方便渲染

        std::vector<SketchLine> Lines;

        SketchComponent() = default;
        SketchComponent(const SketchComponent&) = default;
    };

    struct TransformComponent
    {
        glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f }; // 欧拉角 (弧度)
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

    struct MeshComponent
    {
        Ref<VertexArray> VA;
        AABB BoundingBox;

        Ref<VertexArray> EdgeVA;             // 边的显存对象

        std::vector<LineVertex> LocalLines;  // 边的内存数据 (用于做 CPU 射线检测)
        std::vector<CubeVertex> LocalVertices;
        std::vector<uint32_t> LocalIndices;//索引数据

        std::map<int, TopoDS_Edge> m_IDToEdgeMap;

        MeshComponent() = default;
        MeshComponent(const MeshComponent&) = default;
        MeshComponent(const Ref<VertexArray>& va) : VA(va) {}
        MeshComponent(const Ref<VertexArray>& va, const std::vector<CubeVertex>& verts)
            : VA(va), LocalVertices(verts) {
        }
        MeshComponent(const Ref<VertexArray>& va, const std::vector<CubeVertex>& verts,const std::vector<uint32_t> indices)
            : VA(va), LocalVertices(verts),LocalIndices(indices) {
        }
    };

    struct SpectralMaterialComponent
    {
        enum class MaterialType {
            Diffuse = 0,    // 绝缘体/非金属: Slot0=Reflectance (颜色), Slot1=未使用(或存常数IOR)
            Conductor = 1,  // 导体/金属:   Slot0=n (折射率实部), Slot1=k (消光系数)
            Dielectric = 2  // 透明/玻璃:   Slot0=Transmission (透射色), Slot1=IOR (折射率曲线)
        };

        std::string Name;
        MaterialType Type = MaterialType::Diffuse;

        // --- 数据存储区 (复用内存) ---
        // Slot 0: 
        //    - Diffuse: Reflectance
        //    - Conductor: n
        //    - Dielectric: Transmission
        std::vector<float> SpectrumSlot0;

        // Slot 1:
        //    - Conductor: k
        //    - Dielectric: IOR (用于色散)
        std::vector<float> SpectrumSlot1;

        // 对应的 GPU 缓冲区索引 (由 Renderer3D 填充)
        int GpuBufferIndex0 = -1;
        int GpuBufferIndex1 = -1;

        SpectralMaterialComponent() = default;
        SpectralMaterialComponent(const std::string& name) : Name(name) {}
    };

    struct CADGeometryComponent
    {
        enum class GeometryType { None = 0, Cube, Sphere, Cylinder, Imported, Spline };

        GeometryType Type = GeometryType::None;

        // --- 核心数据 ---
        // 我们使用 void* 来存储 TopoDS_Shape*，避免引入 OCCT 头文件依赖
        // 实际上它指向的是 new TopoDS_Shape()
        void* ShapeHandle = nullptr;

        // --- 参数化数据 (为以后的序列化做准备) ---
        struct {
            float Width = 1.0f, Height = 1.0f, Depth = 1.0f; // 立方体参数
            float Radius = 1.0f; // 球体/圆柱参数
        } Params;

        float LinearDeflection = 0.1f;//精度
        float FilletRadius = 0.1f; // 倒角的半径

        //NURBS
        std::vector<CADControlPoint> SplinePoints;
        int SplineDegree = 3;       // 阶数
        bool SplineClosed = false;  // 是否闭合

        //父亲
        entt::entity SourceEntity = entt::null;

        CADGeometryComponent() = default;
        CADGeometryComponent(const CADGeometryComponent&) = default;
    };

    struct MaterialComponent
    {
        glm::vec3 Albedo = { 1.0f, 1.0f, 1.0f }; // 颜色 (RGB)
        float Roughness = 0.5f;                  // 粗糙度 (0.0 = 镜面, 1.0 = 哑光)
        float Metallic = 0.0f;                   // 金属度 (0.0 = 塑料, 1.0 = 金属)
    };
}