#pragma once
#include "Rongine/Layer.h"
#include <vector>

namespace Rongine {

	class RONG_API LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void pushLayer(Layer* layer);
		void pushOverLayer(Layer* layer);
		void popLayer(Layer* layer);
		void popOverLayer(Layer* layer);


		std::vector<Layer*>::iterator begin() { return m_layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_layers.end(); }
		std::vector<Layer*>::reverse_iterator rbegin() { return m_layers.rbegin(); }
		std::vector<Layer*>::reverse_iterator rend() { return m_layers.rend(); }

		std::vector<Layer*>::const_iterator begin() const { return m_layers.begin(); }
		std::vector<Layer*>::const_iterator end()	const { return m_layers.end(); }
		std::vector<Layer*>::const_reverse_iterator rbegin() const { return m_layers.rbegin(); }
		std::vector<Layer*>::const_reverse_iterator rend() const { return m_layers.rend(); }

	private:
		std::vector<Layer* > m_layers;
		unsigned int m_layerInsertIndex = 0;
	};

}

