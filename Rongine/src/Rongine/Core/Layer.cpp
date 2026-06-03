#include "Rongpch.h"
#include "Layer.h"

namespace Rongine {

	Layer::Layer(const std::string& debugName) 
		:m_debugName(debugName)
	{
	}

	void Layer::attachIfNeeded()
	{
		if(!m_attached)
		{
			onAttach();
			m_attached=true;
		}
	}

	void Layer::detachIfNeeded()
	{
		if(m_attached)
		{
			onDetach();
			m_attached=false;
		}
	}
}
