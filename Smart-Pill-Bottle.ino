#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include "DSRTCLib.h"

//-----------------------------------------------------------------------------------------------//
//----------------------------------Set time for alarm-------------------------------------------//
//-----------------------------------------------------------------------------------------------//
// Set time for alarms in 24-hour time, omitting leading zeros and without a colon 
// example: {200,1414} for 2:00AM and 2:14PM; 
int times[] = {200, 1414}; 

//----------------------------------------------------------------------------------------------//
//---------------------------------------static code--------------------------------------------//
// Logger Setup
const int chipSelect = 10;

// RTC Setup
int ledPin =  13;   // LED connected to digital pin 13
int INT_PIN = 2;    // INTerrupt pin from the RTC. On Arduino Uno, this should be mapped to digital pin 2 or pin 3, which support external interrupts
int int_number = 1; // On Arduino Uno, INT0 corresponds to pin 2, and INT1 to pin 3

DS1339 RTC = DS1339();

int counter = 0;
int checklow = 0;
int checkhigh = 0;

// Button and pager motor setup
const int TOUCH_BUTTON_PIN = 4;  // Input pin for touch state
const int MOTOR = 3;             // Motor is used to indicate an alarm
int alarm = 0;
int pilltaken = 0;
int numtimes = sizeof(times)/sizeof(times[0]);

// Global Variables
int buttonState = LOW; // Variable for reading button
int pills = 16;

void setup() {

  pinMode(ledPin, OUTPUT);    
  pinMode(MOTOR, OUTPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(MOTOR, LOW);
  pinMode(TOUCH_BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(9600);
  Wire.begin();
  
  // Initialize SD
  Serial.print("Initializing SD card...");

  // Check if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  // Open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (dataFile){
    dataFile.println("......................");
    dataFile.println("Date, Time, Count");
    dataFile.close(); 
  }
  else
  {
    Serial.println("Couldn't open data file");
    return;
  }
  // Initialize RTC
  RTC.start();  // ensure RTC oscillator is running, if not already
  // set_time(); //**COMMENT OUT THIS LINE AFTER YOU SET THE TIME**//
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(pills);

  while (1){

    // Getting current date and time
    String ahoraDate;
    String ahoraTime;
    currentTime(ahoraDate, ahoraTime);
    String ahora = ahoraDate + "  " + ahoraTime;
    buttonState = digitalRead(TOUCH_BUTTON_PIN);

    // Turning alarm on, if it is the right time.
    for(int i = 0; i<numtimes; i++){
      
      if(int(RTC.getHours()) == int(times[i]/100)){
        if(int(RTC.getMinutes()) == int(times[i])-int(times[i]/100)*100){
          if(alarm == 0 && pilltaken == 0){
            digitalWrite(MOTOR,HIGH);
            alarm = 1;
            Serial.println("alarm on");
          }
        }
      }
      
    }

    // Check if a pill was missed
    for(int i = 0; i<numtimes; i++){
      
      if(int(RTC.getHours()) == int(times[i]/100)){
        if(int(RTC.getMinutes()) == int(times[i])-int(times[i]/100)*100+1){
          if(pilltaken == 0){
            String dataString = "";
            dataString = String(ahoraDate)  + ", " + String(ahoraTime)  + ", "  + String(counter)+ ", " + "missed pill";
            
            // If the file is available, write to it:
            File dataFile = SD.open("datalog.csv", FILE_WRITE);
            if(dataFile){
              dataFile.println(dataString);
              dataFile.close();
              Serial.println(dataString);
            }
            else {
                Serial.println("Couldn't access file");
            }
          }
          pilltaken = 0;
          alarm = 0;
          digitalWrite(MOTOR,LOW);
        }
      }
      
    }

    // Displaying number of pills
    LedOn(pills);

    // Handle situation data, when button was pressed.
    // Namely logging, resetting alarm
    if (buttonState == HIGH){
      checklow ++;
      if(checklow==80){      
        checkhigh = 0;
        
        String dataString = "";
        // Reducing the number
        pills = pills-1;
        if(pills==0){
          pills = 16;
        }
                
        // Turn off alarm:
        if(alarm == 1){
          digitalWrite(MOTOR,LOW);
          alarm = 0;
          pilltaken =1;
        }

        // Logging
        dataString = String(ahoraDate)  + ", " + String(ahoraTime)  + ", "  + String(counter)+ ", " + String(pills);
        // If the file is available, write to it:
        File dataFile = SD.open("datalog.csv", FILE_WRITE);
        if(dataFile){
          dataFile.println(dataString);
          dataFile.close();
          Serial.println(dataString);
       }
        else
        {
          Serial.println("Couldn't access file");
        }
      }
    }

    // Resetting checklow if conditions is meet
    if(buttonState == LOW){
      checkhigh++;
      if (checkhigh>=80){
        checklow = 0;  
      }
    }
      delay(20);
      counter++;
  }
}

int read_int(char sep)
{
  static byte c;
  static int i;
  i = 0;
  while (1)
  {
    while (!Serial.available())
    {;}
    
    c = Serial.read();
    
    if (c == sep)
    {
      return i;
    }
    if (isdigit(c))
    {
      i = i * 10 + c - '0';
    }
    else
    {
      Serial.print("\r\nERROR: \"");
      Serial.write(c);
      Serial.print("\" is not a digit\r\n");
      return -1;
    }
  }
}

int read_int(int numbytes)
{
  static byte c;
  static int i;
  int num = 0;
  i = 0;
  while (1)
  {
    while (!Serial.available())
    {;}
    
    c = Serial.read();
    num++;
    
    if (isdigit(c))
    {
      i = i * 10 + c - '0';
    }
    else
    {
      Serial.print("\r\nERROR: \"");
      Serial.write(c);
      Serial.print("\" is not a digit\r\n");
      return -1;
    }
    if (num == numbytes)
    {
      // Serial.print("Return value is");
      // Serial.println(i);
      return i;
    }
  }
}

int read_date(int *year, int *month, int *day, int *hour, int* minute, int* second)
{
  *year = read_int(4);
  *month = read_int(2);
  *day = read_int(' ');
  *hour = read_int(':');
  *minute = read_int(':');
  *second = read_int(2);
  return 0;
}

void currentTime(String &nowDate, String &nowTime){
   RTC.readTime();
   nowDate = String(int(RTC.getMonths())) + "/" + String (int(RTC.getDays())) + "/" + String(RTC.getYears()-2000);
   nowTime = String(int(RTC.getHours())) + ":" + String(int(RTC.getMinutes())) + ":" + String(int(RTC.getSeconds()));  
}  
  
void LedOn(int ledNum)
{
  for(int i=5;i<10;i++){
    pinMode(i, INPUT);
    digitalWrite(i, LOW);
  };
  if(ledNum<1 || ledNum>16) return;
  char highpin[16]={5,6,5,7,6,7,6,8,5,8,8,7,9,7,9,8};
  char lowpin[16]= {6,5,7,5,7,6,8,6,8,5,7,8,7,9,8,9};
  ledNum--;
  digitalWrite(highpin[ledNum],HIGH);
  digitalWrite(lowpin[ledNum],LOW);
  pinMode(highpin[ledNum],OUTPUT);
  pinMode(lowpin[ledNum],OUTPUT);
}

void set_time()
{
    Serial.println("Enter date and time (YYYYMMDD HH:MM:SS)");
    int year, month, day, hour, minute, second;
    int result = read_date(&year, &month, &day, &hour, &minute, &second);
    if (result != 0) {
      Serial.println("Date not in correct format!");
      return;
    }    
    RTC.setSeconds(second);
    RTC.setMinutes(minute);
    RTC.setHours(hour);
    RTC.setDays(day);
    RTC.setMonths(month);
    RTC.setYears(year);
    RTC.writeTime();
    read_time();
}

void read_time() 
{
  Serial.print ("The current time is ");
  RTC.readTime();                                          
  printTime(0);
  Serial.println();
}


void printTime(byte type)
{                                                  
  if(!type)
  {
    Serial.print(int(RTC.getMonths()));
    Serial.print("/");  
    Serial.print(int(RTC.getDays()));
    Serial.print("/");  
    Serial.print(RTC.getYears());
  }
  else
  {                                                                                                                 
    {
      Serial.print(int(RTC.getDayOfWeek()));
      Serial.print("th day of week, ");
    }       
    {
      Serial.print(int(RTC.getDays()));
      Serial.print("th day of month, ");      
    }
  }
  Serial.print("  ");
  Serial.print(int(RTC.getHours()));
  Serial.print(":");
  Serial.print(int(RTC.getMinutes()));
  Serial.print(":");
  Serial.print(int(RTC.getSeconds()));  
}
