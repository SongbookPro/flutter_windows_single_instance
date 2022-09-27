import 'dart:async';
import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';

import 'package:ffi/ffi.dart';
import 'package:flutter/services.dart';
import 'package:win32/win32.dart';

class WindowsSingleInstance {
  static const MethodChannel _channel = MethodChannel('windows_single_instance');
  static const _kErrorPipeConnected = 0x80070217;

  WindowsSingleInstance._();

  static int _openPipe(String filename) {
    final cPipe = filename.toNativeUtf16();
    try {
      return CreateFile(cPipe, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, 0);
    } finally {
      free(cPipe);
    }
  }

  static int _createPipe(String filename) {
    final cPipe = filename.toNativeUtf16();
    try {
      return CreateNamedPipe(
        cPipe,
        PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        4096,
        4096,
        0,
        nullptr,
      );
    } finally {
      malloc.free(cPipe);
    }
  }

  static void _readPipe(SendPort writer, int pipeHandle) {
    final overlap = calloc<OVERLAPPED>();
    try {
      while (true) {
        while (true) {
          ConnectNamedPipe(pipeHandle, overlap);
          final err = GetLastError();
          if (err == _kErrorPipeConnected) {
            sleep(const Duration(milliseconds: 200));
            continue;
          } else if (err == ERROR_INVALID_HANDLE) {
            return;
          }
          break;
        }

        var dataSize = 16384;
        var data = calloc<Int8>(dataSize);
        final numRead = calloc<Uint32>();
        try {
          while (GetOverlappedResult(pipeHandle, overlap, numRead, 0) == 0) {
            sleep(Duration(milliseconds: 200));
          }

          ReadFile(pipeHandle, data, dataSize, numRead, overlap);
          final jsonData = data.cast<Utf8>().toDartString();
          writer.send(jsonDecode(jsonData));
        } catch (error) {
          stderr.writeln("[MultiInstanceHandler]: ERROR: $error");
        } finally {
          free(data);
          free(numRead);
          DisconnectNamedPipe(pipeHandle);
        }
      }
    } finally {
      free(overlap);
    }
  }

  static void _writePipeData(String filename, List<String>? arguments) {
    final pipe = _openPipe(filename);
    final bytesString = jsonEncode(arguments ?? []);
    final bytes = bytesString.toNativeUtf8();
    final numWritten = malloc<Uint32>();
    try {
      WriteFile(pipe, bytes, bytesString.length, numWritten, nullptr);
    } finally {
      free(numWritten);
      free(bytes);
      CloseHandle(pipe);
    }
  }

  static void _startReadPipeIsolate(Map args) {
    final pipe = _createPipe(args["pipe"] as String);
    if (pipe == INVALID_HANDLE_VALUE) {
      print("Pipe create failed");
      return;
    }
    _readPipe(args["port"] as SendPort, pipe);
  }

  /// Checks that the current window is unique, and exits the app not.
  ///
  /// __Arguments__\
  /// `arguments`: List of strings that will be passed to the callback function of the open instance if this window is not unique\
  /// `pipeName`: A string unique to your app\
  /// `bringWindowToFront`: Should your active window become visible\
  /// `onSecondWindow`: Callback function that is called when a second window is attempted to be opened.
  static Future ensureSingleInstance(List<String> arguments, String pipeName,
      {Function(List<String>)? onSecondWindow,
      bool bringWindowToFront = false}) async {
    final _pipeName = "\\\\.\\pipe\\$pipeName";
    final bool isSingleInstance =
        await _channel.invokeMethod('isSingleInstance', <String, Object>{"pipe": pipeName});
    if (!isSingleInstance) {
      _writePipeData(_pipeName, arguments);
      exit(0);
    }

    // No callback so don't bother starting pipe
    if (onSecondWindow == null && bringWindowToFront == false) {
      return;
    }

    final reader = ReceivePort()
      ..listen((dynamic msg) {
        if (msg is List) {
          onSecondWindow!(msg.map((o) => o.toString()).toList());
          if (bringWindowToFront) _bringWindowToFront();
        }
      });
    await Isolate.spawn(_startReadPipeIsolate, {"port": reader.sendPort, "pipe": _pipeName});
  }

  static void _bringWindowToFront() {
    // https://stackoverflow.com/questions/916259/win32-bring-a-window-to-top/34414846#34414846

    final lWindowName = 'FLUTTER_RUNNER_WIN32_WINDOW'.toNativeUtf16();
    final m_hWnd = FindWindow(lWindowName, nullptr);
    free(lWindowName);

    final hCurWnd = GetForegroundWindow();
    final dwMyID = GetCurrentThreadId();
    final dwCurID = GetWindowThreadProcessId(hCurWnd, nullptr);
    AttachThreadInput(dwCurID, dwMyID, TRUE);
    SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
    SetForegroundWindow(m_hWnd);
    SetFocus(m_hWnd);
    SetActiveWindow(m_hWnd);
    AttachThreadInput(dwCurID, dwMyID, FALSE);
  }
}
