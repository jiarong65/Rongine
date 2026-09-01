module;
#include <string>
#include <vector>
#include <algorithm>

export module Rongine.LayerStack;

export import Rongine.Core;
export import Rongine.Events;

export namespace Rongine {

	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void onAttach() {};
		virtual void onDetach() {};
		virtual void onUpdate(Timestep ts) {};
		virtual void onImGuiRender() {};
		virtual void onEvent(Event& e) {};

		void attachIfNeeded();
		void detachIfNeeded();

		const std::string& getName() { return m_debugName; }
	protected:
		std::string m_debugName;
		bool m_attached = false;
	};

	class LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void pushLayer(Layer* layer);
		void pushOverLayer(Layer* layer);
		void popLayer(Layer* layer);
		void popOverLayer(Layer* layer);

		void setDeferAttach(bool defer) { m_deferAttach = defer; }
		void attachAll();

		std::vector<Layer*>::iterator begin() { return m_layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_layers.end(); }
		std::vector<Layer*>::reverse_iterator rbegin() { return m_layers.rbegin(); }
		std::vector<Layer*>::reverse_iterator rend() { return m_layers.rend(); }

		std::vector<Layer*>::const_iterator begin() const { return m_layers.begin(); }
		std::vector<Layer*>::const_iterator end() const { return m_layers.end(); }
		std::vector<Layer*>::const_reverse_iterator rbegin() const { return m_layers.rbegin(); }
		std::vector<Layer*>::const_reverse_iterator rend() const { return m_layers.rend(); }

	private:
		std::vector<Layer*> m_layers;
		unsigned int m_layerInsertIndex = 0;
		bool m_deferAttach = false;
	};

	Layer::Layer(const std::string& debugName)
		: m_debugName(debugName)
	{
	}

	void Layer::attachIfNeeded()
	{
		if (!m_attached)
		{
			onAttach();
			m_attached = true;
		}
	}

	void Layer::detachIfNeeded()
	{
		if (m_attached)
		{
			onDetach();
			m_attached = false;
		}
	}

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
		m_layers.emplace(m_layers.begin() + m_layerInsertIndex, layer);
		m_layerInsertIndex++;
		if (!m_deferAttach)
			layer->attachIfNeeded();
	}

	void LayerStack::pushOverLayer(Layer* layer)
	{
		m_layers.emplace_back(layer);
		if (!m_deferAttach)
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
		for (Layer* layer : m_layers)
			layer->attachIfNeeded();
	}
}
