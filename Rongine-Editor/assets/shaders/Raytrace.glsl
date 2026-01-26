#version 450 core

// 1. 定义工作组大小 (8x8 = 64 线程)
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// 2. 输出图像
layout(rgba32f, binding = 0) uniform image2D img_output;

// 3. 数据结构 (保持 vec4 对齐)
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
    vec4 AlbedoRoughness; // rgb=Color, a=Roughness
    vec4 MetalEmission;   // r=Metallic, g=Emission
};

// 4. SSBO 绑定
layout(std430, binding = 1) readonly buffer VerticesBuffer { GPUVertex Vertices[]; };
layout(std430, binding = 2) readonly buffer TrianglesBuffer { TriangleData Triangles[]; };
layout(std430, binding = 3) readonly buffer MaterialsBuffer { GPUMaterial Materials[]; };

// Uniforms
uniform float u_Time;
uniform mat4 u_InverseProjection;
uniform mat4 u_InverseView;
uniform vec3 u_CameraPos;

const float PI = 3.14159265359;

// ===============================================================================================
// PBR 核心函数 (Cook-Torrance BRDF)
// ===============================================================================================

// 1. 法线分布函数 (NDF) - Trowbridge-Reitz GGX
// 决定了高光的大小和形状 (受 Roughness 影响)
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

// 2. 几何函数 (Geometry) - Schlick-GGX
// 模拟微表面互相遮挡导致的变暗 (受 Roughness 影响)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / max(denom, 0.0000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 3. 菲涅尔方程 (Fresnel) - Schlick Approximation
// 描述光线在不同角度下的反射率 (受 Metallic 影响)
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ===============================================================================================
// 射线求交逻辑
// ===============================================================================================
float HitTriangle(vec3 rayOrigin, vec3 rayDir, vec3 v0, vec3 v1, vec3 v2)
{
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(rayDir, edge2);
    float a = dot(edge1, h);
    if (a > -0.0001 && a < 0.0001) return -1.0;
    float f = 1.0 / a;
    vec3 s = rayOrigin - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return -1.0;
    vec3 q = cross(s, edge1);
    float v = f * dot(rayDir, q);
    if (v < 0.0 || u + v > 1.0) return -1.0;
    float t = f * dot(edge2, q);
    return (t > 0.0001) ? t : -1.0;
}

void main() 
{
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 img_size = imageSize(img_output);
    if (pixel_coords.x >= img_size.x || pixel_coords.y >= img_size.y) return;

    // --- 生成射线 ---
    vec2 uv = (vec2(pixel_coords) + 0.5) / vec2(img_size) * 2.0 - 1.0;
    vec4 target = u_InverseProjection * vec4(uv, 1.0, 1.0);
    vec3 rayDir = normalize(mat3(u_InverseView) * normalize(target.xyz / target.w));
    vec3 rayOrigin = u_CameraPos;

    // --- 场景遍历 ---
    float closestT = 10000.0;
    int hitTriangleIndex = -1;
    uint numTriangles = Triangles.length();

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

    // --- PBR 着色计算 ---
    vec3 finalColor = vec3(0.0);

    if (hitTriangleIndex != -1)
    {
        TriangleData tri = Triangles[hitTriangleIndex];
        
        // 1. 准备材质数据
        GPUMaterial rawMat = Materials[tri.MaterialID];
        vec3 albedo     = pow(rawMat.AlbedoRoughness.rgb, vec3(2.2)); // 转换到线性空间
        float roughness = rawMat.AlbedoRoughness.w;
        float metallic  = rawMat.MetalEmission.x;
        float emission  = rawMat.MetalEmission.y;

        // 2. 准备几何数据
        vec3 hitPos = rayOrigin + rayDir * closestT;
        
        vec3 v0 = Vertices[tri.v0].Position;
        vec3 v1 = Vertices[tri.v1].Position;
        vec3 v2 = Vertices[tri.v2].Position;
        // 面法线 (Flat Normal)
        vec3 N = normalize(cross(v1 - v0, v2 - v0)); 
        vec3 V = normalize(u_CameraPos - hitPos); // 视线方向

        // 3. 定义光源 (硬编码一个点光源，方便观察效果)
        // 放在相机右上方
        vec3 lightPos = u_CameraPos + vec3(2.0, 4.0, 2.0); 
        vec3 lightColor = vec3(300.0, 300.0, 300.0); // 高强度光源

        vec3 L = normalize(lightPos - hitPos); // 光照方向
        vec3 H = normalize(V + L);             // 半程向量
        float distance = length(lightPos - hitPos);
        float attenuation = 1.0 / (distance * distance); // 衰减
        vec3 radiance = lightColor * attenuation;

        // 4. 计算反射率 F0 
        // 绝缘体(塑料等) F0 约为 0.04，金属 F0 为其 Albedo 颜色
        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, albedo, metallic);

        // 5. Cook-Torrance BRDF 计算
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // +0.0001 防止除零
        vec3 specular = numerator / denominator;
        
        // 6. 能量守恒
        // 反射出去的光(kS)多了，折射进去变为漫反射的光(kD)就得少
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= (1.0 - metallic); // 纯金属没有漫反射

        // 7. 最终光照合成
        float NdotL = max(dot(N, L), 0.0);                
        
        // 漫反射 + 镜面反射
        vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
        
        // 环境光 (简单的)
        vec3 ambient = vec3(0.03) * albedo * 1.0; // 0.03 = 环境光强度 AO
        
        // 自发光
        vec3 emissive = albedo * emission * 10.0;

        finalColor = ambient + Lo + emissive;

        // 8. 色调映射 (Tone Mapping) 和 Gamma 校正
        // 把 HDR (高动态范围) 映射回 LDR [0, 1]
        finalColor = finalColor / (finalColor + vec3(1.0)); // Reinhard Tone Mapping
        finalColor = pow(finalColor, vec3(1.0/2.2));        // Gamma Correction
    }
    else 
    {
        // 背景 (简单的天空)
        float t = 0.5 * (rayDir.y + 1.0);
        finalColor = mix(vec3(1.0), vec3(0.5, 0.7, 1.0), t);
    }

    imageStore(img_output, pixel_coords, vec4(finalColor, 1.0));
}