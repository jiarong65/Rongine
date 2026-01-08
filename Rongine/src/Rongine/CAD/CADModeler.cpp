#include "Rongpch.h"
#include "CADModeler.h"

// --- 这里才真正引入 OCCT 头文件 ---
#include <TopoDS_Shape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>

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

    void FreeShape(void* shapeHandle)
    {
        if (shapeHandle)
        {
            delete (TopoDS_Shape*)shapeHandle;
        }
    }
}