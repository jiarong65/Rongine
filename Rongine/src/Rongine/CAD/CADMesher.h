#pragma once

#include "Rongine/Core/Core.h" // 包含 Ref 定义
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Renderer3D.h"
#include "Rongine/Scene/Components.h"

// 引入 TopoDS_Shape 定义，方便外部调用
#include <TopoDS_Shape.hxx>

class TopoDS_Shape;

namespace Rongine {

	class CADMesher
	{
	public:
		// 输入：OCCT 形状
		// 输出：一个可以在 OpenGL 里画出来的 VertexArray
		static Ref<VertexArray> CreateMeshFromShape(const TopoDS_Shape& shape, std::vector<CubeVertex>& outVertices, float deflection = 0.1f);

		static Ref<VertexArray> CreateEdgeMeshFromShape(const TopoDS_Shape& shape, std::vector<LineVertex>& outLines, float deflection = 0.1f);
	};

}