#pragma once
#include <string>
#include <map>
#include <vector>
#include <glm/glm.hpp>

namespace Rongine {

    struct SpectralPreset
    {
        std::string Name;
        std::vector<float> Data; // 32个浮点数，对应 400nm - 710nm
        glm::vec3 PreviewColor;  // 用于在UI上显示的近似颜色
    };

    class SpectralAssetManager
    {
    public:
        static void init()
        {
            // --- 1. 黄金 (Gold) ---
            // 反射率在红光波段极高，蓝光波段低
            std::vector<float> gold = {
                0.1f, 0.1f, 0.1f, 0.1f, 0.15f, 0.25f, 0.35f, 0.50f, // 400-470
                0.65f, 0.75f, 0.80f, 0.85f, 0.90f, 0.92f, 0.93f, 0.94f, // 480-550
                0.95f, 0.96f, 0.96f, 0.96f, 0.97f, 0.97f, 0.97f, 0.97f, // 560-630
                0.97f, 0.97f, 0.98f, 0.98f, 0.98f, 0.98f, 0.98f, 0.98f  // 640-710
            };
            s_Presets["Gold"] = { "Gold", gold, {1.0f, 0.84f, 0.0f} };

            // --- 2. 铜 (Copper) ---
            std::vector<float> copper = {
                0.2f, 0.2f, 0.2f, 0.25f, 0.3f, 0.4f, 0.5f, 0.6f,
                0.7f, 0.8f, 0.9f, 0.92f, 0.94f, 0.95f, 0.96f, 0.96f,
                0.97f, 0.97f, 0.97f, 0.97f, 0.98f, 0.98f, 0.98f, 0.98f,
                0.98f, 0.98f, 0.98f, 0.98f, 0.98f, 0.98f, 0.98f, 0.98f
            };
            s_Presets["Copper"] = { "Copper", copper, {0.72f, 0.45f, 0.20f} };

            // --- 3. 银 (Silver) ---
            // 全波段高反射
            std::vector<float> silver(32, 0.96f);
            s_Presets["Silver"] = { "Silver", silver, {0.95f, 0.95f, 0.95f} };

            // --- 4. 绿宝石 (Emerald - 假数据模拟) ---
            std::vector<float> emerald(32, 0.1f);
            // 中间波段(绿色)拉高
            for (int i = 10; i < 20; i++) emerald[i] = 0.8f;
            s_Presets["Emerald"] = { "Emerald", emerald, {0.0f, 0.8f, 0.2f} };
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