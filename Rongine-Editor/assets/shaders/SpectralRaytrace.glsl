#version 450 core

// ===============================================================================================
// SpectralRaytrace.glsl - 光谱路径追踪器 (完整物理版)
// ===============================================================================================

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;
layout(rgba32f, binding = 4) uniform image2D img_accum; 

// ==================== 常量配置 ====================
const float PI = 3.14159265359;
const float INFINITY = 10000.0;
const float EPSILON = 1e-6;
const int   MAX_BOUNCES = 8; // 最大弹射次数

const float RAY_OFFSET = 0.001;   // 防止自相交
const float MATH_EPSILON = 1e-6;  // 数学计算微小量

const float LAMBDA_MIN_CONST = 380.0;
const float LAMBDA_MAX_CONST = 780.0;

// ==================== 数据结构 (保持不变) ====================
struct GPUVertex {
    vec3 Position; float _pad1;
    vec3 Normal;   float _pad2;
    vec2 TexCoord; vec2 _pad3;
};

//struct BVHNode {
//    vec3 AABBMin; float LeftChild;
//    vec3 AABBMax; float RightChild; // Leaf: RightChild = Count
//};

struct BVHNode {
    vec4 Data1; // x,y,z = Min, w = LeftChild
    vec4 Data2; // x,y,z = Max, w = RightChild
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

struct GPUMaterial {
    vec4 AlbedoRoughness; // rgb, a=Roughness
    float Metallic;
    float Emission;
    int SpectralIndex0;   // n / Reflectance
    int SpectralIndex1;   // k / IOR
    int Type;             // 0=Diffuse, 1=Conductor, 2=Dielectric
    float _pad1;
    float _pad2;
    float _pad3;
};

// ==================== SSBO (保持不变) ====================
layout(std430, binding = 1) readonly buffer VerticesBuffer { GPUVertex Vertices[]; };
layout(std430, binding = 2) readonly buffer TrianglesBuffer { TriangleData Triangles[]; };
layout(std430, binding = 3) readonly buffer MaterialsBuffer { GPUMaterial Materials[]; };
layout(std430, binding = 5) readonly buffer SpectralCurvesBuffer { float Curves[]; };
layout(std430, binding = 6) readonly buffer BVHBuffer { BVHNode BVHNodes[]; };
layout(std430, binding = 7) readonly buffer OctreeBuffer { OctreeNode OctreeNodes[]; };
layout(std430, binding = 8) readonly buffer IndexMapBuffer { uint GlobalIndices[]; }; // 间接索引

// ==================== Uniforms (保持不变) ====================
uniform float u_Time;
uniform mat4 u_InverseProjection;
uniform mat4 u_InverseView;
uniform vec3 u_CameraPos;
uniform int u_FrameIndex; 
uniform float u_LambdaMin; 
uniform float u_LambdaMax;
uniform int u_AccelType; // 0=None, 1=BVH, 2=Octree

// ==================== 辅助函数 (RNG & Color) ====================
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

// ==================== 材质采样 ====================
float GetReflectanceFromRGB(vec3 albedo, float lambda) {
    float meanB = 460.0; float sigB = 30.0;
    float meanG = 535.0; float sigG = 35.0;
    float meanR = 610.0; float sigR = 40.0;
    float wB = exp(-0.5 * pow((lambda - meanB) / sigB, 2.0));
    float wG = exp(-0.5 * pow((lambda - meanG) / sigG, 2.0));
    float wR = exp(-0.5 * pow((lambda - meanR) / sigR, 2.0));
    return clamp(albedo.b * wB + albedo.g * wG + albedo.r * wR, 0.0, 1.0);
}

float SampleMeasuredSpectrum(int curveIndex, float lambda) {
    float startLambda = 400.0; float step = 10.0; int sampleCount = 32;
    float t = (lambda - startLambda) / step;
    if (t < 0.0) return Curves[curveIndex * sampleCount];
    if (t >= float(sampleCount - 1)) return Curves[curveIndex * sampleCount + sampleCount - 1];
    int idx0 = int(floor(t));
    float fractPart = t - float(idx0);
    return mix(Curves[curveIndex * sampleCount + idx0], Curves[curveIndex * sampleCount + idx0 + 1], fractPart);
}

// ==================== 物理光学函数 (新增) ====================

// 1. 柯西方程: 计算波长对应的折射率 (产生色散的关键!)
float GetCauchyIOR(float lambda) {
    // 将 nm 转为 um
    float lam = lambda / 1000.0;
    // BK7 玻璃近似参数
    float B = 1.5046;
    float C = 0.00420;
    return B + (C / (lam * lam));
}

// 2. 菲涅尔方程 (电介质) - 决定反射与折射的比例
// 计算复数折射率 (n + ik) 下的反射率
float FresnelConductor(float cosThetaI, float n, float k) {
    float cosThetaI2 = cosThetaI * cosThetaI;
    float sinThetaI2 = 1.0 - cosThetaI2;
    float n2 = n * n;
    float k2 = k * k;

    float t0 = n2 - k2 - sinThetaI2;
    float a2plusb2 = sqrt(t0 * t0 + 4.0 * n2 * k2);
    float t1 = a2plusb2 + cosThetaI2;
    float a = sqrt(0.5 * (a2plusb2 + t0));
    float t2 = 2.0 * cosThetaI * a;
    float Rs = (t1 - t2) / (t1 + t2);

    float t3 = cosThetaI2 * a2plusb2 + sinThetaI2 * sinThetaI2;
    float t4 = t2 * sinThetaI2;
    float Rp = Rs * (t3 - t4) / (t3 + t4);

    return 0.5 * (Rp + Rs);
}

//  电介质菲涅尔 (Dielectric)
float FresnelDielectricExact(float cosThetaI, float etaI, float etaT) {
    cosThetaI = clamp(cosThetaI, -1.0, 1.0);
    bool entering = cosThetaI > 0.0;
    
    // 如果是从介质内部射出，交换 etaI 和 etaT
    if (!entering) {
        float temp = etaI; etaI = etaT; etaT = temp;
        cosThetaI = abs(cosThetaI);
    }

    float sinThetaI = sqrt(max(0.0, 1.0 - cosThetaI * cosThetaI));
    float sinThetaT = (etaI / etaT) * sinThetaI;

    // 全内反射 (TIR)
    if (sinThetaT >= 1.0) return 1.0;

    float cosThetaT = sqrt(max(0.0, 1.0 - sinThetaT * sinThetaT));

    float rParl = ((etaT * cosThetaI) - (etaI * cosThetaT)) / ((etaT * cosThetaI) + (etaI * cosThetaT));
    float rPerp = ((etaI * cosThetaI) - (etaT * cosThetaT)) / ((etaI * cosThetaI) + (etaT * cosThetaT));

    return 0.5 * (rParl * rParl + rPerp * rPerp);
}


// 3. 漫反射采样 (余弦加权半球)
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
    
    // [优化] 剪枝逻辑:
    // 1. tmin <= tmax: 射线穿过盒子
    // 2. tmax > 0.0: 盒子在射线前方
    // 3. tmin < currentClosestT: [关键] 只有当盒子比当前最近交点还近时，才进入！
    return (tmin <= tmax) && (tmax > 0.0) && (tmin < currentClosestT);
}

void TraverseBVH(vec3 rayOrigin, vec3 rayDir, inout float closestT, inout int hitIndex) {
    // [关键优化] 预计算方向倒数，避免在循环中做昂贵的除法
    vec3 invDir = 1.0 / rayDir;

    int stack[32]; 
    int stackPtr = 0;
    stack[stackPtr++] = 0; // 压入根节点 (索引0)

    while (stackPtr > 0) {
        int nodeIdx = stack[--stackPtr];
        
        // 读取节点
        BVHNode node = BVHNodes[nodeIdx];

        // [关键修复] 数据解包 (解决球体空洞问题)
        // 显式从 vec4 中提取数据，确保与 C++ 的内存布局严格对齐
        vec3 aabbMin = node.Data1.xyz;
        float leftChild = node.Data1.w;
        vec3 aabbMax = node.Data2.xyz;
        float rightChild = node.Data2.w;

        // [修改] 调用优化后的 IntersectAABB
        if (!IntersectAABB(rayOrigin, invDir, aabbMin, aabbMax, closestT))
            continue;

        if (leftChild < 0.0) { 
            // === 叶子节点 ===
            // 解码索引：我们在 C++ 存的是 -(start + 1)
            int startIdx = int(-leftChild - 1.0);
            int count = int(rightChild);

            for (int i = 0; i < count; i++) {
                // 通过间接索引表获取真实的三角形 ID
                int triIdx = int(GlobalIndices[startIdx + i]);
                TriangleData tri = Triangles[triIdx];
                
                // 只有当 AABB 测试通过且三角形可能更近时，才进行求交
                float t = HitTriangle(rayOrigin, rayDir, Vertices[tri.v0].Position, Vertices[tri.v1].Position, Vertices[tri.v2].Position);
                
                if (t > 0.0 && t < closestT) { 
                    closestT = t; 
                    hitIndex = triIdx; 
                }
            }
        } else {
            // === 内部节点 ===
            // 简单的压栈顺序
            stack[stackPtr++] = int(leftChild);
            stack[stackPtr++] = int(rightChild);
        }
    }
}

// =========================================================
// 2. 八叉树 遍历逻辑 (占位示意)
// =========================================================
void TraverseOctree(vec3 rayOrigin, vec3 rayDir, inout float closestT, inout int hitIndex) {
    // 八叉树遍历比较复杂，通常涉及点定位或者光线步进
    // 这里写个简单的递归转循环结构
    int stack[32];
    int stackPtr = 0;
    stack[stackPtr++] = 0;

    while (stackPtr > 0) {
        int nodeIdx = stack[--stackPtr];
        OctreeNode node = OctreeNodes[nodeIdx];
        
        // 检查八叉树节点包围盒 (Box = Center +/- Size)
        vec3 bMin = node.Center - vec3(node.Size);
        vec3 bMax = node.Center + vec3(node.Size);
        float tBox;
        if (!IntersectAABB(rayOrigin, rayDir, bMin, bMax, tBox) || tBox > closestT) continue;

        // 如果是叶子 (TriCount > 0)
        if (node.TriCount > 0.0) {
            int start = int(node.TriStart);
            int count = int(node.TriCount);
            for(int i=0; i<count; i++) {
                int triIdx = int(GlobalIndices[start + i]);
                TriangleData tri = Triangles[triIdx];
                float t = HitTriangle(rayOrigin, rayDir, Vertices[tri.v0].Position, Vertices[tri.v1].Position, Vertices[tri.v2].Position);
                if (t > 0.0 && t < closestT) { closestT = t; hitIndex = triIdx; }
            }
        } else {
            // 压入存在的子节点
            for(int i=0; i<8; i++) {
                if (node.Children[i] >= 0.0) stack[stackPtr++] = int(node.Children[i]);
            }
        }
    }
}

// ===============================================================================================
// 主函数 (路径追踪循环)
// ===============================================================================================

void main() 
{
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 img_size = imageSize(img_output);
    if (pixel_coords.x >= img_size.x || pixel_coords.y >= img_size.y) return;

    InitRNG(pixel_coords, u_FrameIndex);

    // --- 1. 波长采样 ---
    float safeMin = max(u_LambdaMin, LAMBDA_MIN_CONST);
    float safeMax = min(u_LambdaMax, LAMBDA_MAX_CONST);
    if (safeMin >= safeMax || safeMin < 1.0) { safeMin = LAMBDA_MIN_CONST; safeMax = LAMBDA_MAX_CONST; }
    float lambda = safeMin + RandomFloat() * (safeMax - safeMin);

    // --- 2. 射线生成 ---
    vec2 jitter = vec2(RandomFloat(), RandomFloat()) - 0.5;
    vec2 uv = (vec2(pixel_coords) + 0.5 + jitter) / vec2(img_size) * 2.0 - 1.0;
    vec4 target = u_InverseProjection * vec4(uv, 1.0, 1.0);
    vec3 rayDir = normalize(mat3(u_InverseView) * normalize(target.xyz / target.w));
    vec3 rayOrigin = u_CameraPos;

    // --- 3. 路径追踪循环 ---
    float throughput = 1.0; 
    float radiance = 0.0;   

    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++)
    {
        // A. 场景求交
        float closestT = INFINITY;
        int hitIndex = -1;

        if (u_AccelType == 1) {
            TraverseBVH(rayOrigin, rayDir, closestT, hitIndex);
        } 
        else if (u_AccelType == 2) {
            TraverseOctree(rayOrigin, rayDir, closestT, hitIndex);
        }
        else{
            uint numTriangles = Triangles.length();
            for (uint i = 0; i < numTriangles; i++) {
                TriangleData tri = Triangles[i];
                float t = HitTriangle(rayOrigin, rayDir, Vertices[tri.v0].Position, Vertices[tri.v1].Position, Vertices[tri.v2].Position);
                if (t > 0.0 && t < closestT) { closestT = t; hitIndex = int(i); }
            }
        }

        // B. 未击中 -> 采样天空光并结束
        if (hitIndex == -1) {
            float t = 0.5 * (rayDir.y + 1.0);
            float skyIntensity = mix(0.5, 1.5, t); 
            radiance += skyIntensity * throughput;
            break; 
        }

        // =========================================================
        // [修复] C. 击中物体 -> 必须先获取数据，定义变量！
        // =========================================================
        
        // 1. 获取数据 (这里定义了 mat)
        TriangleData tri = Triangles[hitIndex];
        GPUMaterial mat = Materials[tri.MaterialID]; 
        
        // 2. 计算位置和法线 (这里定义了 hitPos, normal, frontFace)
        vec3 hitPos = rayOrigin + rayDir * closestT;
        vec3 v0 = Vertices[tri.v0].Position;
        vec3 v1 = Vertices[tri.v1].Position;
        vec3 v2 = Vertices[tri.v2].Position;
        vec3 N = normalize(cross(v1 - v0, v2 - v0));
        
        bool frontFace = dot(rayDir, N) < 0.0;
        vec3 normal = frontFace ? N : -N;

        // 自发光
        if (mat.Emission > 0.0) {
            radiance += mat.Emission * throughput;
            break; 
        }

        // 俄罗斯轮盘赌
        float p = max(mat.AlbedoRoughness.r, max(mat.AlbedoRoughness.g, mat.AlbedoRoughness.b));
        p = clamp(p, 0.1, 0.95);
        if (bounce > 3) {
            if (RandomFloat() > p) break;
            throughput /= p;
        }

        // ==================== 物理材质计算 (使用上面定义的变量) ====================

        // --- 准备材质参数 ---
        float roughness = mat.AlbedoRoughness.a;
        
        // 采样 Slot0 和 Slot1 的值 (根据波长 lambda)
        float val0 = 0.0; 
        if (mat.SpectralIndex0 >= 0) val0 = SampleMeasuredSpectrum(mat.SpectralIndex0, lambda);
        else val0 = GetReflectanceFromRGB(mat.AlbedoRoughness.rgb, lambda); // Fallback

        float val1 = 0.0;
        if (mat.SpectralIndex1 >= 0) val1 = SampleMeasuredSpectrum(mat.SpectralIndex1, lambda);
        
        // 分支：材质类型
        
        // TYPE 1: Conductor (金属)
        if (mat.Type == 1) 
        {
            float n = val0; // Slot0 = n
            float k = val1; // Slot1 = k
            
            // 1. 计算菲涅尔反射率 (物理核心)
            float cosTheta = dot(-rayDir, normal);
            float F = FresnelConductor(cosTheta, n, k);
            
            // 2. 镜面反射采样
            vec3 reflected = reflect(rayDir, normal);
            
            if (roughness > 0.0) {
                reflected = normalize(reflected + SampleCosineHemisphere(normal) * roughness);
            }
            
            rayOrigin = hitPos + normal * RAY_OFFSET;
            rayDir = reflected;
            
            throughput *= F; 
        }
        // TYPE 2: Dielectric (玻璃/水)
        else if (mat.Type == 2)
        {
            float transColor = val0; // Slot0 = 透射颜色
            float ior = val1;        // Slot1 = IOR (带色散!)
            if (mat.SpectralIndex1 < 0) ior = 1.5; 

            float cosTheta = dot(-rayDir, normal);
            float F = FresnelDielectricExact(cosTheta, 1.0, ior); 

            if (RandomFloat() < F) {
                // 反射
                rayOrigin = hitPos + normal * RAY_OFFSET;
                rayDir = reflect(rayDir, normal);
            } else {
                // 折射
                float eta = frontFace ? (1.0 / ior) : ior;
                vec3 refractDir = refract(rayDir, normal, eta);
                
                if (length(refractDir) == 0.0) { // TIR
                    rayOrigin = hitPos + normal * RAY_OFFSET;
                    rayDir = reflect(rayDir, normal);
                } else {
                    rayOrigin = hitPos - normal * RAY_OFFSET;
                    rayDir = normalize(refractDir);
                    throughput *= transColor; 
                }
            }
        }
        // TYPE 0: Diffuse (非金属/默认)
        else 
        {
            float reflectance = val0; // Slot0 = 反射率
            
            rayOrigin = hitPos + normal * RAY_OFFSET;
            rayDir = SampleCosineHemisphere(normal);
            
            throughput *= reflectance; 
        }
    }

    // --- 4. 累积与输出 ---
    vec3 xyzColor = WavelengthToXYZ(lambda) * radiance;

    vec3 oldXYZ = vec3(0.0);
    if (u_FrameIndex > 1) oldXYZ = imageLoad(img_accum, pixel_coords).rgb;
    
    vec3 newXYZ = oldXYZ + xyzColor;
    imageStore(img_accum, pixel_coords, vec4(newXYZ, 1.0));

    vec3 avgXYZ = newXYZ / float(u_FrameIndex);
    avgXYZ *= vec3(0.97, 1.0, 1.2); 
    imageStore(img_output, pixel_coords, vec4(XYZToDisplayRGB(avgXYZ), 1.0));
}