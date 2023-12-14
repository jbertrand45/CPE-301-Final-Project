/****************************
* CS 302, Fall 2023
* Final Project
* Tristan Hughes, Joey Bertrand, Carlos Hernandez
****************************/

#include <Stepper.h>
#include <LiquidCrystal.h>
#include "DHT.h"
#include "RTClib.h"

#define RDA 0x80
#define TBE 0x20

#define RUNNING 1
#define IDLE 2
#define DISABLED 3
#define ERROR 4

const int currState;

LiquidCrystal lcd(13, 12, 11, 3, 2, 4);

RTC_DS1307 rtc;
unsigned long int initialMill;
unsigned int currMill;
unsigned int count;

const int totalSteps = 1024
Stepper stepperMotor(totalSteps, 8, 9, 10, 11);

DHT dht(10, DHT11);

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2; 
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
volatile unsigned char *my_ADMUX    = (unsigned char*) 0x7C; 
volatile unsigned char *my_ADCSRB   = (unsigned char*) 0x7B; 
volatile unsigned char *my_ADCSRA   = (unsigned char*) 0x7A; 
volatile unsigned int  *my_ADC_DATA = (unsigned int*)  0x78;

volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24; 

void setup() {
  
  U0init(9600);

  adc_init();

  lcd.begin(16,2);
}

void loop() {

  currState = getState();

  if(currState == RUNNING){
    LEDon(RUNNING);
    printTemp();
    printHumidity();
    printStateChange();
    currState = getState();
  }
  else if(currState == IDLE){
    LEDon(IDLE);
    printTemp();
    printHumidity();
    printStateChange();
    currState = getState();
  }
  else if(currState == DISABLED){
    LEDon(DISABLED);
    printStateChange();
    currState = getState();
  }
  else if(currState == ERROR){
    LEDon(ERROR);
    printTemp();
    printHumidity();
    printStateChange();
    currState = getState();
  }

  delay(1000);
}

void U0init(int U0baud){
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / (16 * U0baud)) - 1;
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0 = tbaud;
}

void U0putchar(unsigned char U0pdata){
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

void adc_init() {

  *my_ADCSRA |= 0b10000000; 
  *my_ADCSRA &= 0b11011111; 
  *my_ADCSRA &= 0b11011111; 
  *my_ADCSRA &= 0b11011111; 
  
  *my_ADCSRB &= 0b11110111; 
  *my_ADCSRB &= 0b11111000; 
  
  *my_ADMUX  &= 0b01111111;
  *my_ADMUX  |= 0b01000000; 
  *my_ADMUX  &= 0b11011111;
  *my_ADMUX  &= 0b11011111;
  *my_ADMUX  &= 0b11100000; 
}

unsigned int adc_read(unsigned char adc_channel_num){
  *my_ADMUX &= 0b11100000;
  *my_ADCSRB &= 0b11110111;

  if(adc_channel_num > 7){
    adc_channel_num -= 8;
    *my_ADCSRB |= 0b00001000;
  }

  *my_ADMUX += adc_channel_num;
  *my_ADCSRB |= 0x40;

  while((*my_ADCSRA & 0x40) != 0);

  return *my_ADC_DATA;
}

int getState(){
  
}

void LEDon(int LEDnum){
  if(){

  }
  else if(){

  }
  else if(){

  }
  else if(){

  }
}

void allLEDoff(){

}

void printTemp(){

}

void printHumidity(){

}

void printStateChange(){
  
}
