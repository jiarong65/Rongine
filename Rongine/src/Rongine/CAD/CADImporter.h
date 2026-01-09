#pragma once
#include <string>
#include <TopoDS_Shape.hxx>
#include "Rongine/Scene/Components.h"

namespace Rongine {

    class CADImporter
    {
    public:
        // 读取 STEP 文件并返回形状
        static TopoDS_Shape ImportSTEP(const std::string& filepath);

        // 辅助函数 计算AABB
        static AABB CalculateAABB(const TopoDS_Shape& shape);
    };

    class CADExporter
    {
    public:
        // 导出单个 Shape 到 STEP 文件
        static bool ExportSTEP(const std::string& filepath, void* shapeHandle);
    };

}