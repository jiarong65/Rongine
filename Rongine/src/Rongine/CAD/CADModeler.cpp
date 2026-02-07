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
        int inputSize = (int)points.size();
        if (inputSize < 2) return nullptr;

        // 限制阶数
        if (degree > inputSize - 1) degree = inputSize - 1;
        if (degree < 1) degree = 1;

        try
        {
            // 1. 预计算堆叠后的点数量 (为了处理 IsSharp)
            std::vector<CADControlPoint> finalPoints;
            for (int i = 0; i < inputSize; ++i)
            {
                int repeat = 1;
                // 如果是尖点（且不是首尾），则重复 Degree 次
                if (points[i].IsSharp && i > 0 && i < inputSize - 1) {
                    repeat = degree;
                }
                for (int k = 0; k < repeat; ++k) finalPoints.push_back(points[i]);
            }

            int numPoles = (int)finalPoints.size();

            // 安全检查：如果点数太少无法构建指定阶数的曲线
            if (numPoles < 2 || degree > numPoles - 1) return nullptr;

            // 2. 准备 OCC 数组：Poles (控制点) 和 Weights (权重)
            TColgp_Array1OfPnt occPoles(1, numPoles);
            TColStd_Array1OfReal occWeights(1, numPoles);

            for (int i = 0; i < numPoles; ++i)
            {
                occPoles.SetValue(i + 1, gp_Pnt(finalPoints[i].Position.x, finalPoints[i].Position.y, finalPoints[i].Position.z));
                occWeights.SetValue(i + 1, finalPoints[i].Weight);
            }

            // 3. [关键修复] 手动构建 Knots (节点) 和 Mults (多重数)
            // 这是一个标准的 "Clamped" (两端固定) B-Spline 构造方式

            // 计算唯一节点的数量
            // 公式：KnotsLength = NumPoles - Degree + 1
            int numKnots = numPoles - degree + 1;

            TColStd_Array1OfReal occKnots(1, numKnots);
            TColStd_Array1OfInteger occMults(1, numKnots);

            // 填充 Knots 和 Mults
            for (int i = 1; i <= numKnots; ++i)
            {
                // 简单的均匀参数化：节点值 0.0, 1.0, 2.0 ...
                occKnots.SetValue(i, (double)(i - 1));

                // 设置多重数
                if (i == 1 || i == numKnots) {
                    // 首尾节点：多重数 = Degree + 1 (Clamped)
                    occMults.SetValue(i, degree + 1);
                }
                else {
                    // 内部节点：多重数 = 1
                    occMults.SetValue(i, 1);
                }
            }

            // 4. 创建曲线
            // 构造函数参数：Poles, Weights, Knots, Multiplicities, Degree, Periodic, CheckRational
            Handle(Geom_BSplineCurve) curve = new Geom_BSplineCurve(
                occPoles,
                occWeights,
                occKnots,
                occMults,
                degree,
                false // Periodic (暂时不支持周期性闭合，闭合建议通过首尾点重合实现)
            );

            // 5. 生成拓扑边
            BRepBuilderAPI_MakeEdge mkEdge(curve);
            if (mkEdge.IsDone()) {
                return new TopoDS_Shape(mkEdge.Shape());
            }
        }
        catch (...) {
            return nullptr;
        }
        return nullptr;
    }

    void CADModeler::FreeShape(void* shapeHandle)
    {
        if (shapeHandle)
        {
            delete (TopoDS_Shape*)shapeHandle;
        }
    }
}