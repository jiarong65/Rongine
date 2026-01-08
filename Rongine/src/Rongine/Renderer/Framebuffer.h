#pragma once
#include "Rongine/Core/Core.h"

namespace Rongine{

	//支持的纹理格式
	enum class FramebufferTextureFormat {
		None=0,

		RGBA8,
		RED_INTEGER,	//存实体id，颜色拾取法
		RG_INTEGER,    //实体id，面id  支持选中面

		DEPTH24STENCIL8,

		Depth = DEPTH24STENCIL8
	};

	//附件规格
	struct FramebufferTextureSpecification 
	{
		FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;

		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(FramebufferTextureFormat format)
			:TextureFormat(format){
		}
	};

	//附件集合
	struct FramebufferAttachmentSpecification
	{
		std::vector<FramebufferTextureSpecification> Attachments;

		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
			: Attachments(attachments) {
		}
	};

	//帧缓冲规格
	struct FramebufferSpecification {
		uint32_t width, height;
		FramebufferAttachmentSpecification Attachments;

		uint32_t samples = 1;

		bool swapChainTarget = false;
	};

	class Framebuffer
	{
	public:
		virtual void bind() = 0;
		virtual void unbind() = 0;

		virtual const FramebufferSpecification& getSpecification()const = 0;

		virtual void resize(uint32_t width, uint32_t height) = 0;

		virtual int readPixel(uint32_t attachmentIndex,int x, int y) = 0;
		virtual std::pair<int, int> readPixelRG(uint32_t attachmentIndex, int x, int y) = 0;
		virtual void clearAttachment(uint32_t attachmentIndex,int value) = 0;
		virtual uint32_t getColorAttachmentRendererID(uint32_t index=0) const = 0;

		static Ref<Framebuffer> create(const FramebufferSpecification& spec);
	};
}


