#version 450 core

// ===============================================================================================
// SpectralRaytrace.glsl - 光谱路径追踪器 (Spectral Path Tracer)
// ===============================================================================================

// 1. 定义工作组大小
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Binding 0: 输出显示 (sRGB) - 用于直接显示到屏幕
layout(rgba32f, binding = 0) uniform image2D img_output;

// Binding 4: 累积缓冲区 (XYZ 线性空间) - 用于存储光谱累积结果
// 注意：光谱渲染的中间结果通常保存为 XYZ 色空间，精度更高且符合物理叠加原理
layout(rgba32f, binding = 4) uniform image2D img_accum; 

// 常量定义
const float PI = 3.14159265359;
const float INFINITY = 10000.0;
const float EPSILON = 0.001;

// 可见光波长范围 (nm)
const float LAMBDA_MIN = 380.0;
const float LAMBDA_MAX = 780.0;

// ===============================================================================================
// 2. 数据结构 (必须与 C++ RenderTypes.h 严格对齐)
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

// 光谱材质结构 (复用 RGB 结构，通过 Uplifting 算法转换)
struct GPUMaterial {
    vec4 AlbedoRoughness; // rgb = Albedo, a = Roughness
    vec4 MetalEmission;   // r = Metallic, g = Emission
};

// ===============================================================================================
// 3. SSBO 绑定
// ===============================================================================================

layout(std430, binding = 1) readonly buffer VerticesBuffer { GPUVertex Vertices[]; };
layout(std430, binding = 2) readonly buffer TrianglesBuffer { TriangleData Triangles[]; };
layout(std430, binding = 3) readonly buffer MaterialsBuffer { GPUMaterial Materials[]; };

// ===============================================================================================
// 4. Uniforms
// ===============================================================================================

uniform float u_Time;
uniform mat4 u_InverseProjection;
uniform mat4 u_InverseView;
uniform vec3 u_CameraPos;
uniform int u_FrameIndex; // 当前累积帧数

// ===============================================================================================
// 5. 随机数生成器 (PCG Hash)
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

// ===============================================================================================
// 6. 色彩科学核心 (Color Science)
// ===============================================================================================

// XYZ 到 sRGB 的转换 (包括 Gamma 校正)
vec3 XYZToDisplayRGB(vec3 xyz) {
    // 1. XYZ -> Linear RGB (Rec.709 标准矩阵)
    vec3 linearRGB = vec3(
        3.2404542 * xyz.x - 1.5371385 * xyz.y - 0.4985314 * xyz.z,
       -0.9692660 * xyz.x + 1.8760108 * xyz.y + 0.0415560 * xyz.z,
        0.0556434 * xyz.x - 0.2040259 * xyz.y + 1.0572252 * xyz.z
    );

    // 2. Linear RGB -> sRGB (Gamma 2.2 近似)
    // 必须 clamp 防止负值导致 pow 错误
    linearRGB = max(linearRGB, vec3(0.0));
    return pow(linearRGB, vec3(1.0/2.2));
}

// CIE 1931 Color Matching Functions (CMF) 的多高斯拟合近似
// 输入: 波长 lambda (nm)
// 输出: 该波长对应的 CIE XYZ 响应值
vec3 WavelengthToXYZ(float lambda) {
    float x = 0.0, y = 0.0, z = 0.0;
    
    // 拟合参数 (来自图形学文献)
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

// [关键算法] RGB 升维 (RGB Uplifting)
// 将 RGB 颜色转换为特定波长下的光谱反射率
// 输入: albedo (RGB), lambda (当前波长 nm)
// 输出: 反射率 (0.0 ~ 1.0)
float GetReflectanceFromRGB(vec3 albedo, float lambda)
{
    // 定义 R, G, B 三个通道的中心波长 (nm) 和 宽度 (方差)
    // 蓝色中心 ~460nm, 绿色 ~535nm, 红色 ~610nm
    float meanB = 460.0; float sigB = 30.0;
    float meanG = 535.0; float sigG = 35.0;
    float meanR = 610.0; float sigR = 40.0;

    // 计算高斯权重 (Gaussian Weight)
    // 当前波长离哪个颜色通道越近，就越受该通道值的影响
    float wB = exp(-0.5 * pow((lambda - meanB) / sigB, 2.0));
    float wG = exp(-0.5 * pow((lambda - meanG) / sigG, 2.0));
    float wR = exp(-0.5 * pow((lambda - meanR) / sigR, 2.0));

    // 混合反射率
    float reflectance = albedo.b * wB + albedo.g * wG + albedo.r * wR;
    
    // 限制范围
    return clamp(reflectance, 0.0, 1.0);
}

// ===============================================================================================
// 7. 几何求交
// ===============================================================================================

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
// 8. 主函数 (Main)
// ===============================================================================================

void main() 
{
    // 获取像素坐标
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 img_size = imageSize(img_output);
    if (pixel_coords.x >= img_size.x || pixel_coords.y >= img_size.y) return;

    // A. 初始化随机数种子
    InitRNG(pixel_coords, u_FrameIndex);

    // B. [光谱核心] 随机采样一个主波长 (Hero Wavelength)
    // 每一帧，每个像素只计算这一个波长的物理行为
    float lambda = LAMBDA_MIN + RandomFloat() * (LAMBDA_MAX - LAMBDA_MIN);

    // C. 抗锯齿抖动 (Jitter)
    vec2 jitter = vec2(RandomFloat(), RandomFloat()) - 0.5;
    vec2 uv = (vec2(pixel_coords) + 0.5 + jitter) / vec2(img_size) * 2.0 - 1.0;

    // D. 生成射线
    vec4 target = u_InverseProjection * vec4(uv, 1.0, 1.0);
    vec3 rayDirView = normalize(target.xyz / target.w);
    vec3 rayDir = normalize(mat3(u_InverseView) * rayDirView);
    vec3 rayOrigin = u_CameraPos;

    // E. 场景求交 (寻找最近交点)
    float closestT = INFINITY;
    int hitIndex = -1;
    uint numTriangles = Triangles.length();

    for (uint i = 0; i < numTriangles; i++) {
        TriangleData tri = Triangles[i];
        float t = HitTriangle(rayOrigin, rayDir, Vertices[tri.v0].Position, Vertices[tri.v1].Position, Vertices[tri.v2].Position);
        if (t > 0.0 && t < closestT) { closestT = t; hitIndex = int(i); }
    }

    // F. 单色光照计算 (Monochromatic Shading)
    // 注意：这里的 energy 仅仅是一个标量 (Scalar)，代表该波长的强度
    float energy = 0.0; 

    if (hitIndex != -1) {
        TriangleData tri = Triangles[hitIndex];
        GPUMaterial mat = Materials[tri.MaterialID];
        
        // 1. 获取 RGB 颜色
        vec3 albedoRGB = mat.AlbedoRoughness.rgb;
        
        // 2. [关键] 使用 Uplifting 算法估算当前波长下的反射率
        float reflectance = GetReflectanceFromRGB(albedoRGB, lambda);
        
        // 3. 亮度补偿 (因为高斯采样比较窄，积分能量可能会损失，稍微增强一点)
        reflectance *= 2.5;
        reflectance = min(reflectance, 1.0);

        // 4. 简单的 Lambert 光照 (标量运算)
        vec3 hitPos = rayOrigin + rayDir * closestT;
        vec3 v0 = Vertices[tri.v0].Position;
        vec3 v1 = Vertices[tri.v1].Position;
        vec3 v2 = Vertices[tri.v2].Position;
        vec3 N = normalize(cross(v1 - v0, v2 - v0)); // Flat Normal
        
        // 简单的固定光源方向
        vec3 L = normalize(vec3(0.5, 1.0, 0.5));
        float NdotL = max(dot(N, L), 0.0);
        
        // 最终单色能量 = 反射率 * 几何因子 * 光强 (10.0)
        energy = reflectance * NdotL * 10.0; 
    } else {
        // 天空光 (假设是全光谱白光)
        energy = 1.0; 
    }

    // G. 光谱转色 (Spectral to XYZ)
    // 将计算出的单波长能量，乘以该波长对应的 XYZ 响应曲线
    vec3 xyzColor = WavelengthToXYZ(lambda) * energy;

    // H. 时间累积 (Temporal Accumulation) - 在 XYZ 空间进行
    vec3 accumulatedXYZ = xyzColor;
    
    // 如果不是第一帧，读取历史数据并相加
    if (u_FrameIndex > 1) {
        accumulatedXYZ += imageLoad(img_accum, pixel_coords).rgb;
    }
    
    // 写入累积缓冲区
    imageStore(img_accum, pixel_coords, vec4(accumulatedXYZ, 1.0));

    // I. 平均与输出 (Average & Display)
    // 计算平均 XYZ
    vec3 averagedXYZ = accumulatedXYZ / float(u_FrameIndex);
    
    // 转换到 sRGB 进行显示
    vec3 finalRGB = XYZToDisplayRGB(averagedXYZ);
    
    imageStore(img_output, pixel_coords, vec4(finalRGB, 1.0));
}