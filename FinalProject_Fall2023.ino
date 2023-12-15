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

// State Identifiers
#define RUNNING 1
#define IDLE 2
#define DISABLED 3
#define ERROR 4

int currState;

//Buttons
bool startButton = false;
bool restartButton = false;
bool stopButton = false;

// LCD
LiquidCrystal lcd(13, 12, 11, 3, 2, 4);

// Real-Time Clock
RTC_DS1307 rtc;
unsigned int count;

// Stepper Motor
const int totalSteps = 1024;
Stepper stepperMotor(totalSteps, 49, 47, 45, 43);

// DHT setup and ports
DHT dht(8, DHT11);
float ambientTemp;
float waterVoltage;

volatile unsigned int* port_H = (unsigned int*) 0x102;
volatile unsigned int* ddr_h = (unsigned int*) 0x101;

// LED ports
volatile unsigned char* port_a = (unsigned char*) 0x22;
volatile unsigned char* port_c = (unsigned char*) 0x28;

volatile unsigned char* ddr_a = (unsigned char*) 0x21;
volatile unsigned char* ddr_c = (unsigned char*) 0x27;

// Fan Motor
volatile unsigned char* port_g = (unsigned char*) 0x34;
volatile unsigned char* ddr_g = (unsigned char*) 0x33;

// Water Sensor Ports
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
  
  // UART
  U0init(9600);

  // Initialize ADC
  adc_init();

  // Initialize LCD
  lcd.begin(16,2);

  // Initialize DHT
  dht.begin();
  *ddr_h &= 0b00100000;

  // Initialize LEDS
  *ddr_a |= 0b01000000;
  *ddr_c |= 0b10101000;
  LEDoff();

  // Initialize Water Sensor
  *ddr_b |= 0b00000100;

  // Initialize Fan Motor
  *ddr_g |= 0b00000001;

  // Initialize Stepper
  stepperMotor.setSpeed(5);
  stepperMotor.step(0);

  // Initialize RTC
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Set default state
  currState = IDLE;
}

void loop() {

  //startButton = getStartButton();
  //stopButton = getStopButton();
  //restartButton = getRestartButton();

  ambientTemp = getTemp();
  waterVoltage = getWaterVoltage();

  if(currState == RUNNING){
    LEDoff();
    LEDon(RUNNING);
    LCDMinuteTimer();
    printStateChange(RUNNING);
    runFanMotor();
    if(ambientTemp < 20){
      currState = IDLE;
    }
    else if(waterVoltage < 1){
      currState = ERROR;
    }
    else if(stopButton){
      currState = DISABLED;
    }
    else{
      currState = RUNNING;
    }
  }
  else if(currState == IDLE){
    LEDoff();
    LEDon(IDLE);
    LCDMinuteTimer();
    printStateChange(IDLE);
    if(waterVoltage < 1){
      currState = ERROR;
    }
    else if(ambientTemp > 20){
      currState = RUNNING;
    }
    else if(stopButton){
      currState = DISABLED;
    }
    else{
      currState = IDLE;
    }
  }
  else if(currState == DISABLED){
    LEDoff();
    LEDon(DISABLED);
    printStateChange(DISABLED);
    if(startButton){
      currState = RUNNING;
    }
    else{
      currState = DISABLED;
    }
  }
  else if(currState == ERROR){
    LEDoff();
    LEDon(ERROR);
    LCDMinuteTimer();
    printStateChange(ERROR);
    LCDdispError();
    if(restartButton){
      currState = IDLE;
    }
    else if(stopButton){
      currState = DISABLED;
    }
    else{
      currState = ERROR;
    }
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

void LEDon(int LEDnum){
  if(LEDnum == 1){
    *port_c |= 0b00100000;
  }
  else if(LEDnum == 2){
    *port_a |= 0b01000000;
  }
  else if(LEDnum == 3){
    *port_c |= 0b10000000;
  }
  else if(LEDnum == 4){
    *port_c |= 0b00001000;
  }
}

void LEDoff(){
  *port_a &= 0b10111111;
  *port_c &= 0b01010111;
}

void printStats(){
  float humidity;
  float temperature;

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if(temperature > 20){
    runFanMotor();
  }
  else{
    stopFanMotor();
  }

  LCDPrintStats(temperature, humidity);
}

void printTime(){
  DateTime present = rtc.now();
  String presentTime = present.timestamp(DateTime::TIMESTAMP_TIME);
  String presentDate = present.timestamp(DateTime::TIMESTAMP_DATE);

  stringHelper("Time of Change: ");
  stringHelper(presentTime.c_str());
  stringHelper("\n");

  stringHelper("Date: ");
  stringHelper(presentDate.c_str());
  stringHelper("\n");
}

void printStateChange(int stateNum){
  stringHelper("State Changed \n");
  stringHelper("System now in: ");
  if(stateNum == 1){
    stringHelper(RUNNING);
  }
  else if(stateNum == 2){
    stringHelper("IDLE");
  }
  else if(stateNum == 3){
    stringHelper("DISABLED");
  }
  else if(stateNum == 4){
    stringHelper("ERROR");
  }

  stringHelper(" mode \n");

  printTime();
}

void stringHelper(char* userString){  
  for(int i = 0; userString[i] != '\0'; i++){
    U0putchar(userString[i]);
  }
}

void runFanMotor(){
  *port_g |= 0b00000001;
}

void stopFanMotor(){
  *port_g &= 0b11111110;
}

/*bool getStartButton(){

}

bool getStopButton(){

}

bool getRestartButton(){

}*/

void LCDPrintStats(float systemTemp, float systemHumid){
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("Humidity: ");
  lcd.print(systemHumid);

  lcd.setCursor(0,1);
  lcd.print("Temp: ");
  lcd.print(systemTemp);
  lcd.print(" C");

  delay(5000);
  lcd.clear();
}

void LCDdispError(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   Error");

  lcd.setCursor(0,1);
  lcd.print("Water Level Low");

  delay(5000);
  lcd.clear();
}

void LCDMinuteTimer(){
  if(count > 6000){
    printStats();
    startCount();
  }
  else{
    return;
  }
}

void startCount(){
  count = millis();
}

float getTemp(){
  float temp = dht.readTemperature();

  return temp;
}

float getWaterVoltage(){
  float adcValue;
  float waterVal;

  *port_b |= 0b00000100;
  delay(1000);

  adcValue = adc_read(6);

  *port_b &= 0b11111011;

  waterVal = adcValue * (5 / 1023.0);

  return waterVal;
}
