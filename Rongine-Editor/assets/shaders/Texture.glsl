// ================= Vertex Shader =================
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in vec2 a_TexCoord;
layout(location = 4) in float a_TexIndex;
layout(location = 5) in float a_TilingFactor;
layout(location = 6) in int a_FaceID;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;

out vec3 v_Position;
out vec3 v_Normal;
out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;
out float v_TilingFactor;
flat out int v_FaceID;

void main()
{

    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_Position = worldPos.xyz; 
    
    v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    v_TexIndex = a_TexIndex;
    v_TilingFactor = a_TilingFactor;
    v_FaceID = a_FaceID;

    // 如果是 BatchRenderer (drawRotatedCube)，a_Position 已经是世界坐标
    // 如果是 drawModel，这里假设 MVP 矩阵正确处理了变换
    gl_Position = u_ViewProjection * worldPos;
}

// ================= Fragment Shader =================
#type fragment
#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out ivec4 idOutput;    // 输出到 ID 纹理 (Attachment 1)

in vec3 v_Position;
in vec3 v_Normal;
in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;
in float v_TilingFactor;
flat in int v_FaceID;

uniform sampler2D u_Textures[32]; 
uniform vec3 u_ViewPos; // 摄像机位置，用于计算反光

uniform int u_SelectedEntityID;
uniform int u_SelectedFaceID;
uniform int u_HoveredEntityID;
uniform int u_HoveredFaceID;

uniform int u_EntityID; //实体id

uniform vec3 u_Albedo;      // 颜色
uniform float u_Roughness;  // 粗糙度
uniform float u_Metallic;   // 金属度

void main()
{
    // --- 1. 采样纹理颜色 ---
    vec4 texColor = v_Color;
    int index = int(v_TexIndex + 0.5);

    switch(index)
    {
        case 0: texColor *= texture(u_Textures[0], v_TexCoord * v_TilingFactor); break;
        case 1: texColor *= texture(u_Textures[1], v_TexCoord * v_TilingFactor); break;
        case 2: texColor *= texture(u_Textures[2], v_TexCoord * v_TilingFactor); break;
        case 3: texColor *= texture(u_Textures[3], v_TexCoord * v_TilingFactor); break;
        case 4: texColor *= texture(u_Textures[4], v_TexCoord * v_TilingFactor); break;
        case 5: texColor *= texture(u_Textures[5], v_TexCoord * v_TilingFactor); break;
        case 6: texColor *= texture(u_Textures[6], v_TexCoord * v_TilingFactor); break;
        case 7: texColor *= texture(u_Textures[7], v_TexCoord * v_TilingFactor); break;
        case 8: texColor *= texture(u_Textures[8], v_TexCoord * v_TilingFactor); break;
        case 9: texColor *= texture(u_Textures[9], v_TexCoord * v_TilingFactor); break;
        case 10: texColor *= texture(u_Textures[10], v_TexCoord * v_TilingFactor); break;
        case 11: texColor *= texture(u_Textures[11], v_TexCoord * v_TilingFactor); break;
        case 12: texColor *= texture(u_Textures[12], v_TexCoord * v_TilingFactor); break;
        case 13: texColor *= texture(u_Textures[13], v_TexCoord * v_TilingFactor); break;
        case 14: texColor *= texture(u_Textures[14], v_TexCoord * v_TilingFactor); break;
        case 15: texColor *= texture(u_Textures[15], v_TexCoord * v_TilingFactor); break;
        case 16: texColor *= texture(u_Textures[16], v_TexCoord * v_TilingFactor); break;
        case 17: texColor *= texture(u_Textures[17], v_TexCoord * v_TilingFactor); break;
        case 18: texColor *= texture(u_Textures[18], v_TexCoord * v_TilingFactor); break;
        case 19: texColor *= texture(u_Textures[19], v_TexCoord * v_TilingFactor); break;
        case 20: texColor *= texture(u_Textures[20], v_TexCoord * v_TilingFactor); break;
        case 21: texColor *= texture(u_Textures[21], v_TexCoord * v_TilingFactor); break;
        case 22: texColor *= texture(u_Textures[22], v_TexCoord * v_TilingFactor); break;
        case 23: texColor *= texture(u_Textures[23], v_TexCoord * v_TilingFactor); break;
        case 24: texColor *= texture(u_Textures[24], v_TexCoord * v_TilingFactor); break;
        case 25: texColor *= texture(u_Textures[25], v_TexCoord * v_TilingFactor); break;
        case 26: texColor *= texture(u_Textures[26], v_TexCoord * v_TilingFactor); break;
        case 27: texColor *= texture(u_Textures[27], v_TexCoord * v_TilingFactor); break;
        case 28: texColor *= texture(u_Textures[28], v_TexCoord * v_TilingFactor); break;
        case 29: texColor *= texture(u_Textures[29], v_TexCoord * v_TilingFactor); break;
        case 30: texColor *= texture(u_Textures[30], v_TexCoord * v_TilingFactor); break;
        case 31: texColor *= texture(u_Textures[31], v_TexCoord * v_TilingFactor); break;
    }

    // --- 2. PBR ---

    // 基础色 = 材质颜色 * 纹理颜色 
    vec3 albedo = u_Albedo * texColor.rgb;

    vec3 normal = normalize(v_Normal);
    vec3 viewDir = normalize(u_ViewPos - v_Position);
    
    vec3 lightDir = normalize(vec3(-1.0, 1.0, 1.0));
    

    // B. 漫反射 (Diffuse) - 展现物体形状
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * albedo; 

    // C. 镜面光 (Specular) - 高光部分
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float shininess = (1.0 - u_Roughness) * 256.0;
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess); 
    
    vec3 specularColor;
    vec3 finalDiffuse;

    // 金属/非金属 逻辑区分
    if (u_Metallic > 0.5) 
    {
        // 金属：漫反射很弱(黑色)，高光带有物体本身的颜色
        specularColor = albedo; 
        finalDiffuse = vec3(0.0); // 金属几乎没有漫反射
    }
    else 
    {
        // 塑料/电介质：高光是白色的，漫反射是物体颜色
        specularColor = vec3(1.0); 
        finalDiffuse = diffuse;
    }

    vec3 finalSpecular = spec * specularColor;

    // 环境光 (Ambient)
    vec3 ambient = vec3(0.1) * albedo;

    // 组合
    vec3 lighting = ambient + finalDiffuse + finalSpecular;

    // Gamma 矫正 (让颜色看起来更准确，不那么暗)
    lighting = pow(lighting, vec3(1.0 / 2.2));

    vec4 finalColor = vec4(lighting, texColor.a);

    // 1. 判断选中状态 (Selected) - 橙色
    if (u_SelectedEntityID >= 0 && u_EntityID == u_SelectedEntityID)
    {
        if (v_FaceID == u_SelectedFaceID)
        {
            finalColor = mix(finalColor, vec4(1.0, 0.6, 0.0, 1.0), 0.5); // 选中：深橙色
        }
        else if (u_SelectedFaceID == -1)
        {
            finalColor = mix(finalColor, vec4(1.0, 1.0, 0.0, 1.0), 0.3); // 选中整体：黄色
        }
    }
    
    // 2. 判断悬停状态 (Hovered) - 浅亮色 (仅当该面没有被选中时才显示悬停色)
    // 防止“选中”和“悬停”颜色叠加变得很怪
    bool isSelected = (u_EntityID == u_SelectedEntityID && v_FaceID == u_SelectedFaceID);
    
    if (!isSelected && u_HoveredEntityID >= 0 && u_EntityID == u_HoveredEntityID)
    {
        if (v_FaceID == u_HoveredFaceID)
        {
             // 悬停：浅白色/淡黄色叠加，亮度提升
             finalColor = mix(finalColor, vec4(1.0, 1.0, 0.8, 1.0), 0.3); 
        }
    }
    
    color = finalColor;

    idOutput = ivec4(u_EntityID, v_FaceID, -1, -1);
}