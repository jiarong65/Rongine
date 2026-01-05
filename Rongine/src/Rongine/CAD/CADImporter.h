#pragma once
#include <string>
#include <TopoDS_Shape.hxx>

namespace Rongine {

    class CADImporter
    {
    public:
        // 读取 STEP 文件并返回形状
        static TopoDS_Shape ImportSTEP(const std::string& filepath);
    };

}