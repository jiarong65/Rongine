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

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
// 1. 正态分布函数 D (Trowbridge-Reitz GGX)
// 决定高光亮斑的大小和锐利度
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001); // 防止除以0
}

// ----------------------------------------------------------------------------
// 2. 几何函数 G (Smith's Schlick-GGX)
// 模拟微表面的自遮挡，粗糙度越高，遮挡越多，光越暗
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0; // 直接光照下的 k 计算公式

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// ----------------------------------------------------------------------------
// 3. 菲涅尔方程 F (Fresnel-Schlick)
// 描述光线在不同角度下的反射率。F0 是 0 度角的反射率。
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

//模拟环境反射的菲涅尔 (带粗糙度阻尼)
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ----------------------------------------------------------------------------

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

    // B. 准备 PBR 参数
    vec3 albedo     = pow(u_Albedo * texColor.rgb, vec3(2.2)); // 转换到线性空间计算
    float roughness = u_Roughness;
    float metallic  = u_Metallic;
    
    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_ViewPos - v_Position);

    // C. 基础反射率 F0
    // 非金属(电介质)通常是 0.04，金属则是自身的 Albedo 颜色
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // ---------------------------------------------------------
    // D. 光照计算 (针对单个定向光)
    // ---------------------------------------------------------
    
    // 定义一个定向光 (类似于太阳)
    vec3 lightPos = u_ViewPos; 
    vec3 L = normalize(lightPos - v_Position);
    vec3 H = normalize(V + L);
    vec3 radiance = vec3(3.0); // 光源强度 (可以调大一点让高光更亮)

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);
       
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    // kS 是镜面反射比例 (等于 Fresnel)
    vec3 kS = F;
    // kD 是漫反射比例 (能量守恒：进来的光 - 反射掉的光)
    vec3 kD = vec3(1.0) - kS;
    // 金属没有漫反射 (被自由电子吸收了)，所以乘以 (1 - metallic)
    kD *= 1.0 - metallic;	  

    // N dot L (Lambert 因子)
    float NdotL = max(dot(N, L), 0.0);        

    // 最终出射光线 Lo
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    // -------------------------------------------------------------------------
    //  伪造环境光 (Fake IBL) - 让金属看起来像金属
    // -------------------------------------------------------------------------
    
    // A. 计算简单的环境漫反射 (Ambient Diffuse)
    // 类似于半球光：上面亮，下面暗
    vec3 up = vec3(0.0, 1.0, 0.0);
    float hemiMix = (dot(N, up) * 0.5 + 0.5);
    vec3 ambientLightColor = mix(vec3(0.1, 0.1, 0.15), vec3(0.3, 0.3, 0.35), hemiMix); // 地面灰蓝 -> 天空灰白
    vec3 ambientDiffuse = kD * albedo * ambientLightColor;
    
    // B. 计算伪造的环境镜面反射 (Ambient Specular)
    vec3 R = reflect(-V, N); // 反射向量
    
    // 伪造一个“天空盒”颜色：
    // 假设天空是蓝色的，地平线是白色的，地面是深色的
    float horizon = dot(R, up); // -1 (地) 到 1 (天)
    vec3 skyColor = mix(vec3(0.1), vec3(0.5, 0.7, 1.0), smoothstep(-0.2, 0.5, horizon)); // 简单的蓝天梯度
    
    // 粗糙度越高，反射越模糊(也就是越接近平均色)
    vec3 prefilteredColor = skyColor; 
    
    // 环境光的菲涅尔 (从 F0 到 1.0，取决于视角)
    vec3 F_env = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    // 最终的环境镜面光
    vec3 ambientSpecular = prefilteredColor * F_env;
    
    // 粗糙度遮蔽：越粗糙，镜面反射越弱
    // 这是一个经验近似，为了不写复杂的 LUT
    ambientSpecular *= (1.0 - roughness); 

    vec3 ambient = ambientDiffuse + ambientSpecular;

    // -------------------------------------------------------------------------

    vec3 colorLinear = ambient + Lo;
    // ---------------------------------------------------------
    // F. 后处理 (Tone Mapping & Gamma)
    // ---------------------------------------------------------
    
    // HDR Tone Mapping (Reinhard) - 这是一个很简单的版本，防止过曝变纯白
    colorLinear = colorLinear / (colorLinear + vec3(1.0));
    
    // Gamma Correction (转回 sRGB 空间显示)
    colorLinear = pow(colorLinear, vec3(1.0/2.2)); 

    vec4 finalColor = vec4(colorLinear, texColor.a);

    // --- 选中与悬停高亮逻辑 (保持不变) ---
    if (u_SelectedEntityID >= 0 && u_EntityID == u_SelectedEntityID)
    {
        if (v_FaceID == u_SelectedFaceID)
            finalColor = mix(finalColor, vec4(1.0, 0.6, 0.0, 1.0), 0.5); 
        else if (u_SelectedFaceID == -1)
            finalColor = mix(finalColor, vec4(1.0, 1.0, 0.0, 1.0), 0.3);
    }
    
    bool isSelected = (u_EntityID == u_SelectedEntityID && v_FaceID == u_SelectedFaceID);
    if (!isSelected && u_HoveredEntityID >= 0 && u_EntityID == u_HoveredEntityID)
    {
        if (v_FaceID == u_HoveredFaceID)
             finalColor = mix(finalColor, vec4(1.0, 1.0, 0.8, 1.0), 0.3); 
    }
    
    color = finalColor;
    idOutput = ivec4(u_EntityID, v_FaceID, -1, -1);
}