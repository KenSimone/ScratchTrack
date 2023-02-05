#pragma once

#define YEAR 2999

#define VER_MAJOR 0
#define VER_MINOR 5
#define VER_SUBMINOR 1
#define VER_BUILD 1

#define _STR(x) #x
#define STR(x) _STR(x)
#define VER_STRING STR(VER_MAJOR) "." STR(VER_MINOR) "." STR(VER_SUBMINOR) "." STR(VER_BUILD)

