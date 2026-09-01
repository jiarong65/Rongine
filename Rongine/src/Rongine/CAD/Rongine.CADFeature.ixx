module;
#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#define GLM_ENABLE_EXPERIMENTAL
#include <vector>
#include <cstdio>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax3.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>
#include <GeomLProp_SLProps.hxx>
#include <BRepTools.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAdaptor_Surface.hxx>

export module Rongine.CADFeature;

namespace Rongine {

	static TopoDS_Face GetFaceByIndex(const TopoDS_Shape& shape, int index)
	{
		int current = 0;
		TopExp_Explorer explorer(shape, TopAbs_FACE);
		while (explorer.More())
		{
			if (current == index)
			{
				return TopoDS::Face(explorer.Current());
			}
			current++;
			explorer.Next();
		}
		return TopoDS_Face();
	}

	static gp_Vec GetFaceNormal(const TopoDS_Face& face)
	{
		Handle(Geom_Surface) surface = BRep_Tool::Surface(face);

		double umin, umax, vmin, vmax;
		BRepTools::UVBounds(face, umin, umax, vmin, vmax);

		double centerU = (umin + umax) * 0.5;
		double centerV = (vmin + vmax) * 0.5;

		GeomLProp_SLProps props(surface, centerU, centerV, 1, 1e-6);

		if (props.IsNormalDefined())
		{
			gp_Dir normal = props.Normal();

			if (face.Orientation() == TopAbs_REVERSED)
			{
				normal.Reverse();
			}
			return gp_Vec(normal);
		}

		return gp_Vec(0, 1, 0);
	}
}

export namespace Rongine {

	class CADFeature
	{
	public:
		static void* ExtrudeFace(void* shapeHandle, int faceIndex, float height);
		static glm::mat4 GetFaceTransform(void* shapeHandle, int faceIndex);

		static void* MakeFilletShape(void* shapeHandle, int edgeID, float radius);
		static glm::mat4 GetEdgeTransform(void* shapeHandle, int edgeID);

		static bool GetPlanarFaceCoordinateSystem(const TopoDS_Shape& shape, int faceID, gp_Ax3& outAx3, glm::mat4& outMatrix);

		static void* BuildFaceFromSketch(const std::vector<glm::vec3>& points);

	private:
		static TopoDS_Shape GetSubShape(const TopoDS_Shape& shape, TopAbs_ShapeEnum type, int index);
	};

	void* CADFeature::ExtrudeFace(void* shapeHandle, int faceIndex, float height)
	{
		if (!shapeHandle || faceIndex < 0) return nullptr;

		TopoDS_Shape* mainShape = (TopoDS_Shape*)shapeHandle;

		TopoDS_Face face = GetFaceByIndex(*mainShape, faceIndex);
		if (face.IsNull())
		{
			std::printf("Extrude: Face ID %d not found!\n", faceIndex);
			return nullptr;
		}

		try
		{
			gp_Vec normal = GetFaceNormal(face);
			gp_Vec extrudeVec = normal * height;

			BRepPrimAPI_MakePrism prism(face, extrudeVec);
			prism.Build();

			if (prism.IsDone())
			{
				return new TopoDS_Shape(prism.Shape());
			}
		}
		catch (...)
		{
			std::printf("OCCT Extrude Failed!\n");
		}

		return nullptr;
	}

	glm::mat4 CADFeature::GetFaceTransform(void* shapeHandle, int faceIndex)
	{
		if (!shapeHandle || faceIndex < 0) return glm::mat4(1.0f);

		TopoDS_Shape* mainShape = (TopoDS_Shape*)shapeHandle;
		TopoDS_Face face = GetFaceByIndex(*mainShape, faceIndex);
		if (face.IsNull()) return glm::mat4(1.0f);

		Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
		double umin, umax, vmin, vmax;
		BRepTools::UVBounds(face, umin, umax, vmin, vmax);
		double centerU = (umin + umax) * 0.5;
		double centerV = (vmin + vmax) * 0.5;

		gp_Pnt p;
		gp_Vec d1u, d1v;
		surface->D1(centerU, centerV, p, d1u, d1v);

		gp_Vec normal = d1u.Crossed(d1v);
		if (normal.Magnitude() > 1e-7) normal.Normalize();
		else normal = gp_Vec(0, 0, 1);

		if (face.Orientation() == TopAbs_REVERSED) normal.Reverse();

		glm::vec3 N(normal.X(), normal.Y(), normal.Z());
		glm::vec3 Pos(p.X(), p.Y(), p.Z());

		glm::vec3 Up = (std::abs(N.y) < 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
		glm::vec3 Right = glm::normalize(glm::cross(Up, N));
		Up = glm::cross(N, Right);

		glm::mat4 rotation(1.0f);
		rotation[0] = glm::vec4(Right, 0.0f);
		rotation[1] = glm::vec4(Up, 0.0f);
		rotation[2] = glm::vec4(N, 0.0f);
		rotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		glm::mat4 translation = glm::translate(glm::mat4(1.0f), Pos);

		return translation * rotation;
	}

	void* CADFeature::MakeFilletShape(void* shapeHandle, int edgeID, float radius)
	{
		if (!shapeHandle) return nullptr;
		TopoDS_Shape* shape = (TopoDS_Shape*)shapeHandle;

		BRepFilletAPI_MakeFillet filletMaker(*shape);

		int currentID = 0;
		bool found = false;
		TopExp_Explorer explorer(*shape, TopAbs_EDGE);
		for (; explorer.More(); explorer.Next())
		{
			if (currentID == edgeID)
			{
				const TopoDS_Edge& edge = TopoDS::Edge(explorer.Current());
				filletMaker.Add(radius, edge);
				found = true;
				break;
			}
			currentID++;
		}

		if (!found) return nullptr;

		try {
			filletMaker.Build();
			if (filletMaker.IsDone())
			{
				return new TopoDS_Shape(filletMaker.Shape());
			}
		}
		catch (...) {
			return nullptr;
		}
		return nullptr;
	}

	glm::mat4 CADFeature::GetEdgeTransform(void* shapeHandle, int edgeID)
	{
		if (!shapeHandle) return glm::mat4(1.0f);

		TopoDS_Shape* shape = (TopoDS_Shape*)shapeHandle;
		TopoDS_Edge targetEdge;
		bool found = false;

		int currentID = 0;
		TopExp_Explorer explorer(*shape, TopAbs_EDGE);
		for (; explorer.More(); explorer.Next())
		{
			if (currentID == edgeID)
			{
				targetEdge = TopoDS::Edge(explorer.Current());
				found = true;
				break;
			}
			currentID++;
		}

		if (!found)
		{
			return glm::mat4(1.0f);
		}

		GProp_GProps linearProps;
		BRepGProp::LinearProperties(targetEdge, linearProps);
		gp_Pnt center = linearProps.CentreOfMass();

		return glm::translate(glm::mat4(1.0f), glm::vec3((float)center.X(), (float)center.Y(), (float)center.Z()));
	}

	bool CADFeature::GetPlanarFaceCoordinateSystem(const TopoDS_Shape& shape, int faceID, gp_Ax3& outAx3, glm::mat4& outMatrix)
	{
		TopoDS_Face face = TopoDS::Face(CADFeature::GetSubShape(shape, TopAbs_FACE, faceID));
		if (face.IsNull()) return false;

		BRepAdaptor_Surface adaptor(face);
		if (adaptor.GetType() != GeomAbs_Plane) return false;

		GProp_GProps props;
		BRepGProp::SurfaceProperties(face, props);
		gp_Pnt centerP = props.CentreOfMass();

		gp_Pln plane = adaptor.Plane();
		gp_Dir planeNormal = plane.Axis().Direction();
		gp_Dir planeX = plane.XAxis().Direction();

		if (face.Orientation() == TopAbs_REVERSED)
		{
			planeNormal.Reverse();
		}

		gp_Vec zVec(planeNormal);
		gp_Vec xVec(planeX);

		gp_Vec yVec = zVec.Crossed(xVec).Normalized();
		xVec = yVec.Crossed(zVec).Normalized();

		outAx3 = gp_Ax3(centerP, gp_Dir(zVec), gp_Dir(xVec));

		glm::vec3 right = { (float)xVec.X(), (float)xVec.Y(), (float)xVec.Z() };
		glm::vec3 up = { (float)yVec.X(), (float)yVec.Y(), (float)yVec.Z() };
		glm::vec3 front = { (float)zVec.X(), (float)zVec.Y(), (float)zVec.Z() };
		glm::vec3 pos = { (float)centerP.X(), (float)centerP.Y(), (float)centerP.Z() };

		outMatrix = glm::mat4(1.0f);
		outMatrix[0] = glm::vec4(right, 0.0f);
		outMatrix[1] = glm::vec4(up, 0.0f);
		outMatrix[2] = glm::vec4(front, 0.0f);
		outMatrix[3] = glm::vec4(pos, 1.0f);

		return true;
	}

	void* CADFeature::BuildFaceFromSketch(const std::vector<glm::vec3>& points)
	{
		if (points.size() < 3) return nullptr;

		try
		{
			BRepBuilderAPI_MakePolygon mkPoly;

			for (const auto& p : points)
			{
				mkPoly.Add(gp_Pnt(p.x, p.y, p.z));
			}
			mkPoly.Close();

			if (!mkPoly.IsDone()) return nullptr;

			BRepBuilderAPI_MakeFace mkFace(mkPoly.Wire(), true);

			if (mkFace.IsDone())
			{
				return new TopoDS_Shape(mkFace.Shape());
			}
		}
		catch (...) { std::printf("Failed to build face.\n"); }

		return nullptr;
	}

	TopoDS_Shape CADFeature::GetSubShape(const TopoDS_Shape& shape, TopAbs_ShapeEnum type, int index)
	{
		TopExp_Explorer explorer(shape, type);
		int current = 0;
		while (explorer.More())
		{
			if (current == index)
			{
				return explorer.Current();
			}
			explorer.Next();
			current++;
		}
		return TopoDS_Shape();
	}
}
