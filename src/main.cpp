#include <Arduino.h>
#include <DHT12.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#define DEFAULT_REFRESH_TIME_CNT 60

/*************************************************
 * int tm_sec       秒數(0~61)
 * int tm_min       分鐘(0~59)
 * int tm_hour      小時(0~23)
 * int tm_mday      日期(1~31)
 * int tm_mon       月份(0~11，從1月算起)
 * int tm_year      年份(從1900年算起)
 * int tm_wday      星期幾(日→0、一→1、二→2、以此類推)
 * int tm_yday      一年中的第幾天
 * int tm_isdst     夏令時間旗標
 * gmtime();        // time_t to struct tm *
 * mktime();        // sturct tm * to time_t
 * ref: https://bryceknowhow.blogspot.com/2013/10/cc-gmtimemktimetimetstruct-tm.html
 * ESP8266:
 * SDA = GPIO4 D2
 * SCL = GPIO5 D1
 *************************************************/

const uint32_t CpuFreq = ESP.getCpuFreqMHz() * 1000000;
const String daysOfTheWeek[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const uint8_t relay_D5 = 14;         // GPIO14 D5
const uint8_t relay_D6 = 12;         // GPIO12 D6
const char *ssid = "Do you want to connect to WiFi";
const char *passwd = "2.71828182845904523536";
const long utcOffsetInSeconds = +8 * 3600;
WiFiUDP ntpudp;
NTPClient timeClient(ntpudp, "pool.ntp.org", utcOffsetInSeconds);
DHT12 dht12(&Wire);
LiquidCrystal_I2C lcd(0x27, 16, 2);
float t12, h12;
int8_t dht12_status;
struct tm currentTime;
time_t currentTimestamp;
uint32_t refreshTimeCnt = DEFAULT_REFRESH_TIME_CNT;
uint64_t CycleCount;
unsigned long start, end, period, remain;

void printTime() {
    Serial.print(currentTime.tm_year + 1900);
    Serial.print(".");
    Serial.print(currentTime.tm_mon + 1);
    Serial.print(".");
    Serial.print(currentTime.tm_mday);
    Serial.print(", ");
    Serial.print(daysOfTheWeek[currentTime.tm_wday]);
    Serial.print(", ");
    Serial.print(currentTime.tm_hour);
    Serial.print(":");
    Serial.print(currentTime.tm_min);
    Serial.print(":");
    Serial.println(currentTime.tm_sec);
    lcd.print(currentTime.tm_mon + 1);
    lcd.print(".");
    lcd.print(currentTime.tm_mday);
    lcd.print(", ");
    lcd.print(currentTime.tm_hour);
    lcd.print(":");
    lcd.print(currentTime.tm_min);
    lcd.print(":");
    lcd.print(currentTime.tm_sec);
}

void updateTime() {
    lcd.setCursor(0, 0);
    if (WiFi.status() == WL_CONNECTED && refreshTimeCnt == DEFAULT_REFRESH_TIME_CNT) {
        Serial.print("WiFi is connected, refresh time count = ");
        Serial.print(refreshTimeCnt);
        Serial.println(", refresh via NTP server");
        timeClient.update();
        currentTimestamp = timeClient.getEpochTime();
        gmtime_r(&currentTimestamp, &currentTime);
        printTime();
        refreshTimeCnt = 0;
    } else if (WiFi.status() == WL_CONNECTED && refreshTimeCnt != DEFAULT_REFRESH_TIME_CNT) {
        Serial.print("WiFi is connected, refresh time count = ");
        Serial.print(refreshTimeCnt);
        Serial.println(", refresh via counter");
        ++currentTimestamp;
        gmtime_r(&currentTimestamp, &currentTime);
        printTime();
        if (refreshTimeCnt != DEFAULT_REFRESH_TIME_CNT) {
            ++refreshTimeCnt;
        }
    } else if (WiFi.status() != WL_CONNECTED) {
        Serial.print("WiFi is NOT connected, refresh time count = ");
        Serial.print(refreshTimeCnt);
        Serial.println(", refresh via counter");
        ++currentTimestamp;
        gmtime_r(&currentTimestamp, &currentTime);
        printTime();
        if (refreshTimeCnt != DEFAULT_REFRESH_TIME_CNT) {
            ++refreshTimeCnt;
        }
    }
}

void updateRelayStatus() {
    time_t time = currentTimestamp % 86400;

    // relay 1 D5
    if (time >= 6 * 3600 + 0 * 60 + 0 &&         // current time between 06:00:00
        time <= 7 * 3600 + 0 * 60 + 0) {         // and 07:00:00
        digitalWrite(relay_D5, HIGH);            // open relay 1 D5
    } else if (time >= 8 * 3600 + 15 * 60 + 0 && // current time between 08:15:00
               time <= 18 * 3600 + 0 * 60 + 0) { // and 18:00:00
        digitalWrite(relay_D5, HIGH);            // open relay 1 D5
    } else if (time >= 21 * 3600 + 0 * 60 + 0 && // current time between 21:00:00
               time <= 23 * 3600 + 0 * 60 + 0) { // and 23:00:00
        digitalWrite(relay_D5, HIGH);            // open relay 1 D5
    } else {                                     // otherwise,
        digitalWrite(relay_D5, LOW);             // close relay 1 D5
    }

    // relay 2 D6
    if (time >= 0 * 3600 + 0 * 60 + 0 &&           // current time between 00:00:00
        time <= 6 * 3600 + 0 * 60 + 0) {           // and 06:00:00
        digitalWrite(relay_D6, HIGH);              // open relay 2 D6
    } else if (time >= 7 * 3600 + 0 * 60 + 0 &&    // current time between 07:00:00
               time <= 8 * 3600 + 30 * 60 + 0) {   // and 08:30:00
        digitalWrite(relay_D6, HIGH);              // open relay 2 D6
    } else if (time >= 9 * 3600 + 15 * 60 + 0 &&   // current time between 09:15:00
               time <= 16 * 3600 + 0 * 60 + 0) {   // and 16:00:00
        digitalWrite(relay_D6, HIGH);              // open relay 2 D6
    } else if (time >= 18 * 3600 + 0 * 60 + 0 &&   // current time between 18:00:00
               time <= 21 * 3600 + 0 * 60 + 0) {   // and 21:00:00
        digitalWrite(relay_D6, HIGH);              // open relay 2 D6
    } else if (time >= 23 * 3600 + 15 * 60 + 0 &&  // current time between 23:15:00
               time <= 23 * 3600 + 59 * 60 + 59) { // and 23:59:59
        digitalWrite(relay_D6, HIGH);              // open relay 2 D6
    } else {                                       // otherwise
        digitalWrite(relay_D6, LOW);               // close relay 2 D6
    }
}

void updateDHT12Status() {
    dht12_status = dht12.read();
    lcd.setCursor(0, 1);
    switch (dht12_status) {
    case DHT12_OK:
        t12 = dht12.getTemperature();
        h12 = dht12.getHumidity();
        Serial.print("Temp: ");
        Serial.print(t12, 1);
        Serial.print("°C, Humidity: ");
        Serial.print(h12, 1);
        Serial.println("%");
        lcd.print(t12, 1);
        lcd.print("C, ");
        lcd.print(h12, 1);
        lcd.print("%");
        break;
    case DHT12_ERROR_CHECKSUM:
        Serial.println("DHT12: Checksum error.");
        lcd.print("DHT CHKSUM ERR");
        break;
    case DHT12_ERROR_CONNECT:
        Serial.println("DHT12: Connect error.");
        lcd.print("DHT Connect ERR");
        break;
    case DHT12_MISSING_BYTES:
        Serial.println("DHT12: Missing bytes.");
        lcd.print("DHT Miss Byte");
        break;
    default:
        Serial.println("DHT12: Unknown error.");
        lcd.print("DHT Unknown ERR");
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
    pinMode(relay_D5, OUTPUT);
    pinMode(relay_D6, OUTPUT);
    dht12.begin(SDA, SCL);
    lcd.begin(16, 2);
    lcd.backlight();
    lcd.clear();
    WiFi.begin(ssid, passwd);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        lcd.setCursor(0, 0);
        lcd.print("Connecting WiFi");
    }
    Serial.print("\nSuccess to connect to ");
    Serial.println(ssid);
    lcd.clear();
    lcd.print("Connected");
    timeClient.begin();
}

void loop() {
    // put your main code here, to run repeatedly:
    start = millis();
    lcd.clear();
    updateTime();
    updateRelayStatus();
    updateDHT12Status();
    if (WiFi.status() == WL_CONNECTED) {
        lcd.print(" ON");
    } else {
        lcd.print(" OFF");
    }
    Serial.print("CPU cycle cnt: ");
    Serial.println(ESP.getCycleCount());
    Serial.println("");
    end = millis();
    period = end - start;
    remain = 1000 > period ? 1000 - period : 0;
    // Serial.println(start);
    // Serial.println(end);
    // Serial.println(period);
    // Serial.println(remain);
    delay(remain);
}
