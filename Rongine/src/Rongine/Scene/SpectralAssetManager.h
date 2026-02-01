#pragma once
#include <string>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include "Rongine/Scene/Components.h"

namespace Rongine {

    struct SpectralPreset
    {
        std::string Name;

        // 材质类型 (金属/非金属/玻璃)
        SpectralMaterialComponent::MaterialType Type = SpectralMaterialComponent::MaterialType::Diffuse;

        // 双槽位数据
        std::vector<float> Slot0; // Reflectance / n / Transmission
        std::vector<float> Slot1; // Unused / k / IOR

        glm::vec3 PreviewColor;   // UI 显示用
    };

    class SpectralAssetManager
    {
    public:
        static void init()
        {
            // === 1. 真实黄金 (Gold - Conductor) ===
            // 数据来源: Johnson and Christy 1972
            std::vector<float> gold_n = {
                1.66f, 1.50f, 1.30f, 1.10f, 0.80f, 0.60f, 0.40f, 0.30f,
                0.25f, 0.20f, 0.18f, 0.17f, 0.17f, 0.17f, 0.17f, 0.17f,
                0.18f, 0.19f, 0.20f, 0.22f, 0.24f, 0.26f, 0.28f, 0.30f,
                0.32f, 0.35f, 0.38f, 0.40f, 0.42f, 0.45f, 0.47f, 0.50f
            };
            std::vector<float> gold_k = {
                1.96f, 1.88f, 1.78f, 1.65f, 1.50f, 1.40f, 1.30f, 1.50f,
                2.00f, 2.50f, 3.00f, 3.50f, 4.00f, 4.50f, 5.00f, 5.50f,
                6.00f, 6.50f, 7.00f, 7.50f, 8.00f, 8.50f, 9.00f, 9.50f,
                9.80f, 10.0f, 10.2f, 10.4f, 10.6f, 10.8f, 11.0f, 11.2f
            };

            s_Presets["Gold"] = {
                "Gold",
                SpectralMaterialComponent::MaterialType::Conductor,
                gold_n, gold_k,
                {1.0f, 0.84f, 0.0f}
            };

            // === 2. 真实铜 (Copper - Conductor) ===
            // 铜在短波长(蓝)有吸收，长波长(红)反射强
            // 简化数据，真实需查表
            std::vector<float> copper_n(32, 0.2f); // 铜的 n 普遍较低
            // 铜的 k 在红光区急剧上升
            std::vector<float> copper_k = {
                2.0f, 2.1f, 2.2f, 2.3f, 2.4f, 2.5f, 2.5f, 2.6f, // Blue
                2.7f, 2.8f, 2.9f, 3.0f, 4.0f, 5.0f, 5.5f, 6.0f, // Green transition
                6.5f, 7.0f, 7.5f, 8.0f, 8.2f, 8.4f, 8.6f, 8.8f, // Red
                9.0f, 9.2f, 9.4f, 9.6f, 9.8f, 10.0f, 10.2f, 10.4f
            };
            s_Presets["Copper"] = {
                "Copper",
                SpectralMaterialComponent::MaterialType::Conductor,
                copper_n, copper_k,
                {0.95f, 0.64f, 0.54f}
            };

            // === 3. 玻璃 (Glass - Dielectric) ===
            // BK7 玻璃的折射率约为 1.52，略有色散
            std::vector<float> glass_ior(32, 1.52f);
            // 简单的线性色散模拟：蓝光折射率略高 (1.53)，红光略低 (1.51)
            for (int i = 0; i < 32; i++) glass_ior[i] = 1.53f - (float)i * 0.0006f;

            std::vector<float> glass_trans(32, 1.0f); // 全透

            s_Presets["Glass (BK7)"] = {
                "Glass (BK7)",
                SpectralMaterialComponent::MaterialType::Dielectric,
                glass_trans, glass_ior,
                {0.8f, 0.9f, 1.0f} // 淡蓝色预览
            };

            // === 4. 绿宝石 (Emerald - Diffuse 兼容旧模式) ===
            // 旧的“反射率模式”依然可以用作非金属
            std::vector<float> emerald(32, 0.1f);
            for (int i = 10; i < 20; i++) emerald[i] = 0.8f;

            s_Presets["Emerald"] = {
                "Emerald",
                SpectralMaterialComponent::MaterialType::Diffuse,
                emerald, {}, // Slot1 为空
                {0.0f, 0.8f, 0.2f}
            };

            // === 5. 银镜 (Silver Mirror - Conductor) ===
            // n 值非常低 (< 0.15)，k 值随着波长增加而迅速升高。

            std::vector<float> silver_n = {
                0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, // 400-470 (极低)
                0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, // 480-550
                0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, // 560-630
                0.05f, 0.06f, 0.07f, 0.08f, 0.09f, 0.10f, 0.11f, 0.12f  // 640-710
            };

            std::vector<float> silver_k = {
                2.0f, 2.2f, 2.4f, 2.6f, 2.8f, 3.0f, 3.2f, 3.4f, // 400-470
                3.6f, 3.8f, 4.0f, 4.2f, 4.4f, 4.6f, 4.8f, 5.0f, // 480-550
                5.2f, 5.4f, 5.6f, 5.8f, 6.0f, 6.2f, 6.4f, 6.6f, // 560-630
                6.8f, 7.0f, 7.2f, 7.4f, 7.6f, 7.8f, 8.0f, 8.2f  // 640-710
            };

            s_Presets["Mirror (Silver)"] = {
                "Mirror (Silver)",
                SpectralMaterialComponent::MaterialType::Conductor,
                silver_n, silver_k,
                {0.95f, 0.95f, 0.95f} // 预览色：亮白
            };

            // === 6. 夸张的色散玻璃 (Super Dispersion Glass) ===
            // 为了测试效果，我们人为制造一个物理上不太存在、但效果极强的色散材质
            // 蓝光折射率 (400nm) 设为 1.8 (折射极强)
            // 红光折射率 (700nm) 设为 1.5 (折射较弱)
            std::vector<float> super_ior(32);
            for (int i = 0; i < 32; i++) {
                // 线性插值：从 1.8 降到 1.5
                float t = (float)i / 31.0f;
                super_ior[i] = 1.8f * (1.0f - t) + 1.5f * t;
            }

            std::vector<float> clear_trans(32, 1.0f); // 纯透明

            s_Presets["Super Glass"] = {
                "Super Glass",
                SpectralMaterialComponent::MaterialType::Dielectric,
                clear_trans, super_ior,
                {0.7f, 0.9f, 1.0f}
            };
        }

        static const std::map<std::string, SpectralPreset>& GetLibrary() { return s_Presets; }

        static bool GetPreset(const std::string& name, SpectralPreset& outPreset)
        {
            if (s_Presets.find(name) != s_Presets.end())
            {
                outPreset = s_Presets[name];
                return true;
            }
            return false;
        }

    private:
        static inline std::map<std::string, SpectralPreset> s_Presets;
    };
}