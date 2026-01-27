#version 450 core

// ===============================================================================================
// 1. 配置与定义
// ===============================================================================================

// 定义工作组大小 (8x8 = 64 线程)
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// 输出图像 (Binding 0): 用于显示给 ImGui，经过了平均和色彩校正
layout(rgba32f, binding = 0) uniform image2D img_output;

// 累积图像 (Binding 4): 用于存储历史累加值 (线性空间 HDR)
layout(rgba32f, binding = 4) uniform image2D img_accum;

// 常量定义
const float PI = 3.14159265359;
const float EPSILON = 0.0001;
const float INFINITY = 10000.0;

// ===============================================================================================
// 2. 数据结构 (必须与 C++ RenderTypes.h 对齐)
// ===============================================================================================

struct GPUVertex {
    vec3 Position; float _pad1;
    vec3 Normal;   float _pad2;
    vec2 TexCoord; vec2 _pad3;
};

struct TriangleData {
    uint v0, v1, v2;
    uint MaterialID;
};

// 使用 vec4 强制对齐，防止 std430 布局问题
struct GPUMaterial {
    vec4 AlbedoRoughness; // rgb = Albedo (Color), a = Roughness
    vec4 MetalEmission;   // r = Metallic, g = Emission, b,a = Padding
};

// ===============================================================================================
// 3. SSBO 缓冲区绑定
// ===============================================================================================

layout(std430, binding = 1) readonly buffer VerticesBuffer { 
    GPUVertex Vertices[]; 
};

layout(std430, binding = 2) readonly buffer TrianglesBuffer { 
    TriangleData Triangles[]; 
};

layout(std430, binding = 3) readonly buffer MaterialsBuffer { 
    GPUMaterial Materials[]; 
};

// ===============================================================================================
// 4. Uniforms
// ===============================================================================================

uniform float u_Time;
uniform mat4 u_InverseProjection;
uniform mat4 u_InverseView;
uniform vec3 u_CameraPos;
uniform int u_FrameIndex; // 当前帧索引，用于累积平均

// ===============================================================================================
// 5. 随机数生成器 (PCG Hash)
// ===============================================================================================

uint seed = 0;

// 初始化随机数种子 (基于像素坐标和帧数，确保时空随机性)
void InitRNG(uvec2 pixel_coords, uint frame) {
    seed = pixel_coords.y * 1920 + pixel_coords.x + frame * 719393;
}

// 生成 [0, 1] 范围的随机浮点数
float RandomFloat() {
    seed = seed * 747796405 + 2891336453;
    uint result = ((seed >> ((seed >> 28) + 4)) ^ seed) * 277803737;
    result = (result >> 22) ^ result;
    return float(result) / 4294967295.0;
}

// ===============================================================================================
// 6. 几何求交函数 (Ray-Triangle Intersection)
// ===============================================================================================

// Möller–Trumbore 算法
// 返回: t (距离)。如果没击中返回 -1.0
float HitTriangle(vec3 rayOrigin, vec3 rayDir, vec3 v0, vec3 v1, vec3 v2)
{
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(rayDir, edge2);
    float a = dot(edge1, h);

    // 射线与三角形平行
    if (a > -EPSILON && a < EPSILON) return -1.0;

    float f = 1.0 / a;
    vec3 s = rayOrigin - v0;
    float u = f * dot(s, h);

    if (u < 0.0 || u > 1.0) return -1.0;

    vec3 q = cross(s, edge1);
    float v = f * dot(rayDir, q);

    if (v < 0.0 || u + v > 1.0) return -1.0;

    float t = f * dot(edge2, q);

    if (t > EPSILON) return t;
    else return -1.0;
}

// ===============================================================================================
// 7. PBR 核心函数 (Cook-Torrance BRDF)
// ===============================================================================================

// 法线分布函数 (Normal Distribution Function) - Trowbridge-Reitz GGX
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 0.0000001);
}

// 几何遮蔽函数 (Geometry Function) - Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / max(denom, 0.0000001);
}

// 史密斯几何函数 (Smith's method)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 菲涅尔方程 (Fresnel Equation) - Schlick Approximation
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ===============================================================================================
// 8. 主函数 (Main)
// ===============================================================================================

void main() 
{
    // 获取当前像素坐标
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 img_size = imageSize(img_output);

    // 边界检查
    if (pixel_coords.x >= img_size.x || pixel_coords.y >= img_size.y) return;

    // -------------------------------------------------------------------------
    // A. 射线生成 (Ray Generation) & 抗锯齿 (Anti-Aliasing)
    // -------------------------------------------------------------------------
    
    // 初始化随机数状态
    InitRNG(pixel_coords, u_FrameIndex);

    // 计算子像素抖动 (Jitter)
    // 在 [-0.5, 0.5] 范围内随机偏移，使每一帧采样像素的不同位置
    vec2 jitter = vec2(RandomFloat(), RandomFloat()) - 0.5;

    // 将像素坐标转为 NDC [-1, 1]
    vec2 uv = (vec2(pixel_coords) + 0.5 + jitter) / vec2(img_size) * 2.0 - 1.0;

    // 逆投影变换：屏幕空间 -> 观察空间 (View Space)
    vec4 target = u_InverseProjection * vec4(uv, 1.0, 1.0);
    vec3 rayDirView = normalize(target.xyz / target.w);

    // 逆视图变换：观察空间 -> 世界空间 (World Space)
    vec3 rayDir = normalize(mat3(u_InverseView) * rayDirView);
    vec3 rayOrigin = u_CameraPos;

    // -------------------------------------------------------------------------
    // B. 场景遍历 (Scene Traversal) - 寻找最近的交点
    // -------------------------------------------------------------------------

    float closestT = INFINITY;
    int hitTriangleIndex = -1;
    uint numTriangles = Triangles.length();

    // 暴力循环遍历所有三角形 (后续需优化为 BVH)
    for (uint i = 0; i < numTriangles; i++)
    {
        TriangleData tri = Triangles[i];
        
        vec3 v0 = Vertices[tri.v0].Position;
        vec3 v1 = Vertices[tri.v1].Position;
        vec3 v2 = Vertices[tri.v2].Position;

        float t = HitTriangle(rayOrigin, rayDir, v0, v1, v2);

        if (t > 0.0 && t < closestT)
        {
            closestT = t;
            hitTriangleIndex = int(i);
        }
    }

    // -------------------------------------------------------------------------
    // C. 着色计算 (Shading)
    // -------------------------------------------------------------------------

    vec3 currentFrameColor = vec3(0.0); // 线性 HDR 颜色

    if (hitTriangleIndex != -1)
    {
        // 1. 准备材质与几何数据
        TriangleData tri = Triangles[hitTriangleIndex];
        GPUMaterial rawMat = Materials[tri.MaterialID];

        // 解包材质属性
        vec3 albedo     = pow(rawMat.AlbedoRoughness.rgb, vec3(2.2)); // sRGB -> Linear
        float roughness = rawMat.AlbedoRoughness.w;
        float metallic  = rawMat.MetalEmission.x;
        float emission  = rawMat.MetalEmission.y;

        // 计算交点位置与法线
        vec3 hitPos = rayOrigin + rayDir * closestT;
        vec3 v0 = Vertices[tri.v0].Position;
        vec3 v1 = Vertices[tri.v1].Position;
        vec3 v2 = Vertices[tri.v2].Position;
        
        // 计算面法线 (Flat Shading)
        vec3 N = normalize(cross(v1 - v0, v2 - v0)); 
        vec3 V = normalize(u_CameraPos - hitPos); // 观察方向

        // 2. 光源定义 (硬编码的点光源，跟随相机移动)
        vec3 lightPos = u_CameraPos + vec3(2.0, 4.0, 2.0); 
        vec3 lightColor = vec3(300.0, 300.0, 300.0); 

        vec3 L = normalize(lightPos - hitPos); // 光线方向
        vec3 H = normalize(V + L);             // 半程向量
        float dist = length(lightPos - hitPos);
        float attenuation = 1.0 / (dist * dist);
        vec3 radiance = lightColor * attenuation;

        // 3. PBR 计算 (Cook-Torrance)
        
        // F0: 基础反射率
        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, albedo, metallic);

        // Specular BRDF 项
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
        vec3 specular = numerator / denominator;
        
        // 能量守恒 (kS + kD = 1.0)
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= (1.0 - metallic); // 金属没有漫反射

        // 最终光照合成
        float NdotL = max(dot(N, L), 0.0);                
        vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
        
        // 环境光 (简单常数)
        vec3 ambient = vec3(0.03) * albedo;
        
        // 自发光
        vec3 emissive = albedo * emission * 10.0;

        currentFrameColor = ambient + Lo + emissive;
    }
    else 
    {
        // 背景 (简单天空)
        float t = 0.5 * (rayDir.y + 1.0);
        currentFrameColor = mix(vec3(1.0), vec3(0.5, 0.7, 1.0), t);
    }

    // -------------------------------------------------------------------------
    // D. 累积与后处理 (Accumulation & Post-Processing)
    // -------------------------------------------------------------------------

    vec3 accumulatedColor = currentFrameColor;

    // 如果不是第一帧，从 img_accum 读取历史数据并相加
    if (u_FrameIndex > 1)
    {
        vec3 history = imageLoad(img_accum, pixel_coords).rgb;
        accumulatedColor = history + currentFrameColor;
    }

    // 1. 将新的累积值写入历史缓冲区 (保存的是 sum)
    imageStore(img_accum, pixel_coords, vec4(accumulatedColor, 1.0));

    // 2. 计算平均值 (Sum / Count) -> 得到当前的画面
    vec3 averagedColor = accumulatedColor / float(u_FrameIndex);

    // 3. 色调映射 (Tone Mapping) - Reinhard
    // 将 HDR 映射回 LDR [0, 1]
    averagedColor = averagedColor / (averagedColor + vec3(1.0));

    // 4. Gamma 校正 (Linear -> sRGB)
    averagedColor = pow(averagedColor, vec3(1.0/2.2));

    // 5. 写入最终显示缓冲区
    imageStore(img_output, pixel_coords, vec4(averagedColor, 1.0));
}