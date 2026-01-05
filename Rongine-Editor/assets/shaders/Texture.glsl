// ==========================================
// Vertex Shader
// ==========================================
#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;
layout(location = 4) in float a_TilingFactor;

uniform mat4 u_ViewProjection;

out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;
out float v_TilingFactor;

void main()
{
	v_Color = a_Color;
	v_TexCoord = a_TexCoord;
	v_TexIndex = a_TexIndex;
	v_TilingFactor = a_TilingFactor;

	// 直接投影：世界坐标 -> 裁剪坐标
	gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

// ==========================================
// Fragment Shader
// ==========================================
#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;
in float v_TilingFactor;

// 支持 32 个纹理槽 (根据你的 Renderer3DData::MaxTextureSlots)
uniform sampler2D u_Textures[32];

void main()
{
	vec4 texColor = v_Color;

	// ------------------------------------------------------------------
	// 纹理采样逻辑
	// ------------------------------------------------------------------
	
	int index = int(v_TexIndex);
	texColor *= texture(u_Textures[index], v_TexCoord * v_TilingFactor);
	
	color = texColor;
}