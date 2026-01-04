#pragma once
#include "Rongine/Core/Core.h"

namespace Rongine{
	struct FrameSpecification {
		uint32_t width, height;
		uint32_t samples = 1;

		bool swapChainTarget = false;
	};

	class Framebuffer
	{
	public:
		virtual void bind() = 0;
		virtual void unbind() = 0;
		virtual uint32_t getColorAttachmentRendererID() const = 0;

		virtual const FrameSpecification& getSpecification()const = 0;

		virtual void resize(uint32_t width, uint32_t height) = 0;
		
		static Ref<Framebuffer> create(const FrameSpecification& spec);
	};
}


