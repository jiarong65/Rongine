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
uniform mat4 u_LightSpaceMatrix;

out vec3 v_Position;
out vec3 v_Normal;
out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;
out float v_TilingFactor;
flat out int v_FaceID;
out vec4 v_FragPosLightSpace;

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
    v_FragPosLightSpace = u_LightSpaceMatrix * worldPos;

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
in vec4 v_FragPosLightSpace;

uniform sampler2D u_Textures[32];
uniform vec3 u_ViewPos; // 摄像机位置，用于计算反光

// --- PCSS 阴影 ---
uniform sampler2D u_ShadowMap;
uniform int   u_LightEnabled;
uniform vec3  u_LightDirection;   // 光的传播方向（指向场景）
uniform vec3  u_LightColor;       // 颜色 * 强度
uniform float u_LightSize;        // 光源物理尺寸（世界单位）
uniform float u_LightFrustumWidth;// 光相机正交宽度
uniform float u_LightNear;
uniform float u_LightFar;         // 光视锥远平面（世界单位），用于把 near 换算到 [0,1] 深度空间
uniform int   u_ShadowDebugMode;  // 0=关闭 1=显示阴影因子 2=显示深度图内容

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

// ============================================================================
// PCSS (Percentage-Closer Soft Shadows)
// 三段式: ① blocker 搜索(固定半径) → ② 半影宽度 → ③ 变半径 PCF
// ============================================================================

const int   NUM_BLOCKER_SAMPLES = 16;
const int   NUM_PCF_SAMPLES     = 32;
const float SHADOW_BIAS         = 0.004;

const vec2 POISSON_DISK[32] = vec2[](
    vec2(-0.7052452, -0.3246527), vec2(-0.6809258, 0.01951703),
    vec2(-0.6540927, 0.4161015),  vec2(-0.6261093, -0.6687687),
    vec2(-0.4959436, 0.7771183),  vec2(-0.4607681, -0.1352779),
    vec2(-0.4401494, -0.4167025), vec2(-0.3308589, 0.4133028),
    vec2(-0.2246009, 0.7198596),  vec2(-0.198999, -0.7637837),
    vec2(-0.01701182, -0.38794),  vec2(0.03637227, 0.8996478),
    vec2(0.06709866, 0.3129244),  vec2(0.1803883, -0.6278287),
    vec2(0.2221393, 0.1410322),   vec2(0.3026202, 0.5137209),
    vec2(0.3884895, -0.2189956),  vec2(0.4507513, 0.3310486),
    vec2(0.5330818, -0.7020196),  vec2(0.5322811, -0.3493116),
    vec2(0.5469448, 0.7825489),   vec2(0.6708262, -0.1970734),
    vec2(0.7072182, 0.1632816),   vec2(0.7655836, 0.5476787),
    vec2(0.8094895, -0.5883481),  vec2(0.832663, 0.03988485),
    vec2(0.8995229, 0.3119625),   vec2(0.920516, -0.3201718),
    vec2(0.05555886, -0.9192349), vec2(-0.2602444, -0.1865548),
    vec2(0.3867186, 0.822725),    vec2(-0.1596588, 0.2347522)
);

// 光源尺寸换算到 shadow map UV 空间
float lightSizeUV() { return u_LightSize * u_LightNear / max(u_LightFrustumWidth, 0.0001); }

// near 平面在 [0,1] 归一化深度空间的值：ortho 下 ls.z = (d - near) / (far - near)。
// blocker 搜索半径必须和 ls.z 同一空间，否则量纲不一致（near 用世界单位会算出负半径）。
float lightNearUV() { return u_LightNear / max(u_LightFar - u_LightNear, 0.0001); }

// 单次阴影查询；uv 超出 [0,1] 视为光照范围外 → 不受阴影
float sampleShadow(vec2 uv, float compare, float bias)
{
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        return 1.0;
    return (compare - bias > texture(u_ShadowMap, uv).r) ? 0.0 : 1.0;
}

// 斜率缩放 bias: 表面相对光方向越倾斜(掠射角)，光空间深度沿表面变化越快，
// 需要越大的 bias 才能避免自阴影。tan(θ) = sqrt(1-NdotL²)/NdotL。
float shadowBias(float NdotL)
{
    float tanTheta = sqrt(max(0.0, 1.0 - NdotL * NdotL)) / max(NdotL, 0.05);
    return clamp(0.003 * tanTheta, 0.0015, 0.02);
}

// 逐像素随机旋转角：IGN (Interleaved Gradient Noise) 把屏幕坐标哈希到 [0,1) 再映射到 [0,2π)。
// 固定朝向的泊松盘让相邻像素的采样误差空间相关，半影呈条带 banding；
// 每像素随机旋转把结构化条带打散成肉眼自动平均的细噪点。
float pixelDiskRotation(vec2 fragCoord)
{
    const float TWO_PI = 6.28318530718;
    float noise = fract(52.9829189 * fract(dot(fragCoord, vec2(0.06711056, 0.00583715))));
    return TWO_PI * noise;
}

// ① blocker 搜索: 固定半径内被遮挡深度的平均值
float findBlockerDepth(vec3 ls, float bias)
{
    float searchRadius = lightSizeUV() * (ls.z - lightNearUV());
    float a = pixelDiskRotation(gl_FragCoord.xy);
    mat2 rot = mat2(cos(a), -sin(a), sin(a), cos(a));
    float sum = 0.0;
    float count = 0.0;
    for (int i = 0; i < NUM_BLOCKER_SAMPLES; i++)
    {
        vec2 uv = ls.xy + rot * POISSON_DISK[i] * searchRadius;
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
            continue;
        float d = texture(u_ShadowMap, uv).r;
        if (d < ls.z - bias)
        {
            sum += d;
            count += 1.0;
        }
    }
    return count > 0.0 ? sum / count : 1.0; // 1.0 = 全亮(无遮挡)
}

// ② 半影宽度: 遮挡物离受光点越远、光源越大 → 半影越宽
float penumbraWidth(vec3 ls, float blockerDepth)
{
    return (ls.z - blockerDepth) * lightSizeUV() / max(blockerDepth, 0.0001);
}

// ③ 变半径 PCF
float pcfFilter(vec3 ls, float radius, float bias)
{
    float a = pixelDiskRotation(gl_FragCoord.xy);
    mat2 rot = mat2(cos(a), -sin(a), sin(a), cos(a));
    float sum = 0.0;
    for (int i = 0; i < NUM_PCF_SAMPLES; i++)
        sum += sampleShadow(ls.xy + rot * POISSON_DISK[i] * radius, ls.z, bias);
    return sum / float(NUM_PCF_SAMPLES);
}

float PCSS(vec4 fragPosLightSpace, float NdotL)
{
    vec3 ls = fragPosLightSpace.xyz / fragPosLightSpace.w * 0.5 + 0.5;
    if (ls.z > 1.0) return 1.0; // 光视锥远平面之外

    float bias = shadowBias(NdotL);
    float blocker = findBlockerDepth(ls, bias);
    if (blocker >= 1.0) return 1.0;         // 无遮挡 → 全亮
    if (u_LightSize <= 0.0) return 0.0;     // 点光源语义 → 全影

    float radius = max(penumbraWidth(ls, blocker), 1.0 / 2048.0);
    return pcfFilter(ls, radius, bias);
}


// ----------------------------------------------------------------------------

void main()
{
    // --- 阴影调试视图：在一切着色之前直接输出诊断信息 ---
    if (u_ShadowDebugMode > 0 && u_LightEnabled == 1)
    {
        vec3 lsDbg = v_FragPosLightSpace.xyz / v_FragPosLightSpace.w * 0.5 + 0.5;
        if (u_ShadowDebugMode == 1)
        {
            float NdotLdbg = max(dot(normalize(v_Normal), normalize(-u_LightDirection)), 0.0);
            float s = (lsDbg.z > 1.0 || NdotLdbg <= 0.0) ? 1.0 : PCSS(v_FragPosLightSpace, NdotLdbg);
            color = vec4(vec3(s), 1.0);
        }
        else // 2: 深度图内容（均匀灰 = 只有地面；更黑的斑块 = 深度图里的遮挡物）
        {
            float d = (lsDbg.x < 0.0 || lsDbg.x > 1.0 || lsDbg.y < 0.0 || lsDbg.y > 1.0) ? 1.0
                      : texture(u_ShadowMap, lsDbg.xy).r;
            color = vec4(vec3(d), 1.0);
        }
        idOutput = ivec4(u_EntityID, v_FaceID, -1, -1);
        return;
    }

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
    // D. 光照计算（仅方向光/太阳，见 E 段；kD 供环境光使用）
    // ---------------------------------------------------------

    // Fresnel 决定漫反射/镜面反射的能量分配（供环境光段使用）
    vec3 kS = FresnelSchlick(max(dot(N, V), 0.0), F0);
    // kD 是漫反射比例 (能量守恒：进来的光 - 反射掉的光)
    vec3 kD = vec3(1.0) - kS;
    // 金属没有漫反射 (被自由电子吸收了)，所以乘以 (1 - metallic)
    kD *= 1.0 - metallic;

    // ---------------------------------------------------------
    // E. 方向光（太阳）+ PCSS 软阴影
    // ---------------------------------------------------------
    vec3 Lo = vec3(0.0);
    if (u_LightEnabled == 1)
    {
        vec3 Lsun = normalize(-u_LightDirection);
        vec3 Hsun = normalize(V + Lsun);
        float NdotLsun = max(dot(N, Lsun), 0.0);
        if (NdotLsun > 0.0)
        {
            float NDFs = DistributionGGX(N, Hsun, roughness);
            float Gs   = GeometrySmith(N, V, Lsun, roughness);
            vec3  Fs   = FresnelSchlick(max(dot(Hsun, V), 0.0), F0);
            vec3 specularSun = (NDFs * Gs * Fs) / (4.0 * max(dot(N, V), 0.0) * NdotLsun + 0.0001);
            vec3 kSsun = Fs;
            vec3 kDsun = (vec3(1.0) - kSsun) * (1.0 - metallic);

            float shadow = PCSS(v_FragPosLightSpace, NdotLsun);
            Lo += (kDsun * albedo / PI + specularSun) * u_LightColor * NdotLsun * shadow;
        }
    }

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