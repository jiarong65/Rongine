#pragma once

#include <TopoDS_Shape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>

namespace Rongine {

    class CADModeler
    {
    public:
        // 创建一个立方体的 Shape，返回 void* 指针
        static void* MakeCube(float x, float y, float z);

        // 创建球体
        static void* MakeSphere(float radius);
        // 创建圆柱
        static void* MakeCylinder(float radius, float height);

        // 释放 Shape 内存的辅助函数 (非常重要，防止内存泄漏)
        static void FreeShape(void* shapeHandle);
    };
}