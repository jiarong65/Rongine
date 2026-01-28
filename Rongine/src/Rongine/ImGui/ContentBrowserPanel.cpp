#include "Rongpch.h"
#include "ContentBrowserPanel.h"
#include "imgui.h"

void Rongine::ContentBrowserPanel::onImGuiRender()
{
    ImGui::Begin("Materials Library");

    // 获取材质库
    auto& library = SpectralAssetManager::GetLibrary();

    // 简单的网格布局
    float panelWidth = ImGui::GetContentRegionAvail().x;
    float cellSize = 80.0f;
    float padding = 16.0f;
    int columnCount = (int)(panelWidth / (cellSize + padding));
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    for (auto& [name, preset] : library)
    {
        ImGui::PushID(name.c_str());

        // 1. 绘制颜色按钮 (模拟材质球预览)
        // 这里用 PushStyleColor 把按钮染成材质的近似色
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(preset.PreviewColor.r, preset.PreviewColor.g, preset.PreviewColor.b, 1.0f));
        ImGui::Button("##Material", ImVec2(cellSize, cellSize));

        // ==================== 拖拽源 (Source) ====================
        if (ImGui::BeginDragDropSource())
        {
            // 设置 Payload：标签是 "SPECTRAL_MAT_ITEM"，数据是材质的名字
            ImGui::SetDragDropPayload("SPECTRAL_MAT_ITEM", name.c_str(), (name.size() + 1) * sizeof(char));

            // 拖拽时的预览图
            ImGui::Text("Assign %s", name.c_str());
            ImGui::ColorButton("Preview", ImVec4(preset.PreviewColor.r, preset.PreviewColor.g, preset.PreviewColor.b, 1.0f), 0, ImVec2(32, 32));

            ImGui::EndDragDropSource();
        }
        // ========================================================

        ImGui::PopStyleColor();

        // 2. 显示名字
        ImGui::TextWrapped("%s", name.c_str());

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);
    ImGui::End();
}
