// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _DEBUG
#pragma comment(lib,"glibd_shared.lib")
#else
#pragma comment(lib,"glib.lib")
#endif

#include "targetver.h"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

// TODO: reference additional headers your program requires here
#define _AFX
#include "glib.h"
