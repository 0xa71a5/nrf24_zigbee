//(desired tcnt2_init)*F_CPU = (255-TCNT2)*prescaler
//max achievable tcnt2_init (F_CPU=16MHz) = 16.320ms
//

#ifndef TIMERTWO_cpp
#define TIMERTWO_cpp

#include "TimerTwo.h"

TimerTwo Timer2;              // preinstatiate

//initialise private variables
static unsigned long TimerTwo::cycles=0;
static uint8_t TimerTwo::tcnt2_init=0;
static uint8_t TimerTwo::clockSelectBits=0;
static uint8_t *TimerTwo::pwm_arr;

ISR(TIMER2_OVF_vect)          // timer overflow interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
    if(Timer2.pwm_arr!=NULL && Timer2.pwm_arr[0]!=Timer2.tcnt2_init){
        Timer2.pwm_arr[3]^=0x01;
        Timer2.PinToDigital(Timer2.pwm_arr[2],Timer2.pwm_arr[3]);
        TCNT2=(RESOLUTION-1)-Timer2.pwm_arr[Timer2.pwm_arr[3]];
	}
    else{
		Timer2.isrCallback();
        TCNT2=Timer2.tcnt2_init;
	}	
		
}

uint8_t TimerTwo::PinToDigital(uint8_t pwm_pin,uint8_t Output){
#if defined(__AVR_ATmega1280__)|| defined(__AVR_ATmega2560__) //Mega
	switch(pwm_pin)
	{
		case 0:
			if(Output==1) PORTE|=1;
			else PORTE&=~(1);
		break;

		case 1:
			if(Output==1) PORTE|=(1<<1);
			else PORTE&=~(1<<1);
		break;

		case 2:
			if(Output==1) PORTE|=(1<<4);
			else PORTE&=~(1<<4);
		break;

		case 3:
			if(Output==1) PORTE|=(1<<5);
			else PORTE&=~(1<<5);
		break;

		case 4:
			if(Output==1) PORTG|=(1<<5);
			else PORTG&=~(1<<5);
		break;

		case 5:
			if(Output==1) PORTE|=(1<<3);
			else PORTE&=~(1<<3);
		break;

		case 6:
			if(Output==1) PORTH|=(1<<3);
			else PORTH&=~(1<<3);
		break;

		case 7:
			if(Output==1) PORTH|=(1<<4);
			else PORTH&=~(1<<4);
		break;

		case 8:
			if(Output==1) PORTH|=(1<<5);
			else PORTH&=~(1<<5);
		break;

		case 9:
			if(Output==1) PORTH|=(1<<6);
			else PORTH&=~(1<<6);
		break;

		case 10:
			if(Output==1) PORTB|=(1<<4);
			else PORTB&=~(1<<4);
		break;

		case 11:
			if(Output==1) PORTB|=(1<<5);
			else PORTB&=~(1<<5);
		break;

		case 12:
			if(Output==1) PORTB|=(1<<6);
			else PORTB&=~(1<<6);
		break;

		case 13:
			if(Output==1) PORTB|=(1<<7);
			else PORTB&=~(1<<7);
		break;

		case 14:
			if(Output==1) PORTJ|=(1<<1);
			else PORTJ&=~(1<<1);
		break;

		case 15:
			if(Output==1) PORTJ|=1;
			else PORTJ&=~(1);
		break;

		case 16:
			if(Output==1) PORTH|=(1<<1);
			else PORTH&=~(1<<1);
		break;

		case 17:
			if(Output==1) PORTH|=1;
			else PORTH&=~(1);
		break;

		case 18:
			if(Output==1) PORTD|=(1<<3);
			else PORTD&=~(1<<3);
		break;

		case 19:
			if(Output==1) PORTD|=(1<<2);
			else PORTD&=~(1<<2);
		break;

		case 20:
			if(Output==1) PORTD|=(1<<1);
			else PORTD&=~(1<<1);
		break;

		case 21:
			if(Output==1) PORTD|=1;
			else PORTD&=~(1);
		break;

		case 22:
			if(Output==1) PORTA|=1;
			else PORTA&=~(1);
		break;

		case 23:
			if(Output==1) PORTA|=(1<<1);
			else PORTA&=~(1<<1);
		break;

		case 24:
			if(Output==1) PORTA|=(1<<2);
			else PORTA&=~(1<<2);
		break;

		case 25:
			if(Output==1) PORTA|=(1<<3);
			else PORTA&=~(1<<3);
		break;

		case 26:
			if(Output==1) PORTA|=(1<<4);
			else PORTA&=~(1<<4);
		break;

		case 27:
			if(Output==1) PORTA|=(1<<5);
			else PORTA&=~(1<<5);
		break;

		case 28:
			if(Output==1) PORTA|=(1<<6);
			else PORTA&=~(1<<6);
		break;

		case 29:
			if(Output==1) PORTA|=(1<<7);
			else PORTA&=~(1<<7);
		break;

		case 30:
			if(Output==1) PORTC|=(1<<7);
			else PORTC&=~(1<<7);
		break;

		case 31:
			if(Output==1) PORTC|=(1<<6);
			else PORTC&=~(1<<6);
		break;

		case 32:
			if(Output==1) PORTC|=(1<<5);
			else PORTC&=~(1<<5);
		break;

		case 33:
			if(Output==1) PORTC|=(1<<4);
			else PORTC&=~(1<<4);
		break;

		case 34:
			if(Output==1) PORTC|=(1<<3);
			else PORTC&=~(1<<3);
		break;

		case 35:
			if(Output==1) PORTC|=(1<<2);
			else PORTC&=~(1<<2);
		break;

		case 36:
			if(Output==1) PORTC|=(1<<1);
			else PORTC&=~(1<<1);
		break;

		case 37:
			if(Output==1) PORTC|=1;
			else PORTC&=~(1);
		break;

		case 38:
			if(Output==1) PORTD|=(1<<7);
			else PORTD&=~(1<<7);
		break;

		case 39:
			if(Output==1) PORTG|=(1<<2);
			else PORTG&=~(1<<2);
		break;

		case 40:
			if(Output==1) PORTG|=(1<<1);
			else PORTG&=~(1<<1);
		break;

		case 41:
			if(Output==1) PORTG|=1;
			else PORTG&=~(1);
		break;

		case 42:
			if(Output==1) PORTL|=(1<<7);
			else PORTL&=~(1<<7);
		break;

		case 43:
			if(Output==1) PORTL|=(1<<6);
			else PORTL&=~(1<<6);
		break;

		case 44:
			if(Output==1) PORTL|=(1<<5);
			else PORTL&=~(1<<5);
		break;

		case 45:
			if(Output==1) PORTL|=(1<<4);
			else PORTL&=~(1<<4);
		break;

		case 46:
			if(Output==1) PORTL|=(1<<3);
			else PORTL&=~(1<<3);
		break;

		case 47:
			if(Output==1) PORTL|=(1<<2);
			else PORTL&=~(1<<2);
		break;

		case 48:
			if(Output==1) PORTL|=(1<<1);
			else PORTL&=~(1<<1);
		break;

		case 49:
			if(Output==1) PORTL|=1;
			else PORTL&=~(1);
		break;

		case 50:
			if(Output==1) PORTB|=(1<<3);
			else PORTB&=~(1<<3);
		break;

		case 51:
			if(Output==1) PORTB|=(1<<2);
			else PORTB&=~(1<<2);
		break;

		case 52:
			if(Output==1) PORTB|=(1<<1);
			else PORTB&=~(1<<1);
		break;

		case 53:
			if(Output==1) PORTB|=1;
			else PORTB&=~(1);
		break;
	}
	
	return 1;
#elif defined(__AVR_ATmega8__)|| defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
	if(pwm_pin<8){
		if(Output==1) PORTD|=(1<<pwm_pin);
		else PORTD&=~(1<<pwm_pin);
	}
	else{
		if(Output==1) PORTB=(1<<(pwm_pin-8));
		else PORTB&=~(1<<(pwm_pin-8));
	}
	
	return 1;
#endif
	
	return 0;
}

void TimerTwo::setPeriod(unsigned long microseconds){
  //oldSREG = SREG;
  TIMSK2 &= ~(1<<TOIE2);                                   // clears the timer overflow interrupt enable bit
  TCCR2B &= ~((1<<CS20) | (1<<CS21) | (1<<CS22));          // clears all clock selects bits (Timer/Counter stopped)  
  
  cycles=(microseconds*(F_CPU/1000000));
  
  if(cycles < RESOLUTION){
    clockSelectBits = 0x01;              // no prescale
	tcnt2_init = cycles;
	tcnt2_init = (RESOLUTION-1)-tcnt2_init;
	TCNT2 = tcnt2_init;						   //initialise TCNT2
  }
  else if(cycles < (RESOLUTION*8)){
	clockSelectBits = 0x02;              // prescale by /8
	tcnt2_init = cycles/8;
	TCNT2 = (RESOLUTION-1)-tcnt2_init;
  }
  else if(cycles < (RESOLUTION*32)){
    clockSelectBits = 0x03;  // prescale by /32
	tcnt2_init = cycles/32;
	tcnt2_init = (RESOLUTION-1)-tcnt2_init;
	TCNT2 = tcnt2_init;						   //initialise TCNT2
  }
  else if(cycles < (RESOLUTION*64)){
    clockSelectBits = 0x04;  // prescale by /64;
	tcnt2_init = cycles/64;
	tcnt2_init = (RESOLUTION-1)-tcnt2_init;
	TCNT2 = tcnt2_init;						   //initialise TCNT2
  }
  else if(cycles < (RESOLUTION*128)){
    clockSelectBits = 0x05;  // prescale by /128
	tcnt2_init = cycles/128;
	tcnt2_init = (RESOLUTION-1)-tcnt2_init;
	TCNT2 = tcnt2_init;						   //initialise TCNT2
  }
  else if(cycles < (RESOLUTION*256)){
    clockSelectBits = 0x06;  // prescale by /256
	tcnt2_init = cycles/256;
	tcnt2_init = (RESOLUTION-1)-tcnt2_init;
	TCNT2 = tcnt2_init;						   //initialise TCNT2
  }
  else if(cycles < (RESOLUTION*1024)){
    clockSelectBits = 0x07;  // prescale by /1024
	tcnt2_init = cycles/1024;
	tcnt2_init = (RESOLUTION-1)-tcnt2_init;
	TCNT2 = tcnt2_init;						   //initialise TCNT2
  }
  else{
	cycles = RESOLUTION - 1;
	clockSelectBits = 0x07;  // request was out of bounds, set as maximum
	tcnt2_init = 0;
	TCNT2 = 0;						   //initialise TCNT2
  }
  
  TIMSK2 |= (1<<TOIE2);                                     // sets the timer overflow interrupt enable bit
  TCCR2B |= clockSelectBits;       //set prescale bits
  //SREG = oldSREG;
}

void TimerTwo::EnableTimerInterrupt(void (*isr)(), unsigned long microseconds=0)
{
  if(microseconds > 0){
	TCCR2A = 0;                 // clear control register A
	TCCR2A &= ~((1<<WGM21)|(1<<WGM20));        // set mode 0: Normal
	TCCR2B &= ~(1<<WGM22);
	isrCallback = isr;                                       // register the user's callback with the real ISR
	setPeriod(microseconds);
	
	//if(pwm_arr!=NULL) delete [] pwm_arr;
  }										
}

void TimerTwo::DisableTimerInterrupt()
{
  TIMSK2 &= ~(1<<TOIE2);                                   // clears the timer overflow interrupt enable bit
  TCCR2B &= ~((1<<CS20) | (1<<CS21) | (1<<CS22));          // clears all clock selects bits (Timer/Counter stopped)
}

void TimerTwo::ResetTimer() //allows resetting of timer tcnt2_init after EnableTimerInterrupt
{
	TIMSK2 &= ~(1<<TOIE2);     // clears the timer overflow interrupt enable bit
	TCCR2B &= ~((1<<CS20) | (1<<CS21) | (1<<CS22));          // clears all clock selects bits (Timer/Counter stopped)
	TIFR2 &= ~(1<<TOV2);       //Clear Timer overflow flag
	TCNT2 = tcnt2_init;		   //reset TCNT2
	TCCR2B |= clockSelectBits; //Timer/Counter restarted
	TIMSK2 |= (1<<TOIE2);      // sets the timer overflow interrupt enable bit
	//sei();
}

void TimerTwo::ResumeTimer()
{ 
  TCCR2B |= clockSelectBits; //Timer/Counter resumes from where it stopped
}

void TimerTwo::StopTimer()
{
  TCCR2B &= ~((1<<CS20) | (1<<CS21) | (1<<CS22));          // clears all clock selects bits (Timer/Counter stopped)
}

uint8_t TimerTwo::EnablePWM(uint8_t pin,unsigned long microseconds=0,uint8_t duty=0)
{	
	if(microseconds > 0 && PinToDigital(pin,0)){
		pwm_arr = new uint8_t[4];
		pinMode(pin,OUTPUT);
		pwm_arr[2]=pin; //pwm pin
		TCCR2A = 0;                 // clear control register A
		TCCR2A &= ~((1<<WGM21)|(1<<WGM20));        // set mode 0: Normal
		TCCR2B &= ~(1<<WGM22);                     //
		setPeriod(microseconds);
		AdjustPwmDuty(duty);
		return 1;
	}
	return 0;
}

void TimerTwo::AdjustPwmDuty(uint8_t duty)
{
	volatile uint16_t period;
	
	StopTimer();
	
	period = (RESOLUTION-1)-tcnt2_init;
	
    if(duty==0||duty==100){
        pwm_arr[3] = (duty>>2)&1; //Output State
        pwm_arr[0] = tcnt2_init;
        PinToDigital(pwm_arr[2],pwm_arr[3]);
    }
    else{
        pwm_arr[1] = (period*duty)/100; //ONtime
        pwm_arr[0] = period-pwm_arr[1]; //OFFtime
    }
	
	ResumeTimer();
}

void TimerTwo::DisablePWM()
{ 
	DisableTimerInterrupt();
	delete [] pwm_arr;
}

#endif