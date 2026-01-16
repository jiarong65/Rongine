#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Rongine/Renderer/VertexArray.h" // 包含你的 VertexArray
#include "Rongine/Renderer/Renderer3D.h"

#include <TopoDS_Edge.hxx>
#include <gp_Ax3.hxx> // OCCT 的坐标系类

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

    // AABB 结构体
    struct AABB {
        glm::vec3 Min = { 0.0f, 0.0f, 0.0f };
        glm::vec3 Max = { 0.0f, 0.0f, 0.0f };

        glm::vec3 GetCenter() const { return (Min + Max) * 0.5f; }
        glm::vec3 GetSize() const { return Max - Min; }
    };

    struct MeshComponent
    {
        Ref<VertexArray> VA;
        AABB BoundingBox;

        Ref<VertexArray> EdgeVA;             // 边的显存对象

        std::vector<LineVertex> LocalLines;  // 边的内存数据 (用于做 CPU 射线检测)
        std::vector<CubeVertex> LocalVertices;

        std::map<int, TopoDS_Edge> m_IDToEdgeMap;

        MeshComponent() = default;
        MeshComponent(const MeshComponent&) = default;
        MeshComponent(const Ref<VertexArray>& va) : VA(va) {}
        MeshComponent(const Ref<VertexArray>& va, const std::vector<CubeVertex>& verts)
            : VA(va), LocalVertices(verts) {
        }
    };

    // 为未来预留的光谱组件
    struct SpectralMaterialComponent
    {
        int MaterialID = -1;
        // std::string H5FilePath; // 以后可能是这个
    };

    struct CADGeometryComponent
    {
        enum class GeometryType { None = 0, Cube, Sphere, Cylinder, Imported };

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

        CADGeometryComponent() = default;
        CADGeometryComponent(const CADGeometryComponent&) = default;
    };
}