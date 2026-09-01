module;
#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#include <string>
#include <cstdio>
#include <TopoDS_Shape.hxx>
#include <STEPControl_Reader.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <STEPControl_Writer.hxx>
#include <Interface_Static.hxx>

export module Rongine.CADImporter;

export import Rongine.RendererTypes;

export namespace Rongine {

	class CADImporter
	{
	public:
		static TopoDS_Shape ImportSTEP(const std::string& filepath);
		static AABB CalculateAABB(const TopoDS_Shape& shape);
	};

	class CADExporter
	{
	public:
		static bool ExportSTEP(const std::string& filepath, void* shapeHandle);
	};

	TopoDS_Shape CADImporter::ImportSTEP(const std::string& filepath)
	{
		STEPControl_Reader reader;

		IFSelect_ReturnStatus status = reader.ReadFile(filepath.c_str());

		if (status != IFSelect_RetDone)
		{
			return TopoDS_Shape();
		}

		int nbr = reader.NbRootsForTransfer();
		reader.TransferRoots();

		TopoDS_Shape resultShape = reader.OneShape();

		return resultShape;
	}

	AABB CADImporter::CalculateAABB(const TopoDS_Shape& shape)
	{
		Bnd_Box box;
		BRepBndLib::Add(shape, box, true);

		double xmin, ymin, zmin, xmax, ymax, zmax;
		box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

		AABB result;
		result.Min = { (float)xmin, (float)ymin, (float)zmin };
		result.Max = { (float)xmax, (float)ymax, (float)zmax };
		return result;
	}

	bool CADExporter::ExportSTEP(const std::string& filepath, void* shapeHandle)
	{
		if (!shapeHandle) return false;

		TopoDS_Shape* shape = (TopoDS_Shape*)shapeHandle;

		STEPControl_Writer writer;

		IFSelect_ReturnStatus status = writer.Transfer(*shape, STEPControl_AsIs);

		if (status != IFSelect_RetDone)
		{
			std::printf("STEP Export: Transfer failed!\n");
			return false;
		}

		status = writer.Write(filepath.c_str());

		if (status != IFSelect_RetDone)
		{
			std::printf("STEP Export: Write failed!\n");
			return false;
		}

		return true;
	}
}
