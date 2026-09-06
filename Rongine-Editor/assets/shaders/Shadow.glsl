#type vertex
#version 450 core

// 光视角深度 pass：只写深度，不输出颜色。
// 深度由光 space 变换后的 z 决定，PCSS 在 GeometryPass 里用它做遮挡查询。

layout(location = 0) in vec3 a_Position;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;

void main()
{
    gl_Position = u_LightSpaceMatrix * u_Model * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

// 无颜色输出（shadow FBO 的 DrawBuffer 为 GL_NONE）

void main()
{
}
