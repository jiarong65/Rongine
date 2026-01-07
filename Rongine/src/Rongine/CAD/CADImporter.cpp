#include "Rongpch.h"
#include "CADImporter.h"

// --- OCCT STEP 读取头文件 ---
#include <STEPControl_Reader.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include "BRepBndLib.hxx" 
#include "Bnd_Box.hxx"

namespace Rongine {

    TopoDS_Shape CADImporter::ImportSTEP(const std::string& filepath)
    {
        STEPControl_Reader reader;

        // 1. 读取文件
        IFSelect_ReturnStatus status = reader.ReadFile(filepath.c_str());

        if (status != IFSelect_RetDone)
        {
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
    
    AABB CADImporter::CalculateAABB(const TopoDS_Shape& shape)
    {
        Bnd_Box box;
        // 使用 Triangulation 数据计算包围盒会更贴合 Mesh，但用几何数据也行 (useTriangulation = true)
        BRepBndLib::Add(shape, box, true);

        double xmin, ymin, zmin, xmax, ymax, zmax;
        box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

        AABB result;
        result.Min = { (float)xmin, (float)ymin, (float)zmin };
        result.Max = { (float)xmax, (float)ymax, (float)zmax };
        return result;
    }
}