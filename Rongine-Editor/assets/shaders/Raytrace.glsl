#version 450 core

// ===============================================================================================
// Raytrace.glsl - 标准 RGB 路径追踪器 (用于对比或回退)
// ===============================================================================================

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;
layout(rgba32f, binding = 4) uniform image2D img_accum;

// ==================== 常量配置 ====================
const float PI = 3.14159265359;
const float INFINITY = 10000.0;
const float EPSILON = 1e-6; // 保持与光谱版一致的精度
const int MAX_BOUNCES = 8;

const float RAY_OFFSET = 0.001; 
const float MATH_EPSILON = 1e-6;

// ==================== 数据结构 (必须与 C++ 完全一致!) ====================
struct GPUVertex {
    vec3 Position; float _pad1;
    vec3 Normal;   float _pad2;
    vec2 TexCoord; vec2 _pad3;
};

struct BVHNode {
    vec4 Data1; // Min, Left
    vec4 Data2; // Max, Right
};

struct OctreeNode {
    vec3 Center;  float Size;
    float Children[8];
    float TriStart; float TriCount; float _pad1; float _pad2;
};

struct TriangleData {
    uint v0, v1, v2;
    uint MaterialID;
};

// [重要修改] 同步最新的材质结构体
struct GPUMaterial {
    vec4 AlbedoRoughness; // rgb, a=Roughness
    float Metallic;
    float Emission;
    int SpectralIndex0;   // 在 RGB 模式下忽略
    int SpectralIndex1;   // 在 RGB 模式下忽略
    int Type;             // 0=Diffuse, 1=Conductor, 2=Dielectric
    float _pad1;
    float _pad2;
    float _pad3;
};

// ==================== SSBO ====================
layout(std430, binding = 1) readonly buffer VerticesBuffer { GPUVertex Vertices[]; };
layout(std430, binding = 2) readonly buffer TrianglesBuffer { TriangleData Triangles[]; };
layout(std430, binding = 3) readonly buffer MaterialsBuffer { GPUMaterial Materials[]; };
// Binding 5 (SpectralCurves) 在 RGB 模式下不使用
layout(std430, binding = 6) readonly buffer BVHBuffer { BVHNode BVHNodes[]; };
layout(std430, binding = 7) readonly buffer OctreeBuffer { OctreeNode OctreeNodes[]; };
layout(std430, binding = 8) readonly buffer IndexMapBuffer { uint GlobalIndices[]; };

// ==================== Uniforms ====================
uniform float u_Time;
uniform mat4 u_InverseProjection;
uniform mat4 u_InverseView;
uniform vec3 u_CameraPos;
uniform int u_FrameIndex;
uniform int u_AccelType; 

// ==================== 辅助函数 ====================
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

vec3 SampleCosineHemisphere(vec3 N) {
    float r1 = RandomFloat();
    float r2 = RandomFloat();
    float r = sqrt(r1);
    float theta = 2.0 * PI * r2;
    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(max(0.0, 1.0 - r1));
    vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    return normalize(tangent * x + bitangent * y + N * z);
}

// 简单的 Schlick 菲涅尔近似 (RGB 模式够用了)
float FresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float HitTriangle(vec3 rayOrigin, vec3 rayDir, vec3 v0, vec3 v1, vec3 v2) {
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(rayDir, edge2);
    float a = dot(edge1, h);
    if (a > -MATH_EPSILON && a < MATH_EPSILON) return -1.0;
    float f = 1.0 / a;
    vec3 s = rayOrigin - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return -1.0;
    vec3 q = cross(s, edge1);
    float v = f * dot(rayDir, q);
    if (v < 0.0 || u + v > 1.0) return -1.0;
    float t = f * dot(edge2, q);
    return (t > MATH_EPSILON) ? t : -1.0;
}

// ... (BVH / Octree 遍历逻辑，请复制 SpectralRaytrace.glsl 中的 TraverseBVH 和 IntersectAABB 优化版本) ...
// 为了节省篇幅，这里假设你已经把优化过的 TraverseBVH 复制过来了。
// 如果没复制，RGB模式依然会卡顿。务必同步这两个函数的优化！
// =========================================================
// 1. BVH 遍历逻辑
// =========================================================
bool IntersectAABB(vec3 rayOrig, vec3 invDir, vec3 boxMin, vec3 boxMax, float currentClosestT) {
    vec3 t0s = (boxMin - rayOrig) * invDir;
    vec3 t1s = (boxMax - rayOrig) * invDir;
    
    vec3 tsmaller = min(t0s, t1s);
    vec3 tbigger  = max(t0s, t1s);
    
    float tmin = max(tsmaller.x, max(tsmaller.y, tsmaller.z));
    float tmax = min(tbigger.x, min(tbigger.y, tbigger.z));
    
    return (tmin <= tmax) && (tmax > 0.0) && (tmin < currentClosestT);
}

void TraverseBVH(vec3 rayOrigin, vec3 rayDir, inout float closestT, inout int hitIndex) {
    vec3 invDir = 1.0 / rayDir;
    int stack[32]; 
    int stackPtr = 0;
    stack[stackPtr++] = 0; 

    while (stackPtr > 0) {
        int nodeIdx = stack[--stackPtr];
        BVHNode node = BVHNodes[nodeIdx];
        vec3 aabbMin = node.Data1.xyz;
        float leftChild = node.Data1.w;
        vec3 aabbMax = node.Data2.xyz;
        float rightChild = node.Data2.w;

        if (!IntersectAABB(rayOrigin, invDir, aabbMin, aabbMax, closestT)) continue;

        if (leftChild < 0.0) { 
            int startIdx = int(-leftChild - 1.0);
            int count = int(rightChild);
            for (int i = 0; i < count; i++) {
                int triIdx = int(GlobalIndices[startIdx + i]);
                TriangleData tri = Triangles[triIdx];
                float t = HitTriangle(rayOrigin, rayDir, Vertices[tri.v0].Position, Vertices[tri.v1].Position, Vertices[tri.v2].Position);
                if (t > 0.0 && t < closestT) { closestT = t; hitIndex = triIdx; }
            }
        } else {
            stack[stackPtr++] = int(leftChild);
            stack[stackPtr++] = int(rightChild);
        }
    }
}

void TraverseOctree(vec3 rayOrigin, vec3 rayDir, inout float closestT, inout int hitIndex) {
    // 简化的占位，实际应与 Spectral 版本一致
    uint numTriangles = Triangles.length();
    for (uint i = 0; i < numTriangles; i++) {
        TriangleData tri = Triangles[i];
        float t = HitTriangle(rayOrigin, rayDir, Vertices[tri.v0].Position, Vertices[tri.v1].Position, Vertices[tri.v2].Position);
        if (t > 0.0 && t < closestT) { closestT = t; hitIndex = int(i); }
    }
}

// ==================== 主函数 ====================
void main() 
{
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 img_size = imageSize(img_output);
    if (pixel_coords.x >= img_size.x || pixel_coords.y >= img_size.y) return;

    InitRNG(pixel_coords, u_FrameIndex);

    vec2 jitter = vec2(RandomFloat(), RandomFloat()) - 0.5;
    vec2 uv = (vec2(pixel_coords) + 0.5 + jitter) / vec2(img_size) * 2.0 - 1.0;
    vec4 target = u_InverseProjection * vec4(uv, 1.0, 1.0);
    vec3 rayDir = normalize(mat3(u_InverseView) * normalize(target.xyz / target.w));
    vec3 rayOrigin = u_CameraPos;

    vec3 throughput = vec3(1.0); 
    vec3 radiance = vec3(0.0);   

    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++)
    {
        float closestT = INFINITY;
        int hitIndex = -1;

        if (u_AccelType == 1) TraverseBVH(rayOrigin, rayDir, closestT, hitIndex);
        else if (u_AccelType == 2) TraverseOctree(rayOrigin, rayDir, closestT, hitIndex);
        else {
            uint numTriangles = Triangles.length();
            for (uint i = 0; i < numTriangles; i++) {
                TriangleData tri = Triangles[i];
                float t = HitTriangle(rayOrigin, rayDir, Vertices[tri.v0].Position, Vertices[tri.v1].Position, Vertices[tri.v2].Position);
                if (t > 0.0 && t < closestT) { closestT = t; hitIndex = int(i); }
            }
        }

        if (hitIndex == -1) {
            // 简单的天空光 (RGB)
            float t = 0.5 * (rayDir.y + 1.0);
            vec3 skyColor = mix(vec3(0.5, 0.7, 1.0), vec3(1.0), t) * 1.0;
            radiance += skyColor * throughput;
            break; 
        }

        // 获取数据 (使用新结构体读取，保证对齐正确)
        TriangleData tri = Triangles[hitIndex];
        GPUMaterial mat = Materials[tri.MaterialID]; 
        
        vec3 hitPos = rayOrigin + rayDir * closestT;
        vec3 v0 = Vertices[tri.v0].Position;
        vec3 v1 = Vertices[tri.v1].Position;
        vec3 v2 = Vertices[tri.v2].Position;
        vec3 N = normalize(cross(v1 - v0, v2 - v0));
        bool frontFace = dot(rayDir, N) < 0.0;
        vec3 normal = frontFace ? N : -N;

        if (mat.Emission > 0.0) {
            radiance += vec3(mat.Emission) * throughput;
            break; 
        }

        // --- RGB 材质逻辑 (简化的 Disney/PBR) ---
        vec3 albedo = mat.AlbedoRoughness.rgb;
        float roughness = mat.AlbedoRoughness.a;
        float metallic = mat.Metallic;

        // 根据 Type 强制覆盖属性 (兼容光谱流程的设置)
        if (mat.Type == 1) { // Conductor
            metallic = 1.0; 
        } else if (mat.Type == 2) { // Dielectric
            metallic = 0.0;
            // RGB模式下没法做真色散，简单模拟玻璃
        }

        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 F = vec3(FresnelSchlick(max(dot(-rayDir, normal), 0.0), F0.r)); 
        // 简化：取红色通道做平均，或者完整算RGB Fresnel

        // 简单分支：镜面 vs 漫反射
        float specularProb = (metallic > 0.9) ? 1.0 : 0.04; // 简化概率
        if (mat.Type == 2) specularProb = 0.1; // 玻璃反射率低

        // 玻璃特殊处理
        if (mat.Type == 2) {
            float ior = 1.5;
            float cosTheta = dot(-rayDir, normal);
            float F_glass = FresnelSchlick(abs(cosTheta), 0.04);
            
            if (RandomFloat() < F_glass) {
                rayOrigin = hitPos + normal * RAY_OFFSET;
                rayDir = reflect(rayDir, normal);
            } else {
                float eta = frontFace ? (1.0 / ior) : ior;
                vec3 refractDir = refract(rayDir, normal, eta);
                if (length(refractDir) == 0.0) {
                    rayOrigin = hitPos + normal * RAY_OFFSET;
                    rayDir = reflect(rayDir, normal);
                } else {
                    rayOrigin = hitPos - normal * RAY_OFFSET;
                    rayDir = normalize(refractDir);
                    throughput *= albedo; // 玻璃体色
                }
            }
        }
        else if (RandomFloat() < metallic) {
            // 金属反射
            rayOrigin = hitPos + normal * RAY_OFFSET;
            vec3 reflected = reflect(rayDir, normal);
            if (roughness > 0.0) reflected = normalize(reflected + SampleCosineHemisphere(normal) * roughness);
            rayDir = reflected;
            throughput *= albedo; // 金属带颜色
        } 
        else {
            // 漫反射
            rayOrigin = hitPos + normal * RAY_OFFSET;
            rayDir = SampleCosineHemisphere(normal);
            throughput *= albedo;
        }
        
        // 俄罗斯轮盘赌
        float p = max(throughput.r, max(throughput.g, throughput.b));
        if (bounce > 3) {
            if (RandomFloat() > p) break;
            throughput /= p;
        }
    }

    vec3 oldColor = vec3(0.0);
    if (u_FrameIndex > 1) oldColor = imageLoad(img_accum, pixel_coords).rgb;
    
    vec3 newColor = oldColor + radiance;
    imageStore(img_accum, pixel_coords, vec4(newColor, 1.0));

    vec3 avgColor = newColor / float(u_FrameIndex);
    // 简单的 Tone Mapping
    avgColor = avgColor / (avgColor + vec3(1.0)); 
    avgColor = pow(avgColor, vec3(1.0/2.2)); 

    imageStore(img_output, pixel_coords, vec4(avgColor, 1.0));
}