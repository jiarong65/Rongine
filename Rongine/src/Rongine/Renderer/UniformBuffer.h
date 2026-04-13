#pragma once
#include "Rongine/Core/Core.h"

namespace Rongine {

	class UniformBuffer
	{
	public:
		virtual ~UniformBuffer() = default;

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void bind(uint32_t bindingPoint) const = 0;

		static Ref<UniformBuffer> create(uint32_t size, uint32_t bindingPoint);
	};

}
