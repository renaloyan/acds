/*************************************************************
  WARNING!
    It's very tricky to get it working. Please read this article:
    http://help.blynk.cc/hardware-and-libraries/arduino/esp8266-with-at-firmware

  You’ll need:
   - Blynk IoT app (download from App Store or Google Play)
   - Arduino Uno board
   - Decide how to connect to Blynk
     (USB, Ethernet, Wi-Fi, Bluetooth, ...)

  There is a bunch of great example sketches included to show you how to get
  started. Think of them as LEGO bricks  and combine them as you wish.
  For example, take the Ethernet Shield sketch and combine it with the
  Servo example, or choose a USB sketch and add a code from SendData
  example.
 *************************************************************/

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial
const int buzzer = 5;
const int relay = 12;
int LowLevel = 38;
int HighLevel = 5; //cm
float levelCM;
int Level1 = (LowLevel * 90) / 100;  //low level
#define TRIGPIN 9
#define ECHOPIN 8
#define DHTPIN 7
#define DHTTYPE DHT22

#include <EasyUltrasonic.h>
#include <DHT.h>
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

// Your WiFi credentials.
// Set password to "" for open networks.

char ssid[] = "wifi";
char pass[] = "password";

// Hardware Serial on Mega, Leonardo, Micro...
//#define EspSerial Serial1

// or Software Serial on Uno, Nano...
#include <SoftwareSerial.h>
SoftwareSerial EspSerial(2, 3);  // RX, TX

// Your ESP8266 baud rate:
#define ESP8266_BAUD 38400

ESP8266 wifi(&EspSerial);
DHT dht(DHTPIN, DHTTYPE);
EasyUltrasonic ultrasonic;
BlynkTimer timer;
WidgetRTC rtc;

BLYNK_CONNECTED() {
  Serial.print("CONNECTED");
  Blynk.syncAll();
  rtc.begin();
}

// Digital clock display of the time
void clockDisplay() {
  // You can call hour(), minute(), ... at any time
  // Please see Time library examples for details

  String currentTime = "TIME:" + String(hour()) + ":" + minute() + ":" + second();
  String currentDate = "DATE:" + String(month()) + " " + day() + " " + year();
  
  //send time data to blynk app
  Blynk.virtualWrite(V0, currentTime);
  Blynk.virtualWrite(V2, currentDate);
}

//set dht22
void send_Data(void) {

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  bool result = Blynk.connected();

  //set the relay off when disconnected
  if(result == false){
    digitalWrite(relay, LOW);
    Serial.println("Disconnected");
  }  

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  float distanceIN = ultrasonic.getDistanceIN();
  levelCM = convertToCM(distanceIN);

  //alarm when low level
  if(Level1 <= levelCM){
    digitalWrite(relay, LOW);
    tone(buzzer, 1000);
    delay(100);
    noTone(buzzer);
    delay(100);    
  }
  else {
    noTone(buzzer);
  } 

  //converts into percentage
  levelCM = levelCM/(LowLevel - HighLevel);
  levelCM = levelCM * 100;
  int percentageLevel = 100 - levelCM;
  if (percentageLevel > 100) {percentageLevel = 100;}
  int blynkLevelPercentage = percentageLevel;

  if (percentageLevel <= 100) {
    Blynk.virtualWrite(V4, blynkLevelPercentage);
  } else {
    Blynk.virtualWrite(V4, 0);
  }

  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V5, h);
  Blynk.virtualWrite(V6, t);
}

BLYNK_WRITE(V1) {

  float Relay = param.asInt();
  if(Relay == 1){
    digitalWrite(relay, HIGH);
    Serial.println("PUMP ON");
    tone(buzzer, 10000);
  } else {
    digitalWrite(relay, LOW);
    Serial.println("PUMP OFF");
    noTone(buzzer);    
  }
  
}

BLYNK_WRITE(V3) {

  float Buzzer = param.asInt();
  if(Buzzer == 1){
      tone(buzzer, 10000);
      delay(10000);
      noTone(buzzer);
    }
  else {
    noTone(buzzer);
  }
  
}

void setup() {
  // Debug console
  Serial.begin(9600);

  //ultrasonic
  ultrasonic.attach(TRIGPIN, ECHOPIN);

  //buzzer
  pinMode(buzzer, OUTPUT);

  //dht22
  dht.begin();

  //relay
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);

  // Set ESP8266 baud rate
  EspSerial.begin(ESP8266_BAUD);
  delay(10);

  Blynk.begin(BLYNK_AUTH_TOKEN, wifi, ssid, pass);
  // You can also specify server:
  //Blynk.begin(BLYNK_AUTH_TOKEN, wifi, ssid, pass, "blynk.cloud", 80);
  //Blynk.begin(BLYNK_AUTH_TOKEN, wifi, ssid, pass, IPAddress(192,168,1,100), 8080);

  setSyncInterval(10 * 60);  // Sync interval in seconds (10 minutes)

  timer.setInterval(500L, send_Data);
  timer.setInterval(1000L, clockDisplay);
}

void loop() {
  Blynk.run();
  timer.run();
}
