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

#include "imgui_impl_qt.hpp"

#include <deque>

#include <QApplication>
#include <QClipboard>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QWidget>
#include <QWindow>

#ifndef IMGUI_HAS_DOCK
#   define ImGuiConfigFlags_ViewportsEnable 0
#endif

// Connect to internal ImGui debug log
namespace ImGui
{
void DebugLog(const char* fmt, ...);
}

struct ImGui_ImplQt_Data;

class ImGuiQtBackend : public QObject
{
private:
   Q_DISABLE_COPY(ImGuiQtBackend)

public:
   explicit ImGuiQtBackend(ImGuiIO& io, ImGui_ImplQt_Data* bd);
   ~ImGuiQtBackend() = default;

   bool eventFilter(QObject* watched, QEvent* event);

   bool Init();
   void Shutdown();

   template<class T>
   void NewFrame(T* object);

   void UpdateKeyModifiers(QObject* watched, Qt::KeyboardModifiers modifiers);
   void UpdateMouseCursor();
   void UpdateMouseData();
   void UpdateMonitors();
   void InitPlatformInterface();
   void ShutdownPlatformInterface();

   void MonitorCallback();

   void RegisterObject(QObject* object);

   static ImGuiMouseButton
                          ButtonToImGuiMouseButton(Qt::MouseButton mouseButton);
   static Qt::CursorShape ImGuiCursorToCursorShape(ImGuiMouseCursor cursor);
   static ImGuiKey KeyToImGuiKey(Qt::Key key, Qt::KeyboardModifiers modifiers);

private:
   void HandleEnter(QObject* watched, QEnterEvent* event);
   void HandleLeave(QObject* watched, QEvent* event);
   void HandleFocus(QObject* watched, QFocusEvent* event);
   void HandleKeyPress(QObject* watched, QKeyEvent* event);
   void HandleMouseMove(QObject* watched, QMouseEvent* event);
   void HandleMouseButtonPress(QObject* watched, QMouseEvent* event);
   void HandleWheel(QObject* watched, QWheelEvent* event);

   ImGuiIO&           io_;
   ImGui_ImplQt_Data* bd_;

   std::unordered_map<QObject*, std::deque<std::function<void()>>> eventQueue_;

   std::chrono::steady_clock::time_point time_ {};
   bool                                  debugEnabled_ {};
   bool                                  wantUpdateMonitors_ {};
   QObject*                              focusedObject_ {};
   QObject*                              keyboardObject_ {};
   QObject*                              mouseObject_ {};
   ImVec2 lastValidMousePosition_ {-FLT_MAX, -FLT_MAX};
};

struct ImGui_ImplQt_Data
{
   std::unique_ptr<ImGuiQtBackend> backend_ {};
   std::string                     clipboard_ {};

   std::vector<QObject*> registeredObjects_ {};
};

ImGuiQtBackend::ImGuiQtBackend(ImGuiIO& io, ImGui_ImplQt_Data* bd) :
    io_ {io}, bd_ {bd}
{
}

// Backend data stored in io.BackendPlatformUserData to allow support for
// multiple Dear ImGui contexts.
static ImGui_ImplQt_Data* ImGui_ImplQt_GetBackendData()
{
   return ImGui::GetCurrentContext() ?
             static_cast<ImGui_ImplQt_Data*>(
                ImGui::GetIO().BackendPlatformUserData) :
             nullptr;
}

// Functions
static const char* ImGui_ImplQt_GetClipboardText(void* userData)
{
   ImGui_ImplQt_Data* bd = static_cast<ImGui_ImplQt_Data*>(userData);
   return bd->clipboard_.c_str();
}

static void ImGui_ImplQt_SetClipboardText(void* /* userData */,
                                          const char* text)
{
   QClipboard* clipboard = QGuiApplication::clipboard();
   clipboard->setText(text);
}

static const std::unordered_map<Qt::Key, ImGuiKey> keyToImGuiKeyMap_ {
   {Qt::Key_Tab, ImGuiKey_Tab},
   {Qt::Key_Left, ImGuiKey_LeftArrow},
   {Qt::Key_Right, ImGuiKey_RightArrow},
   {Qt::Key_Up, ImGuiKey_UpArrow},
   {Qt::Key_Down, ImGuiKey_DownArrow},
   {Qt::Key_PageUp, ImGuiKey_PageUp},
   {Qt::Key_PageDown, ImGuiKey_PageDown},
   {Qt::Key_Home, ImGuiKey_Home},
   {Qt::Key_End, ImGuiKey_End},
   {Qt::Key_Insert, ImGuiKey_Insert},
   {Qt::Key_Delete, ImGuiKey_Delete},
   {Qt::Key_Backspace, ImGuiKey_Backspace},
   {Qt::Key_Space, ImGuiKey_Space},
   {Qt::Key_Return, ImGuiKey_Enter},
   {Qt::Key_Enter, ImGuiKey_Enter},
   {Qt::Key_Escape, ImGuiKey_Escape},
   {Qt::Key_Control, ImGuiKey_LeftCtrl},
   {Qt::Key_Shift, ImGuiKey_LeftShift},
   {Qt::Key_Alt, ImGuiKey_LeftAlt},
   {Qt::Key_Super_L, ImGuiKey_LeftSuper},
   // FIXME: Qt doesn't differentiate left/right keys
   // {Qt::Key_RightCtrl, ImGuiKey_RightCtrl},
   // {Qt::Key_RightShift, ImGuiKey_RightShift},
   // {Qt::Key_RightAlt, ImGuiKey_RightAlt},
   {Qt::Key_Super_R, ImGuiKey_RightSuper},
   {Qt::Key_Menu, ImGuiKey_Menu},
   {Qt::Key_1, ImGuiKey_1},
   {Qt::Key_2, ImGuiKey_2},
   {Qt::Key_3, ImGuiKey_3},
   {Qt::Key_4, ImGuiKey_4},
   {Qt::Key_5, ImGuiKey_5},
   {Qt::Key_6, ImGuiKey_6},
   {Qt::Key_7, ImGuiKey_7},
   {Qt::Key_8, ImGuiKey_8},
   {Qt::Key_9, ImGuiKey_9},
   {Qt::Key_0, ImGuiKey_0},
   {Qt::Key_Exclam, ImGuiKey_1},
   {Qt::Key_At, ImGuiKey_2},
   {Qt::Key_NumberSign, ImGuiKey_3},
   {Qt::Key_Dollar, ImGuiKey_4},
   {Qt::Key_Percent, ImGuiKey_5},
   {Qt::Key_AsciiCircum, ImGuiKey_6},
   {Qt::Key_Ampersand, ImGuiKey_7},
   {Qt::Key_Asterisk, ImGuiKey_8},
   {Qt::Key_ParenLeft, ImGuiKey_9},
   {Qt::Key_ParenRight, ImGuiKey_0},
   {Qt::Key_A, ImGuiKey_A},
   {Qt::Key_B, ImGuiKey_B},
   {Qt::Key_C, ImGuiKey_C},
   {Qt::Key_D, ImGuiKey_D},
   {Qt::Key_E, ImGuiKey_E},
   {Qt::Key_F, ImGuiKey_F},
   {Qt::Key_G, ImGuiKey_G},
   {Qt::Key_H, ImGuiKey_H},
   {Qt::Key_I, ImGuiKey_I},
   {Qt::Key_J, ImGuiKey_J},
   {Qt::Key_K, ImGuiKey_K},
   {Qt::Key_L, ImGuiKey_L},
   {Qt::Key_M, ImGuiKey_M},
   {Qt::Key_N, ImGuiKey_N},
   {Qt::Key_O, ImGuiKey_O},
   {Qt::Key_P, ImGuiKey_P},
   {Qt::Key_Q, ImGuiKey_Q},
   {Qt::Key_R, ImGuiKey_R},
   {Qt::Key_S, ImGuiKey_S},
   {Qt::Key_T, ImGuiKey_T},
   {Qt::Key_U, ImGuiKey_U},
   {Qt::Key_V, ImGuiKey_V},
   {Qt::Key_W, ImGuiKey_W},
   {Qt::Key_X, ImGuiKey_X},
   {Qt::Key_Y, ImGuiKey_Y},
   {Qt::Key_Z, ImGuiKey_Z},
   {Qt::Key_F1, ImGuiKey_F1},
   {Qt::Key_F2, ImGuiKey_F2},
   {Qt::Key_F3, ImGuiKey_F3},
   {Qt::Key_F4, ImGuiKey_F4},
   {Qt::Key_F5, ImGuiKey_F5},
   {Qt::Key_F6, ImGuiKey_F6},
   {Qt::Key_F7, ImGuiKey_F7},
   {Qt::Key_F8, ImGuiKey_F8},
   {Qt::Key_F9, ImGuiKey_F9},
   {Qt::Key_F10, ImGuiKey_F10},
   {Qt::Key_F11, ImGuiKey_F11},
   {Qt::Key_F12, ImGuiKey_F12},
   {Qt::Key_Apostrophe, ImGuiKey_Apostrophe},
   {Qt::Key_QuoteDbl, ImGuiKey_Apostrophe},
   {Qt::Key_Comma, ImGuiKey_Comma},
   {Qt::Key_Less, ImGuiKey_Comma},
   {Qt::Key_Minus, ImGuiKey_Minus},
   {Qt::Key_Underscore, ImGuiKey_Minus},
   {Qt::Key_Period, ImGuiKey_Period},
   {Qt::Key_Greater, ImGuiKey_Period},
   {Qt::Key_Slash, ImGuiKey_Slash},
   {Qt::Key_Question, ImGuiKey_Slash},
   {Qt::Key_Semicolon, ImGuiKey_Semicolon},
   {Qt::Key_Colon, ImGuiKey_Semicolon},
   {Qt::Key_Equal, ImGuiKey_Equal},
   {Qt::Key_Plus, ImGuiKey_Equal},
   {Qt::Key_BracketLeft, ImGuiKey_LeftBracket},
   {Qt::Key_BraceLeft, ImGuiKey_LeftBracket},
   {Qt::Key_Backslash, ImGuiKey_Backslash},
   {Qt::Key_Bar, ImGuiKey_Backslash},
   {Qt::Key_BracketRight, ImGuiKey_RightBracket},
   {Qt::Key_BraceRight, ImGuiKey_RightBracket},
   {Qt::Key_QuoteLeft, ImGuiKey_GraveAccent},
   {Qt::Key_AsciiTilde, ImGuiKey_GraveAccent},
   {Qt::Key_CapsLock, ImGuiKey_CapsLock},
   {Qt::Key_ScrollLock, ImGuiKey_ScrollLock},
   {Qt::Key_NumLock, ImGuiKey_NumLock},
   {Qt::Key_Print, ImGuiKey_PrintScreen},
   {Qt::Key_Pause, ImGuiKey_Pause}};

static const std::unordered_map<Qt::Key, ImGuiKey> numpadKeyToImGuiKeyMap_ {
   {Qt::Key_1, ImGuiKey_Keypad1},
   {Qt::Key_2, ImGuiKey_Keypad2},
   {Qt::Key_3, ImGuiKey_Keypad3},
   {Qt::Key_4, ImGuiKey_Keypad4},
   {Qt::Key_5, ImGuiKey_Keypad5},
   {Qt::Key_6, ImGuiKey_Keypad6},
   {Qt::Key_7, ImGuiKey_Keypad7},
   {Qt::Key_8, ImGuiKey_Keypad8},
   {Qt::Key_9, ImGuiKey_Keypad9},
   {Qt::Key_0, ImGuiKey_Keypad0},
   {Qt::Key_Period, ImGuiKey_KeypadDecimal},
   {Qt::Key_Slash, ImGuiKey_KeypadDivide},
   {Qt::Key_Asterisk, ImGuiKey_KeypadMultiply},
   {Qt::Key_Minus, ImGuiKey_KeypadSubtract},
   {Qt::Key_Plus, ImGuiKey_KeypadAdd},
   {Qt::Key_Equal, ImGuiKey_KeypadEqual},
   {Qt::Key_Enter, ImGuiKey_KeypadEnter}};

static const std::unordered_map<int, Qt::CursorShape> imGuiCursorMap_ {
   {ImGuiMouseCursor_Arrow, Qt::CursorShape::ArrowCursor},
   {ImGuiMouseCursor_TextInput, Qt::CursorShape::IBeamCursor},
   {ImGuiMouseCursor_ResizeNS, Qt::CursorShape::SizeVerCursor},
   {ImGuiMouseCursor_ResizeEW, Qt::CursorShape::SizeHorCursor},
   {ImGuiMouseCursor_Hand, Qt::CursorShape::PointingHandCursor},
   {ImGuiMouseCursor_ResizeAll, Qt::CursorShape::SizeAllCursor},
   {ImGuiMouseCursor_ResizeNESW, Qt::CursorShape::SizeBDiagCursor},
   {ImGuiMouseCursor_ResizeNWSE, Qt::CursorShape::SizeFDiagCursor},
   {ImGuiMouseCursor_NotAllowed, Qt::CursorShape::ForbiddenCursor}};

ImGuiKey ImGuiQtBackend::KeyToImGuiKey(Qt::Key               key,
                                       Qt::KeyboardModifiers modifiers)
{
   // Numpad
   if (modifiers & Qt::KeyboardModifier::KeypadModifier)
   {
      auto it = numpadKeyToImGuiKeyMap_.find(key);
      if (it != numpadKeyToImGuiKeyMap_.cend())
      {
         return it->second;
      }
   }

   // Standard key
   auto it = keyToImGuiKeyMap_.find(key);
   if (it != keyToImGuiKeyMap_.cend())
   {
      return it->second;
   }

   // Key not found
   if (key != 0)
   {
      ImGui::DebugLog("Unknown key: %d", key);
   }

   return ImGuiKey_None;
}

Qt::CursorShape
ImGuiQtBackend::ImGuiCursorToCursorShape(ImGuiMouseCursor cursor)
{
   auto it = imGuiCursorMap_.find(cursor);
   if (it != imGuiCursorMap_.cend())
   {
      return it->second;
   }

   return Qt::CursorShape::ArrowCursor;
}

void ImGuiQtBackend::UpdateKeyModifiers(QObject*              watched,
                                        Qt::KeyboardModifiers modifiers)
{
   eventQueue_.at(watched).push_back(
      [=]()
      {
         io_.AddKeyEvent(ImGuiMod_Ctrl,
                         (modifiers & Qt::KeyboardModifier::ControlModifier) !=
                            0);
         io_.AddKeyEvent(ImGuiMod_Shift,
                         (modifiers & Qt::KeyboardModifier::ShiftModifier) !=
                            0);
         io_.AddKeyEvent(ImGuiMod_Alt,
                         (modifiers & Qt::KeyboardModifier::AltModifier) != 0);
         io_.AddKeyEvent(ImGuiMod_Super,
                         (modifiers & Qt::KeyboardModifier::MetaModifier) != 0);
      });
}

ImGuiMouseButton
ImGuiQtBackend::ButtonToImGuiMouseButton(Qt::MouseButton mouseButton)
{
   switch (mouseButton)
   {
   case Qt::MouseButton::LeftButton:
      return ImGuiMouseButton_Left;

   case Qt::MouseButton::RightButton:
      return ImGuiMouseButton_Right;

   case Qt::MouseButton::MiddleButton:
      return ImGuiMouseButton_Middle;

   default:
      return -1;
   }
}

void ImGuiQtBackend::HandleMouseButtonPress(QObject*     watched,
                                            QMouseEvent* event)
{
   ImGuiMouseButton button = ButtonToImGuiMouseButton(event->button());
   if (button == -1)
   {
      // Ignore press
      return;
   }

   mouseObject_           = watched;
   QEvent::Type eventType = event->type();

   eventQueue_.at(watched).push_back(
      [=]()
      {
         io_.AddMouseButtonEvent(button,
                                 eventType == QEvent::Type::MouseButtonPress);
      });
}

void ImGuiQtBackend::HandleWheel(QObject* watched, QWheelEvent* event)
{
   QPoint  numPixels  = event->pixelDelta();
   QPointF numDegrees = QPointF(event->angleDelta()) / 8.0f;

   mouseObject_ = watched;

   if (!numPixels.isNull())
   {
      eventQueue_.at(watched).push_back(
         [=]()
         {
            io_.AddMouseWheelEvent(static_cast<float>(numPixels.x()),
                                   static_cast<float>(numPixels.y()));
         });
   }
   else if (!numDegrees.isNull())
   {
      eventQueue_.at(watched).push_back(
         [=]()
         {
            QPointF numSteps = numDegrees / 15.0f;
            io_.AddMouseWheelEvent(static_cast<float>(numSteps.x()),
                                   static_cast<float>(numSteps.y()));
         });
   }
}

void ImGuiQtBackend::HandleKeyPress(QObject* watched, QKeyEvent* event)
{
   keyboardObject_ = watched;

   if (debugEnabled_)
   {
      ImGui::DebugLog("%s: 0x%02x, 0x%02x, 0x%02x, 0x%08x, 0x%08x\n",
                      event->type() == QEvent::Type::KeyPress ? "KP" : "KR",
                      event->key(),
                      event->nativeScanCode(),
                      event->nativeVirtualKey(),
                      static_cast<uint32_t>(event->modifiers()),
                      event->nativeModifiers());
   }

   UpdateKeyModifiers(watched, event->modifiers());

   ImGuiKey imguiKey =
      KeyToImGuiKey(static_cast<Qt::Key>(event->key()), event->modifiers());
   io_.SetKeyEventNativeData(
      imguiKey, event->nativeVirtualKey(), event->nativeScanCode());

   QEvent::Type eventType = event->type();
   std::string  eventText = event->text().toStdString();

   eventQueue_.at(watched).push_back(
      [=]()
      {
         io_.AddKeyEvent(imguiKey, eventType == QEvent::Type::KeyPress);
         if (eventType == QEvent::Type::KeyPress && eventText.size() > 0)
         {
            io_.AddInputCharactersUTF8(eventText.c_str());
         }
      });
}

void ImGuiQtBackend::HandleFocus(QObject* watched, QFocusEvent* event)
{
   if (event->type() == QEvent::Type::FocusOut && focusedObject_ != watched)
      return;

   focusedObject_ = event->type() == QEvent::Type::FocusIn ? watched : nullptr;
   QEvent::Type eventType = event->type();

   eventQueue_.at(watched).push_back(
      [=]() { io_.AddFocusEvent(eventType == QEvent::Type::FocusIn); });
}

void ImGuiQtBackend::HandleMouseMove(QObject* watched, QMouseEvent* event)
{
   QPointF position;

   if (io_.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
   {
      position = event->globalPosition();
   }
   else
   {
      position = event->position();
   }

   mouseObject_            = watched;
   lastValidMousePosition_ = ImVec2(position.x(), position.y());

   eventQueue_.at(watched).push_back(
      [=]() { io_.AddMousePosEvent(position.x(), position.y()); });
}

void ImGuiQtBackend::HandleEnter(QObject* watched, QEnterEvent* event)
{
   QPointF position;

   if (io_.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
   {
      position = event->globalPosition();
   }
   else
   {
      position = event->position();
   }

   mouseObject_            = watched;
   lastValidMousePosition_ = ImVec2(position.x(), position.y());

   eventQueue_.at(watched).push_back(
      [=]() { io_.AddMousePosEvent(position.x(), position.y()); });
}

void ImGuiQtBackend::HandleLeave(QObject* watched, QEvent* /* event */)
{
   if (mouseObject_ == watched)
   {
      mouseObject_            = nullptr;
      lastValidMousePosition_ = io_.MousePos;
   }

   eventQueue_.at(watched).push_back(
      [=]() { io_.AddMousePosEvent(-FLT_MAX, -FLT_MAX); });
}

void ImGuiQtBackend::MonitorCallback()
{
   wantUpdateMonitors_ = true;
}

bool ImGui_ImplQt_Init()
{
   ImGuiIO& io = ImGui::GetIO();
   IM_ASSERT(io.BackendPlatformUserData == nullptr &&
             "Already initialized a platform backend!");

   // Setup backend capabilities flags
   ImGui_ImplQt_Data* bd      = IM_NEW(ImGui_ImplQt_Data)();
   io.BackendPlatformUserData = static_cast<void*>(bd);
   io.BackendPlatformName     = "imgui_impl_qt";

   io.BackendUsingLegacyKeyArrays = 0; // Backend uses new key event processing
   io.BackendFlags |=
      ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values
                                         // (optional)

   bd->backend_ = std::make_unique<ImGuiQtBackend>(io, bd);

   // Configure clipboard
   bd->clipboard_        = QGuiApplication::clipboard()->text().toStdString();
   io.SetClipboardTextFn = ImGui_ImplQt_SetClipboardText;
   io.GetClipboardTextFn = ImGui_ImplQt_GetClipboardText;
   QObject::connect(QGuiApplication::clipboard(),
                    &QClipboard::dataChanged,
                    bd->backend_.get(),
                    [=]() {
                       bd->clipboard_ =
                          QGuiApplication::clipboard()->text().toStdString();
                    });

   return bd->backend_->Init();
}

bool ImGuiQtBackend::Init()
{
   // Update monitors the first time
   wantUpdateMonitors_ = true;
   UpdateMonitors();
   QGuiApplication* app =
      reinterpret_cast<QGuiApplication*>(QCoreApplication::instance());
   QObject::connect(app,
                    &QGuiApplication::screenAdded,
                    this,
                    &ImGuiQtBackend::MonitorCallback);
   QObject::connect(app,
                    &QGuiApplication::screenRemoved,
                    this,
                    &ImGuiQtBackend::MonitorCallback);

   if (io_.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
   {
      InitPlatformInterface();
   }

   return true;
}

void ImGui_ImplQt_Shutdown()
{
   ImGui_ImplQt_Data* bd = ImGui_ImplQt_GetBackendData();
   IM_ASSERT(bd != nullptr &&
             "No platform backend to shutdown, or already shutdown?");
   bd->backend_->Shutdown();
   ImGuiIO& io = ImGui::GetIO();

   io.BackendPlatformName     = nullptr;
   io.BackendPlatformUserData = nullptr;
   IM_DELETE(bd);
}

void ImGuiQtBackend::Shutdown()
{
   ShutdownPlatformInterface();
}

void ImGuiQtBackend::UpdateMouseData()
{
   // Stub for set mouse position support
}

void ImGuiQtBackend::UpdateMouseCursor()
{
   ImGuiIO& io = ImGui::GetIO();
   if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
      return;

   ImGuiMouseCursor   imguiCursor = ImGui::GetMouseCursor();
   ImGui_ImplQt_Data* bd          = ImGui_ImplQt_GetBackendData();

   for (QObject* object : bd->registeredObjects_)
   {
      if (imguiCursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
      {
         // Hide mouse cursor if imgui is drawing it or if it wants no cursor
         if (object->isWidgetType())
         {
            reinterpret_cast<QWidget*>(object)->setCursor(Qt::BlankCursor);
         }
         else if (object->isWindowType())
         {
            reinterpret_cast<QWindow*>(object)->setCursor(Qt::BlankCursor);
         }
      }
      else
      {
         // Show mouse cursor
         if (object->isWidgetType())
         {
            reinterpret_cast<QWidget*>(object)->setCursor(
               ImGuiCursorToCursorShape(imguiCursor));
         }
         else if (object->isWindowType())
         {
            reinterpret_cast<QWindow*>(object)->setCursor(
               ImGuiCursorToCursorShape(imguiCursor));
         }
      }
   }
}

void ImGuiQtBackend::UpdateMonitors()
{
   // Stub for multi-viewport
}

bool ImGuiQtBackend::eventFilter(QObject* watched, QEvent* event)
{
   bool widgetNeedsUpdate = false;

   switch (event->type())
   {
   case QEvent::Enter:
      // Mouse enters widget's boundaries
      HandleEnter(watched, reinterpret_cast<QEnterEvent*>(event));
      widgetNeedsUpdate = true;
      break;

   case QEvent::Leave:
      // Mouse leaves widget's boundaries
      HandleLeave(watched, event);
      widgetNeedsUpdate = true;
      break;

   case QEvent::FocusIn:
   case QEvent::FocusOut:
      // Widget or Window gains/loses keyboard focus
      HandleFocus(watched, reinterpret_cast<QFocusEvent*>(event));
      widgetNeedsUpdate = true;
      break;

   case QEvent::KeyPress:
   case QEvent::KeyRelease:
      // Key press/release
      HandleKeyPress(watched, reinterpret_cast<QKeyEvent*>(event));
      widgetNeedsUpdate = true;
      break;

   case QEvent::MouseButtonPress:
   case QEvent::MouseButtonRelease:
      // Mouse press/release
      HandleMouseButtonPress(watched, reinterpret_cast<QMouseEvent*>(event));
      widgetNeedsUpdate = true;
      break;

   case QEvent::MouseMove:
      // Mouse move
      HandleMouseMove(watched, reinterpret_cast<QMouseEvent*>(event));
      widgetNeedsUpdate = true;
      break;

   case QEvent::Wheel:
      // Mouse wheel moved
      HandleWheel(watched, reinterpret_cast<QWheelEvent*>(event));
      widgetNeedsUpdate = true;
      break;
   }

   if (widgetNeedsUpdate && watched->isWidgetType())
   {
      reinterpret_cast<QWidget*>(watched)->update();
   }

   return QObject::eventFilter(watched, event);
}

void ImGuiQtBackend::RegisterObject(QObject* object)
{
   // Make sure an entry exists in the event queue for this object
   eventQueue_[object];
}

template<class T>
static void ImGui_ImplQt_RegisterObject(T* object)
{
   ImGui_ImplQt_Data* bd = ImGui_ImplQt_GetBackendData();
   IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplQt_Init()?");

   // Register the object with the backend
   bd->backend_->RegisterObject(object);

   // Setup widget callbacks
   object->installEventFilter(bd->backend_.get());

   // Add to list of registered objects
   bd->registeredObjects_.push_back(object);
}

void ImGui_ImplQt_RegisterWidget(QWidget* widget)
{
   widget->setMouseTracking(true);
   ImGui_ImplQt_RegisterObject(widget);
}

void ImGui_ImplQt_RegisterWindow(QWindow* window)
{
   ImGui_ImplQt_RegisterObject(window);
}

template<class T>
static void ImGui_ImplQt_UnregisterObject(T* object)
{
   ImGui_ImplQt_Data* bd = ImGui_ImplQt_GetBackendData();
   if (bd == nullptr)
   {
      // Ignore when context has already been destroyed
      return;
   }

   // Uninstall widget callbacks
   object->removeEventFilter(bd->backend_.get());

   // Remove from list of registered objects
   auto it = std::find(
      bd->registeredObjects_.begin(), bd->registeredObjects_.end(), object);
   if (it != bd->registeredObjects_.end())
   {
      bd->registeredObjects_.erase(it);
   }
}

void ImGui_ImplQt_UnregisterWidget(QWidget* widget)
{
   ImGui_ImplQt_UnregisterObject(widget);
}

void ImGui_ImplQt_UnregisterWindow(QWindow* window)
{
   ImGui_ImplQt_UnregisterObject(window);
}

void ImGui_ImplQt_NewFrame(QWidget* widget)
{
   ImGui_ImplQt_Data* bd = ImGui_ImplQt_GetBackendData();
   IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplQt_Init()?");

   bd->backend_->NewFrame(widget);
}

void ImGui_ImplQt_NewFrame(QWindow* window)
{
   ImGui_ImplQt_Data* bd = ImGui_ImplQt_GetBackendData();
   IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplQt_Init()?");

   bd->backend_->NewFrame(window);
}

template<class T>
void ImGuiQtBackend::NewFrame(T* object)
{
   // Setup display size (every frame to accommodate for window resizing)
   QSize widgetSize            = object->size();
   io_.DisplaySize             = ImVec2(static_cast<float>(widgetSize.width()),
                            static_cast<float>(widgetSize.height()));
   io_.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

   if (wantUpdateMonitors_)
   {
      UpdateMonitors();
   }

   // Setup time step
   auto currentTime = std::chrono::steady_clock::now();
   io_.DeltaTime =
      time_ > std::chrono::steady_clock::time_point {} ?
         std::chrono::duration<float>(currentTime - time_).count() :
         (float) (1.0f / 60.0f);
   time_ = currentTime;

   // If there are events in the queue, trigger an additional update
   if (!eventQueue_.at(object).empty() && object->isWidgetType())
   {
      reinterpret_cast<QWidget*>(object)->update();
   }

   // Process events
   while (!eventQueue_.at(object).empty())
   {
      eventQueue_.at(object).front()();
      eventQueue_.at(object).pop_front();
   }

   UpdateMouseData();
   UpdateMouseCursor();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create
// and handle multiple viewports simultaneously. If you are new to dear imgui or
// creating a new binding for dear imgui, it is recommended that you completely
// ignore this section first..
//--------------------------------------------------------------------------------------------------------

void ImGuiQtBackend::InitPlatformInterface()
{
   // Stub for multi-viewport
}

void ImGuiQtBackend::ShutdownPlatformInterface()
{
   // Stub for multi-viewport
}
