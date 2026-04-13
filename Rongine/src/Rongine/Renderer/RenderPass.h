#pragma once
#include "Rongine/Core/Core.h"
#include "Rongine/Renderer/Framebuffer.h"
#include <glm/glm.hpp>
#include <string>
#include <functional>

namespace Rongine {

	struct RenderPassSpec
	{
		std::string Name = "Unnamed Pass";
		Ref<Framebuffer> TargetFramebuffer = nullptr; // nullptr = 默认帧缓冲
		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		bool ClearColorBuffer = true;
		bool ClearDepthBuffer = true;
		int ClearAttachmentIndex = -1;    // 需要额外清除的附件索引 (-1 = 不清除)
		int ClearAttachmentValue = -1;    // 清除附件的填充值
	};

	class RenderPass
	{
	public:
		using ExecuteFn = std::function<void()>;

		RenderPass(const RenderPassSpec& spec);
		~RenderPass() = default;

		void begin();
		void end();

		const std::string& getName() const { return m_spec.Name; }
		const RenderPassSpec& getSpec() const { return m_spec; }
		Ref<Framebuffer> getTargetFramebuffer() const { return m_spec.TargetFramebuffer; }

	private:
		RenderPassSpec m_spec;
	};

}
