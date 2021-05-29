#pragma once
// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the INJECTDLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// INJECTDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef _DLL_H_
#define _DLL_H_

#include "pch.h"
#include<conio.h>
#include<iostream>
#include <string>

#define INJECTDLL_EXPORTS 1

#ifdef INJECTDLL_EXPORTS
#define INJECTDLL_API __declspec(dllexport)
#else
#define INJECTDLL_API __declspec(dllimport)
#endif

#define TARGET_ALLOCATION "malloc"
#define TARGET_DEALLOCATION "free"

#define IMPORT_TABLE_OFFSET 1
using namespace std;

extern "C" __declspec(dllexport) void dummy(void) { return; }

PVOID CDECL new_malloc(size_t _Size);
VOID CDECL new_free(PVOID _Size);

bool IAThooking(HMODULE, LPCSTR, PVOID);
bool rewriteThunk(PIMAGE_THUNK_DATA pThunk, void* newFunc);
PIMAGE_IMPORT_DESCRIPTOR getImportTable(HMODULE);
BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved);


#endif /* _DLL_H_ */