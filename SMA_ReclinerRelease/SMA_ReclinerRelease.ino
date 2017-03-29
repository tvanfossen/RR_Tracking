
/****************************************************
FILE:  AMS_5600_example

Author: Tom Denton
www.ams.com
Date: 15 Dec 2014
Version 1.00

Description:  AS5600 "Potuino" demonstration application

AMS5600 Programming Sketch
/***************************************************/

#include <Wire.h>
#include "AMS_5600.h"
#include <SoftwareSerial.h>
#include <CapacitiveSensor.h>


#define rxPin 3  // pin 3 connects to smcSerial TX  (not used in this example)
#define txPin 4  // pin 4 connects to smcSerial RX
SoftwareSerial smcSerial = SoftwareSerial(rxPin, txPin);
CapacitiveSensor   cs_9_8 = CapacitiveSensor(9,8);        // 10M resistor between pins 4 & 8, pin 8 is sensor pin, add a wire and or foil


String lastResponse;
String noMagnetStr = "Error: magnet not detected";

AMS_5600 ams5600;

/*******************************************************
/* function: setup
/* In: none
/* Out: none
/* Description: called by system at startup
/*******************************************************/
void setup(){
 Serial.begin(9600);
 Wire.begin();
 // initialize software serial object with baud rate of 19.2 kbps
 smcSerial.begin(19200);
 cs_9_8.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
 
  // the Simple Motor Controller must be running for at least 1 ms
  // before we try to send serial data, so we delay here for 5 ms
  delay(5);
 
  // if the Simple Motor Controller has automatic baud detection
  // enabled, we first need to send it the byte 0xAA (170 in decimal)
  // so that it can learn the baud rate
  smcSerial.write(0xAA);  // send baud-indicator byte
 
  // next we need to send the Exit Safe Start command, which
  // clears the safe-start violation and lets the motor run
  exitSafeStart();  // clear the safe-start violation and let the motor run
}
/*******************************************************
/* Function: convertRawAngleToDegrees
/* In: angle data from AMS_5600::getRawAngle
/* Out: human readable degrees as float
/* Description: takes the raw angle and calculates 
/* float value in degrees.
/*******************************************************/
float convertRawAngleToDegrees(word newAngle)
{
  /* Raw data reports 0 - 4095 segments, which is 0.087 of a degree */    
  float retVal = newAngle * 0.087;
  return retVal;
}

/*******************************************************
/* Function: convertScaledAngleToDegrees
/* In: angle data from AMS_5600::getScaledAngle
/* Out: human readable degrees as float
/* Description: takes the scaled angle and calculates 
/* float value in degrees.
/*******************************************************/
float convertScaledAngleToDegrees(word newAngle)
{
  word startPos = ams5600.getStartPosition();
  word endPos = ams5600.getEndPosition();
  word maxAngle = ams5600.getMaxAngle();
  
  float multipler = 0;
  
  /* max angle and end position are mutually exclusive*/
  if(maxAngle >0)
  {
    if(startPos == 0)
      multipler = (maxAngle*0.0878)/4096;
    else  /*startPos is set to something*/
      multipler = ((maxAngle*0.0878)-(startPos * 0.0878))/4096;
  }
  else
  {
    if((startPos == 0) && (endPos == 0))
      multipler = 0.0878;
    else if ((startPos > 0 ) && (endPos == 0))
      multipler = ((360 * 0.0878) - (startPos * 0.0878)) / 4096; 
    else if ((startPos == 0 ) && (endPos > 0))
      multipler = (endPos*0.0878) / 4096;
    else if ((startPos > 0 ) && (endPos > 0))
      multipler = ((endPos*0.0878)-(startPos * 0.0878))/ 4096;
  }
  return (newAngle * multipler);
}

/*******************************************************
/* Function: burnAngle
/* In: none
/* Out: human readable string of success or failure
/* Description: attempts to burn angle data to AMS5600 
/*******************************************************/
String burnAngle()
{
  int burnResult = ams5600.burnAngle();
  String returnStr = "Brun angle error: ";              
  
  switch (burnResult)
  {
    case 1:
      returnStr = "Brun angle success";
      break;
    case -1:
      returnStr += "no magnet detected";
      break;
    case -2:
      returnStr += "no more burns left";
      break;
    case -3:
      returnStr += "no positions set";
      break;
    default:
      returnStr += "unknown";
      break;
  }
  return returnStr;
}

/*******************************************************
/* Function: burnMaxAngleAndConfig
/* In: none
/* Out: human readable string of sucess or failure
/* Description: attempts to burn max angle and config data
/* to AMS5600 
/*******************************************************/
String burnMaxAngleAndConfig()
{
  int burnResult = ams5600.burnMaxAngleAndConfig();
  String retStr = "Brun max angle and config error: ";
  
  switch(burnResult)
  {
    case 1:
      retStr = "Brun max angle and config success";
      break;
    case -1:
      retStr += "chip has been burned once already";
      break;
    case -2:
      retStr += "max angle less than 18 degrees";
      break;
    default:
      retStr += "unknown";
      break;
  }
  return retStr;
}

// required to allow motors to move
// must be called when controller restarts and after any error
void exitSafeStart()
{
  smcSerial.write(0x83);
}
 
// speed should be a number from -3200 to 3200
void setMotorSpeed(int speed)
{
  if (speed < 0)
  {
    smcSerial.write(0x86);  // motor reverse command
    speed = -speed;  // make speed positive
  }
  else
  {
    smcSerial.write(0x85);  // motor forward command
  }
  smcSerial.write(speed & 0x1F);
  smcSerial.write(speed >> 5);
}

/*******************************************************
/* Function: loop
/* In: none
/* Out: none
/* Description: main program loop 
/*******************************************************/
boolean buttonPressed = false;
boolean startFlag = true;
boolean endFlag = false;



void loop()
{   
  long total =  cs_9_8.capacitiveSensor(30);
  int unlockVal = 400;
  int startVal = 150;

  if (total>200)
  {
    buttonPressed = true;
  }
  if (total<=200)
  {
    buttonPressed = false;
  }
  Serial.println(ams5600.getRawAngle());
  Serial.println (buttonPressed);
  
 if (buttonPressed && ams5600.getRawAngle()<unlockVal) //button pressed, in start position
 {
   if (startFlag)
   {
   setMotorSpeed(1600); // Set to 50% PWM speed
   }
   else setMotorSpeed(0);
 }
 else if (buttonPressed && ams5600.getRawAngle()>=unlockVal)
 {
   setMotorSpeed(0);
 }
 else if (!buttonPressed)
 {
   if (ams5600.getRawAngle()<startVal)
   {
     startFlag = true;
   }
   else startFlag = false;
   
   if (ams5600.getRawAngle()>unlockVal - 150)
   {
     endFlag = true;
   }
   
   if (endFlag)
   {
     if (ams5600.getRawAngle()<unlockVal + 250)
     {
       setMotorSpeed(1600);
     }
     else if (ams5600.getRawAngle()>unlockVal + 250)
     {
       setMotorSpeed(0);
       endFlag = false;
     }
   }
   else setMotorSpeed(0);
 }
  
  
}
