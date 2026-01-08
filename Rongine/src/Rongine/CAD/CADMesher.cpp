#include "Rongpch.h"
#include "CADMesher.h"

#include "Rongine/Renderer/Renderer3D.h" // 获取 CubeVertex 定义
#include "Rongine/Renderer/Buffer.h"

// --- OCCT 算法头文件 (只在 cpp 中包含，加快编译) ---
#include <TopoDS.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Face.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <Poly_Array1OfTriangle.hxx>

namespace Rongine {

    Ref<VertexArray> CADMesher::CreateMeshFromShape(const TopoDS_Shape& shape, std::vector<CubeVertex>& outVertices)
    {
        // 0. 清空传入的容器，确保数据干净
        outVertices.clear();

        // 1. 离散化 (Meshing)
        // deflection (0.1) 控制网格精度，越小越平滑
        BRepMesh_IncrementalMesh mesher(shape, 0.1);

        // 注意：vertices 变量删除了，直接使用 outVertices
        std::vector<uint32_t> indices;
        uint32_t indexOffset = 0;

        int faceID = 0;
        // 2. 遍历所有的面 (Face)
        TopExp_Explorer explorer(shape, TopAbs_FACE);
        while (explorer.More())
        {
            const TopoDS_Face& face = TopoDS::Face(explorer.Current());
            TopLoc_Location location;

            // 获取三角网格数据
            Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);

            if (!triangulation.IsNull())
            {
                gp_Trsf trsf = location.Transformation();

                int nodeCount = triangulation->NbNodes();
                int triangleCount = triangulation->NbTriangles();

                // 记录当前面的顶点偏移量
                uint32_t currentFaceOffset = indexOffset;

                // --- A. 提取顶点 ---
                for (int i = 1; i <= nodeCount; i++)
                {
                    gp_Pnt p = triangulation->Node(i).Transformed(trsf);

                    CubeVertex v;
                    v.Position = { (float)p.X(), (float)p.Y(), (float)p.Z() };

                    // TODO: 法线计算优化
                    v.Normal = { 0.0f, 1.0f, 0.0f };

                    v.Color = { 0.8f, 0.8f, 0.8f, 1.0f };
                    v.TexCoord = { 0.0f, 0.0f };
                    v.TexIndex = 0.0f;
                    v.TilingFactor = 1.0f;
                    v.FaceID = faceID;

                    // 【修改】推入到传入的 outVertices 中
                    outVertices.push_back(v);
                }

                // --- B. 提取三角形索引 ---
                for (int i = 1; i <= triangleCount; i++)
                {
                    const Poly_Triangle& tri = triangulation->Triangle(i);

                    int n1, n2, n3;
                    tri.Get(n1, n2, n3);

                    indices.push_back(currentFaceOffset + (n1 - 1));
                    indices.push_back(currentFaceOffset + (n2 - 1));
                    indices.push_back(currentFaceOffset + (n3 - 1));
                }

                indexOffset += nodeCount;
            }
            explorer.Next();

            faceID++;
        }

        // 3. 创建 OpenGL 资源
        // 【修改】检查 outVertices 是否为空
        if (outVertices.empty()) return nullptr;

        Ref<VertexArray> va = VertexArray::create();

        // 创建 VertexBuffer
        // 【修改】使用 outVertices 的数据指针和大小
        Ref<VertexBuffer> vb = VertexBuffer::create((float*)outVertices.data(), (uint32_t)(outVertices.size() * sizeof(CubeVertex)));

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

        Ref<IndexBuffer> ib = IndexBuffer::create(indices.data(), (uint32_t)indices.size());
        va->setIndexBuffer(ib);

        return va;
    }

}