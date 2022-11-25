# imgui-backend-qt
Qt Backend for Dear ImGui

## Overview
Qt Backend for Dear ImGui is designed based on the interface of the existing [ImGui](https://github.com/ocornut/imgui) backends, providing functionality to ImGui widgets drawn within Qt Windows and Widgets managed external to ImGui. Each Qt Widget should use its own ImGui context, in order for Qt signals to properly reach the intended ImGui target.

Because multi-viewports and docking are currently managed internal to Dear ImGui, support has not been given to this feature. Multiple ImGui contexts are preferred.

## Requirements
Qt Backend for Dear ImGui requires at least C++11, and is tested with Qt 6.x and ImGui v1.89+.

## Usage
Below is some sample code showing basic usage.

```cpp
void InitializeImGui()
{
   ...

   // Initialize ImGui Context
   ImGui::CreateContext();
   ImGui_ImplQt_Init();
   ImGui_ImplOpenGL3_Init();

   ...
}
```

```cpp
ExampleWidget::ExampleWidget(QObject* parent) : QOpenGLWidget(parent)
{
   // Register this widget with the ImGui Qt backend
   ImGui_ImplQt_RegisterWidget(this);
}
```

```cpp
void ExampleWidget::paintGL()
{
    // Start ImGui Frame
   ImGui_ImplQt_NewFrame(this);
   ImGui_ImplOpenGL3_NewFrame();
   ImGui::NewFrame();

   ...

   // Render ImGui Frame
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
```