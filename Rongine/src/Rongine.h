#pragma once

#include "Rongine/Core/Core.h"
#include "Rongine/Core/Application.h"

#include "Rongine/Core/Timestep.h"

#include "Rongine/Core/Log.h"
#include "Rongine/Events/Event.h"
#include "Rongine/Events/ApplicationEvent.h"
#include "Rongine/Events/KeyEvent.h"
#include "Rongine/Events/MouseEvent.h"
#include "Rongine/Core/Input.h"
#include "Rongine/Renderer/OrthographicCameraController.h"
#include "Rongine/Renderer/PerspectiveCameraController.h"

// ---Renderer------------------------
#include "Rongine/Renderer/Renderer.h"
#include "Rongine/Renderer/Renderer2D.h"
#include "Rongine/Renderer/Renderer3D.h"
#include "Rongine/Renderer/RenderCommand.h"

#include "Rongine/Renderer/Buffer.h"
#include "Rongine/Renderer/Shader.h"
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Texture.h"
#include "Rongine/Renderer/Framebuffer.h"

#include "Rongine/Renderer/OrthographicCamera.h"
#include "Rongine/Renderer/SpectralRenderer.h"

#include "Rongine/ImGui/ImGuiLayer.h"
#include "Rongine/ImGui/SceneHierarchyPanel.h"
#include "Rongine/ImGui/ContentBrowserPanel.h"

#include "Rongine/CAD/CADImporter.h" 
#include "Rongine/CAD/CADMesher.h"
#include "Rongine/CAD/CADModeler.h"
#include "Rongine/CAD/CADBoolean.h"
#include "Rongine/CAD/CADFeature.h"

#include "Rongine/Commands/Command.h"
#include "Rongine/Commands/TransformCommand.h"
#include "Rongine/Commands/CADModifyCommand.h"
#include "Rongine/Commands/DeleteCommand.h"

#include "Rongine/Utils/GeometryUtils.h"
#include "Rongine/Utils/PlatformUtils.h"

#include "Rongine/Scene/Components.h"
#include "Rongine/Scene/Entity.h"
#include "Rongine/Scene/Scene.h"
#include "Rongine/Scene/SceneSerializer.h"
#include "Rongine/Scene/SpectralAssetManager.h"

#include "Rongine/Math/Math.h"
