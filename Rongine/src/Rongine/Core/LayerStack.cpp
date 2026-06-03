#include "Rongpch.h"
#include "LayerStack.h"

namespace Rongine {

	LayerStack::~LayerStack()
	{
		for (Layer* layer : m_layers)
		{
			layer->detachIfNeeded();
			delete layer;
		}
	}

	void LayerStack::pushLayer(Layer* layer)
	{
		m_layers.emplace(m_layers.begin()+m_layerInsertIndex, layer);
		m_layerInsertIndex++;
		if(!m_deferAttach)
			layer->attachIfNeeded();
	}

	void LayerStack::pushOverLayer(Layer* layer)
	{
		m_layers.emplace_back(layer);
		if(!m_deferAttach)
			layer->attachIfNeeded();
	}

	void LayerStack::popLayer(Layer* layer)
	{
		auto it = std::find(m_layers.begin(), m_layers.end(), layer);
		if (it != m_layers.end())
		{
			(*it)->detachIfNeeded();
			m_layers.erase(it);
			m_layerInsertIndex--;
		}
	}

	void LayerStack::popOverLayer(Layer* layer)
	{
		auto it = std::find(m_layers.begin(), m_layers.end(), layer);
		if (it != m_layers.end())
		{
			(*it)->detachIfNeeded();
			m_layers.erase(it);
		}
	}

	void LayerStack::attachAll()
	{
		for(Layer* layer : m_layers)
			layer->attachIfNeeded();
	}

}

