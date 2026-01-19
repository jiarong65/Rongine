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
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepTools.hxx>

namespace Rongine {

    Ref<VertexArray> CADMesher::CreateMeshFromShape(const TopoDS_Shape& shape, std::vector<CubeVertex>& outVertices, std::vector<uint32_t>& outIndices, float deflection)
    {
        // 0. 清空传入的容器，确保数据干净
        outVertices.clear();
        outIndices.clear(); 

        // 1. 离散化 (Meshing)
        BRepMesh_IncrementalMesh mesher(shape, deflection);

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
                // ========================================================
                // 获取面的方向
                // 如果是 REVERSED，说明几何法线与逻辑法线相反，需要翻转顶点顺序
                // ========================================================
                bool isReversed = (face.Orientation() == TopAbs_REVERSED);

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
                    v.Normal = { 0.0f, 1.0f, 0.0f }; // 简单默认法线
                    v.Color = { 0.8f, 0.8f, 0.8f, 1.0f };
                    v.TexCoord = { 0.0f, 0.0f };
                    v.TexIndex = 0.0f;
                    v.TilingFactor = 1.0f;
                    v.FaceID = faceID;

                    outVertices.push_back(v);
                }

                // --- B. 提取三角形索引 ---
                for (int i = 1; i <= triangleCount; i++)
                {
                    const Poly_Triangle& tri = triangulation->Triangle(i);

                    int n1, n2, n3;
                    tri.Get(n1, n2, n3);

                    // OCCT 的索引是从 1 开始的，我们需要减 1 变成从 0 开始
                    uint32_t idx1 = currentFaceOffset + (n1 - 1);
                    uint32_t idx2 = currentFaceOffset + (n2 - 1);
                    uint32_t idx3 = currentFaceOffset + (n3 - 1);

                    // ========================================================
                    // 根据 Orientation 调整顶点写入顺序
                    // ========================================================
                    if (isReversed)
                    {
                        // 如果是反向面，交换 2 和 3 的顺序 (1-3-2)
                        // 存入 outIndices
                        outIndices.push_back(idx1);
                        outIndices.push_back(idx3);
                        outIndices.push_back(idx2);
                    }
                    else
                    {
                        // 正常顺序 (1-2-3)
                        outIndices.push_back(idx1);
                        outIndices.push_back(idx2);
                        outIndices.push_back(idx3);
                    }
                }

                indexOffset += nodeCount;
            }
            explorer.Next();
            faceID++;
        }

        // 3. 创建 OpenGL 资源
        if (outVertices.empty()) return nullptr;

        Ref<VertexArray> va = VertexArray::create();

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

        // 使用 outIndices 创建索引缓冲
        Ref<IndexBuffer> ib = IndexBuffer::create(outIndices.data(), (uint32_t)outIndices.size());
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

            //entity.GetComponent<MeshComponent>().m_IDToEdgeMap[edgeID] = edge;

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

    // 带 Entity 版本的重载 (负责建立映射)
    Ref<VertexArray> CADMesher::CreateEdgeMeshFromShape(Entity entity, const TopoDS_Shape& shape, std::vector<LineVertex>& outLines, float deflection)
    {
        outLines.clear();
        auto& mesh = entity.GetComponent<MeshComponent>();

        // 1. 确保 Map 是空的
        mesh.m_IDToEdgeMap.clear();

        int edgeID = 0;
        TopExp_Explorer explorer(shape, TopAbs_EDGE);
        while (explorer.More())
        {
            const TopoDS_Edge& edge = TopoDS::Edge(explorer.Current());

            // 2. [关键] 记录 ID -> Edge 映射
            mesh.m_IDToEdgeMap[edgeID] = edge;

            BRepAdaptor_Curve curveAdaptor(edge);
            GCPnts_TangentialDeflection discretizer;
            discretizer.Initialize(curveAdaptor, deflection, 0.1);

            int nPoints = discretizer.NbPoints();
            if (nPoints > 1)
            {
                for (int i = 1; i < nPoints; i++)
                {
                    gp_Pnt p1 = discretizer.Value(i);
                    gp_Pnt p2 = discretizer.Value(i + 1);

                    outLines.push_back({ {(float)p1.X(), (float)p1.Y(), (float)p1.Z()}, edgeID });
                    outLines.push_back({ {(float)p2.X(), (float)p2.Y(), (float)p2.Z()}, edgeID });
                }
            }
            explorer.Next();
            edgeID++;
        }

        if (outLines.empty()) return nullptr;

        Ref<VertexArray> va = VertexArray::create();
        Ref<VertexBuffer> vb = VertexBuffer::create((float*)outLines.data(), (uint32_t)(outLines.size() * sizeof(LineVertex)));
        vb->setLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Int,    "a_EntityID" }
            });
        va->addVertexBuffer(vb);
        return va;
    }

    //传入引用
    Ref<VertexArray> CADMesher::CreateEdgeMeshFromShape(
        const TopoDS_Shape& shape,
        std::vector<LineVertex>& outLines,
        std::map<int, TopoDS_Edge>& outEdgeMap, // <--- 接收 Map
        float deflection)
    {
        outLines.clear();
        outEdgeMap.clear(); // 1. 务必先清空，防止残留旧数据

        int edgeID = 0; // 这里的 ID 必须从 0 开始，和 Shader/Pixel 读取逻辑一致

        // 2. 遍历所有边
        TopExp_Explorer explorer(shape, TopAbs_EDGE);
        while (explorer.More())
        {
            const TopoDS_Edge& edge = TopoDS::Edge(explorer.Current());

            // 3. 【核心】建立映射关系：ID -> Edge
            outEdgeMap[edgeID] = edge;

            // --- 下面是原本的离散化逻辑 ---
            BRepAdaptor_Curve curveAdaptor(edge);
            GCPnts_TangentialDeflection discretizer;
            discretizer.Initialize(curveAdaptor, deflection, 0.1);

            int nPoints = discretizer.NbPoints();
            if (nPoints > 1)
            {
                for (int i = 1; i < nPoints; i++)
                {
                    gp_Pnt p1 = discretizer.Value(i);
                    gp_Pnt p2 = discretizer.Value(i + 1);

                    // 写入顶点数据，注意这里写入了 edgeID
                    LineVertex v1, v2;
                    v1.Position = { (float)p1.X(), (float)p1.Y(), (float)p1.Z() };
                    v1.EntityID = edgeID;

                    v2.Position = { (float)p2.X(), (float)p2.Y(), (float)p2.Z() };
                    v2.EntityID = edgeID;

                    outLines.push_back(v1);
                    outLines.push_back(v2);
                }
            }
            // -----------------------------

            explorer.Next();
            edgeID++; // ID 自增
        }

        if (outLines.empty()) return nullptr;

        // 创建 VA/VB ...
        Ref<VertexArray> va = VertexArray::create();
        Ref<VertexBuffer> vb = VertexBuffer::create((float*)outLines.data(), (uint32_t)(outLines.size() * sizeof(LineVertex)));
        vb->setLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Int,    "a_EntityID" }
            });
        va->addVertexBuffer(vb);
        return va;
    }

    void CADMesher::ApplyFillet(Entity entity, int edgeID, float radius)
    {
        // 1. 基础组件检查
        if (!entity.HasComponent<CADGeometryComponent>() || !entity.HasComponent<MeshComponent>())
            return;

        auto& cad = entity.GetComponent<CADGeometryComponent>();
        auto& mesh = entity.GetComponent<MeshComponent>();

        TopoDS_Shape* currentShape = static_cast<TopoDS_Shape*>(cad.ShapeHandle);
        if (!currentShape || currentShape->IsNull()) {
            RONG_CORE_ERROR("ApplyFillet: Invalid Shape Handle.");
            return;
        }

        // 2. 参数检查
        if (edgeID < 0) {
            RONG_CORE_WARN("ApplyFillet: Invalid Edge ID (-1).");
            return;
        }
        if (radius <= 0.001f) {
            RONG_CORE_WARN("ApplyFillet: Radius is too small or negative.");
            return;
        }

        // 3. 从 Map 中查找边
        if (mesh.m_IDToEdgeMap.find(edgeID) == mesh.m_IDToEdgeMap.end())
        {
            RONG_CORE_WARN("ApplyFillet: Edge ID {0} not found in map! (Map size: {1})", edgeID, mesh.m_IDToEdgeMap.size());
            return;
        }

        // 获取 Map 中记录的边 (这可能是一个过期的拓扑引用)
        TopoDS_Edge targetEdge = mesh.m_IDToEdgeMap[edgeID];

        // =========================================================================================
        // 安全性检查：验证这条边是否真的属于当前的 Shape
        // 防止因为 Map 数据未及时更新，或者拓扑结构改变(如连续倒角)导致的野指针崩溃
        // =========================================================================================
        bool isEdgeValid = false;
        TopExp_Explorer checkExp(*currentShape, TopAbs_EDGE);
        while (checkExp.More())
        {
            // IsSame 检查底层的 TShape 指针是否一致
            if (checkExp.Current().IsSame(targetEdge))
            {
                isEdgeValid = true;
                break;
            }
            checkExp.Next();
        }

        if (!isEdgeValid)
        {
            RONG_CORE_ERROR("CRITICAL: Safety Check Failed! Edge {0} does not belong to current shape topology.", edgeID);
            RONG_CORE_WARN("Please re-select the edge.");
            return; // 强制返回，避免崩溃
        }

        // 4. 执行倒角操作
        try
        {
            // OCC_CATCH_SIGNALS; 

            BRepFilletAPI_MakeFillet filletMaker(*currentShape);
            filletMaker.Add(radius, targetEdge);
            filletMaker.Build();

            if (filletMaker.IsDone())
            {
                // A. 获取新形状
                TopoDS_Shape newShape = filletMaker.Shape();

                // B. 检查结果是否有效 (防止生成空形状)
                if (newShape.IsNull()) {
                    RONG_CORE_ERROR("Fillet result is null.");
                    return;
                }

                // C. 更新 CAD 组件数据
                // 最好删除旧的 shape 指针避免内存泄漏
                // delete currentShape; 
                *currentShape = newShape; // 或者直接覆盖内容

                // D. 调用重建函数刷新渲染 (这会清空 Map 并重新生成 ID)
                RebuildMesh(entity);

                RONG_CORE_INFO("Success: Fillet applied to Edge {0} with Radius {1}", edgeID, radius);
            }
            else
            {
                // 获取具体错误状态
                RONG_CORE_WARN("Hint: Radius might be too large for this geometry.");
            }
        }
        catch (Standard_Failure& e) // 捕获 OCCT 标准错误
        {
            RONG_CORE_ERROR("OCCT Fatal Error: {0}", e.GetMessageString());
        }
        catch (const std::exception& e) // 捕获 C++ 标准异常
        {
            RONG_CORE_ERROR("C++ Exception: {0}", e.what());
        }
        catch (...) // 捕获未知错误
        {
            RONG_CORE_ERROR("Unknown Crash occurred during Fillet operation.");
        }
    }


    // 重建
    void CADMesher::RebuildMesh(Entity entity)
    {
        if (!entity.HasComponent<CADGeometryComponent>() || !entity.HasComponent<MeshComponent>())
            return;

        auto& cad = entity.GetComponent<CADGeometryComponent>();
        auto& mesh = entity.GetComponent<MeshComponent>();

        // 获取最新的几何形状
        TopoDS_Shape* shapePtr = static_cast<TopoDS_Shape*>(cad.ShapeHandle);
        if (!shapePtr || shapePtr->IsNull()) return;
        const TopoDS_Shape& shape = *shapePtr;

        // 1. [关键] 清理 OCCT 内部的网格缓存
        // 如果不清理，BRepMesh 会以为形状没变，直接复用旧的网格，导致画面不更新
        BRepTools::Clean(shape);

        // 2. 清理我们自己的 ID 映射表，准备重新填入
        mesh.m_IDToEdgeMap.clear();
        mesh.LocalLines.clear();

        // 3. 重新生成面片网格 (Face Mesh)
        std::vector<CubeVertex> newVertices;
        std::vector<uint32_t> newIndices;
        // 使用组件里存的精度参数
        mesh.VA = CreateMeshFromShape(shape, newVertices, newIndices, cad.LinearDeflection);
        mesh.LocalVertices = newVertices; // 更新 CPU 端数据供拾取/Gizmo使用
        mesh.LocalIndices = newIndices;

        // 4. 重新生成边框网格 (Edge Mesh) 并重建 m_IDToEdgeMap
        // 注意：CreateEdgeMeshFromShape 内部会填充 m_IDToEdgeMap
        mesh.EdgeVA = CreateEdgeMeshFromShape(entity, shape, mesh.LocalLines, cad.LinearDeflection);

        RONG_CORE_INFO("Rebuild Complete. Mapped {0} Edges.", mesh.m_IDToEdgeMap.size());
    }


}