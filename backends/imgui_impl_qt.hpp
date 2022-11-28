// dear imgui: Platform Backend for Qt
// This needs to be used along with a Renderer (e.g. OpenGL3, Vulkan, WebGPU..)
// (Requires: Qt 6.0+)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent()
//  function. Pass ImGuiKey values to all key functions e.g.
//  ImGui::IsKeyPressed(ImGuiKey_Space).
//  [X] Platform: Mouse cursor shape and visibility. Disable with
//  'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.

// You can use unmodified imgui_impl_* files in your project. See examples/
// folder for examples of using this. Prefer including the entire imgui/
// repository into your project (either as a copy or as a submodule), and only
// build the backends you need. If you are new to Dear ImGui, read documentation
// from the docs/ folder + read the top of imgui.cpp. Read online:
// https://github.com/ocornut/imgui/tree/master/docs

#pragma once

#include <imgui.h> // IMGUI_IMPL_API

class QWidget;
class QWindow;

IMGUI_IMPL_API bool ImGui_ImplQt_Init();
IMGUI_IMPL_API void ImGui_ImplQt_Shutdown();
IMGUI_IMPL_API void ImGui_ImplQt_NewFrame(QWidget* widget);
IMGUI_IMPL_API void ImGui_ImplQt_NewFrame(QWindow* window);

IMGUI_IMPL_API void ImGui_ImplQt_RegisterWidget(QWidget* widget);
IMGUI_IMPL_API void ImGui_ImplQt_RegisterWindow(QWindow* window);
IMGUI_IMPL_API void ImGui_ImplQt_UnregisterWidget(QWidget* widget);
IMGUI_IMPL_API void ImGui_ImplQt_UnregisterWindow(QWindow* window);
