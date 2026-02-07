#include "Rongpch.h"
#include "CADModeler.h"
#include <BRepBuilderAPI_MakeEdge.hxx>

#include <Geom_BSplineCurve.hxx> 
#include <Geom_Curve.hxx>

// --- 引入 OCCT 头文件 ---


namespace Rongine {

    void* CADModeler::MakeCube(float x, float y, float z)
    {
        // 调用 OCCT 算法创建盒子
        // MakeBox 默认角点在 (0,0,0)。为了方便旋转，我们通常希望中心在原点。
        // 但为了简单，第一步我们先不管中心偏移，直接生成。
        BRepPrimAPI_MakeBox maker(x, y, z);
        TopoDS_Shape shape = maker.Shape();

        // 在堆上分配内存存储这个 Shape，以便在 Component 中持有
        return new TopoDS_Shape(shape);
    }

    void* CADModeler::MakeSphere(float radius)
    {
        BRepPrimAPI_MakeSphere maker(radius);
        TopoDS_Shape shape = maker.Shape();
        return new TopoDS_Shape(shape);
    }

    void* CADModeler::MakeCylinder(float radius, float height)
    {
        // 创建圆柱，默认底面圆心在 (0,0,0)，沿 Z 轴向上延伸
        BRepPrimAPI_MakeCylinder maker(radius, height);
        TopoDS_Shape shape = maker.Shape();
        return new TopoDS_Shape(shape);
    }

    void* CADModeler::MakeNURBSCurve(const std::vector<CADControlPoint>& points, int degree, bool closed)
    {
        int numPoints = (int)points.size();
        if (numPoints < 2) return nullptr;

        try {
            // 1. 修正阶数 (OpenCASCADE 要求)
            if (degree < 1) degree = 1;
            if (degree > 9) degree = 9; // 限制最大阶数

            // 对于非闭合曲线，点数必须 > 阶数
            // 如果点太少，自动降阶
            if (!closed && numPoints <= degree) degree = numPoints - 1;

            // 2. 准备数据
            std::vector<gp_Pnt> finalPoles;
            std::vector<double> finalWeights;

            for (size_t i = 0; i < points.size(); ++i)
            {
                const auto& p = points[i];
                double w = std::max((double)p.Weight, 0.0001); // 权重防零

                finalPoles.push_back(gp_Pnt(p.Position.x, p.Position.y, p.Position.z));
                finalWeights.push_back(w);

                // 处理尖点 (IsSharp)
                // 仅在中间点生效，通过重复插入点来提升重数 (Multiplicity)
                if (p.IsSharp && i > 0 && i < points.size() - 1)
                {
                    // 重复插入 (Degree - 1) 次，总共 Degree 次 -> 强制穿过且形成尖角
                    for (int k = 0; k < degree - 1; ++k) {
                        finalPoles.push_back(gp_Pnt(p.Position.x, p.Position.y, p.Position.z));
                        finalWeights.push_back(w);
                    }
                }
            }

            // 处理闭合逻辑 (Geometric Closure)
            // 简单闭合：把前 Degree 个点复制到末尾 (这是构建 Periodic B-Spline 的一种简单模拟方式)
            // OCCT 支持真正的 Periodic，但参数设置较繁琐，这里用“伪闭合”保证几何形状闭合
            if (closed)
            {
                for (int k = 0; k < degree; ++k) {
                    int idx = k % points.size(); // 防止越界
                    auto& p = points[idx];
                    finalPoles.push_back(gp_Pnt(p.Position.x, p.Position.y, p.Position.z));
                    finalWeights.push_back(std::max((double)p.Weight, 0.0001));
                }
            }

            int nPoles = (int)finalPoles.size();

            // 3. 构建 OCCT 数组
            TColgp_Array1OfPnt occtPoles(1, nPoles);
            TColStd_Array1OfReal occtWeights(1, nPoles);

            for (int i = 0; i < nPoles; ++i) {
                occtPoles.SetValue(i + 1, finalPoles[i]);
                occtWeights.SetValue(i + 1, finalWeights[i]);
            }

            // 4. 构建节点向量 (Knots) - 准均匀 (Clamped)
            // 公式: Knots数量 = Poles数量 - Degree + 1
            int nKnots = nPoles - degree + 1;
            TColStd_Array1OfReal knots(1, nKnots);
            TColStd_Array1OfInteger mults(1, nKnots);

            for (int i = 1; i <= nKnots; ++i) {
                knots.SetValue(i, (double)(i - 1));

                // 首尾 Knot 重数 = Degree + 1 (Clamped)
                if (i == 1 || i == nKnots)
                    mults.SetValue(i, degree + 1);
                else
                    mults.SetValue(i, 1);
            }

            // 5. 创建几何曲线
            Handle(Geom_BSplineCurve) curve = new Geom_BSplineCurve(
                occtPoles, occtWeights, knots, mults, degree,
                Standard_False, // Periodic (我们手动处理了闭合点，所以这里填False)
                Standard_True   // CheckRational
            );

            // 6. 生成 Shape
            BRepBuilderAPI_MakeEdge edgeMaker(curve);
            if (!edgeMaker.IsDone()) return nullptr;

            return new TopoDS_Shape(edgeMaker.Shape());
        }
        catch (...) {
            return nullptr;
        }
    }

    void CADModeler::FreeShape(void* shapeHandle)
    {
        if (shapeHandle)
        {
            delete (TopoDS_Shape*)shapeHandle;
        }
    }
}