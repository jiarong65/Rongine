#pragma once
#include "Rongine/Core/Core.h"

namespace Rongine {

	enum class ShaderStorageBufferUsage
	{
		None = 0,
		StaticDraw,  // 数据上传一次，几乎不修改 
		DynamicDraw  // 数据频繁修改 (每帧更新的粒子、动画)
	};

	class ShaderStorageBuffer
	{
	public:
		virtual ~ShaderStorageBuffer() = default;

		virtual void bind(uint32_t bindingPoint) const = 0;
		virtual void unbind() const = 0;

		virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		virtual uint32_t getSize() const = 0;

		virtual void resize(uint32_t size) = 0;

		static Ref<ShaderStorageBuffer> create(uint32_t size, ShaderStorageBufferUsage usage = ShaderStorageBufferUsage::DynamicDraw);
	};

}