#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Rongine/Renderer/VertexArray.h" // 包含你的 VertexArray

namespace Rongine {

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

        MeshComponent() = default;
        MeshComponent(const MeshComponent&) = default;
        MeshComponent(const Ref<VertexArray>& va) : VA(va) {}
    };

    // 为未来预留的光谱组件
    struct SpectralMaterialComponent
    {
        int MaterialID = -1;
        // std::string H5FilePath; // 以后可能是这个
    };
}