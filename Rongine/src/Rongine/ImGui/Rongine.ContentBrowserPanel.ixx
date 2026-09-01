module;
#include "imgui.h"

export module Rongine.ContentBrowserPanel;

import Rongine.SpectralAssetManager;

export namespace Rongine {

	class ContentBrowserPanel
	{
	public:
		void onImGuiRender();
	};

	void ContentBrowserPanel::onImGuiRender()
	{
		ImGui::Begin("Materials Library");

		auto& library = SpectralAssetManager::GetLibrary();

		float panelWidth = ImGui::GetContentRegionAvail().x;
		float cellSize = 80.0f;
		float padding = 16.0f;
		int columnCount = (int)(panelWidth / (cellSize + padding));
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, 0, false);

		for (auto& [name, preset] : library)
		{
			ImGui::PushID(name.c_str());

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(preset.PreviewColor.r, preset.PreviewColor.g, preset.PreviewColor.b, 1.0f));
			ImGui::Button("##Material", ImVec2(cellSize, cellSize));

			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("SPECTRAL_MAT_ITEM", name.c_str(), (name.size() + 1) * sizeof(char));

				ImGui::Text("Assign %s", name.c_str());
				ImGui::ColorButton("Preview", ImVec4(preset.PreviewColor.r, preset.PreviewColor.g, preset.PreviewColor.b, 1.0f), 0, ImVec2(32, 32));

				ImGui::EndDragDropSource();
			}

			ImGui::PopStyleColor();

			ImGui::TextWrapped("%s", name.c_str());

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
		ImGui::End();
	}
}
