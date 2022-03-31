
import 'dart:async';

import 'package:flutter/services.dart';

class WindowsSingleInstance {
  static const MethodChannel _channel = MethodChannel('windows_single_instance');

  static Future<String?> get platformVersion async {
    final String? version = await _channel.invokeMethod('getPlatformVersion');
    return version;
  }
}
