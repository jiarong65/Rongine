#include "Rongpch.h"
#include "RenderGraph.h"
#include "Rongine/Core/Log.h"

namespace Rongine {

	void RenderGraph::addPass(const RenderPassSpec& spec, PassExecuteFn executeFn)
	{
		PassNode node;
		node.Pass = CreateRef<RenderPass>(spec);
		node.Execute = std::move(executeFn);
		node.Enabled = true;
		m_passes.push_back(std::move(node));
	}

	void RenderGraph::setPassEnabled(const std::string& name, bool enabled)
	{
		for (auto& node : m_passes)
		{
			if (node.Pass->getName() == name)
			{
				node.Enabled = enabled;
				return;
			}
		}
		RONG_CORE_WARN("RenderGraph: Pass '{0}' not found", name);
	}

	void RenderGraph::execute()
	{
		for (auto& node : m_passes)
		{
			if (!node.Enabled)
				continue;

			node.Pass->begin();
			node.Execute(*node.Pass);
			node.Pass->end();
		}
	}

	void RenderGraph::clear()
	{
		m_passes.clear();
	}

}
