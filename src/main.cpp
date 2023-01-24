// Author       : Firdaus Nuur Rhamadhan
// Project      : Bitanic 2.0
// Code Version : 1.0
// Last Update  :
// Description  : Development process of Bitanic 2.0

#include <Arduino.h>
#include <string.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// Panel Declaration
String project = "Bitanic 2.0";
String id = "BT05";
String clientID = "bitanicv2 " + id;

// WiFi Setting
const char *ssid = "Setting";
const char *password = "admin1234";
const char *mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char *mqttUser = "";
const char *mqttPassword = "";

WiFiManager wm;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// Publis Topic
String publisTopic = "bitanic";
// Subscribe Topic
String subscribeTopic = "bitanic/" + id;

// EEPROM Address
int SundayEEPROM = 210;
int MondayEEPROM = 220;
int TuesdayEEPROM = 230;
int WednesdayEEPROM = 240;
int ThursdayEEPROM = 250;
int FridayEEPROM = 260;
int SaturdayEEPROM = 270;
int JumlahMingguEEPROM = 200;
int Minggu0EEPROM = 205;
int SetOnTime1EEPROM = 100;
int SetOnTime1MinuteEEPROM = 110;

// Pin Declaration
#define relay1 25
#define relay2 26
#define soilMoisture1 13
#define soilMoisture2 14
#define DHTPIN 15
#define Buzzer 12

// Variable Declaration
int soilMoisture1Value = 0;
int soilMoisture2Value = 0;
String soilMoisture1Status = "";
String soilMoisture2Status = "";
int motor1Value = 0;
int motor2Value = 0;
String motor1Status = "";
String motor2Status = "";
float humidity = 0;
float temperature = 0;
float heatIndex = 0;
String timeNow = "";
String dateNow = "";
String JadwalHari = "";
// int countEND = 0; // untuk menghitung jumlah END yang ada di EEPROM

// LiquidCrystal_I2C Declaration
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

// DHT Declaration
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

int Detik, PortValue, countWifi, count2, State1, GlobalCount, h1, h2, LockTime, CountTime, CountMotor, TimeMotor, CountMinggu, TotalMinggu, OnDay, SaveOn, CountDayOn, countEND;
float t1, t2;
unsigned long GlobalClock;
String DATA[10], INPUTDATA, TimeNOW, DateNOW;
bool Lock1, Lock2, Lock3, Lock4, LockDate, MotorLock, LockMQTT;
// String ssid, password;

// RTC Declaration
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Class untuk mengeksekusi program secara multiprocessing
class Task
{
public:
  Task(unsigned long interval, void (*callback)())
  {
    this->interval = interval;
    this->callback = callback;
    this->nextRun = millis() + interval;
  }

  void update()
  {
    if (millis() >= nextRun)
    {
      nextRun = millis() + interval;
      callback();
    }
  }

private:
  unsigned long interval;
  unsigned long nextRun;
  void (*callback)();
};

void buzz(int delayTime, int repeat)
{
  for (int i = 0; i < repeat; i++)
  {
    tone(Buzzer, 2000);
    delay(delayTime);
    tone(Buzzer, 0);
    delay(delayTime);
  }
}

void lcdPrint(String text1, String text2)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(text1);
  lcd.setCursor(0, 1);
  lcd.print(text2);
}

void reconnect()
{
  while (!client.connected())
  {
    if (client.connect(clientID.c_str()))
    {
      client.subscribe(subscribeTopic.c_str());
    }
    else
    {
      delay(1000);
    }
  }
}

void sendData(const char *topic, const char *payload)
{
  client.publish(topic, payload);
  Serial.println("Sent: " + String(payload));
}

String getNumber(String str, int index)
{
  for (int i = 0; i < index; i++)
  {
    int commaIndex = str.indexOf(',');
    if (commaIndex == -1)
    { // jika tidak ditemukan koma
      return "";
    }
    str = str.substring(commaIndex + 1);
  }
  int commaIndex = str.indexOf(',');
  if (commaIndex == -1)
  {
    return str;
  }
  return str.substring(0, commaIndex);
}

void sendMotorStatus(String id, int motor1Value, int motor2Value, String publishTopic)
{
  DynamicJsonDocument doc(1024);
  doc["ID"] = id;
  doc["MOTOR"]["STATUS_MOTOR1"] = motor1Value;
  doc["MOTOR"]["STATUS_MOTOR2"] = motor2Value;
  String statusMotorData;
  serializeJson(doc, statusMotorData);
  sendData(publishTopic.c_str(), statusMotorData.c_str());
}
void callbackResponse(String topic, String payload){
  if (String(topic) == subscribeTopic)
  {
    String command = payload.substring(0, payload.indexOf(","));
    if (command == id){
      DynamicJsonDocument doc(1024);
      doc["ID"] = id;
      doc["STATUS_AKTIF"] = 1;
      String statusData;
      serializeJson(doc, statusData);
      sendData(publisTopic.c_str(), statusData.c_str());
    }
    else if (command == "MOTOR1"){
      String status = getNumber(payload.c_str(), 1);
      Serial.println("Status MOTOR1 : " + status);
      if (status == "1")
      {
        digitalWrite(relay1, HIGH);
        motor1Value = digitalRead(relay1);
        sendMotorStatus(id, motor1Value, motor2Value, publisTopic);
        lcdPrint("Motor 1", "ON");
        buzz(1500, 3);
      }
      else if (status == "0")
      {
        digitalWrite(relay1, LOW);
        motor1Value = digitalRead(relay1);
        sendMotorStatus(id, motor1Value, motor2Value, publisTopic);
        lcdPrint("Motor 1", "OFF");
        buzz(1500, 3);
      }
    }
    else if (command == "MOTOR2"){
      String status = getNumber(payload.c_str(), 1);
      Serial.println("Status MOTOR2 : " + status);
      if (status == "1")
      {
        digitalWrite(relay2, HIGH);
        motor2Value = digitalRead(relay2);
        sendMotorStatus(id, motor1Value, motor2Value, publisTopic);
        lcdPrint("Motor 2", "ON");
        buzz(1500, 3);
      }
      else if (status == "0")
      {
        digitalWrite(relay2, LOW);
        motor2Value = digitalRead(relay2);
        sendMotorStatus(id, motor1Value, motor2Value, publisTopic);
        lcdPrint("Motor 2", "ON");
        buzz(1500, 3);
      }
    }
    else if (command == "SETHARI")
    {
      String TT;
      if (getNumber(payload, 1) == "1")
      {
        EEPROM.writeString(SundayEEPROM, "Sunday");
        TT = "1";
      }
      else
      {
        EEPROM.writeString(SundayEEPROM, "NONE");
        TT = "0";
      }
      if (getNumber(payload, 2) == "1")
      {
        EEPROM.writeString(MondayEEPROM, "Monday");
        TT += "1";
      }
      else
      {
        EEPROM.writeString(MondayEEPROM, "NONE");
        TT += "0";
      }
      if (getNumber(payload, 3) == "1")
      {
        EEPROM.writeString(TuesdayEEPROM, "Tuesday");
        TT += "1";
      }
      else
      {
        EEPROM.writeString(TuesdayEEPROM, "NONE");
        TT += "0";
      }
      if (getNumber(payload, 4) == "1")
      {
        EEPROM.writeString(WednesdayEEPROM, "Wednesday");
        TT += "1";
      }
      else
      {
        EEPROM.writeString(WednesdayEEPROM, "NONE");
        TT += "0";
      }
      if (getNumber(payload, 5) == "1")
      {
        EEPROM.writeString(ThursdayEEPROM, "Thursday");
        TT += "1";
      }
      else
      {
        EEPROM.writeString(ThursdayEEPROM, "NONE");
        TT += "0";
      }
      if (getNumber(payload, 6) == "1")
      {
        EEPROM.writeString(FridayEEPROM, "Friday");
        TT += "1";
      }
      else
      {
        EEPROM.writeString(FridayEEPROM, "NONE");
        TT += "0";
      }
      if (getNumber(payload, 7) == "1")
      {
        EEPROM.writeString(SaturdayEEPROM, "Saturday");
        TT += "1";
      }
      else
      {
        EEPROM.writeString(SaturdayEEPROM, "NONE");
        TT += "0";
      }
      EEPROM.writeString(300, TT);
      Serial.println("SETHARI OK");
      EEPROM.commit();
      lcdPrint("SETHARI OK", "Commit");
      buzz(500, 3);
    }
    else if (command == "SETMINGGU")
    {
      countEND = 0;
      EEPROM.writeString(JumlahMingguEEPROM, getNumber(payload, 1));
      EEPROM.writeString(Minggu0EEPROM, "0");
      Serial.println("SETMINGGU OK");
      EEPROM.commit();
      lcdPrint("SETMINGGU OK", "Commit");
      buzz(500, 3);
    }
    else if (command == "SETONTIME1"){
      EEPROM.writeString(SetOnTime1EEPROM, getNumber(payload, 1));
      EEPROM.writeString(SetOnTime1MinuteEEPROM, getNumber(payload, 2));
      Serial.println("SETONTIME1 OK");
      EEPROM.commit();
      lcdPrint("SETONTIME OK", "Commit");
      buzz(500, 3);
    }
    else if (command == "GETDATA"){
      Serial.println(EEPROM.readString(SundayEEPROM));
      Serial.println(EEPROM.readString(MondayEEPROM));
      Serial.println(EEPROM.readString(TuesdayEEPROM));
      Serial.println(EEPROM.readString(WednesdayEEPROM));
      Serial.println(EEPROM.readString(ThursdayEEPROM));
      Serial.println(EEPROM.readString(FridayEEPROM));
      Serial.println(EEPROM.readString(SaturdayEEPROM));
      Serial.println(EEPROM.readString(JumlahMingguEEPROM));
      Serial.println(EEPROM.readString(Minggu0EEPROM));
      Serial.println(EEPROM.readString(SetOnTime1EEPROM));
      Serial.println(EEPROM.readString(SetOnTime1MinuteEEPROM));
    }
  }
}
void callback(char *topic, byte *payload, unsigned int length)
{
  String message;
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  Serial.println("Message arrived on topic: " + String(topic));
  Serial.println("Received: " + message);
  callbackResponse(topic, message);
}

void setup()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  Serial.begin(9600);
  EEPROM.begin(512);

  // LCD Initialization
  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcdPrint("Bitaniv V2.0", id);
  buzz(100, 2);
  buzz(500, 1);
  String wifiAP = "BitanicV2 " + id;
  lcdPrint(wifiAP.c_str(), "192.162.4.1");
  // WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(1000);
  //   Serial.println("Connecting to WiFi...");
  // }
  // Serial.println("Connected to WiFi");

  bool res;
  res = wm.autoConnect(wifiAP.c_str()); // password protected ap
  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    lcdPrint("WiFi", "Connected");
    buzz(100, 4);
    buzz(700, 1);
  }

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  adcAttachPin(soilMoisture1);
  adcAttachPin(soilMoisture2);
  // pinMode declaration
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(soilMoisture1, INPUT);
  pinMode(soilMoisture2, INPUT);

  // DHT Initialization
  dht.begin();

// RTC Initialization
#ifndef ESP8266
  while (!Serial)
    ; // wait for serial port to connect. Needed for native USB
#endif
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1)
      delay(10);
  }
  if (rtc.lostPower())
  {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

// task untuk mencetak hello world pada serial monitor per 10 detik sekali
Task dataUpdate(1000, [](){
  DateTime now = rtc.now(); // get the current time
  // = = = = = = = = = = = = = variable feed = = = = = = = = = = = = =
  soilMoisture1Value = digitalRead(soilMoisture1); // read soil moisture 1
  delay(500);
  soilMoisture2Value = digitalRead(soilMoisture2); // read soil moisture 2
  soilMoisture1Status = (soilMoisture1Value == 0) ? "Basah" : "Kering";
  soilMoisture2Status = (soilMoisture2Value == 0) ? "Basah" : "Kering";
  motor1Status = (motor1Value == 1) ? "HIDUP" : "MATI";
  motor2Status = (motor2Value == 1) ? "HIDUP" : "MATI";

  motor1Value = digitalRead(relay1);
  motor2Value = digitalRead(relay2);
  // = = = = = = = = = = = = = DHT11 = = = = = = = = = = = = =

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  heatIndex = dht.computeHeatIndex(temperature, humidity, false);
  timeNow = String(now.hour(), DEC) + ":" + String(now.minute(), DEC) + ":" + String(now.second(), DEC);
  dateNow = String(now.day(), DEC) + "/" + String(now.month(), DEC) + "/" + String(now.year(), DEC);

  // // HIDUP MATIKAN POMPA dari Sensor
  // if (soilMoisture1Value == 1){
  //   digitalWrite(relay1, HIGH);
  //   sendMotorStatus(id, motor1Value, motor2Value, publisTopic);
  // }
  // else if (soilMoisture1Value == 0){
  //   digitalWrite(relay1, LOW);
  //   sendMotorStatus(id, motor1Value, motor2Value, publisTopic);
  // }
  // if (soilMoisture2Value == 1){
  //   digitalWrite(relay2, HIGH);
  //   sendMotorStatus(id, motor1Value, motor2Value, publisTopic);
  // }
  // else if (soilMoisture2Value == 0){
  //   digitalWrite(relay2, LOW);
  // }
});

int lcdMilis = 0;
int lcdQueue = 0;
Task lcdUpdate(1500, [](){
  // = = = = = = = = = = = = = LCD Print = = = = = = = = = = = = =
  // Start Milis for print LCD every 1000ms
  if (millis() - lcdMilis >= 1500)
  {
    lcdMilis = millis();
    if (lcdQueue == 0)
    {
      lcdPrint(timeNow, dateNow);
      lcdQueue = 1;
    }
    else if (lcdQueue == 1)
    {
      lcdPrint("Soil 1 : " + soilMoisture1Status, "Soil 2 : " + soilMoisture2Status);
      lcdQueue = 2;
    }
    else if (lcdQueue == 2)
    {
      lcdPrint("Motor 1 : " + motor1Status, "Motor 2 : " + motor2Status);
      lcdQueue = 3;
    }
    else if (lcdQueue == 3)
    {
      lcdPrint("T : " + String(temperature) + " C", "H : " + String(humidity) + " %");
      lcdQueue = 0;
    }
  }
  // End Milis for print LCD every 1000ms
});

Task mqttUpdate(30000, [](){
  // Kirim data JSON ke MQTT
  DynamicJsonDocument doc(1024);
  doc["ID"] = id;
  doc["TELEMETRI"]["soil1"] = soilMoisture1Value;
  doc["TELEMETRI"]["soil2"] = soilMoisture2Value;
  doc["TELEMETRI"]["motor1"] = motor1Value;
  doc["TELEMETRI"]["motor2"] = motor2Value;
  doc["TELEMETRI"]["temperature"] = temperature;
  doc["TELEMETRI"]["humidity"] = humidity;
  doc["TELEMETRI"]["heatIndex"] = heatIndex;
  doc["TELEMETRI"]["time"] = timeNow;
  doc["TELEMETRI"]["date"] = dateNow;
  String telemetriData;
  serializeJson(doc, telemetriData);
  sendData(publisTopic.c_str(), telemetriData.c_str()); 
});

Task serialUpdate(2000, [](){
  // = = = = = = = = = = = = = Serial Print = = = = = = = = = = = = =
  Serial.println("ID              : " + id);
  Serial.println("Soil Moisture 1 : " + String(soilMoisture1Value));
  Serial.println("Soil Moisture 2 : " + String(soilMoisture2Value));
  Serial.println("Motor 1         : " + String(motor1Value));
  Serial.println("Motor 2         : " + String(motor2Value));
  Serial.println("Suhu            : " + String(temperature) + " C");
  Serial.println("Kelembaban      : " + String(humidity) + " %");
  Serial.println("Indeks Panas    : " + String(heatIndex) + " C");
  Serial.println();
  Serial.println(subscribeTopic.c_str());
  Serial.println(publisTopic.c_str());
  Serial.println();
  // baca eeeprom 0 - 6 untuk mendapatkann nnilah status hari  
  Serial.println(); 
});

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  dataUpdate.update();
  lcdUpdate.update();
  mqttUpdate.update();
  // serialUpdate.update();
  OnDay = 0;
  SaveOn = EEPROM.readString(280).toInt();
  if (EEPROM.readString(100) != "00:00:00") {
    OnDay++;
  } if (EEPROM.readString(120) != "00:00:00") {
    OnDay++;
  } if (EEPROM.readString(140) != "00:00:00") {
    OnDay++;
  } if (EEPROM.readString(160) != "00:00:00") {
    OnDay++;
  } if (EEPROM.readString(180) != "00:00:00") {
    OnDay++;
  }
  DateTime now = rtc.now();
  if (now.minute() % 60 == 0) {
    Serial.println("[580] "+String(now.minute()% 60)+" == 0");
    // display.dim(true);
  }
  if (now.minute() % 60 == 1) {
    Serial.println("[584] "+String(now.minute()% 60)+" == 1");
    // display.dim(false);
  }
  if (now.second() != GlobalClock) {
    CountDayOn = 0;
    //    GlobalCount++;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[591] WiFi.status() != WL_CONNECTED");
      Lock1 = false;
      State1 = 0;
    }
    if (!client.connected()) {
      Lock1 = false;
      State1 = 4;
    }
    if (Lock1 == false) {
      if (digitalRead(relay1) != HIGH || digitalRead(relay2) != HIGH) {
        // digitalWrite(LED, LOW);
        Serial.println("[601] digitalWrite(LED, LOW);");
        Detik++;
        if (Detik == 5) {
          State1 = 5;
          // ESP.restart();
          Serial.println("[606] ESP.restart();");
        }
      }
    }
    if (EEPROM.readString(210) == "Sunday") {
      CountDayOn++;
    } if (EEPROM.readString(220) == "Monday") {
      CountDayOn++;
    } if (EEPROM.readString(230) == "Tuesday") {
      CountDayOn++;
    } if (EEPROM.readString(240) == "Wednesday") {
      CountDayOn++;
    } if (EEPROM.readString(250) == "Thursday") {
      CountDayOn++;
    } if (EEPROM.readString(260) == "Friday") {
      CountDayOn++;
    } if (EEPROM.readString(270) == "Saturday") {
      CountDayOn++;
    }
    if (EEPROM.readString(200) != "END") {
      if (EEPROM.readString(310).toInt() == CountDayOn) {
        if(EEPROM.readString(200) != "0"){
        int YY = EEPROM.readString(205).toInt();
        YY++;
        EEPROM.writeString(205, String() + YY);
        }
        EEPROM.writeString(320, "0000000");
        EEPROM.writeString(310, "0");
      }
      if (LockTime > 0 && Lock4 == false) {
        digitalWrite(relay1, HIGH);
        digitalWrite(relay2, HIGH);
        int KK = EEPROM.readString(280).toInt();
        KK++;
        EEPROM.writeString(280, String() + KK);
        if (EEPROM.readString(280).toInt() == OnDay) {
          EEPROM.writeString(280, "0   ");
          KK = EEPROM.readString(310).toInt();
          KK++;
          EEPROM.writeString(310, String() + KK);
          int LL = 320 + now.dayOfTheWeek();
          EEPROM.writeChar(LL, '1');
        }
        // Send();
        Serial.println("[650] Send();");
        Lock4 = true;
      }
    }
    if (now.hour() == 12) {
      int LL = 320 + now.dayOfTheWeek();
      EEPROM.writeChar(LL, '1');
    }
    if (EEPROM.readString(205) == EEPROM.readString(200) && EEPROM.readString(200) != 0) {
      EEPROM.writeString(200, "END");
      EEPROM.writeString(205, "0");
    }
    if (Lock4 == true) {
      TimeMotor++;
    }
    if (TimeMotor >= CountMotor * 60 && Lock4 == true) {
      digitalWrite(relay1, LOW);
      digitalWrite(relay2, LOW);
      // Send();
      Serial.println("[669] Send();");
      Lock4 = false;
      LockTime = 0;
      TimeMotor = 0;
    }
    // display.clearDisplay();
    // display.setTextSize(1);
    // display.setTextColor(SSD1306_WHITE);
    Serial.println("[677] display.clearDisplay();");
    lcdPrint("Display", "Clear");
    String Load = String() + daysOfTheWeek[now.dayOfTheWeek()] + ", ";
    if (now.day() < 10) {
      Load += String() + "0" + now.day();
    } else {
      Load += now.day(), DEC;
    }
    Load += "/";
    if (now.month() < 10) {
      Load += String() + "0" + now.month();
    } else {
      Load += now.month(), DEC;
    }
    Load += String() + "/" + now.year();
    if (Load.length() >= 21) {
      // display.setCursor(0, 0);
      Serial.println("[694] display.setCursor(0, 0);");
    } else if (Load.length() == 20) {
      // display.setCursor(3, 0);
      Serial.println("[697] display.setCursor(3, 0);");
    } else if (Load.length() == 19) {
      // display.setCursor(5, 0);
      Serial.println("[700] display.setCursor(5, 0);");
    } else if (Load.length() == 18) {
      // display.setCursor(8, 0);
      Serial.println("[703] display.setCursor(8, 0);");
    }
    Load += "\n      ";
    if (now.hour() < 10) {
      Load += String() + "0" + now.hour();
    } else {
      Load += now.hour(), DEC;
    }
    Load += ":";
    if (now.minute() < 10) {
      Load += String() + "0" + now.minute();
    } else {
      Load += now.minute(), DEC;
    }
    Load += ":";
    if (now.second() < 10) {
      Load += String() + "0" + now.second();
    } else {
      Load += now.second(), DEC;
    }
    //    Load += "\n\nDHT1   :\nDHT2   :\nMOTOR1 :\nMOTOR2 :\n   WIFI CONNECTED\nA";
    if (now.second() % 10 <= 4) {
      h1 = 123;
      t1 = 123;
      h2 = 456;
      t2 = 456;

      if (isnan(h1) || isnan(t1)) {
        Serial.println(F("Failed to read from DHT sensor 1!"));
        Load += "\n\nDHT1   :NOT CONNECT\n";
      } else {
        Load += String() + "\n\nDHT1  :" + t1 + "'C " + h1 + "%\n";
      }
      if (isnan(h2) || isnan(t2)) {
        Serial.println(F("Failed to read from DHT sensor 2!"));
        Load += "DHT2   :NOT CONNECT\n";
      } else {
        Load += String() + "DHT2  :" + t2 + "'C " + h2 + "%\n";
      }
      if (digitalRead(relay1) == 1) {
        Load += String() + "RELAY1:NYALA\n";
      } else {
        Load += String() + "RELAY1:MATI\n";
      }
      if (digitalRead(relay2) == 1) {
        Load += String() + "RELAY2:NYALA\n";
      } else {
        Load += String() + "RELAY2:MATI\n";
      }
      if (State1 == 0) {
        Load += "  WIFI NOT CONNECTED";
      } else if (State1 == 2) {
        Load += "   WIFI CONNECTED";
      } else if (State1 == 3) {
        Load += "    CONNECTION OK";
      } else if (State1 == 4) {
        Load += "    NO CONNECTION";
      } else if (State1 == 5) {
        Load += "    RESTARTING...";
        // display.println(Load);
        Serial.println("[763] display.println(Load);");
        // display.display();
        lcdPrint("R E S T A R T", "1 1 1 1 1 1 ");
        delay(1000);
        // ESP.restart();
        Serial.println("[768] ESP.restart();");
      }
    } else {
      Load += "\n\nDEV :NF01";
      Load += String() + "\nSSID:" + ssid;
      Load += "\nSERV:broker.hivemq";
      Load += "\nPORT:1883\n";
      if (State1 == 0) {
        Load += "  WIFI NOT CONNECTED";
      } else if (State1 == 2) {
        Load += "   WIFI CONNECTED";
      } else if (State1 == 3) {
        Load += "    CONNECTION OK";
      } else if (State1 == 4) {
        Load += "    NO CONNECTION";
      } else if (State1 == 5) {
        Load += "    RESTARTING...";
        // display.println(Load);
        // display.display();
        lcdPrint("R E S T A R T", "2 2 2 2 22 ");
        delay(1000);
        // ESP.restart();
        Serial.println("[790] ESP.restart();");
      }
    }
    //    Serial.println(Load);
    // display.println(Load);
    // display.display();
    lcdPrint(Load.substring(0, 16), Load.substring(16, 32));
    client.loop();
    GlobalClock = now.second();
  }
  if (digitalRead(relay1) == HIGH || digitalRead(relay2) == HIGH) {
    MotorLock = true;
  } else {
    MotorLock = false;
  }
  if (now.hour() < 10) {
    TimeNOW = String() + "0" + now.hour();
  } else {
    TimeNOW = now.hour(), DEC;
  }
  TimeNOW += ":";
  if (now.minute() < 10) {
    TimeNOW += String() + "0" + now.minute();
  } else {
    TimeNOW += now.minute(), DEC;
  }
  TimeNOW += ":";
  if (now.second() < 10) {
    TimeNOW += String() + "0" + now.second();
  } else {
    TimeNOW += now.second(), DEC;
  }
  if (EEPROM.readString(200) != "END") {
    TotalMinggu = EEPROM.readString(200).toInt();
    DateNOW = daysOfTheWeek[now.dayOfTheWeek()];
    if (DateNOW == EEPROM.readString(210) || DateNOW == EEPROM.readString(220) || DateNOW == EEPROM.readString(230) || DateNOW == EEPROM.readString(240) || DateNOW == EEPROM.readString(250) || DateNOW == EEPROM.readString(260) || DateNOW == EEPROM.readString(270)) {
      LockDate = true;
    } else {
      LockDate = false;
    }
    if (LockDate == true && CountMinggu <= TotalMinggu && EEPROM.readString(200).toInt() > 0) {
      if (TimeNOW == EEPROM.readString(100) && EEPROM.readString(100) != "00:00:00") {
        LockTime = 1;
      } else if (TimeNOW == EEPROM.readString(120) && EEPROM.readString(120) != "00:00:00") {
        LockTime = 2;
      } else if (TimeNOW == EEPROM.readString(140) && EEPROM.readString(140) != "00:00:00") {
        LockTime = 3;
      } else if (TimeNOW == EEPROM.readString(160) && EEPROM.readString(160) != "00:00:00") {
        LockTime = 4;
      } else if (TimeNOW == EEPROM.readString(180) && EEPROM.readString(180) != "00:00:00") {
        LockTime = 5;
      }
      if (LockTime == 1) {
        CountMotor = EEPROM.readString(110).toInt();
      } else if (LockTime == 2) {
        CountMotor = EEPROM.readString(130).toInt();
      } else if (LockTime == 3) {
        CountMotor = EEPROM.readString(150).toInt();
      } else if (LockTime == 4) {
        CountMotor = EEPROM.readString(170).toInt();
      } else if (LockTime == 5) {
        CountMotor = EEPROM.readString(190).toInt();
      }
    }
    countEND = 0;
  }
  if (EEPROM.readString(200) == "END") {
    EEPROM.writeString(200, "END");
    EEPROM.writeString(205, "0");
    EEPROM.writeString(280, "0");
    EEPROM.writeString(310, "0");
    EEPROM.writeString(320, "0000000");
  }
  //  Serial.println(String() + "DATA SET ON 1: " + EEPROM.readString(100) + " MINUTE : " + EEPROM.readString(110));
  //  Serial.println(String() + "DATA SET ON 2: " + EEPROM.readString(120) + " MINUTE : " + EEPROM.readString(130));
  //  Serial.println(String() + "DATA SET ON 3: " + EEPROM.readString(140) + " MINUTE : " + EEPROM.readString(150));
  //  Serial.println(String() + "DATA SET ON 4: " + EEPROM.readString(160) + " MINUTE : " + EEPROM.readString(170));
  //  Serial.println(String() + "DATA SET ON 5: " + EEPROM.readString(180) + " MINUTE : " + EEPROM.readString(190));
  //  Serial.println(String() + "DATA MINGGU : " + EEPROM.readString(200));
  //  Serial.println(String() + "DATA CURRENT MINGGU: " + EEPROM.readString(205));
  //  Serial.println(String() + "DATA MINGGU: " + EEPROM.readString(210));
  //  Serial.println(String() + "DATA SENIN: " + EEPROM.readString(220));
  //  Serial.println(String() + "DATA SELASA: " + EEPROM.readString(230));
  //  Serial.println(String() + "DATA RABU: " + EEPROM.readString(240));
  //  Serial.println(String() + "DATA KAMIS: " + EEPROM.readString(250));
  //  Serial.println(String() + "DATA JUMAT: " + EEPROM.readString(260));
  //  Serial.println(String() + "DATA SABTU: " + EEPROM.readString(270));
  //  Serial.println(String() + "ONDAY: " + OnDay);
  //  Serial.println(String() + "SAVEON: " + EEPROM.readString(280));
  //  Serial.println(String() + "COUNT DAY ON: " + EEPROM.readString(310));
  //  Serial.println(String() + "STRING DAY ON: " + EEPROM.readString(320));
  //  Serial.println("\n\n");
  EEPROM.commit();
  CountDayOn = 0;
}
