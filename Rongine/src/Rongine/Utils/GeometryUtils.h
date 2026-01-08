#pragma once
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Renderer3D.h" // 获取 CubeVertex 定义
#include <vector>
#include <cmath>

namespace Rongine {

    class GeometryUtils {
    public:
        // majorRadius: 大环半径, minorRadius: 管子半径
        static Ref<VertexArray> CreateTorus(float majorRadius, float minorRadius, int majorSegments, int minorSegments) {
            std::vector<CubeVertex> vertices;
            std::vector<uint32_t> indices;

            const float PI = 3.14159265359f;

            for (int i = 0; i <= majorSegments; ++i) {
                float u = (float)i / majorSegments * 2.0f * PI;
                float cosU = cos(u);
                float sinU = sin(u);

                for (int j = 0; j <= minorSegments; ++j) {
                    float v = (float)j / minorSegments * 2.0f * PI;
                    float cosV = cos(v);
                    float sinV = sin(v);

                    // 1. 计算位置
                    float x = (majorRadius + minorRadius * cosV) * cosU;
                    float y = (majorRadius + minorRadius * cosV) * sinU;
                    float z = minorRadius * sinV;

                    // 2. 计算法线 (关键：管子表面点减去管子中心点)
                    float centerX = majorRadius * cosU;
                    float centerY = majorRadius * sinU;
                    float centerZ = 0.0f;
                    glm::vec3 normal = glm::normalize(glm::vec3(x - centerX, y - centerY, z - centerZ));

                    // 3. 填充顶点数据
                    CubeVertex vertex;
                    vertex.Position = { x, y, z };
                    vertex.Normal = normal;
                    vertex.Color = { 1.0f, 0.4f, 0.4f, 1.0f }; // 默认骚粉色，方便看高光
                    vertex.TexCoord = { (float)i / majorSegments, (float)j / minorSegments };
                    vertex.TexIndex = 0.0f; // 使用白纹理
                    vertex.TilingFactor = 1.0f;
                    vertex.FaceID = -1;

                    vertices.push_back(vertex);
                }
            }

            // 4. 生成索引
            for (int i = 0; i < majorSegments; ++i) {
                for (int j = 0; j < minorSegments; ++j) {
                    int stride = minorSegments + 1;
                    int nextI = (i + 1);
                    // 最后一个环要连回第一个环的顶点数据（如果顶点没重复生成，这里需要模运算，但上面循环是 <=，所以数据是重复的，直接取即可）

                    uint32_t v0 = i * stride + j;
                    uint32_t v1 = nextI * stride + j;
                    uint32_t v2 = i * stride + j + 1;
                    uint32_t v3 = nextI * stride + j + 1;

                    // 逆时针顺序
                    indices.push_back(v0);
                    indices.push_back(v1);
                    indices.push_back(v2);

                    indices.push_back(v2);
                    indices.push_back(v1);
                    indices.push_back(v3);
                }
            }

            Ref<VertexArray> va = VertexArray::create();
            Ref<VertexBuffer> vb = VertexBuffer::create((float*)vertices.data(), vertices.size() * sizeof(CubeVertex));
            vb->setLayout({
                { ShaderDataType::Float3, "a_Position" },
                { ShaderDataType::Float3, "a_Normal" },
                { ShaderDataType::Float4, "a_Color" },
                { ShaderDataType::Float2, "a_TexCoord" },
                { ShaderDataType::Float,  "a_TexIndex" },
                { ShaderDataType::Float,  "a_TilingFactor" },
                { ShaderDataType::Int,    "a_FaceID" }
                });
            va->addVertexBuffer(vb);
            Ref<IndexBuffer> ib = IndexBuffer::create(indices.data(), indices.size());
            va->setIndexBuffer(ib);

            return va;
        }
    };
}