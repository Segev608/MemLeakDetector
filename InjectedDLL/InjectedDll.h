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
using namespace std;

#define INJECTDLL_EXPORTS 1

#ifdef INJECTDLL_EXPORTS
#define INJECTDLL_API __declspec(dllexport)
#else
#define INJECTDLL_API __declspec(dllimport)
#endif

extern "C" __declspec(dllexport) void dummy(void) { return; }
#define IMPORT_TABLE_OFFSET 1

#define TARGET_ALLOCATION "malloc"
#define TARGET_DEALLOCATION "free"
#define TARGET_REALLOCATION "realloc"

typedef PVOID(CDECL* MemAlloc)(size_t);
typedef VOID(CDECL* MemFree)(PVOID);
typedef PVOID(CDECL* MemRealloc)(PVOID, size_t);

PVOID CDECL new_alloc(size_t _Size);
VOID CDECL new_free(PVOID _Size);
PVOID CDECL new_realloc(PVOID, size_t);

template<class... Args>
void log_file(const char*, Args...);
bool IAThooking(HMODULE);
PIMAGE_IMPORT_DESCRIPTOR getImportTable(HMODULE);
BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved);


#endif /* _DLL_H_ */