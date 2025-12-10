#pragma once
#include "Rongine/Renderer/RendererAPI.h"
#include "Rongine/Renderer/RenderCommand.h"
#include "Rongine/Renderer/OrthographicCamera.h"
#include "Rongine/Renderer/Shader.h"

namespace Rongine {


	class Renderer
	{
	public:
		static void init();

		static void beginScene(const OrthographicCamera& camera);
		static void endScene();

		static void submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

		inline static RendererAPI::API getAPI() { return RendererAPI::getAPI(); }
	private:
		struct SceneData 
		{
			glm::mat4 viewProjectionMatrix;
		};

		static SceneData* s_sceneData;
	};

}


