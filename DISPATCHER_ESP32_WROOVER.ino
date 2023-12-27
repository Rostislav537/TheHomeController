#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "RTClib.h"

#include "base.h"


void setup() 
{
 
  Serial.begin(115200);
  pinMode(xyzPins[0], INPUT_PULLUP); //z axis is a button.
  pinMode(xyzPins[1], INPUT_PULLUP); //z axis is a button.
  pinMode(xyzPins[2], INPUT_PULLUP); //z axis is a button.
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading..");
  lcd.setCursor(0, 1);
  lcd.print("Setup time");
  delay(400);
  setupTime();
  delay(400);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading..");
  lcd.setCursor(0, 1);
  lcd.print("Time setuped");
  delay(400);
  setupLcd();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading");
  delay(1200);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading.");
  lcd.setCursor(0, 1);
  lcd.print("Setup WiFi");
  setupWiFi();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading.");
  lcd.setCursor(0, 1);
  lcd.print("WiFi Setuped");
  delay(400);


  stupVaraibles();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading..");
  lcd.setCursor(0, 1);
  lcd.print("Variables done");

  delay(200);

  lcd.setCursor(0, 1);
  lcd.print("All is done!");
  delay(400);

  update_();

}

void loop() 
{
  client.loop();
  Scrolling_();
}

