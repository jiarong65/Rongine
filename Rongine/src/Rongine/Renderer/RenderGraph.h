#pragma once
#include "Rongine/Core/Core.h"
#include "Rongine/Renderer/RenderPass.h"
#include <string>
#include <vector>
#include <functional>

namespace Rongine {

	class RenderGraph
	{
	public:
		using PassExecuteFn = std::function<void(RenderPass&)>;

		struct PassNode
		{
			Ref<RenderPass> Pass;
			PassExecuteFn Execute;
			bool Enabled = true;
		};

		RenderGraph() = default;
		~RenderGraph() = default;

		void addPass(const RenderPassSpec& spec, PassExecuteFn executeFn);

		void setPassEnabled(const std::string& name, bool enabled);

		void execute();

		void clear();

		const std::vector<PassNode>& getPasses() const { return m_passes; }

	private:
		std::vector<PassNode> m_passes;
	};

}
