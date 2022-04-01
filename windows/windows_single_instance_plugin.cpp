#include "include/windows_single_instance/windows_single_instance_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

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
  if (method_call.method_name().compare("isSingleInstance") == 0) {
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
