#pragma once
#include <glm/glm.hpp>

namespace Rongine {

    class CADFeature
    {
    public:
        // 拉伸选中的面
        // shapeHandle: 原物体的 Handle
        // faceIndex: 选中的面 ID
        // height: 拉伸高度
        // 返回: 新生成的形状 Handle (void*)
        static void* ExtrudeFace(void* shapeHandle, int faceIndex, float height);
        // 获取指定面的局部变换矩阵 (中心点为原点，Z轴为法线)
        static glm::mat4 GetFaceTransform(void* shapeHandle, int faceIndex);
    };
}