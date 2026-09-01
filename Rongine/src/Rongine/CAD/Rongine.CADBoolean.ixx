module;
#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#include <glm/glm.hpp>
#include <cstdio>
#include <TopoDS_Shape.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>

export module Rongine.CADBoolean;

export import Rongine.CADBooleanData;

namespace Rongine {

	static gp_Trsf GlmToGpTrsf(const glm::mat4& mat)
	{
		gp_Trsf trsf;
		trsf.SetValues(
			mat[0][0], mat[1][0], mat[2][0], mat[3][0],
			mat[0][1], mat[1][1], mat[2][1], mat[3][1],
			mat[0][2], mat[1][2], mat[2][2], mat[3][2]
		);
		return trsf;
	}
}

export namespace Rongine {

	class CADBoolean
	{
	public:
		static void* Perform(void* shapeA, const glm::mat4& transformA,
			void* shapeB, const glm::mat4& transformB,
			Operation op);
	};

	void* CADBoolean::Perform(void* shapeA, const glm::mat4& transformA,
		void* shapeB, const glm::mat4& transformB,
		Operation op)
	{
		if (!shapeA || !shapeB) return nullptr;

		TopoDS_Shape* occShapeA = (TopoDS_Shape*)shapeA;
		TopoDS_Shape* occShapeB = (TopoDS_Shape*)shapeB;

		gp_Trsf trsfA = GlmToGpTrsf(transformA);
		gp_Trsf trsfB = GlmToGpTrsf(transformB);

		TopoDS_Shape worldShapeA = BRepBuilderAPI_Transform(*occShapeA, trsfA, true).Shape();
		TopoDS_Shape worldShapeB = BRepBuilderAPI_Transform(*occShapeB, trsfB, true).Shape();

		TopoDS_Shape resultShape;
		try
		{
			switch (op)
			{
			case Operation::Cut:
			{
				BRepAlgoAPI_Cut algo(worldShapeA, worldShapeB);
				algo.Build();
				if (algo.IsDone()) resultShape = algo.Shape();
				else std::printf("Boolean Cut Failed!\n");
				break;
			}
			case Operation::Fuse:
			{
				BRepAlgoAPI_Fuse algo(worldShapeA, worldShapeB);
				algo.Build();
				if (algo.IsDone()) resultShape = algo.Shape();
				else std::printf("Boolean Fuse Failed!\n");
				break;
			}
			case Operation::Common:
			{
				BRepAlgoAPI_Common algo(worldShapeA, worldShapeB);
				algo.Build();
				if (algo.IsDone()) resultShape = algo.Shape();
				else std::printf("Boolean Common Failed!\n");
				break;
			}
			}
		}
		catch (...)
		{
			std::printf("OCCT Exception during Boolean Operation\n");
			return nullptr;
		}

		if (resultShape.IsNull()) return nullptr;

		return new TopoDS_Shape(resultShape);
	}
}
