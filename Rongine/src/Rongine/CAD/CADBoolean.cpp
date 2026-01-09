#include "Rongpch.h"
#include "CADBoolean.h"
#include "Rongine/Core/Log.h"

// --- OCCT 头文件 ---
#include <TopoDS_Shape.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_Transform.hxx> // 用于应用变换
#include <gp_Trsf.hxx> // OCCT 的变换矩阵

namespace Rongine {

    // --- 内部辅助：将 GLM 矩阵转换为 OCCT 的 gp_Trsf ---
    static gp_Trsf GlmToGpTrsf(const glm::mat4& mat)
    {
        gp_Trsf trsf;
        // gp_Trsf 是 3x4 矩阵 (行主序还是列主序需要极其小心)
        // GLM 是列主序 (Column-Major)，OCCT 的 SetValues 接受的是：
        // a11, a12, a13, a14 (x translation)
        // a21, a22, a23, a24 (y translation) ...

        trsf.SetValues(
            mat[0][0], mat[1][0], mat[2][0], mat[3][0],
            mat[0][1], mat[1][1], mat[2][1], mat[3][1],
            mat[0][2], mat[1][2], mat[2][2], mat[3][2]
        );
        return trsf;
    }

    void* CADBoolean::Perform(void* shapeA, const glm::mat4& transformA,
        void* shapeB, const glm::mat4& transformB,
        Operation op)
    {
        if (!shapeA || !shapeB) return nullptr;

        TopoDS_Shape* occShapeA = (TopoDS_Shape*)shapeA;
        TopoDS_Shape* occShapeB = (TopoDS_Shape*)shapeB;

        // 1. 预处理：将形状变换到世界坐标系 (Baking Transform)
        // 我们不能修改原始 Shape，所以要生成临时的变换副本
        gp_Trsf trsfA = GlmToGpTrsf(transformA);
        gp_Trsf trsfB = GlmToGpTrsf(transformB);

        // BRepBuilderAPI_Transform 的第二个参数 true 表示复制一份 Shape
        TopoDS_Shape worldShapeA = BRepBuilderAPI_Transform(*occShapeA, trsfA, true).Shape();
        TopoDS_Shape worldShapeB = BRepBuilderAPI_Transform(*occShapeB, trsfB, true).Shape();

        // 2. 执行布尔运算
        TopoDS_Shape resultShape;
        try
        {
            switch (op)
            {
            case Operation::Cut:
            {
                // A - B
                BRepAlgoAPI_Cut algo(worldShapeA, worldShapeB);
                algo.Build();
                if (algo.IsDone()) resultShape = algo.Shape();
                else RONG_CORE_ERROR("Boolean Cut Failed!");
                break;
            }
            case Operation::Fuse:
            {
                // A + B
                BRepAlgoAPI_Fuse algo(worldShapeA, worldShapeB);
                algo.Build();
                if (algo.IsDone()) resultShape = algo.Shape();
                else RONG_CORE_ERROR("Boolean Fuse Failed!");
                break;
            }
            case Operation::Common:
            {
                // A ∩ B
                BRepAlgoAPI_Common algo(worldShapeA, worldShapeB);
                algo.Build();
                if (algo.IsDone()) resultShape = algo.Shape();
                else RONG_CORE_ERROR("Boolean Common Failed!");
                break;
            }
            }
        }
        catch (...)
        {
            RONG_CORE_ERROR("OCCT Exception during Boolean Operation");
            return nullptr;
        }

        // 3. 检查结果
        if (resultShape.IsNull()) return nullptr;

        // 4. 返回结果
        // 注意：这个结果是在世界坐标系下的，它的 Transform 应该是 Identity (0,0,0)
        return new TopoDS_Shape(resultShape);
    }
}