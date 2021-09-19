#include <WiFi.h>
#include "time.h"
#include <FirebaseESP32.h>
#include "addons/TokenHelper.h"//Provide the token generation process info.
#include "addons/RTDBHelper.h"//Provide the RTDB payload printing info and other helper functions.

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
Adafruit_PWMServoDriver  pca9685 = Adafruit_PWMServoDriver(0x40);
#define SERVOMIN  75  // found 74 but set 120 for 0 degrees
#define SERVOMAX  560  //found 560 but set 540 for 180 degree

#define I2C_SDA 13 // Orange //15 and more importantly 2 and 12 are strapping pins
#define I2C_SCL 14 //Yellow
#define SER0  3   //Servo Motor 0 on connector 0 // Define servo motor connections (expand as required)
#define SER1  7  //Servo Motor 1 on connector 12
#define SER2  11   //Servo Motor 0 on connector 0 // Define servo motor connections (expand as required)
#define SER3  15
int pwm0, pwm1, pwm2, pwm3;// Variables for Servo Motor positions (expand as required)

#define WIFI_SSID "4207 Hyperoptic 2.4ghz"
#define WIFI_PASSWORD "SA6cL5AEDYmz"
#define API_KEY "AIzaSyCIOWckVbYTrLsujeseL8eB-lVVD4O43nE"
#define DATABASE_URL "fun-iot-default-rtdb.europe-west1.firebasedatabase.app"
#define USER_EMAIL "kumbhat10@gmail.com"
#define USER_PASSWORD "Kanakrajj10"
#define FIREBASE_FCM_SERVER_KEY "AAAAjk8pWaQ:APA91bHTJdIS8Oj4jTZ1-6ooCkl6x2PPmPuyc6k1vvWqfHh56WiQSruilPjhlhy-b5ZJfZcvMr35dYrsVoI-y3G5hWxj65wPvPVJj7KYeI-X3yK6S_FRvBN6Exh8Ur694MT_lvmDNRJ-"

const char* ntpServer = "pool.ntp.org"; const long  gmtOffset_sec = 0; const int   daylightOffset_sec = 3600;
char date1 [8]; char time1 [10];
int date2 = 0;

bool ledState = true;
double ledStateBlinkCount = 3;
int ledBlinkTime = 40; //in milliseconds
int ledPrevMillis = 0;

const int LED_BUILTIN = 33;
int ADC_Max = 4096;
FirebaseJsonData lx, ly, rx, ry, tr;
FirebaseData stream, fbdo, fbdo1;
FirebaseAuth auth; FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;

void setup()
{
  Serial.begin(115200);  Serial.println();
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.setSleep(false);

  Wire.begin(I2C_SDA, I2C_SCL);// Initialize I2C connection
  pca9685.begin();  // Initialize PCA9685
  pca9685.setPWMFreq(50);  // This is the maximum PWM frequency

  Serial.print("Connecting to ");  Serial.print(WIFI_SSID); Serial.print("........"); WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    if (ledState) {
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
    ledState = !ledState;
    delay(ledBlinkTime);
  }
  digitalWrite(LED_BUILTIN, LOW); ledState = true;
  Serial.println();  Serial.print("Connected to " + String(WIFI_SSID) + " with IP: ");  Serial.print(WiFi.localIP()); Serial.print(" Strength : "); Serial.print(WiFi.RSSI()); Serial.println(); // Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  config.api_key = API_KEY;  config.database_url = DATABASE_URL;  config.token_status_callback = tokenStatusCallback;  auth.user.email = USER_EMAIL;  auth.user.password = USER_PASSWORD;
  Firebase.begin(&config, &auth);  Firebase.reconnectWiFi(true);//  Firebase.setMaxErrorQueue(fbdo, 3);
  Serial.println("Connecting to Firebase..."); //while(Firebase
  Serial.println();
  while (!Firebase.ready()) {
    if (ledState) {
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
    ledState = !ledState;
    delay(ledBlinkTime);
  }
  digitalWrite(LED_BUILTIN, LOW); ledState = true;
  Serial.println("Sending Cloud message notification...");
  Serial.println();
  sendMessage();
  Serial.println("Begin streaming");
  Serial.println();
  if (!Firebase.beginStream(stream, "/Control/data"))
    Serial.printf("sream begin error, %s\n\n", stream.errorReason().c_str());
  Firebase.setStreamCallback(stream, streamCallback, streamTimeoutCallback);
}

void loop()
{
  if (ledStateBlinkCount > 0) {
    if (millis() - ledPrevMillis > ledBlinkTime || ledPrevMillis == 0 ) {
      //      Serial.println(ledState);
      ledPrevMillis = millis();
      if (ledState) {
        digitalWrite(LED_BUILTIN, HIGH);
      } else {
        digitalWrite(LED_BUILTIN, LOW);
      }
      ledState = !ledState;
      ledStateBlinkCount = ledStateBlinkCount - 0.5;
    }
  } else {
    digitalWrite(LED_BUILTIN, LOW); ledState = true;
  }

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 300000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    printLocalTime(date1, time1);
    sscanf(date1, "%d", &date2);
    FirebaseJson json;
    json.add("D", date2 );
    json.add("T", time1 );
    //Serial.printf("Upload to database ---> %s\n\n", Firebase.setJSON(fbdo, "/Check", json) ? "Successful" : fbdo.errorReason().c_str());
    //Firebase.setJSON(fbdo, "/Check", json);
  }
}

void streamCallback(StreamData data)
{
  count ++;
  ledStateBlinkCount = 3;
  Serial.print(count); Serial.print(" : data --> ");
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json)
  {
    FirebaseJson *json = data.to<FirebaseJson *>();
    Serial.println(json->raw());
    json -> get(lx, "lx");
    json -> get(ly, "ly");
    json -> get(rx, "rx");
    json -> get(ry, "ry");
    DriveServo();
  }
  else if (data.dataTypeEnum() == fb_esp_rtdb_data_type_array)
  {
    Serial.println(" Json array ");
    FirebaseJsonArray *arr = data.to<FirebaseJsonArray *>();
    Serial.println(arr->raw());
  }
  else {
    Serial.println("Not Json ");
    Serial.println(data.dataTypeEnum());
    Serial.println(data.dataType());
    Serial.println(data.to<int>());
    pca9685.setPWM(SER0, 0, data.to<int>());
    //    Serial.println(data);
  }
}

void DriveServo()
{
  pca9685.setPWM(SER0, 0, map(ry.to<int>(), 0, 180, SERVOMIN, SERVOMAX));
  pca9685.setPWM(SER1, 0, map(rx.to<int>(), 0, 180, SERVOMIN, SERVOMAX));
  pca9685.setPWM(SER2, 0, map(ly.to<int>(), 0, 180, SERVOMIN, SERVOMAX));
  pca9685.setPWM(SER3, 0, map(lx.to<int>(), 0, 180, SERVOMIN, SERVOMAX));
  Serial.println("Trying servo");
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
    Serial.println("stream timeout, resuming...\n");
  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}

void sendMessage()
{
  fbdo1.fcm.begin(FIREBASE_FCM_SERVER_KEY);    fbdo1.fcm.setTopic("Alert");    fbdo1.fcm.setPriority("high");    fbdo1.fcm.setTimeToLive(1000);
//  fbdo1.fcm.setNotifyMessage("IoT Robot is Online now! ", "Robot was restarted");
  fbdo.fcm.setDataMessage("{'title':'Dushyant'}");
  Serial.printf("Send message... %s\n", Firebase.sendTopic(fbdo1) ? "Successful" : fbdo1.errorReason().c_str());
  if (fbdo1.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK)
    Serial.println(fbdo1.fcm.getSendResult());
  Serial.println();
}

void printLocalTime(char* date1, char* time1)
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  strftime (date1, 9, "%Y%m%d", &timeinfo);
  strftime (time1, 9, "%H:%M:%S", &timeinfo);
}
