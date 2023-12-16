/****************************
* CS 302, Fall 2023
* Final Project
* Tristan Hughes, Joey Bertrand, Carlos Hernandez
****************************/

// Libraries
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

// Fan & Water Sensor Ports
volatile unsigned int* port_j = (unsigned int*) 0x105;
volatile unsigned int* ddr_j = (unsigned int*) 0x104;

volatile unsigned int* port_h = (unsigned int*) 0x102;
volatile unsigned int* ddr_h = (unsigned int*) 0x101;

// LED ports
volatile unsigned char* port_a = (unsigned char*) 0x22;
volatile unsigned char* port_c = (unsigned char*) 0x28;

volatile unsigned char* ddr_a = (unsigned char*) 0x21;
volatile unsigned char* ddr_c = (unsigned char*) 0x27;

// ADC addresses
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2; 
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
volatile unsigned char *my_ADMUX    = (unsigned char*) 0x7C; 
volatile unsigned char *my_ADCSRB   = (unsigned char*) 0x7B; 
volatile unsigned char *my_ADCSRA   = (unsigned char*) 0x7A; 
volatile unsigned int  *my_ADC_DATA = (unsigned int*)  0x78;

void setup() {
  
  // UART
  U0init(9600);

  // Initialize ADC
  adc_init();

  // Initialize LCD
  lcd.begin(16,2);

  // Initialize DHT
  dht.begin();
  *ddr_h &= 0b00100111;

  // Initialize LEDS
  *ddr_a |= 0b01000000;
  *ddr_c |= 0b10101010;
  LEDoff();

  // Initialze Fan & Water Sensor
  *ddr_j |= 0b00000011;

  // Initialize Stepper
  stepperMotor.setSpeed(5);
  stepperMotor.step(0);

  // Initialize RTC
  rtc.begin();
  DateTime presentTime = DateTime(2023, 12, 15, 0, 0, 0);
  rtc.adjust(presentTime);

  // Set default state
  currState = IDLE;
  startCount();
}

void loop() {
  // The printStateChange() functions within the if-else loops are commented out
  // because they spam random characters into the serial monitor, potentially crashing the program.
  //
  // Returns the statuses of the buttons
  startButton = getStartButton();
  stopButton = getStopButton();
  restartButton = getRestartButton();

  // Returns temperature and water level
  ambientTemp = getTemp();
  waterVoltage = getWaterVoltage();

  // Running State
  if(currState == RUNNING){
    LEDoff();
    LEDon(RUNNING);
    LCDMinuteTimer();
    //printStateChange(RUNNING);
    changeVentPosition(RUNNING);
    runFanMotor();
    if(ambientTemp < 20){
      currState = IDLE;
      stopMotor();
    }
    else if(waterVoltage < 0.75){
      currState = ERROR;
      stopMotor();
    }
    else if(stopButton){
      currState = DISABLED;
      stopMotor();
    }
    else{
      currState = RUNNING;
    }
  }
  // Idle State
  else if(currState == IDLE){
    LEDoff();
    LEDon(IDLE);
    LCDMinuteTimer();
    //printStateChange(IDLE);
    if(waterVoltage < 0.75){
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
  // Disabled State
  else if(currState == DISABLED){
    LEDoff();
    LEDon(DISABLED);
    //printStateChange(DISABLED);
    if(startButton){
      currState = RUNNING;
    }
    else{
      currState = DISABLED;
    }
  }
  // Error State
  else if(currState == ERROR){
    LEDoff();
    LEDon(ERROR);
    LCDMinuteTimer();
    //printStateChange(ERROR);
    LCDdispError();
    changeVentPosition(ERROR);
    if(restartButton){
      currState = IDLE;
      stopMotor();
    }
    else if(stopButton){
      currState = DISABLED;
      stopMotor();
    }
    else{
      currState = ERROR;
      waterVoltage = getWaterVoltage();
    }
  }

  delay(1000);
}

// UART Functions
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

// ADC functions
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

// LED Functions
void LEDon(int LEDnum){
  if(LEDnum == 1){
    *port_c |= 0b00100010;
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
  *port_c &= 0b01010101;
}

// Gets humidity and temperature readings and displays on LCD
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

// Prints current date and time in serial monitor
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

// Prints state change in serial monitor
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

// Prints strings to serial monitor
void stringHelper(char* userString){  
  char* printString = userString;
  for(int i = 0; printString[i] != '\0'; i++){
    U0putchar(printString[i]);
  }
}

// Fan Functions
void runFanMotor(){
  *port_j |= 0b00000010;
}

void stopFanMotor(){
  *port_j &= ~(0b11111101);
}

// Button Functions
bool getStartButton(){
 *port_h &= (0b00000100);
 if(PINH & (1 << 2)){
  return true;
 }
 else{
  return false;
 }
}

bool getStopButton(){
  *port_h &= (0b00000001);
  if(PINH & (1 << 0)){
    return true;
  }
  else{
    return false;
  }
}

bool getRestartButton(){
  *port_h &= (0b00000010);
  if(PINH & (1 << 1)){
    return true;
  }
  else{
    return false;
  }
}

// LCD functions
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
    printStats();
    return;
  }
}

// Timer function
void startCount(){
  count = millis();
}

// DHT and Voltage functions
float getTemp(){
  float temp = dht.readTemperature();

  return temp;
}

float getWaterVoltage(){
  float adcValue;
  float waterVal;

  *port_j |= 0b00000001;

  adcValue = adc_read(1);
  delay(1000);

  *port_j &= ~(0b11111110);

  waterVal = adcValue * (5 / 1023.0);

  return waterVal;
}

// Motor Functions
void changeVentPosition(int currentState){
  if(currentState == 1){
    stepperMotor.step(2048);
  }
  else{
    stepperMotor.step(2048);
  }
}

void stopMotor(){
  stepperMotor.step(0);
}
