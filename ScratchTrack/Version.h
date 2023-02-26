#pragma once

#define YEAR 2023

#define VER_MAJOR 0
#define VER_MINOR 5
#define VER_SUBMINOR 23
#define VER_BUILD 57

#define _STR(x) #x
#define STR(x) _STR(x)
#define VER_STRING STR(VER_MAJOR) "." STR(VER_MINOR) "." STR(VER_SUBMINOR) "." STR(VER_BUILD)

#define VER_STRINGW L STR(VER_MAJOR) L"." L STR(VER_MINOR) L"." L STR(VER_SUBMINOR) L"." L STR(VER_BUILD)

#define COPYRIGHT_STRING "Copyright (C) " STR(YEAR) " Ken Simone"
