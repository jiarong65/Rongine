
#type vertex
#version 330 core
			
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

out vec3 v_Position;
out vec4 v_Color;
out vec2 v_TexCoord;

void main()
{
	v_Position = a_Position;
	v_Color=a_Color;
	v_TexCoord=a_TexCoord;
	//gl_Position = u_ViewProjection*u_Transform*vec4(a_Position,1.0f);	
	gl_Position = u_ViewProjection*vec4(a_Position,1.0f);
}

#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec3 v_Position;
in vec4 v_Color;
in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color;
uniform float u_TilingFactor;

void main()
{
	//color = texture(u_Texture,v_TexCoord*u_TilingFactor)*u_Color;
	color=v_Color;
}