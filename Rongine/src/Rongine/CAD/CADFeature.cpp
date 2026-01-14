#include "Rongpch.h"
#include "CADFeature.h"

// --- OCCT 头文件 ---
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Surface.hxx>
#include <GeomLProp_SLProps.hxx>
#include <BRepTools.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <BRepFilletAPI_MakeFillet.hxx>

#include "Rongine/Core/Log.h"

namespace Rongine {

    // --- 内部辅助：根据索引找到 Face ---
    static TopoDS_Face GetFaceByIndex(const TopoDS_Shape& shape, int index)
    {
        int current = 0;
        TopExp_Explorer explorer(shape, TopAbs_FACE);
        while (explorer.More())
        {
            if (current == index)
            {
                return TopoDS::Face(explorer.Current());
            }
            current++;
            explorer.Next();
        }
        return TopoDS_Face(); // 返回空面
    }

    // --- 内部辅助：计算面的中心法线 ---
    static gp_Vec GetFaceNormal(const TopoDS_Face& face)
    {
        Handle(Geom_Surface) surface = BRep_Tool::Surface(face);

        // 1. 直接获取参数范围
        double umin, umax, vmin, vmax;
        BRepTools::UVBounds(face, umin, umax, vmin, vmax);

        // 2. 取参数中心
        // 对于圆柱，U范围是[0, 2PI]，中点 PI 刚好在背面，是非常合法的点
        double centerU = (umin + umax) * 0.5;
        double centerV = (vmin + vmax) * 0.5;

        // 3. 计算法线
        GeomLProp_SLProps props(surface, centerU, centerV, 1, 1e-6);

        if (props.IsNormalDefined())
        {
            gp_Dir normal = props.Normal();

            // 考虑面的方向
            if (face.Orientation() == TopAbs_REVERSED)
            {
                normal.Reverse();
            }
            return gp_Vec(normal);
        }

        return gp_Vec(0, 1, 0); // 兜底
    }

    void* CADFeature::ExtrudeFace(void* shapeHandle, int faceIndex, float height)
    {
        if (!shapeHandle || faceIndex < 0) return nullptr;

        TopoDS_Shape* mainShape = (TopoDS_Shape*)shapeHandle;

        // 1. 找到对应的面
        TopoDS_Face face = GetFaceByIndex(*mainShape, faceIndex);
        if (face.IsNull())
        {
            RONG_CORE_ERROR("Extrude: Face ID {0} not found!", faceIndex);
            return nullptr;
        }

        try
        {
            // 2. 计算拉伸向量 (法线 * 高度)
            gp_Vec normal = GetFaceNormal(face);
            gp_Vec extrudeVec = normal * height;

            // 3. 执行拉伸 (Make Prism)
            // 这会生成一个新的实体 (Solid)，底面是我们选中的面
            BRepPrimAPI_MakePrism prism(face, extrudeVec);
            prism.Build();

            if (prism.IsDone())
            {
                // 返回新的 Shape 指针
                return new TopoDS_Shape(prism.Shape());
            }
        }
        catch (...)
        {
            RONG_CORE_ERROR("OCCT Extrude Failed!");
        }

        return nullptr;
    }
    glm::mat4 CADFeature::GetFaceTransform(void* shapeHandle, int faceIndex)
    {
        if (!shapeHandle || faceIndex < 0) return glm::mat4(1.0f);

        TopoDS_Shape* mainShape = (TopoDS_Shape*)shapeHandle;
        TopoDS_Face face = GetFaceByIndex(*mainShape, faceIndex);
        if (face.IsNull()) return glm::mat4(1.0f);

        // 1. 获取中心点和法线
        // (这里我们复用之前的逻辑，稍微整理一下代码或者重写一遍)
        Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
        double umin, umax, vmin, vmax;
        BRepTools::UVBounds(face, umin, umax, vmin, vmax);
        double centerU = (umin + umax) * 0.5;
        double centerV = (vmin + vmax) * 0.5;

        gp_Pnt p;
        gp_Vec d1u, d1v;
        surface->D1(centerU, centerV, p, d1u, d1v);

        gp_Vec normal = d1u.Crossed(d1v);
        if (normal.Magnitude() > 1e-7) normal.Normalize();
        else normal = gp_Vec(0, 0, 1);

        if (face.Orientation() == TopAbs_REVERSED) normal.Reverse();

        // 2. 构建坐标系 (Z = Normal)
        glm::vec3 N(normal.X(), normal.Y(), normal.Z());
        glm::vec3 Pos(p.X(), p.Y(), p.Z());

        // 我们需要构建一个 LookAt 矩阵的逆矩阵 (因为 LookAt 是 View 矩阵，我们要的是 Model 矩阵)
        // 或者手动构建基向量：
        glm::vec3 Up = (std::abs(N.y) < 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 Right = glm::normalize(glm::cross(Up, N));
        Up = glm::cross(N, Right); // 重新计算正交的 Up

        // 构建旋转矩阵 (列主序: Right, Up, Normal)
        // 注意：ImGuizmo 默认 Z 轴是操作轴，所以我们将 N 设为 Z 轴
        glm::mat4 rotation(1.0f);
        rotation[0] = glm::vec4(Right, 0.0f);
        rotation[1] = glm::vec4(Up, 0.0f);
        rotation[2] = glm::vec4(N, 0.0f);
        rotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        glm::mat4 translation = glm::translate(glm::mat4(1.0f), Pos);

        return translation * rotation;
    }

    void* CADFeature::MakeFilletShape(void* shapeHandle, int edgeID, float radius)
    {
        if (!shapeHandle) return nullptr;
        TopoDS_Shape* shape = (TopoDS_Shape*)shapeHandle;

        // 1. 初始化倒角工具
        BRepFilletAPI_MakeFillet filletMaker(*shape);

        // 2. 找到对应的边
        // 注意：这里的遍历顺序必须和 Mesher 生成 ID 的顺序完全一致
        int currentID = 0;
        bool found = false;
        TopExp_Explorer explorer(*shape, TopAbs_EDGE);
        for (; explorer.More(); explorer.Next())
        {
            if (currentID == edgeID)
            {
                const TopoDS_Edge& edge = TopoDS::Edge(explorer.Current());
                filletMaker.Add(radius, edge);
                found = true;
                break;
            }
            currentID++;
        }

        if (!found) return nullptr;

        // 3. 构建
        try {
            filletMaker.Build();
            if (filletMaker.IsDone())
            {
                return new TopoDS_Shape(filletMaker.Shape());
            }
        }
        catch (...) {
            return nullptr;
        }
        return nullptr;
    }

    glm::mat4 CADFeature::GetEdgeTransform(void* shapeHandle, int edgeID)
    {
        if (!shapeHandle) return glm::mat4(1.0f);

        TopoDS_Shape* shape = (TopoDS_Shape*)shapeHandle;
        TopoDS_Edge targetEdge;
        bool found = false;

        // 1. 遍历寻找对应 ID 的边
        int currentID = 0;
        TopExp_Explorer explorer(*shape, TopAbs_EDGE);
        for (; explorer.More(); explorer.Next())
        {
            if (currentID == edgeID)
            {
                targetEdge = TopoDS::Edge(explorer.Current());
                found = true;
                break;
            }
            currentID++;
        }

        if (!found)
        {
            return glm::mat4(1.0f);
        }

        // 2. 计算边的质心 
        GProp_GProps linearProps;
        BRepGProp::LinearProperties(targetEdge, linearProps);
        gp_Pnt center = linearProps.CentreOfMass();

        // 3. 构建平移矩阵
        return glm::translate(glm::mat4(1.0f), glm::vec3((float)center.X(), (float)center.Y(), (float)center.Z()));
    }
}