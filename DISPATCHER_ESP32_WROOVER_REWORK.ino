
#include <ESP8266_LCD_1602_RUS.h>
#include <Wire.h>
#include <map>
#include <iostream>
#include <vector>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "RTClib.h"
#include <EEPROM.h> // подключаем библиотеку EEPROM

#define SDA 13 //Define SDA pins
#define SCL 14 //Define SCL pins

LCD_1602_RUS lcd(0x27, 16, 2);

RTC_DS1307 rtc;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const char* ssid     = "yourssid";
const char* password = "yourpass";

bool isScrolling=true;
bool changeTime=false;
bool offBackLight=true;
bool timeIsOnScreen=false;
bool dataIsOnScreen=false;

const byte ledPins[] = {23, 21, 22}; //define red, green, blue led pins
const byte chns[] = {0, 1, 2}; //define the pwm channels

int red, green, blue;


int offtime;
int offbl;
int showtime;
int alert;


unsigned long previousMillisToOffBackLight = 0; 
long intervalToOffBackLight = offtime*1000;
unsigned long currentMillis;
int xyzPins[] = {5, 19, 18}; // 5 - взниз 19 - вверх 18 - ок

bool down, up, ok;

int multipip_ = 0;
int cursorRow = 0;

typedef void (*FunctionPointer)();

unsigned long previousMillisEeprom = 0; 
long intervalEeprom = 40000;
unsigned long currentMillisEeprom;

std::vector<String> board = {"ESP07_GARAJE", "NODEMCU_OUTDOOR", "Time", "Settings"};
std::vector<String> boardru = {"Гараж", "Улица",  "Время", "Настройки"};

std::vector<String> ESP07_GARAJE = {"temp:", "lux:","led1:","led2:", "back"};
std::vector<String> ESP07_GARAJEru = {"Темп-ра:", "Яркость:","Реле1:","Реле2:", "Назад"};

std::vector<String> Time_ = {"date:", "time:","sync time", "back"};
std::vector<String> Time_ru = {"Дата:", "Время:","Синхронизация", "Назад"};

std::vector<String> Settings = {"BL off:", "Show time:","Off time:","alert:", "back"};
std::vector<String> Settingsru = {"Подсветка:", "ОтобрВремя:","ТаймаутПод:","Сигнализация:", "Назад"};

std::vector<String> NODEMCU_OUTDOOR = {"temp:", "lux:","hum:","motion:", "rele1:", "rele2:", "multipip:", "back"};        //NODEMCU_OUTDOOR
std::vector<String> NODEMCU_OUTDOORru = {"Темп-ра:", "Яркость:","Влажность:","Движение:", "Гирлянда:", "Свет:", "Мультипип:", "Назад"};        //NODEMCU_OUTDOOR

std::map<String, std::vector<String>> menues;
std::map<String, std::map<String, String>> _variables;
std::map<String, std::map<String, String>> mqtt_commands;
std::map<String, std::map<String, FunctionPointer>> commands;
std::map<String, std::vector<String>> menuesru;

String MenuState = "board";
String select_ = ">";
String previosState = "board";
String selectedString = "ESP07_GARAJE";

const char* mqtt_server = "yourmqtt";
long mqtt_port = 15170;
const char* mqtt_user = "youruser";
const char* mqtt_password = "yourpass";
String checkTopicNow;
String msgFromCheckTopic = "";

String oldTimeChar;

void update_(bool reset_=true);
void printLCD(String text, int col, int row, bool isClean=true);




void setup() 
{
  Serial.begin(115200);
  EEPROM.begin(32);
  pinMode(xyzPins[0], INPUT_PULLUP); //z axis is a button.
  pinMode(xyzPins[1], INPUT_PULLUP); //z axis is a button.
  pinMode(xyzPins[2], INPUT_PULLUP); //z axis is a button.
  for (int i = 0; i < 3; i++) 
  { //setup the pwm channels,1KHz,8bit
    ledcSetup(chns[i], 1000, 8);
    ledcAttachPin(ledPins[i], chns[i]);
  }
  // EEPROM.put(10, offtime);
  // EEPROM.put(14, offbl);
  // EEPROM.put(18, showtime);
  // EEPROM.commit();
  delay(10);
  EEPROM.get(10, offtime);
  EEPROM.get(14, offbl);
  EEPROM.get(18, showtime);
  EEPROM.get(24, alert);
  intervalToOffBackLight = offtime*1000;
  delay(10);
  Serial.println(offtime);
  Wire.begin(SDA, SCL); // attach the IIC pin
  if (!i2CAddrTest(0x27)) 
  {
    lcd = LCD_1602_RUS(0x3F, 16, 2);
  }
  lcd.init(); // LCD driver initialization
  lcd.backlight(); // Open the backlight

//***********************************

  printLCD("Загрузка.", 0, 0, true);
  printLCD("Подготовка WI-FI", 0, 1, false);

//*******WI-FI_INIT******************

  Serial.print("Connecting to "); 
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  timeClient.setTimeOffset(7200);
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
//**********************************

  printLCD("Загрузка.", 0, 0, true);
  printLCD("WI-FI готов!", 0, 1, false);
  delay(200);

  printLCD("Загрузка..", 0, 0, true);
  printLCD("Считываем Время", 0, 1, false);

//*******TIME_INIT******************

  if (! rtc.begin()) {
    printLCD("RTC not found", 0, 1, false);
    while (1) delay(10);
  }
  delay(1000);

//**********************************

  printLCD("Загрузка..", 0, 0, true);
  printLCD("Время получено!", 0, 1, false);

  printLCD("Загрузка...", 0, 0, true);
  printLCD("Загружаем данные.", 0, 1, false);

//*********VARIABLES_INIT***********
  

  menues["board"] = board;
  menues["ESP07_GARAJE"] = ESP07_GARAJE;
  menues["Time"] = Time_;
  menues["NODEMCU_OUTDOOR"]=NODEMCU_OUTDOOR;
  menues["Settings"] = Settings;

  menuesru["board"] = boardru;
  menuesru["ESP07_GARAJE"] = ESP07_GARAJEru;
  menuesru["Time"] = Time_ru;
  menuesru["NODEMCU_OUTDOOR"]=NODEMCU_OUTDOORru;
  menuesru["Settings"] = Settingsru;
  

  _variables["ESP07_GARAJE"] = std::map<String, String>();
  // Заполняем внутренний std::map значениями
  _variables["ESP07_GARAJE"]["temp:"] = "5.0";
  _variables["ESP07_GARAJE"]["lux:"] = "46.0";
  _variables["ESP07_GARAJE"]["led1:"] = "OFF";
  _variables["ESP07_GARAJE"]["led2:"] = "OFF";
  // _variables["ESP07_GARAJE"]["multipip:"] = "0";
  // _variables["ESP07_GARAJE"]["zummer:"] = "0";
  _variables["ESP07_GARAJE"]["back"] = "";

  _variables["board"] = std::map<String, String>();
  // Заполняем внутренний std::map значениями
  _variables["board"]["ESP07_GARAJE"] = "";
  _variables["board"]["NODEMCU_OUTDOOR"] = "";
  _variables["board"]["Time"] = "";


  _variables["Time"] = std::map<String, String>();
  // Заполняем внутренний std::map значениями
  DateTime now = rtc.now();
  char buf1[] ="hh:mm:ss";
  String TimeChar = now.toString(buf1);
  char buf2[] ="DD-MM-YYYY";
  String DateChar = now.toString(buf2);
  oldTimeChar = TimeChar;
  Serial.println(now.minute());
  Serial.println(now.second());
  
  _variables["Time"]["date:"] = DateChar;
  _variables["Time"]["time:"] = TimeChar;
  _variables["Time"]["sync time"] = "";
  _variables["Time"]["back"] = "";

  _variables["Settings"]["BL off:"] = (offbl==1) ? "ON" :"OFF" ;   //BL off, Show time,Off time, back
  _variables["Settings"]["Show time:"] = (showtime==1) ? "ON" :"OFF";
  _variables["Settings"]["Off time:"] = String(offtime);
  _variables["Settings"]["alert:"] = (alert==1) ? "ON" :"OFF";;
  _variables["Settings"]["back"] = "";

  _variables["NODEMCU_OUTDOOR"]["temp:"]="0.0";
  _variables["NODEMCU_OUTDOOR"]["lux:"]="0.0";
  _variables["NODEMCU_OUTDOOR"]["hum:"]="0.0";
  _variables["NODEMCU_OUTDOOR"]["motion:"]="NOT";
  _variables["NODEMCU_OUTDOOR"]["rele1:"]="OFF";
  _variables["NODEMCU_OUTDOOR"]["rele2:"]="OFF";
  _variables["NODEMCU_OUTDOOR"]["multipip:"]="0";
  _variables["NODEMCU_OUTDOOR"]["back:"]="";


  mqtt_commands["ESP07_GARAJE"] = std::map<String, String>();
  mqtt_commands["ESP07_GARAJE"]["led1:"] = "ESP07_GARAJE/led1";
  mqtt_commands["ESP07_GARAJE"]["led2:"] = "ESP07_GARAJE/led2";
  mqtt_commands["NODEMCU_OUTDOOR"]["rele1:"] = "NODEMCU_OUTDOOR/rele1";
  mqtt_commands["NODEMCU_OUTDOOR"]["rele2:"] = "NODEMCU_OUTDOOR/rele2";
  mqtt_commands["NODEMCU_OUTDOOR"]["multipip:"] = "NODEMCU_OUTDOOR/multipip";
  
  commands["board"] = std::map<String,  FunctionPointer>();
  commands["board"]["ESP07_GARAJE"] = setESP07_GARAJE;
  commands["board"]["Time"] = setTime__;
  commands["board"]["Settings"] = setSettings;
  commands["board"]["NODEMCU_OUTDOOR"]=setNODEMCU_OUTDOOR;
  
  commands["ESP07_GARAJE"] = std::map<String, FunctionPointer>();
  commands["ESP07_GARAJE"]["led1:"] = setESP07_GARAJE_led1;
  commands["ESP07_GARAJE"]["led2:"] = setESP07_GARAJE_led2;
  commands["ESP07_GARAJE"]["back"] = goBack;

  commands["Time"] = std::map<String, FunctionPointer>();
  commands["Time"]["sync time"] = setSynctime; 
  commands["Time"]["back"] = goBack;

  commands["Settings"] = std::map<String, FunctionPointer>();
  commands["Settings"]["BL off:"] = setBloff;
  commands["Settings"]["Show time:"] = setShowtime;
  commands["Settings"]["Off time:"] = setOfftime;
  commands["Settings"]["alert:"] = setAlert;
  commands["Settings"]["back"] = goBack;
  
  commands["NODEMCU_OUTDOOR"] = std::map<String, FunctionPointer>();
  commands["NODEMCU_OUTDOOR"]["rele1:"] = setRele1;
  commands["NODEMCU_OUTDOOR"]["rele2:"] = setRele2;
  commands["NODEMCU_OUTDOOR"]["multipip:"] = setMultipip;
  commands["NODEMCU_OUTDOOR"]["back"] = goBack;

  reconnect();
//***********************************

  printLCD("Загрузка...", 0, 0, true);
  printLCD("Данные загружены!", 0, 1, false);

  delay(200);

  printLCD("Всё готово!", 0, 1, false);
  delay(400);
  setColor(0, 80, 0);

  update_();
  currentMillis = millis();
  previousMillisToOffBackLight = currentMillis;
}

void loop() 
{
  client.loop();
  TickEeprom();
  Scrolling_();
}


void printLCD(String text, int col, int row, bool isClean)
{
  if(isClean)
  {
    lcd.clear();
  }
  lcd.setCursor(col, row);
  lcd.print(text);
}

void update_(bool reset_)
{
 if (reset_)
  {
    cursorRow=0;
  }
  String firistVal=select_+menuesru[MenuState][cursorRow], secondVal=menuesru[MenuState][cursorRow+1];

  if(_variables[MenuState][menues[MenuState][cursorRow]] != "")
  {
    firistVal+=_variables[MenuState][menues[MenuState][cursorRow]];
  }
  if(_variables[MenuState][menues[MenuState][cursorRow+1]] != "")
  {
    secondVal+=_variables[MenuState][menues[MenuState][cursorRow+1]];
  }
  if (menues[MenuState][cursorRow] == "time:" || menues[MenuState][cursorRow+1] == "time:" || menues[MenuState][cursorRow] == "date:" || menues[MenuState][cursorRow+1] == "date:")
  {
    timeIsOnScreen=true;
  }
  else
  {
    timeIsOnScreen=false;
  }
  if(menues[MenuState][cursorRow+1] == "temp:" ||menues[MenuState][cursorRow+1] == "hum:" ||menues[MenuState][cursorRow+1] == "lux:" || menues[MenuState][cursorRow] == "temp:" ||menues[MenuState][cursorRow] == "hum:" ||menues[MenuState][cursorRow] == "lux:")
  {
    dataIsOnScreen=true;
  }
  else
  {
    dataIsOnScreen=false;
  }
  
  String rowsStringOnWrite[]={firistVal, secondVal};
  
  lcd.clear();
  for (int i = 0 ; i<2; i++)
  {
    String onWrite = rowsStringOnWrite[i];
    printLCD(onWrite, 0, i, false); // The print content is displayed on the LCD
  }  
  
}

void update_time()
{

    DateTime now = rtc.now();
    char buf1[] ="hh:mm:ss";
    String TimeString = now.toString(buf1);
    char buf2[] ="DD-MM-YYYY";
    String DateString = now.toString(buf2);
    oldTimeChar=TimeString;
    _variables["Time"]["date:"] = DateString;
    _variables["Time"]["time:"] = TimeString;
  
}

bool i2CAddrTest(uint8_t addr) 
{
  Wire.begin();
  Wire.beginTransmission(addr);
  if (Wire.endTransmission() == 0) 
  {
    return true;
  }
  return false;
}

void subTopic(const char* topic)
{
  client.subscribe(topic);
}

void setCheckTopic(String topic)
{
  msgFromCheckTopic = "";
  checkTopicNow = topic;
}

String getCurrentMsg()
{
  return msgFromCheckTopic;
}


void reconnect()
{
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_password)) 
    {
      Serial.println("connected");
    

      client.subscribe("ESP07_GARAJE/temp");
      client.subscribe("ESP07_GARAJE/lux");
      client.subscribe("ESP07_GARAJE/led1");
      client.subscribe("NODEMCU_OUTDOOR/temp");
      client.subscribe("NODEMCU_OUTDOOR/lux");
      client.subscribe("NODEMCU_OUTDOOR/hum");
      client.subscribe("NODEMCU_OUTDOOR/rele1");
      client.subscribe("NODEMCU_OUTDOOR/rele2");
      client.subscribe("NODEMCU_OUTDOOR/motion");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void publishData(const char *serialData, bool isretained = false)
{
  if (!client.connected()) {
    reconnect();
  }
  client.publish(mqtt_commands[MenuState][selectedString].c_str(), serialData, isretained);
}

void callback(char* topic, byte *payload, unsigned int length) 
{  
  Serial.write(payload, length);
  Serial.println();
  //Serial.write(topic);
  char* slashPosition = strchr(topic, '/');
  
  if (slashPosition != NULL) {
    int slashIndex = slashPosition - topic;
    
    String firstPart = String(topic).substring(0, slashIndex);
    String secondPart = String(topic).substring(slashIndex + 1);
    Serial.println(firstPart);
    Serial.println(secondPart+":");

    String strmsg = String (payload, length);
    Serial.println(strmsg);

    if(secondPart=="led1")
    {
       _variables[firstPart]["led1:"]=(strmsg=="1") ? "ON":"OFF";
    }
    else if(secondPart=="rele1")
    {
      _variables[firstPart]["rele1:"]=(strmsg=="1") ? "ON":"OFF";
    }

    else if(secondPart=="rele2")
    {
      _variables[firstPart]["rele2:"]=(strmsg=="1") ? "ON":"OFF";
    }

    else if(secondPart == "motion")
    {
      _variables[firstPart]["motion:"]=(strmsg=="1") ? "DETECTED!":"NOT";
      if(alert && _variables[firstPart]["motion:"]=="DETECTED!")
      {
        alertShow();
      }
      else if(_variables[firstPart]["motion:"]=="NOT")
      {
        setColor(0, 255, 0);
      }
    }

    else if (_variables.count(firstPart)>0 && _variables[firstPart].count(secondPart+":")>0)
    {
      Serial.println("OK");
      for (int i = 0; i < strmsg.length(); i++) 
      {
        if (strmsg.charAt(i) == ' ') {
          strmsg.remove(i, 1);
          i--; // уменьшаем счетчик, чтобы не пропустить следующий символ
        }
      }
      _variables[firstPart][secondPart+":"]=strmsg;
      if(MenuState==firstPart && timeIsOnScreen)
      {
        update_(false);
      }  
    } 
  }
}




void sendMultiPipVal()
{
  printLCD("Sended ", 0, 0, true);
  delay(1000);
  previousMillisToOffBackLight = currentMillis;
}

void _setTime()
{
  printLCD("Дата:"+_variables["Time"]["date:"], 0, 0);
  printLCD("Время:"+_variables["Time"]["time:"], 0, 1, false);
  delay(1);
}

void viewTime()
{
  while(true)
  {
    down = !digitalRead(xyzPins[0]);
    up = !digitalRead(xyzPins[1]);
    ok = !digitalRead(xyzPins[2]);
    currentMillis = millis();
    TickEeprom();
    client.loop();

    if(canUpdate()) {update_time();_setTime();};  
    delay(1);
    if(ok || down || up)
    {
      while(ok){ok = !digitalRead(xyzPins[2]);}
      break;
    }
    if (currentMillis - previousMillisToOffBackLight < intervalToOffBackLight || currentMillis - previousMillisToOffBackLight != intervalToOffBackLight) 
    {
      break;
      update_(false);
    }
  }
}

void changeRowsListDown(String vector)
{
  if(cursorRow==menues[MenuState].size()-1 && vector=="UP")
  {
    cursorRow=0; 
  }
  else if(vector=="UP")
  {
    cursorRow=cursorRow+1;  
  }

  if(cursorRow==0 && vector=="DOWN")
  {
    cursorRow=menues[MenuState].size()-1; 
  }
  else if(vector=="DOWN")
  {
    cursorRow=cursorRow-1;
  }
  
  lcd.clear();
  String firistVar, secondVar, firistString, secondString;
  if(cursorRow<menues[MenuState].size()-1)
  {
    if (cursorRow>0)
    {
      if (vector=="DOWN")
      {
       firistVar=menuesru[MenuState][cursorRow-1];
       secondVar=select_+menuesru[MenuState][cursorRow];

       firistString=menuesru[MenuState][cursorRow-1];
       secondString=menuesru[MenuState][cursorRow];

       if(_variables[MenuState][menues[MenuState][cursorRow-1]] != "")
       {
          firistVar+=_variables[MenuState][menues[MenuState][cursorRow-1]];
       }
       if(_variables[MenuState][menues[MenuState][cursorRow]] != "")
       {
          secondVar+=_variables[MenuState][menues[MenuState][cursorRow]];
       }
      }
      else
      {
       firistVar=select_+menuesru[MenuState][cursorRow];
       secondVar=menuesru[MenuState][cursorRow+1];
       
       firistString=menues[MenuState][cursorRow];
       secondString=menues[MenuState][cursorRow+1];
       if(_variables[MenuState][menues[MenuState][cursorRow]] != "")
       {
          firistVar+=_variables[MenuState][menues[MenuState][cursorRow]];
       }
       if(_variables[MenuState][menues[MenuState][cursorRow+1]] != "")
       {
          secondVar+=_variables[MenuState][menues[MenuState][cursorRow+1]];
       }
      }
    }
    else
    {
       firistVar=select_+menuesru[MenuState][cursorRow];
       secondVar=menuesru[MenuState][cursorRow+1];

       firistString=menues[MenuState][cursorRow];
       secondString=menues[MenuState][cursorRow+1];
       if(_variables[MenuState][menues[MenuState][cursorRow]] != "")
       {
          firistVar+=_variables[MenuState][menues[MenuState][cursorRow]];
       }
       if(_variables[MenuState][menues[MenuState][cursorRow+1]] != "")
       {
          secondVar+=_variables[MenuState][menues[MenuState][cursorRow+1]];
       }
    }
  } 
  else
  {
    firistVar=menuesru[MenuState][cursorRow-1];
    secondVar=select_+menuesru[MenuState][cursorRow];

    firistString=menuesru[MenuState][cursorRow-1];
    secondString=menuesru[MenuState][cursorRow];

    if(_variables[MenuState][menues[MenuState][cursorRow-1]] != "")
    {
      firistVar+=_variables[MenuState][menues[MenuState][cursorRow-1]];
    }
    if(_variables[MenuState][menues[MenuState][cursorRow]] != "")
    {
      secondVar+=_variables[MenuState][menues[MenuState][cursorRow]];
    }
  }

  if (firistString == "time:" || secondString == "time:" || firistString == "date:" || secondString == "date:")
  {
    timeIsOnScreen=true;
  }
  else
  {
    timeIsOnScreen=false;
  }

  String rowsStringOnWrite[] = {firistVar,  secondVar};
  Serial.println(rowsStringOnWrite[0]);
  Serial.println(rowsStringOnWrite[1]);
  Serial.println(cursorRow);
  selectedString =menues[MenuState][cursorRow];
  for (int i = 0 ; i<2; i++)
  {
    String onWrite = rowsStringOnWrite[i];
    lcd.setCursor(0, i); // Move the cursor to row 0, column i
    lcd.print(onWrite); // The print content is displayed on the LCD
  }  
  previousMillisToOffBackLight = currentMillis;
}
void Scrolling_()
{
  down = !digitalRead(xyzPins[0]);
  up = !digitalRead(xyzPins[1]);
  ok = !digitalRead(xyzPins[2]);
  currentMillis = millis();

  if(timeIsOnScreen)
  {
    if (canUpdate())
    {
      update_time();
      update_(false);
    }
    delay(1);    
  }

  /*if(dataIsOnScreen)
  {
    if (canUpdate())
    {
      update_time();
      update_(false);
    }
    delay(1);    
  }*/

  if (down)
    {
      changeRowsListDown("DOWN");
      delay(300);

    }
  else if (up)
    {
      changeRowsListDown("UP");
      delay(300);
    }

  if(ok)
  {
    
    while(ok){ok = !digitalRead(xyzPins[2]);}
    Serial.println("OK");
    if (commands[MenuState].count(menues[MenuState][cursorRow])>0)
    {
      commands[MenuState][menues[MenuState][cursorRow]]();
    }
    
    previousMillisToOffBackLight = currentMillis;
  }
  if (currentMillis - previousMillisToOffBackLight >= intervalToOffBackLight) 
  {
    if(offbl)
    {
      lcd.noBacklight();
    }
    if(showtime)
    {
      viewTime();
    }
  }
  else lcd.backlight();
}

bool canUpdate()
{
  DateTime now = rtc.now();
  char buf1[] ="hh:mm:ss";
  String TimeChar = now.toString(buf1);
  char buf2[] ="DD-MM-YYYY";
  String DateChar = now.toString(buf2);
  if (oldTimeChar!=TimeChar)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void updateEEPROM()
{
  int newOffbl, newShowtime, newOfftime, newAlert;
  EEPROM.get(10, newOfftime);
  EEPROM.get(14, newOffbl);
  EEPROM.get(18, newShowtime);
  EEPROM.get(24, newAlert);
  bool isUpdate = false;
  if(newOffbl!=offbl)
  {
    Serial.println("Update!");
    EEPROM.put(14, offbl);
    isUpdate = true;
  }
  if(newShowtime!=showtime)
  {
    Serial.println("Update!");
    EEPROM.put(18, showtime);
    isUpdate = true;
  }
  if(newOfftime!=offtime)
  {
    Serial.println("Update!");
    EEPROM.put(10, offtime);
    isUpdate = true;
  }

  if(newAlert!=alert)
  {
    Serial.println("Update!");
    EEPROM.put(24, alert);
    isUpdate = true;
  }

  if(isUpdate)
  {
    EEPROM.commit();
  }
  
}










void setESP07_GARAJE()
{
  previosState=MenuState;
  MenuState="ESP07_GARAJE";
  update_();
  delay(300);
}

void setTime__()
{
  previosState=MenuState;
  MenuState="Time";
  previousMillisToOffBackLight = currentMillis;
  update_();
}

void setESP07_GARAJE_led1()
{
  if(_variables[MenuState]["led1:"] == "OFF"){_variables[MenuState]["led1:"]="ON";}else{_variables[MenuState]["led1:"]="OFF";}
  
  String State_;
  if (_variables[MenuState][selectedString]=="ON")
  {
    State_="ON";
    publishData("1", true);
  }
  else
  {
    State_="OFF";
    publishData("0", true);
  }
  
  printLCD("Свет 1 "+State_, 0, 0, true);
  delay(1000);
  previousMillisToOffBackLight = currentMillis;
  update_(false);
  delay(300);
}

void goBack()
{
  MenuState=previosState;
  update_();
  delay(300);
}

void setSynctime()
{
  printLCD("Синхронизация", 0, 0, true);
  printLCD(".", 0, 1, false);

  timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();  // Get epoch time
  delay(1000);
  printLCD("..", 0, 1, false);
  rtc.adjust(DateTime(unix_epoch));  // Set RTC time using NTP epoch time
  update_time();
  printLCD("..Готово!", 0, 1, false);
  delay(3000);
  update_(true);
  delay(300);
}

void setBloff()
{
  _variables[MenuState][selectedString]=(offbl==0) ? "ON" :"OFF" ;
  String State_;
  if (_variables[MenuState][selectedString]=="ON")
  {
    State_="ON";
    offbl=1;
  }
  else
  {
    State_="OFF";
    offbl=0;
  }
  printLCD("Off bl "+State_, 0, 0, true);
  delay(1000);
  update_(false);
  delay(300);
}

void setShowtime()
{
  _variables[MenuState][selectedString]=(showtime==0) ? "ON" :"OFF" ;
  String State_;
  if (_variables[MenuState][selectedString]=="ON")
  {
    State_="ON";
    showtime=1;
  }
  else
  {
    State_="OFF";
    showtime=0;
  }
  
  printLCD("Show time "+State_, 0, 0, true);
  delay(1000);
  previousMillisToOffBackLight = currentMillis;
  update_(false);
  delay(300);
  previousMillisToOffBackLight = currentMillis;
}

void setAlert()
{
  _variables[MenuState][selectedString]=(alert==0) ? "ON" :"OFF" ;
  String State_;
  if (_variables[MenuState][selectedString]=="ON")
  {
    State_="ON";
    alert=1;
  }
  else
  {
    State_="OFF";
    alert=0;
  }
  
  printLCD("Alert "+State_, 0, 0, true);
  delay(1000);
  previousMillisToOffBackLight = currentMillis;
  update_(false);
  delay(300);
  previousMillisToOffBackLight = currentMillis;
}

void setOfftime()
{
  Serial.println("OK");
  isScrolling=false;
  while(!isScrolling)
  {
    down = !digitalRead(xyzPins[0]);
    up = !digitalRead(xyzPins[1]);
    ok = !digitalRead(xyzPins[2]);
    if(down && (_variables[MenuState]["Off time:"].toInt()<60))  //Костыль
    {
      Serial.println("down+");
      _variables[MenuState]["Off time:"]=String(_variables[MenuState]["Off time:"].toInt()+1);
      update_(false);
      delay(300);
      previousMillisToOffBackLight = currentMillis;
    }

    if(up &&(_variables[MenuState]["Off time:"].toInt()>0))   //kostil
    {
      Serial.println("down-");
      _variables[MenuState]["Off time:"]=String(_variables[MenuState]["Off time:"].toInt()-1);
      update_(false);
      delay(300);
      previousMillisToOffBackLight = currentMillis;
    }  

    if(ok)
    {
      Serial.println("OK");
      offtime=_variables[MenuState]["Off time:"].toInt();
      intervalToOffBackLight = offtime*1000;
      isScrolling=true;
      delay(300);
      previousMillisToOffBackLight = currentMillis;

    }
    currentMillis = millis();
    previousMillisToOffBackLight = currentMillis;
    delay(1);
  }
}
void setSettings()
{
  previosState=MenuState;
  MenuState="Settings";
  previousMillisToOffBackLight = currentMillis;
  update_();
}
void TickEeprom()
{
  currentMillisEeprom = millis();
  if (currentMillisEeprom - previousMillisEeprom >= intervalEeprom) 
  {
    updateEEPROM();
    previousMillisEeprom = currentMillis;
  }
}

void setESP07_GARAJE_led2()
{
  if(_variables[MenuState]["led2:"] == "OFF"){_variables[MenuState]["led2:"]="ON";}else{_variables[MenuState]["led2:"]="OFF";}
  
  String State_;
  if (_variables[MenuState][selectedString]=="ON")
  {
    State_="ON";
    publishData("1", true);
  }
  else
  {
    State_="OFF";
    publishData("0", true);
  }
  
  printLCD("Свет 2 "+State_, 0, 0, true);
  delay(1000);
  previousMillisToOffBackLight = currentMillis;
  update_(false);
  delay(300);
}

void setRele1()
{
  if(_variables[MenuState]["rele1:"] == "OFF"){_variables[MenuState]["rele1:"]="ON";}else{_variables[MenuState]["rele1:"]="OFF";}
  
  String State_;
  if (_variables[MenuState][selectedString]=="ON")
  {
    State_="ON";
    publishData("1", true);
  }
  else
  {
    State_="OFF";
    publishData("0", true);
  }
  
  printLCD("Гирлянда "+State_, 0, 0, true);
  delay(1000);
  previousMillisToOffBackLight = currentMillis;
  update_(false);
  delay(300);
}
void setNODEMCU_OUTDOOR()
{
  previosState=MenuState;
  MenuState="NODEMCU_OUTDOOR";
  previousMillisToOffBackLight = currentMillis;
  update_();
}
void alertShow()
{
  currentMillis = millis();
  lcd.backlight();
  previousMillisToOffBackLight = currentMillis;
  setColor(255, 0, 0);
}

void setColor(byte r, byte g, byte b) 
{
  ledcWrite(chns[0], 255 - r); //Common anode LED, low level to turn on the led.
  ledcWrite(chns[1], 255 - g);
  ledcWrite(chns[2], 255 - b);
}

void setRele2()
{
  if(_variables[MenuState]["rele2:"] == "OFF"){_variables[MenuState]["rele2:"]="ON";}else{_variables[MenuState]["rele2:"]="OFF";}
  
  String State_;
  if (_variables[MenuState][selectedString]=="ON")
  {
    State_="ON";
    publishData("1", true);
  }
  else
  {
    State_="OFF";
    publishData("0", true);
  }
  
  printLCD("Свет "+State_, 0, 0, true);
  delay(1000);
  previousMillisToOffBackLight = currentMillis;
  update_(false);
  delay(300);
}

void setMultipip()
{
  isScrolling=false;
  while(!isScrolling)
  {
    down = !digitalRead(xyzPins[0]);
    up = !digitalRead(xyzPins[1]);
    ok = !digitalRead(xyzPins[2]);
    if(down && (_variables[MenuState]["multipip:"].toInt()<11))  //Костыль
    {
      Serial.println("down+");
      _variables[MenuState]["multipip:"]=String(_variables[MenuState]["multipip:"].toInt()+1);
      update_(false);
      delay(300);
      previousMillisToOffBackLight = currentMillis;
    }

    if(up &&(_variables[MenuState]["multipip:"].toInt()>0))   //kostil
    {
      Serial.println("down-");
      _variables[MenuState]["multipip:"]=String(_variables[MenuState]["multipip:"].toInt()-1);
      update_(false);
      delay(300);
      previousMillisToOffBackLight = currentMillis;
    }  

    if(ok)
    {
      Serial.println("OK");
      intervalToOffBackLight = offtime*1000;
      isScrolling=true;
      delay(300);
      previousMillisToOffBackLight = currentMillis;

    }

    currentMillis = millis();
    previousMillisToOffBackLight = currentMillis;
    delay(1);
  }
  publishData(_variables[MenuState]["multipip:"].c_str());
  _variables[MenuState]["multipip:"]="0";
  update_(false);
}
