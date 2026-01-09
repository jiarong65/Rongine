#pragma once

#include "Rongine/Scene/Scene.h"
#include "Rongine/Core/Core.h"

namespace Rongine {

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);

		// 保存场景到文件
		void Serialize(const std::string& filepath);

		// 从文件加载场景
		bool Deserialize(const std::string& filepath);

		// (可选) 二进制序列化，暂时先做文本的 YAML
		// void SerializeRuntime(const std::string& filepath);
		// bool DeserializeRuntime(const std::string& filepath);

	private:
		Ref<Scene> m_Context;
	};

}