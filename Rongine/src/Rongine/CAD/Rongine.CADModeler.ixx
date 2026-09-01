module;
#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#include <vector>
#include <TopoDS_Shape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Curve.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <gp_Pnt.hxx>

export module Rongine.CADModeler;

export import Rongine.RendererTypes;

export namespace Rongine {

	class CADModeler
	{
	public:
		static void* MakeCube(float x, float y, float z);
		static void* MakeSphere(float radius);
		static void* MakeCylinder(float radius, float height);
		static void* MakeNURBSCurve(const std::vector<CADControlPoint>& points, int degree = 3, bool closed = false);
		static void FreeShape(void* shapeHandle);
	};

	void* CADModeler::MakeCube(float x, float y, float z)
	{
		BRepPrimAPI_MakeBox maker(x, y, z);
		TopoDS_Shape shape = maker.Shape();
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
		BRepPrimAPI_MakeCylinder maker(radius, height);
		TopoDS_Shape shape = maker.Shape();
		return new TopoDS_Shape(shape);
	}

	void* CADModeler::MakeNURBSCurve(const std::vector<CADControlPoint>& points, int degree, bool closed)
	{
		int inputSize = (int)points.size();
		if (inputSize < 2) return nullptr;

		if (degree > inputSize - 1) degree = inputSize - 1;
		if (degree < 1) degree = 1;

		try
		{
			std::vector<CADControlPoint> finalPoints;
			for (int i = 0; i < inputSize; ++i)
			{
				int repeat = 1;
				if (points[i].IsSharp && i > 0 && i < inputSize - 1) {
					repeat = degree;
				}
				for (int k = 0; k < repeat; ++k) finalPoints.push_back(points[i]);
			}

			int numPoles = (int)finalPoints.size();

			if (numPoles < 2 || degree > numPoles - 1) return nullptr;

			TColgp_Array1OfPnt occPoles(1, numPoles);
			TColStd_Array1OfReal occWeights(1, numPoles);

			for (int i = 0; i < numPoles; ++i)
			{
				occPoles.SetValue(i + 1, gp_Pnt(finalPoints[i].Position.x, finalPoints[i].Position.y, finalPoints[i].Position.z));
				occWeights.SetValue(i + 1, finalPoints[i].Weight);
			}

			int numKnots = numPoles - degree + 1;

			TColStd_Array1OfReal occKnots(1, numKnots);
			TColStd_Array1OfInteger occMults(1, numKnots);

			for (int i = 1; i <= numKnots; ++i)
			{
				occKnots.SetValue(i, (double)(i - 1));

				if (i == 1 || i == numKnots) {
					occMults.SetValue(i, degree + 1);
				}
				else {
					occMults.SetValue(i, 1);
				}
			}

			Handle(Geom_BSplineCurve) curve = new Geom_BSplineCurve(
				occPoles,
				occWeights,
				occKnots,
				occMults,
				degree,
				false
			);

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
