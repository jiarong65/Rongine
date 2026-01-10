// assets/shaders/Line.glsl

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in int a_EntityID;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

// 传给片元着色器 (flat 表示不插值，保证整数传递准确)
flat out int v_EntityID;

void main()
{
    v_EntityID = a_EntityID;
    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out int entityID;

uniform vec4 u_Color;
flat in int v_EntityID;

void main()
{
    color = u_Color;       // 线条颜色 (比如黑色)
    entityID = v_EntityID; // 实体ID (用于鼠标拾取)
}