module;
#include <cstdint>
#include <vector>
#include <initializer_list>

export module Rongine.FramebufferData;

export namespace Rongine {

	enum class FramebufferTextureFormat {
		None = 0,

		RGBA8,
		RED_INTEGER,	// Entity ID picking
		RG_INTEGER,
		RGBA_INTEGER,   // entity id, face id, edge id

		DEPTH24STENCIL8,

		Depth = DEPTH24STENCIL8
	};

	struct FramebufferTextureSpecification
	{
		FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;

		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(FramebufferTextureFormat format)
			: TextureFormat(format) {
		}
	};

	struct FramebufferAttachmentSpecification
	{
		std::vector<FramebufferTextureSpecification> Attachments;

		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
			: Attachments(attachments) {
		}
	};

	struct FramebufferSpecification {
		uint32_t width, height;
		FramebufferAttachmentSpecification Attachments;

		uint32_t samples = 1;

		bool swapChainTarget = false;
	};
}
