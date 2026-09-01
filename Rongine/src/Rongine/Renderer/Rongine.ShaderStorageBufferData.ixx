module;

export module Rongine.ShaderStorageBufferData;

export namespace Rongine {

	enum class ShaderStorageBufferUsage
	{
		None = 0,
		StaticDraw,  // uploaded once, rarely modified
		DynamicDraw  // modified frequently (per-frame particles, animation)
	};
}
