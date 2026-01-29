#ifndef MAIN_STM32_H
#define MAIN_STM32_H

#include <version.h>

// STM32 specific configuration
// SERIAL_COMM_MODE is defined via build flags in platformio.ini
#ifndef SERIAL_COMM_MODE
#define SERIAL_COMM_MODE
#endif

#endif // MAIN_STM32_H
