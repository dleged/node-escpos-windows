﻿#pragma once

void parseFromV8String(v8::Local<v8::String> &deviceType, char* deviceTypeBuffer, bool &retflag);
const GUID USB_GUID = { 0xa5dcbf10, 0x6530, 0x11d2,{ 0x90, 0x1f, 0x00, 0xc0, 0x4f, 0xb9, 0x51, 0xed } };
