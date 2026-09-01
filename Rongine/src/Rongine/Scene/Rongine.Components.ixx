module;
#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <TopoDS_Edge.hxx>
#include "entt.hpp"

export module Rongine.Components;

export import Rongine.Core;
export import Rongine.SceneData;
export import Rongine.RendererData;
export import Rongine.Renderer;

export namespace Rongine {

	// MeshComponent 已上移到 Rongine.Renderer（见 Rongine.Renderer.Interfaces.ixx），
	// 本模块 export import Rongine.Renderer，对使用者依然可见。

	struct CADGeometryComponent
	{
		enum class GeometryType { None = 0, Cube, Sphere, Cylinder, Imported, Spline };

		GeometryType Type = GeometryType::None;

		void* ShapeHandle = nullptr;

		struct {
			float Width = 1.0f, Height = 1.0f, Depth = 1.0f;
			float Radius = 1.0f;
		} Params;

		float LinearDeflection = 0.1f;
		float FilletRadius = 0.1f;

		std::vector<CADControlPoint> SplinePoints;
		int SplineDegree = 3;
		bool SplineClosed = false;

		entt::entity SourceEntity = entt::null;

		CADGeometryComponent() = default;
		CADGeometryComponent(const CADGeometryComponent&) = default;
	};
}
