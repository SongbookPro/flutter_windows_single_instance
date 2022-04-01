#include "include/windows_single_instance/windows_single_instance_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <map>
#include <memory>
#include <sstream>

namespace {

class WindowsSingleInstancePlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  WindowsSingleInstancePlugin();

  virtual ~WindowsSingleInstancePlugin();

 private:
  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  bool isSingleInstance();

  private:
    HANDLE mutex = NULL;
};

// static
void WindowsSingleInstancePlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "windows_single_instance",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<WindowsSingleInstancePlugin>();

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

WindowsSingleInstancePlugin::WindowsSingleInstancePlugin() {}

WindowsSingleInstancePlugin::~WindowsSingleInstancePlugin() {
  if (mutex != NULL) {
      ::ReleaseMutex(mutex);
  }
}

void WindowsSingleInstancePlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare("getPlatformVersion") == 0) {
    std::ostringstream version_stream;
    version_stream << "Windows ";
    if (IsWindows10OrGreater()) {
      version_stream << "10+";
    } else if (IsWindows8OrGreater()) {
      version_stream << "8";
    } else if (IsWindows7OrGreater()) {
      version_stream << "7";
    }
    result->Success(flutter::EncodableValue(version_stream.str()));
  } else if (method_call.method_name().compare("isSingleInstance") == 0) {
    result->Success(flutter::EncodableValue(isSingleInstance()));
  } else {
    result->NotImplemented();
  }
}

bool WindowsSingleInstancePlugin::isSingleInstance() {
  // Only call once
  if (mutex != NULL) {
    return true;
  }

  // Check for existing window
  mutex = ::CreateMutex(NULL, TRUE, L"songbookpro.win.mutex");
  if (mutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {

  // for (HWND hwnd = GetTopWindow(NULL); hwnd != NULL; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT))
  // {   

  //     if (!IsWindowVisible(hwnd))
  //         continue;

  //     // int length = GetWindowTextLength(hwnd);
  //     // if (length == 0)
  //     //     continue;

  //     std::wstring title;
  //     title.reserve(GetWindowTextLength(hwnd) + 1);
  //     GetWindowText(hwnd, const_cast<WCHAR *>(title.c_str()), (int)title.capacity());

  //     // char* title = new char[length+1];
  //     // GetWindowText(hwnd, title, length+1);

  //     // if (title == L"windows_single_instance_example")
  //     // {
  //         ::SetForegroundWindow(hwnd);
  //         // continue;
  //     // }

  //     // std::cout << "HWND: " << hwnd << " Title: " << title << std::endl;

  // }

      HWND existingApp = ::FindWindow(L"FLUTTER_RUNNER_WIN32_WINDOW", L"windows_single_instance_example");
      if (existingApp) ::SetForegroundWindow(existingApp);

      return false;
  }

  return true;
}

}  // namespace

void WindowsSingleInstancePluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  WindowsSingleInstancePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
