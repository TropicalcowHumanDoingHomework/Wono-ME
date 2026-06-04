// Force-included FIRST to define CONFIG_H and DISPLAY_H before ANY code.
// This prevents the original config.h and display.h (with Arduino/U8g2 dependencies)
// from being processed, even when project header files include "config.h" directly.
#define CONFIG_H 1
#define DISPLAY_H 1

// Provide standard C library headers needed by project source files (e.g. window.cpp
// uses strcpy/strtok/strcmp/NULL without including the corresponding headers).
// These are safe to force-include before windows.h as they only declare functions.
#include <string.h>
#include <stdlib.h>
#include <math.h>
