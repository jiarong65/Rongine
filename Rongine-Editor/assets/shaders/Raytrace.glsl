#version 450 core

// 1. 定义工作组大小 (8x8 = 64 线程)
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// 2. 输出图像 (绑定点 0)
layout(rgba32f, binding = 0) uniform image2D img_output;

// 3. 数据结构 (必须与 C++ RenderTypes.h 对齐)
struct GPUVertex {
    vec3 Position;
    float _pad1;
    vec3 Normal;
    float _pad2;
    vec2 TexCoord;
    vec2 _pad3;
};

struct TriangleData {
    uint v0, v1, v2;
    uint MaterialID;
};

// 4. SSBO 绑定 (绑定点 1 和 2)
layout(std430, binding = 1) readonly buffer VerticesBuffer {
    GPUVertex Vertices[];
};

layout(std430, binding = 2) readonly buffer TrianglesBuffer {
    TriangleData Triangles[];
};

// Uniforms
uniform float u_Time;

// --- 简单的射线-三角形求交 (Möller–Trumbore 算法) ---
// 返回: distance (t). 如果没击中返回 -1.0
float HitTriangle(vec3 rayOrigin, vec3 rayDir, vec3 v0, vec3 v1, vec3 v2)
{
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(rayDir, edge2);
    float a = dot(edge1, h);

    if (a > -0.0001 && a < 0.0001) return -1.0; // 射线平行于三角形

    float f = 1.0 / a;
    vec3 s = rayOrigin - v0;
    float u = f * dot(s, h);

    if (u < 0.0 || u > 1.0) return -1.0;

    vec3 q = cross(s, edge1);
    float v = f * dot(rayDir, q);

    if (v < 0.0 || u + v > 1.0) return -1.0;

    float t = f * dot(edge2, q);

    if (t > 0.0001) return t;
    else return -1.0;
}

void main() 
{
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 img_size = imageSize(img_output);

    if (pixel_coords.x >= img_size.x || pixel_coords.y >= img_size.y) return;

    // --- 1. 生成射线 (简单的正交相机/透视相机模拟) ---
    // 为了简单测试，我们先假设一个固定的相机位置，或者你可以通过 Uniform 传入相机矩阵
    // 这里我们先写一个“假相机”来验证数据：
    
    // 归一化坐标 (-1 到 1)
    vec2 uv = (vec2(pixel_coords) / vec2(img_size)) * 2.0 - 1.0;
    uv.x *= float(img_size.x) / float(img_size.y); // 修正长宽比

    // 假设相机在 (0, 0, 5)，看向 (0, 0, -1)
    vec3 rayOrigin = vec3(0.0, 0.0, 5.0);
    vec3 rayDir = normalize(vec3(uv, -1.0));

    // --- 2. 遍历场景三角形 (暴力循环，没有 BVH) ---
    float closestT = 10000.0;
    int hitTriangleIndex = -1;

    // 获取三角形总数 (通过数组长度)
    uint numTriangles = Triangles.length();

    for (uint i = 0; i < numTriangles; i++)
    {
        TriangleData tri = Triangles[i];
        
        // 读取顶点位置
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

    // --- 3. 输出颜色 ---
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0); // 默认黑色背景

    if (hitTriangleIndex != -1)
    {
        // 击中了！根据距离显示颜色 (深度图效果)
        // 或者直接显示红色
        color = vec4(1.0, 0.0, 0.0, 1.0); 
        
        // 可选：基于法线的简单着色
        // TriangleData tri = Triangles[hitTriangleIndex];
        // vec3 n = Vertices[tri.v0].Normal; 
        // color = vec4(n * 0.5 + 0.5, 1.0); 
    }
    else 
    {
        // 背景渐变，证明 Shader 在运行
        color = vec4(0.2, 0.3, 0.3 + 0.2 * sin(u_Time), 1.0);
    }

    imageStore(img_output, pixel_coords, color);
}