#pragma once

#define YEAR 2023

#define VER_MAJOR 0
#define VER_MINOR 5
#define VER_SUBMINOR 23
#define VER_BUILD 38

#define _STR(x) #x
#define STR(x) _STR(x)
#define VER_STRING STR(VER_MAJOR) "." STR(VER_MINOR) "." STR(VER_SUBMINOR) "." STR(VER_BUILD)

#define COPYRIGHT_STRING "Copyright (C) " STR(YEAR) " Ken Simone"
