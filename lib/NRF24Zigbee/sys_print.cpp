#include "sys_print.h"

#ifdef USE_PRINTF
int serial_putc( char c, struct __file * )
{
  Serial.write( c );
  return c;
}
void printf_begin(void)
{
  fdevopen( &serial_putc, 0);
}
#else
void printf_begin(void)
{
}
#endif
