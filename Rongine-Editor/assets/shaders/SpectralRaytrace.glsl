#version 450 core

// ===============================================================================================
// SpectralRaytrace.glsl - 光谱路径追踪器 (修复版)
// ===============================================================================================

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;
layout(rgba32f, binding = 4) uniform image2D img_accum; 

const float PI = 3.14159265359;
const float INFINITY = 10000.0;
const float EPSILON = 0.001;

// 默认兜底范围
const float LAMBDA_MIN_CONST = 380.0;
const float LAMBDA_MAX_CONST = 780.0;

// ===============================================================================================
// 数据结构 
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

struct GPUMaterial {
    vec4 AlbedoRoughness; 
    float Metallic;       
    float Emission;       
    int   SpectralIndex;  // 关键：用于判断是否使用真实光谱
    float _pad;           
};

// ===============================================================================================
// SSBO 绑定
// ===============================================================================================

layout(std430, binding = 1) readonly buffer VerticesBuffer { GPUVertex Vertices[]; };
layout(std430, binding = 2) readonly buffer TrianglesBuffer { TriangleData Triangles[]; };
layout(std430, binding = 3) readonly buffer MaterialsBuffer { GPUMaterial Materials[]; };
// [关键] 光谱数据仓库
layout(std430, binding = 5) readonly buffer SpectralCurvesBuffer { float Curves[]; };

// ===============================================================================================
// Uniforms
// ===============================================================================================

uniform float u_Time;
uniform mat4 u_InverseProjection;
uniform mat4 u_InverseView;
uniform vec3 u_CameraPos;
uniform int u_FrameIndex; 

uniform float u_LambdaMin; 
uniform float u_LambdaMax;

// ===============================================================================================
// 辅助函数
// ===============================================================================================

uint seed = 0;
void InitRNG(uvec2 pixel_coords, uint frame) {
    seed = pixel_coords.y * 1920 + pixel_coords.x + frame * 719393;
}
float RandomFloat() {
    seed = seed * 747796405 + 2891336453;
    uint result = ((seed >> ((seed >> 28) + 4)) ^ seed) * 277803737;
    result = (result >> 22) ^ result;
    return float(result) / 4294967295.0;
}

vec3 XYZToDisplayRGB(vec3 xyz) {
    vec3 linearRGB = vec3(
        3.2404542 * xyz.x - 1.5371385 * xyz.y - 0.4985314 * xyz.z,
       -0.9692660 * xyz.x + 1.8760108 * xyz.y + 0.0415560 * xyz.z,
        0.0556434 * xyz.x - 0.2040259 * xyz.y + 1.0572252 * xyz.z
    );
    linearRGB = max(linearRGB, vec3(0.0));
    return pow(linearRGB, vec3(1.0/2.2));
}

vec3 WavelengthToXYZ(float lambda) {
    float x = 0.0, y = 0.0, z = 0.0;
    float d1 = (lambda - 442.0) * ((lambda < 442.0) ? 0.0624 : 0.0374);
    float d2 = (lambda - 599.8) * ((lambda < 599.8) ? 0.0264 : 0.0323);
    float d3 = (lambda - 501.1) * ((lambda < 501.1) ? 0.0490 : 0.0382);
    x = 0.362 * exp(-0.5 * d1 * d1) + 1.056 * exp(-0.5 * d2 * d2) - 0.065 * exp(-0.5 * d3 * d3);

    d1 = (lambda - 568.8) * ((lambda < 568.8) ? 0.0213 : 0.0247);
    d2 = (lambda - 530.9) * ((lambda < 530.9) ? 0.0613 : 0.0322);
    y = 0.821 * exp(-0.5 * d1 * d1) + 0.286 * exp(-0.5 * d2 * d2);

    d1 = (lambda - 437.0) * ((lambda < 437.0) ? 0.0845 : 0.0278);
    d2 = (lambda - 459.0) * ((lambda < 459.0) ? 0.0385 : 0.0725);
    z = 1.217 * exp(-0.5 * d1 * d1) + 0.681 * exp(-0.5 * d2 * d2);
    return vec3(x, y, z);
}

float GetReflectanceFromRGB(vec3 albedo, float lambda)
{
    float meanB = 460.0; float sigB = 30.0;
    float meanG = 535.0; float sigG = 35.0;
    float meanR = 610.0; float sigR = 40.0;
    float wB = exp(-0.5 * pow((lambda - meanB) / sigB, 2.0));
    float wG = exp(-0.5 * pow((lambda - meanG) / sigG, 2.0));
    float wR = exp(-0.5 * pow((lambda - meanR) / sigR, 2.0));
    float reflectance = albedo.b * wB + albedo.g * wG + albedo.r * wR;
    return clamp(reflectance, 0.0, 1.0);
}

// [新增] 从 SSBO 中采样真实光谱数据
float SampleMeasuredSpectrum(int curveIndex, float lambda)
{
    // 数据规范定义 (需与 C++ 上传逻辑一致)
    float startLambda = 400.0; // 假设数据从 400nm 开始
    float step        = 10.0;  // 步长 10nm
    int   sampleCount = 32;    // 总点数 (C++ resize 为 32)

    // 计算索引
    float t = (lambda - startLambda) / step;
    
    // 边界处理
    if (t < 0.0) return Curves[curveIndex * sampleCount];
    if (t >= float(sampleCount - 1)) return Curves[curveIndex * sampleCount + sampleCount - 1];

    // 线性插值
    int idx0 = int(floor(t));
    int idx1 = idx0 + 1;
    float fractPart = t - float(idx0);

    int globalIdx0 = curveIndex * sampleCount + idx0;
    int globalIdx1 = curveIndex * sampleCount + idx1;

    // 从 SSBO 读取并插值
    return mix(Curves[globalIdx0], Curves[globalIdx1], fractPart);
}

float HitTriangle(vec3 rayOrigin, vec3 rayDir, vec3 v0, vec3 v1, vec3 v2) {
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(rayDir, edge2);
    float a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON) return -1.0;
    float f = 1.0 / a;
    vec3 s = rayOrigin - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return -1.0;
    vec3 q = cross(s, edge1);
    float v = f * dot(rayDir, q);
    if (v < 0.0 || u + v > 1.0) return -1.0;
    float t = f * dot(edge2, q);
    return (t > EPSILON) ? t : -1.0;
}

// ===============================================================================================
// 主函数
// ===============================================================================================

void main() 
{
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 img_size = imageSize(img_output);
    if (pixel_coords.x >= img_size.x || pixel_coords.y >= img_size.y) return;

    InitRNG(pixel_coords, u_FrameIndex);

    // --- 波长采样逻辑 (UI 控制范围) ---
    float safeMin = max(u_LambdaMin, LAMBDA_MIN_CONST);
    float safeMax = min(u_LambdaMax, LAMBDA_MAX_CONST);
    
    // 如果 Uniform 未初始化(为0)，强行回退到全光谱
    if (safeMin >= safeMax || safeMin < 1.0) {
        safeMin = LAMBDA_MIN_CONST;
        safeMax = LAMBDA_MAX_CONST;
    }

    float lambda = safeMin + RandomFloat() * (safeMax - safeMin);

    // 摄像机射线生成
    vec2 jitter = vec2(RandomFloat(), RandomFloat()) - 0.5;
    vec2 uv = (vec2(pixel_coords) + 0.5 + jitter) / vec2(img_size) * 2.0 - 1.0;
    vec4 target = u_InverseProjection * vec4(uv, 1.0, 1.0);
    vec3 rayDirView = normalize(target.xyz / target.w);
    vec3 rayDir = normalize(mat3(u_InverseView) * rayDirView);
    vec3 rayOrigin = u_CameraPos;

    // 场景求交
    float closestT = INFINITY;
    int hitIndex = -1;
    uint numTriangles = Triangles.length();

    for (uint i = 0; i < numTriangles; i++) {
        TriangleData tri = Triangles[i];
        float t = HitTriangle(rayOrigin, rayDir, Vertices[tri.v0].Position, Vertices[tri.v1].Position, Vertices[tri.v2].Position);
        if (t > 0.0 && t < closestT) { closestT = t; hitIndex = int(i); }
    }

    // --- 核心光照计算 ---
    float energy = 0.0; 

    if (hitIndex != -1) {
        TriangleData tri = Triangles[hitIndex];
        GPUMaterial mat = Materials[tri.MaterialID];
        
        float reflectance = 0.0;

        // [核心修复] 分支逻辑：如果有真实光谱索引，使用查表；否则使用 RGB 升维
        if (mat.SpectralIndex >= 0)
        {
            // 使用测量数据 (Binding 5)
            reflectance = SampleMeasuredSpectrum(mat.SpectralIndex, lambda);
            // 真实数据通常不需要亮度补偿，直接 clamp
            reflectance = clamp(reflectance, 0.0, 1.0);
        }
        else
        {
            // 使用 RGB 升维算法 (旧逻辑)
            vec3 albedoRGB = mat.AlbedoRoughness.rgb;
            reflectance = GetReflectanceFromRGB(albedoRGB, lambda);
            reflectance *= 2.5; // 升维补偿
            reflectance = min(reflectance, 1.0);
        }

        // 简单的 Lambert 光照
        vec3 v0 = Vertices[tri.v0].Position;
        vec3 v1 = Vertices[tri.v1].Position;
        vec3 v2 = Vertices[tri.v2].Position;
        vec3 N = normalize(cross(v1 - v0, v2 - v0)); 
        vec3 L = normalize(vec3(0.5, 1.0, 0.5));
        float NdotL = max(dot(N, L), 0.0);
        
        energy = reflectance * NdotL * 10.0; 
    } else {
        energy = 1.0; 
    }

    // 光谱转色与累积
    vec3 xyzColor = WavelengthToXYZ(lambda) * energy;

    vec3 accumulatedXYZ = xyzColor;
    if (u_FrameIndex > 1) {
        accumulatedXYZ += imageLoad(img_accum, pixel_coords).rgb;
    }
    imageStore(img_accum, pixel_coords, vec4(accumulatedXYZ, 1.0));

    vec3 averagedXYZ = accumulatedXYZ / float(u_FrameIndex);
    vec3 finalRGB = XYZToDisplayRGB(averagedXYZ);
    
    imageStore(img_output, pixel_coords, vec4(finalRGB, 1.0));
}