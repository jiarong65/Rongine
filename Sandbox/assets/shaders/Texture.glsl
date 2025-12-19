
#type vertex
#version 330 core
			
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;
layout(location = 4) in float a_TilingFactor;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

out vec3 v_Position;
out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;
out float v_TilingFactor;

void main()
{
	v_Position = a_Position;
	v_Color=a_Color;
	v_TexCoord=a_TexCoord;
	v_TexIndex=a_TexIndex;
	v_TilingFactor=a_TilingFactor;
	//gl_Position = u_ViewProjection*u_Transform*vec4(a_Position,1.0f);	
	gl_Position = u_ViewProjection*vec4(a_Position,1.0f);
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec3 v_Position;
in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;
in float v_TilingFactor;

uniform sampler2D u_Textures[32];
uniform vec4 u_Color;
uniform float u_TilingFactor;

void main()
{
	//color = texture(u_Texture,v_TexCoord*u_TilingFactor)*u_Color;
	color=texture(u_Textures[int(v_TexIndex)],v_TexCoord*v_TilingFactor)*v_Color;
}