/*
 * sys.c
 *
 *  Created on: 26-12-2012
 *      Author: jachu
 */
#include "sys.h"
#include "misc.h"
#include "motor.h"
#include "ctrl.h"
#include "led.h"
#include "adc.h"
#include "lcd.h"
#include "buttons.h"
#include "lineDet.h"
#include "eeprom.h"
#include "algorithm.h"

volatile unsigned int del;
volatile bool lcdEnable;

void sysInit(){
	lcdEnable = false;
	ledInit();
	lineDetInit();
	//ctrlInit();
	motorInit(/*9, 30, 0,*/ 1, 1, 64*19);
	adcInit();
	buttonsInit();
	startModuleInit();
	SysTick_Config(SystemCoreClock / SYS_freq);
	LcdInit();

	/* Unlock the Flash Program Erase controller */
	FLASH_Unlock();
	/* EEPROM Init */
	EE_Init();

	lcdEnable = true;
}

void sysDelayMs(unsigned int ms){
	del = ms;
	while(del > 0) {}
}

void sysDelayMsRet(unsigned int ms){
	del = ms;
}

bool sysIfDelayExp(){
	return del == 0;
}

uint8_t ftoa(float num, char* buffer, uint8_t dig, uint8_t dec){
	uint8_t pos = 0;
	if(num < 0){
		buffer[pos++] = '-';
		num = -num;
	}
	pos += uitoa(num, buffer + pos, 10);
	int decDig = min(dig - pos, dec);
	if(decDig > 0){
		buffer[pos++] = '.';
	}
	for(uint8_t i = 0; i < decDig; i++){
		num *= 10;
		buffer[pos++] = (char)((((int)num) % 10) + 0x30);
	}
	buffer[pos] = 0;
	return pos;
}

uint8_t uitoa(uint32_t num, char* buffer, uint8_t base){
	uint8_t pos = 0;
	if(num == 0){
		buffer[pos++] = '0';
		return pos;
	}
	while(num > 0){
		buffer[pos++] = (char)(num % base + 0x30);
		num /= base;
	}
	for(uint8_t i = 0; i < pos/2; i++){
		char tmp = buffer[i];
		buffer[i] = buffer[pos - i - 1];
		buffer[pos - i - 1] = tmp;
	}
	buffer[pos] = 0;
	return pos;
}

uint8_t atof(float* num, char* buffer){
	uint8_t pos = 0;
	uint32_t tmp;
	int8_t mul = 1;
	if(buffer[pos] == '-'){
		mul = -1;
		pos++;
	}
	pos += atoui(&tmp, buffer + pos);
	*num = tmp;
	if(buffer[pos] == '.'){
		pos++;
		uint32_t pow = 1;
		while(0x30 <= buffer[pos] && buffer[pos] < 0x3a){
			pow *= 10;
			*num += (float)(buffer[pos++] - 0x30)/pow;
		}
	}
	*num *= mul;
	return pos;
}

uint8_t atoui(uint32_t* num, char* buffer){
	uint8_t pos = 0;
	*num = 0;
	while(0x30 <= buffer[pos] && buffer[pos] < 0x3a){
		*num *= 10;
		*num += (buffer[pos++] - 0x30);
	}
	return pos;
}

uint32_t min(uint32_t a, uint32_t b){
	return a <= b ? a : b;
}


uint8_t strcmpLen(const char* str1, const char* str2, uint16_t len){
	for(uint16_t i = 0; i < len; i++){
		if(str1[i] != str2[i]){
			return 1;
		}
	}
	return 0;
}

extern "C" {

void SysTick_Handler(void){
	if(del != 0){
		del--;
	}
	static uint16_t cnt = 0;
	/*if(cnt % (uint16_t)(SYS_freq/INFO_freq) == 0){
		ctrlSendInfo();
	}*/
	/*uint16_t ccrVal = TIM1->CCR3;
	if(ccrVal > 0){
		ledSet(ledPins[0]);
	}
	else{
		ledReset(ledPins[0]);
	}*/
	if(cnt % (uint16_t)(SYS_freq/PID_freq) == 1){
//		static bool ledState = false;
		motorPID(MotorLeft);
		motorPID(MotorRight);
//		if(ledState == false){
//			ledSet(ledPins[0]);
//			ledState = true;
//		}
//		else{
//			ledReset(ledPins[0]);
//			ledState = false;
//		}
	}
	if(cnt % (uint16_t)(SYS_freq/ADC_freq) == 0){
		updateMeasVol();
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	}
	if(cnt % (uint16_t)(SYS_freq/BUTTONS_freq) == 3){
		buttonsSys();
	}
	if(cnt % (uint16_t)(SYS_freq/LCD_freq) == 4 && lcdEnable == true){
		LcdUpdate();
	}
	if(cnt % (uint16_t)(SYS_freq/RAMP_freq) == 5){
		motorRamp(RAMP_freq, MotorLeft);
		motorRamp(RAMP_freq, MotorRight);
	}
	if(cnt % (uint16_t)(SYS_freq/LINE_DET_freq) == 6){
		lineDetSys();
	}
	if(cnt % (uint16_t)(SYS_freq/INDICATOR_freq) == 7){
		static bool ledState = false;
		if(ledState == false){
			ledSet(ledPins[0]);
			ledState = true;
		}
		else{
			ledReset(ledPins[0]);
			ledState = false;
		}

		if(motorIsCurLimited(MotorLeft)){
			ledSet(ledPins[2]);
		}
		else{
			ledReset(ledPins[2]);
		}
		if(motorIsCurLimited(MotorRight)){
			ledSet(ledPins[3]);
		}
		else{
			ledReset(ledPins[3]);
		}
	}

	cnt++;
	cnt %= SYS_freq;
}

}
