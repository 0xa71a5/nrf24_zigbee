#ifndef TIMERTWO_h
#define TIMERTWO_h

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#define RESOLUTION 256UL    // Timer2 is 8 bit

#if defined(__AVR_ATmega1280__)|| defined(__AVR_ATmega2560__) || defined(__AVR_ATmega8__)|| defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
#else
#error "This Timer2 library is not compatible with this board"
#endif

class TimerTwo
{
  public:
    // methods
	void setPeriod(unsigned long cycles);
	void EnableTimerInterrupt(void (*isr)(), unsigned long microseconds=0);
    void DisableTimerInterrupt();
	void ResetTimer();
	void ResumeTimer();
	void StopTimer();
    void DisablePWM();  
    void AdjustPwmDuty(uint8_t duty);
	uint8_t EnablePWM(uint8_t pin,unsigned long microseconds=0,uint8_t duty=0);
	uint8_t PinToDigital(uint8_t pwm_pin,uint8_t Output);
    
	void (*isrCallback)();
	static uint8_t tcnt2_init;
	static uint8_t *pwm_arr;
	
  private:
	static uint8_t clockSelectBits;
	static unsigned long cycles;
};

extern TimerTwo Timer2;
#endif