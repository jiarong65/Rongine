// assets/shaders/Line.glsl

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in int a_EdgeID;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

// 传给片元着色器 (flat 表示不插值，保证整数传递准确)
flat out int v_EdgeID;

void main()
{
    v_EdgeID = a_EdgeID;
    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out ivec4 idOutput; // 输出到实体 ID 纹理

uniform vec4 u_Color;        // 默认颜色 (黑色)
uniform int u_EntityID;      // 当前画的物体 ID
uniform int u_SelectedEdgeID;// 当前选中的边 ID (用于高亮)

uniform int u_HoveredEntityID;
uniform int u_HoveredEdgeID;

flat in int v_EdgeID;

void main()
{
// 1. 颜色逻辑
    vec4 finalColor = u_Color;

    // 1. 选中高亮 (橙色)
    if (u_SelectedEdgeID != -1 && v_EdgeID == u_SelectedEdgeID)
    {
        finalColor = vec4(1.0, 0.6, 0.0, 1.0); 
    }
    // 2. 悬停高亮 (青色/亮白) - 仅当未被选中时
    else if (u_HoveredEntityID == u_EntityID && u_HoveredEdgeID != -1 && v_EdgeID == u_HoveredEdgeID)
    {
        finalColor = vec4(0.5, 0.8, 1.0, 1.0); // 悬停：青色
    }

    color = finalColor;

    // 2. 拾取逻辑
    // 写入 (物体ID, 边ID)
    // 这样鼠标读到这里时，就知道是哪个物体的哪条边
    idOutput = ivec4(u_EntityID, -1, v_EdgeID, -1);
    //idOutput = ivec4(-1);
}