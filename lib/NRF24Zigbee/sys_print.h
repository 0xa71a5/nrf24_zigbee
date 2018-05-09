#ifndef SYS_PRINT_H
#define SYS_PRINT_H

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

extern uint8_t log_print_control;

#define USE_PRINTF
#define USE_LOG_CONTROL

#ifdef USE_PRINTF
	#define debug_printf(...) printf(__VA_ARGS__)
#else
	#define debug_printf(...)
	/*
	#define debug_printf(...)  do {\
	                              char r[45];\
	                              sprintf(r, __VA_ARGS__);\
	                              Serial.print(r);\
	                            }\
	                            while (0);
	*/
#endif


#ifdef USE_LOG_CONTROL
	#define DISBALE_LOG_OUTPUT()	log_print_control = 0;
	#define ENABLE_LOG_OUTPUT() 	log_print_control = 1;
	#define pr_err(...) if(log_print_control) debug_printf(__VA_ARGS__)
	#define pr_info(...) if(log_print_control) debug_printf(__VA_ARGS__)
	#define pr_debug(...) if(log_print_control) debug_printf(__VA_ARGS__)
#else
	#define DISBALE_LOG_OUTPUT()
	#define ENABLE_LOG_OUTPUT()
	#define pr_err(...)  debug_printf(__VA_ARGS__)
	#define pr_info(...)  debug_printf(__VA_ARGS__)
	#define pr_debug(...)  debug_printf(__VA_ARGS__)
#endif

void printf_begin(void);
#endif