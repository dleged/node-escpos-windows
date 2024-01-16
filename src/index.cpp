#include <nan.h>
#include <Windows.h>
#include "getDeviceList.h"
#include "escposPrint.h"
#include <list>
#include <regex>
#include "index.h"
#include <devguid.h>
#include <setupapi.h>
#include <Usbiodef.h>
#include <Ntddpar.h>

using namespace std;
using v8::Array;
using v8::ArrayBuffer;
using v8::Boolean;
using v8::Exception;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Value;

string Utf8ToGbk(const std::string& strUtf8) {
  // UTF-8转unicode
  int len = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, NULL, 0);
  wchar_t* strUnicode = new wchar_t[len];
  wmemset(strUnicode, 0, len);
  MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, strUnicode, len);

  // unicode转gbk
  len = WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, NULL, 0, NULL, NULL);
  char* strGbk = new char[len];
  memset(strGbk, 0, len);
  WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, strGbk, len, NULL, NULL);

  std::string strTemp(strGbk);  // 此时的strTemp是GBK编码
  delete[] strUnicode;
  delete[] strGbk;
  return strTemp;
}

string GbkToUtf8(const std::string& strGbk) {
  // gbk转unicode
  int len = MultiByteToWideChar(CP_ACP, 0, strGbk.c_str(), -1, NULL, 0);
  wchar_t* strUnicode = new wchar_t[len];
  wmemset(strUnicode, 0, len);
  MultiByteToWideChar(CP_ACP, 0, strGbk.c_str(), -1, strUnicode, len);

  // unicode转UTF-8
  len = WideCharToMultiByte(CP_UTF8, 0, strUnicode, -1, NULL, 0, NULL, NULL);
  char* strUtf8 = new char[len];
  WideCharToMultiByte(CP_UTF8, 0, strUnicode, -1, strUtf8, len, NULL, NULL);

  std::string strTemp(strUtf8);  // 此时的strTemp是UTF-8编码
  delete[] strUnicode;
  delete[] strUtf8;
  return strTemp;
}

void GetDeviceList(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  list<DeviceInfo> deviceList;

  if (args.Length() >= 1) {
    Local<String> deviceType = args[0]->ToString(isolate);
    int len = deviceType->Utf8Length(isolate);

    char* deviceTypeBuffer = (char*)malloc(len + 1);
    if (deviceTypeBuffer == nullptr) {
      return;
    }
    deviceType->WriteUtf8(isolate, deviceTypeBuffer, len);
    deviceTypeBuffer[deviceType->Utf8Length(isolate)] = 0;

    if (!strcmp(deviceTypeBuffer, "USB")) {
      GetDeviceList(deviceList, GUID_DEVINTERFACE_USB_DEVICE);
      free(deviceTypeBuffer);
    } else if (!strcmp(deviceTypeBuffer, "LPT")) {
      GUID guid;
      GUID* guidP;
      DWORD i = sizeof(GUID);
      guidP = &guid;
      if (SetupDiClassGuidsFromName(deviceTypeBuffer, guidP, i, &i)) {
        GetDeviceList(deviceList, GUID_DEVINTERFACE_PARCLASS);
      }
      free(deviceTypeBuffer);
    } else if (!strcmp(deviceTypeBuffer, "COM")) {
      GUID guid;
      GUID* guidP;
      DWORD i = sizeof(GUID);
      guidP = &guid;
      if (SetupDiClassGuidsFromName(deviceTypeBuffer, guidP, i, &i)) {
        GetDeviceList(deviceList, GUID_DEVINTERFACE_COMPORT);
      }
      free(deviceTypeBuffer);
    } else {
      isolate->ThrowException(Exception::Error(
          String::NewFromUtf8(isolate, "Wrong type, must be Usb or Ports")));
      free(deviceTypeBuffer);
      return;
    }
  } else if (args.Length() == 0) {
    GetDeviceList(deviceList, GUID_DEVINTERFACE_USB_DEVICE);
  }

  list<DeviceInfo>::iterator itor = deviceList.begin();
  Local<Array> resultArr = Array::New(isolate, deviceList.size());
  int count = 0;
  while (itor != deviceList.end()) {
    Local<Object> info = Object::New(isolate);
    Local<String> pathKey = String::NewFromUtf8(isolate, "path");
    Local<String> pathValue =
        String::NewFromUtf8(isolate, (itor)->path.c_str());
    info->Set(pathKey, pathValue);
    info->Set(String::NewFromUtf8(isolate, "name"),
              String::NewFromUtf8(isolate, (itor->name).c_str()));
    info->Set(String::NewFromUtf8(isolate, "desc"),
              String::NewFromUtf8(isolate, GbkToUtf8(itor->desc).c_str()));
    info->Set(String::NewFromUtf8(isolate, "service"),
              String::NewFromUtf8(isolate, GbkToUtf8(itor->service).c_str()));
    resultArr->Set(count, info);
    count++;
    itor++;
  }
  args.GetReturnValue().Set(resultArr);
}

BOOL parseFromV8String(v8::Local<v8::String>& v8String, char*& charBuffer) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  int len = v8String->Utf8Length(isolate);
  charBuffer = (char*)malloc(len + 1);
  if (charBuffer == nullptr) {
    return FALSE;
  }
  v8String->WriteUtf8(isolate, charBuffer, len);
  charBuffer[v8String->Utf8Length(isolate)] = 0;
  return TRUE;
}

void DisConnect(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  if (args.Length() < 1) {
    isolate->ThrowException(Exception::Error(
        String::NewFromUtf8(isolate,
                            "pls call thisFunction like this "
                            "DisconnectDevice(devicePath: string)")));
    return;
  }
  if (!args[0]->IsString()) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "the argument must be a string")));
    return;
  }
  Local<String> devicePath = args[0]->ToString(isolate);
  const int len = devicePath->Utf8Length(isolate);

  char* devicePathBf = (char*)malloc(len + 1);
  if (nullptr == devicePathBf) {
    return;
  }
  devicePath->WriteUtf8(isolate, devicePathBf, len);
  devicePathBf[len] = 0;
  BOOL disconnectResult = DisConnectDevice(devicePathBf);
  args.GetReturnValue().Set(Boolean::New(isolate, disconnectResult));
}

void PrintRaw(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  if (args.Length() < 2) {
    isolate->ThrowException(Exception::Error(
        String::NewFromUtf8(isolate, "Wrong number of arguments, must be 2")));
    return;
  }
  if (!args[0]->IsString()) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "the first argument must be a string")));
    return;
  }

  if (!args[1]->IsObject()) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "the second argument must be a buffer")));
    return;
  }
  Local<String> devicePath = args[0]->ToString(isolate);
  Local<Object> bufferObj = args[1]->ToObject(isolate);
  size_t bufferLength = node::Buffer::Length(bufferObj);

  char* bfData = node::Buffer::Data(bufferObj);
  char* deviceBf = (char*)malloc(devicePath->Utf8Length(isolate) + 1);
  if (deviceBf == nullptr) {
    return;
  }

  devicePath->WriteUtf8(isolate, deviceBf, devicePath->Utf8Length(isolate));
  deviceBf[devicePath->Utf8Length(isolate)] = 0;
  string sDevice(deviceBf);
  regex reg1("^LPT\\d+");
  smatch r2;
  PrintResult* printResult = (PrintResult*)malloc(sizeof(PrintResult));
  if (regex_match(sDevice, r2, reg1)) {
    PrintRawDataByLpt(deviceBf, bfData, bufferLength, printResult);
  } else {
    PrintRawData(deviceBf, bfData, bufferLength, printResult);
  }
  Local<Object> ret = Object::New(isolate);
  ret->Set(String::NewFromUtf8(isolate, "success"),
           Boolean::New(isolate, printResult->success));
  ret->Set(String::NewFromUtf8(isolate, "err"),
           Number::New(isolate, printResult->err));
  args.GetReturnValue().Set(ret);
  free(printResult);
  free(deviceBf);
}

void Initialize(Local<Object> exports) {
  NODE_SET_METHOD(exports, "GetDeviceList", GetDeviceList);
  NODE_SET_METHOD(exports, "PrintRaw", PrintRaw);
  NODE_SET_METHOD(exports, "DisConnect", DisConnect);
}

NODE_MODULE(addon, Initialize)