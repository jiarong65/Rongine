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
#include <TopoDS_Edge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <CADFeature.cpp>

namespace Rongine {

    Ref<VertexArray> CADMesher::CreateMeshFromShape(const TopoDS_Shape& shape, std::vector<CubeVertex>& outVertices, float deflection )
    {
        // 0. 清空传入的容器，确保数据干净
        outVertices.clear();

        // 1. 离散化 (Meshing)
        // deflection (0.1) 控制网格精度，越小越平滑
        BRepMesh_IncrementalMesh mesher(shape, deflection);

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

    Ref<VertexArray> CADMesher::CreateEdgeMeshFromShape(const TopoDS_Shape& shape, std::vector<LineVertex>& outLines, float deflection)
    {
        outLines.clear();
        int edgeID = 0; // 给每条边编个号，方便以后拾取

        // 1. 遍历所有的边 (Edge)
        TopExp_Explorer explorer(shape, TopAbs_EDGE);
        while (explorer.More())
        {
            const TopoDS_Edge& edge = TopoDS::Edge(explorer.Current());

            // 2. 将边离散化为线段
            // BRepAdaptor_Curve 用于获取边的几何属性
            BRepAdaptor_Curve curveAdaptor(edge);

            // GCPnts_TangentialDeflection: 基于切向偏差的离散化算法
            // deflection: 线性偏差 (控制光滑度)
            // angularDeflection: 角度偏差 (控制转角处的精度)，这里给 0.1弧度 (约5.7度)
            GCPnts_TangentialDeflection discretizer;
            discretizer.Initialize(curveAdaptor, deflection, 0.1);

            int nPoints = discretizer.NbPoints();

            // 3. 构建线段 (GL_LINES 模式：点1-点2, 点2-点3...)
            // 也就是每两个点构成一条独立的线段
            if (nPoints > 1)
            {
                for (int i = 1; i < nPoints; i++)
                {
                    gp_Pnt p1 = discretizer.Value(i);
                    gp_Pnt p2 = discretizer.Value(i + 1);

                    // 存入第一个点
                    LineVertex v1;
                    v1.Position = { (float)p1.X(), (float)p1.Y(), (float)p1.Z() };
                    v1.EntityID = edgeID; // 存入边ID
                    outLines.push_back(v1);

                    // 存入第二个点
                    LineVertex v2;
                    v2.Position = { (float)p2.X(), (float)p2.Y(), (float)p2.Z() };
                    v2.EntityID = edgeID; // 存入边ID
                    outLines.push_back(v2);
                }
            }

            explorer.Next();
            edgeID++;
        }

        if (outLines.empty()) return nullptr;

        // 4. 创建 OpenGL 资源
        Ref<VertexArray> va = VertexArray::create();
        Ref<VertexBuffer> vb = VertexBuffer::create((float*)outLines.data(), (uint32_t)(outLines.size() * sizeof(LineVertex)));

        // 布局必须匹配 Shader (pos: vec3, entityID: int)
        vb->setLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Int,    "a_EntityID" }
            });

        va->addVertexBuffer(vb);

        // 线框渲染通常不需要 IndexBuffer，直接 drawArrays 即可
        return va;
    }

}