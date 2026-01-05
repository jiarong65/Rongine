#include "Rongpch.h"
#include "CADImporter.h"

// --- OCCT STEP 读取头文件 ---
#include <STEPControl_Reader.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>

namespace Rongine {

    TopoDS_Shape CADImporter::ImportSTEP(const std::string& filepath)
    {
        STEPControl_Reader reader;

        // 1. 读取文件
        IFSelect_ReturnStatus status = reader.ReadFile(filepath.c_str());

        if (status != IFSelect_RetDone)
        {
            // 这里应该用你的 Log 系统输出错误
            // RONG_ERROR("Failed to read STEP file: {0}", filepath);
            return TopoDS_Shape(); // 返回空形状
        }

        // 2. 转换数据 (Transfer roots)
        // 打印正在转换的信息 (可选)
        // reader.PrintCheckLoad(false, "Full", "State");

        int nbr = reader.NbRootsForTransfer();
        reader.TransferRoots();

        // 3. 获取结果形状
        // 如果文件里有多个零件，OneShape() 会把它们打包成一个 Compound
        TopoDS_Shape resultShape = reader.OneShape();

        return resultShape;
    }
}