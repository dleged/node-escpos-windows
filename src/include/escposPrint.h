#pragma once

#include <windows.h>
#include <string>
#include <IOSTREAM>
#include <winioctl.h>
#include <setupapi.h>
#include <tchar.h>
#include <stdio.h>
#include <winnt.h>
#pragma comment(lib, "setupapi.lib")

using namespace std;

class PrintDevice {
 public:
  string Port;
  int BaudRate;
  int DataBits;
  char Parity;
  int ReceiveBuffer;
  int StopBits;
};
struct PrintResult {
  BOOL success;
  DWORD err;
};

// SetupDiGetInterfaceDeviceDetail所需要的输出长度，定义足够大
#define INTERFACE_DETAIL_SIZE (1024)

// 设备数量上限，假设16台上限
#define MAX_DEVICE 16
BOOL PrintRawDataByLpt(string devicePath, char *meg, size_t size,
                       PrintResult *result);
BOOL PrintRawData(string devicePath, char *meg, size_t size,
                  PrintResult *result);
BOOL DisConnectDevice(string devicePath);

int WriteRawData(const char *str, HANDLE hPort, size_t size);
HANDLE InitPort(PrintDevice &device);
void InitializeDevicePar(PrintDevice &device);
void SetPrintResult(PrintResult *result, BOOL success, DWORD errCode);