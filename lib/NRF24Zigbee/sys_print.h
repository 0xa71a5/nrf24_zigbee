#ifndef SYS_PRINT_H
#define SYS_PRINT_H

#if defined(ARDUINO) && ARDUINO >= 100                                                                            
#include <Arduino.h>                                                                                              
#else                                                                                                             
#include <WProgram.h>                                                                                             
#endif  


#define USE_PRINTF

#ifdef USE_PRINTF
#define debug_printf(...) printf(__VA_ARGS__)

#else
//#define debug_printf(...)
#define debug_printf(...)  do {\
                              char r[45];\
                              sprintf(r, __VA_ARGS__);\
                              Serial.print(r);\
                            }\
                            while (0);

#endif

#define pr_err(...) debug_printf(__VA_ARGS__)
#define pr_info(...) debug_printf(__VA_ARGS__)
#define pr_debug(...) debug_printf(__VA_ARGS__)

void printf_begin(void);
#endif