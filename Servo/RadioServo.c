//
//  manyServos.c
//
//
//  Created by Nafis Abrar  on 8/15/19.
//

#define F_CPU 1000000UL
#include <stdio.h>
#include<math.h>
#include <avr/io.h>
#include<avr/interrupt.h>
#include<util/delay.h>

#define UART_BAUD_RATE 4800
#include "uart.h"
#include "stdutils.h"

volatile unsigned int timerCount = 0;
volatile int state = 0;
volatile int servo1 = 90;
volatile int servoTarget1 = 90;

volatile int servo2 = 90;
volatile int servoTarget2 = 90;

volatile int servo3 = 90;
volatile int servoTarget3 = 90;


int getPulseWidth(int angle) {
	//for servo, full clockwise is 0 degrees, full anticlockwise is 180 degrees
	//map values from 0 - 180 to 400-2200
	
	
	int result = angle * 10;
	result = result + 400;
	//    double result = angle / 180.0; //= 0-1
	//    result = result * 1800.0; //=0-1800
	//    result = result + 400.0; //400-2200
	
	//    double x = round(result);
	//    int y = (int) x;
	
	//extreme values check
	if(result < 400) return 400;
	if(result > 2200) return 2200;
	
	//return y;
	return result;
}

void setAngle(int pin, double angle) {
	
	
	
}

int main(void) {
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU));
	sei();
	//all servos in port A
	
	DDRB = 0x00;	
	DDRA = 0xFF;
	TCCR1A |= 1 << WGM11;
	TCCR1B |= 1 << WGM12 | 1 << WGM13 | 1 << CS10;
	TIMSK  |= 1 << OCIE1A;
	ICR1 = 19999;
	unsigned char startyaw = 0b00110011;
	unsigned char rollpitch = 0b11111111;
	unsigned char clawend = 0b11001100;

	sei();

	servo1 = 0;
	servo2 = 90;
	servo3 = 90;
	//state = 0; //0 for counting up, 1 for counting down
	while(1) {
		
		//Here, values to servo1 and servo2 etc... will be processed from gyro readings
		
		if(TCNT1 >= 300 && TCNT1 <= 2300) {
			if(TCNT1 >= getPulseWidth(servo1) && bit_is_set(PORTA, PINA0)) PORTA &= ~(1 << PINA0);
			if(TCNT1 >= getPulseWidth(servo2) && bit_is_set(PORTA, PINA1)) PORTA &= ~(1 << PINA1);
			if(TCNT1 >= getPulseWidth(servo3) && bit_is_set(PORTA, PINA2)) PORTA &= ~(1 << PINA2);
			
		}
		
		if(TCNT1 < 300 || TCNT1 > 2300) {
			
			if (state == 0){	//BEFORE START
				if ((PINB & 0b00001100) == 0b00001100){		//GOT YAW
					uart_puts("HEREEEEE\n\n");
					clawend = 0b11001100;
					state++;
					startyaw = (PINB << 4);
					startyaw = (startyaw & 0b00110000);
					if ((startyaw & 0b00100000)  && (startyaw & 0b00010000)) {
                        //error
						startyaw = 0b00110011;
						state=0;
					}
					else if (startyaw & 0b00100000) {
						uart_puts("yaw++\n");
						servoTarget1++;
					}
					else if (startyaw & 0b00010000) {
						uart_puts("yaw--\n");
						servoTarget1--;
					}
					else {
                        uart_puts("YAWN\n");
                        
                    }
					
					
				}
			}
			else if (state == 1) {
				if ((PINB & 0b00001100) != 0b00001100){			
					state++;
					startyaw = 0b00110011;
					
					rollpitch = (PINB << 4);
					rollpitch = rollpitch & 0b11110000;
					if ( ((rollpitch & 0b10000000) && (rollpitch & 0b01000000))  || ((rollpitch & 0b00100000) && (rollpitch & 0b00010000)) ){
                        //error
						rollpitch = 0b11111111;
						state=0;
						continue;
					}
					else if(rollpitch & 0b10000000){
						 uart_puts("Roll++\n");
						 servoTarget2++;
					}
					else if(rollpitch & 0b01000000) {
						uart_puts("Roll--\n");
						servoTarget2--;
						
					}
					else {
						uart_puts("RollNo\n");
					}
					if(rollpitch & 0b00100000) {
						uart_puts("Pitch++\n");
						servoTarget3++;
					}
					else if(rollpitch & 0b00010000) {
						uart_puts("Pitch--\n");
						servoTarget3--;
					}
					else {
						uart_puts("Unpitch\n");
					}
									
					
				}
				
			}
			else if (state == 2) {
				if (((PINB & 0b00001100) == 0b00001100) || ((PINB & 0b00000011) != 0b00000011) )state = 0;  //error
				else if ( ((PINB << 4) & 0b11110000) != rollpitch ){		//found claw
					rollpitch = 0b11111111;
					state = 0;
					clawend = (PINB << 4);
					clawend = clawend & 0b11110000;
					if(clawend & 0b10000000) {
						uart_puts("Claw Open\n");
					}
					else if(clawend & 0b01000000) {
						uart_puts("Claw Close\n");
					}
					else {
						uart_puts("CLAWN\n");
					}
				}
				
				
				
			}
		}
	
		
	
	}
}


ISR(TIMER1_COMPA_vect) {
	//interrupt called when TCNT1 value reaches ICR1 value (every 20ms)
	
	//PORTA = 0xFF;
	timerCount++;
	if(timerCount >= 10) {  //every 200ms
		if(servo1 < servoTarget1 - 7) {
			servo1 = servo1 + 10;
		}
		if (servo1 > servoTarget1 + 7) {
			servo1 = servo1 - 10;
		}
		
		if(servo2 < servoTarget2 - 7) {
			servo2 = servo2 + 10;
		}
		if (servo2 > servoTarget2 + 7) {
			servo2 = servo2 - 10;
		}
		
		if(servo3 < servoTarget3 - 7) {
			servo3 = servo3 + 10;
		}
		if (servo3 > servoTarget3 + 7) {
			servo3 = servo3 - 10;
		}

		timerCount = 0;
	}
}
