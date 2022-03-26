#include <WiFi.h>
//#include <FS.h>
//#include <SPIFFS.h>
#include "Preferences.h"
#include "SoftwareSerial.h"
#include <ESP32Ticker.h>
//// #include <Arduino.h>
#include <Bounce2.h>
#include <ModbusRTU.h>
#include <ModbusIP_ESP8266.h>
#include "MB11016P_ESP.h"
#include "SensorRain.h"
#include <Teplica.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

// #define USE_WEB_SERIAL

#ifdef USE_WEB_SERIAL // использование web сервера для отладки
#include <WebSerial.h>
void recvMsg(uint8_t *data, size_t len);
void webSerialSend(void *pvParameters);
#endif

Preferences flash;
AsyncWebServer server(80);

#define RXDNEX 23
#define TXDNEX 22
#define RXDMASTER 25
#define TXDMASTER 33
#define ON HIGH
#define OFF LOW
#define DEBUG_WIFI

const String VER = "Ver - 4.6.1. Date - 26.03.22\r";
int IDSLAVE = 47; // адрес в сети Modbus

char* ssid = "yastrebovka";
char* password =  "zerNo32_";
//char *ssid = "Home-RP";
//char *password = "12rp1974";

//цвет на экране
const double LIGHT = 57048;
const double RED = 55688;
const double GREEN = 2016;
const double BLUE = 1566;

const double AIRTIME = 900000; // длительность проветривания и осушения 
const int RAIN1 = 300; // средний дождь
const int RAIN2 = 500; // сильный дождь

SoftwareSerial SerialNextion;
Ticker tickerWiFiConnect;

enum
{
  rs485mode4 = 0,          //  1 - 1 - режим работы теплица4, 3 - насос4, 4 - доп. нагреватель4, 9-16 - уставка теплица4
  rs485mode41,             //  2 - 1-8 - уровень открытия окна теплица4, 9-16 сдвиг уставки для окон
  rs485mode5,              //  3 - 1 - режим работы теплица5, 3 - насос5, 4 - доп. нагреватель5, 9-16 - уставка теплица5
  rs485mode51,             //  4 - 1-8 - уровень открытия окна теплица5,  9-16 сдвиг уставки для окон
  rs485mode6,              //  5 - 1 - режим работы теплица6, 3 - насос6, 4 - доп. нагреватель6, 9-16 - уставка теплица6
  rs485mode61,             //  6 - 1-8 - уровень открытия окна теплица6,  9-16 сдвиг уставки для окон
  rs485mode7,              //  7 - 1 - режим работы теплица7, 3 - насос7, 4 - доп. нагреватель6, 9-16 - уставка теплица6
  rs485mode71,             //  8 - 1-8 - уровень открытия окна теплица7,  9-16 сдвиг уставки для окон
  rs485settings,           //  9 - 1 - наличие датчика дождя, 2 - наличие датчика температуры наружного воздуха, 6-13 - время открытия окна теплица4 (сек), 14-16 - величина гистерезиса включения насосов (гр.С)
  rs485temperature4,       //  10 - температура в теплиц4
  rs485temperature5,       //  11 - температура в теплиц5
  rs485temperature6,       //  12 - температура в теплиц6
  rs485temperature7,       //  13 - температура в теплиц6
  rs485temperatureoutdoor, //  14 - температура на улице
  rs485humidity45,         //  15 - 1-8 влажность теплица4, 9-16 влажность теплица5
  rs485humidity67,         //  16 - 1-8 влажность теплица6, 9-16 влажность на улице
  rs485humidityoutdoor,    //  17 - 1-8 влажность теплица7
  rs485error45,            //  18 - 1-8 ошибки теплица4 , 9-16 ошибки теплица5
  rs485error67,            //  29 - 1-8 ошибки теплица6 , 9-16 ошибки теплица7
  rs485error8,             //  20 - ошибки температура на улице
  rs485rain,               //  21 - 1-8 - показания датчика дождя, 9-16 ошибки датчик дождя
  test,
  rs485_HOLDING_REGS_SIZE //  leave this one
};

enum
{
  WiFimode4 = 100,          //  режим работы теплица 4
  WiFipump4,                //  насос 4
  WiFiheat4,                //  доп. нагреватель 4
  WiFisetpump4,             //  уставка теплица 4
  WiFisetheat4,             //  уставка доп.нагрветель 4
  WiFisetwindow4,           //  уставка окна теплица 4
  WiFitemperature4,         //  температура в теплиц 4
  WiFihumidity4,            //  влажность теплица 4
  WiFierror4,               //  ошибки теплица 4
  WiFiLevel4,               //  уровень открытия окна теплица 4
  WiFiHysteresis4,           // гистерезис насосов теплица 4
  WiFiOpenTimeWindow4,       // время открытия окон теплица 4

  WiFimode5,                //  режим работы теплица 5
  WiFipump5,                //  насос 5
  WiFiheat5,                //  доп. нагреватель 5
  WiFisetpump5,             //  уставка теплица 5
  WiFisetheat5,             //  уставка доп.нагрветель 5
  WiFisetwindow5,           //  уставка окна теплица 5
  WiFitemperature5,         //  температура в теплиц 5
  WiFihumidity5,            //  влажность теплица 5
  WiFierror5,               //  ошибки теплица 5
  WiFiLevel5,               //  уровень открытия окна теплица 5
  WiFiHysteresis5,           // гистерезис насосов теплица 5
  WiFiOpenTimeWindow5,       // время открытия окон теплица 5

  WiFimode6,                //  режим работы теплица 6
  WiFipump6,                //  насос 6
  WiFiheat6,                //  доп. нагреватель 6
  WiFisetpump6,             //  уставка теплица 6
  WiFisetheat6,             //  уставка доп.нагрветель 6
  WiFisetwindow6,           //  уставка окна теплица 6
  WiFitemperature6,         //  температура в теплиц 6
  WiFihumidity6,            //  влажность теплица 6
  WiFierror6,               //  ошибки теплица 6
  WiFiLevel6,               //  уровень открытия окна теплица 6
  WiFiHysteresis6,           // гистерезис насосов теплица 6
  WiFiOpenTimeWindow6,       // время открытия окон теплица 6

  WiFimode7,                //  режим работы теплица 7
  WiFipump7,                //  насос 7
  WiFiheat7,                //  доп. нагреватель 7
  WiFisetpump7,             //  уставка теплица 4
  WiFisetheat7,             //  уставка доп.нагрветель 4
  WiFisetwindow7,           //  уставка окна теплица 4
  WiFitemperature7,         //  температура в теплиц 7
  WiFihumidity7,            //  влажность теплица 7
  WiFierror7,               //  ошибки теплица 7
  WiFiLevel7,               //  уровень открытия окна теплица 7
  WiFiHysteresis7,           // гистерезис насосов теплица 4
  WiFiOpenTimeWindow7,       // время открытия окон теплица 4

  WiFitemperatureoutdoor,   //  температура на улице
  WiFihumidityoutdoor,      //  влажность на улице
  WiFierroroutdoor,         //  ошибки датчика температуры на улице
  WiFirain,                 //  показания датчика дождя
  WiFierrorrain,            //  ошибки датчик дождя
  WiFi_HOLDING_REGS_SIZE    //  leave this one
};

enum
{
  wifi_flag_edit_4 = 300,
  wifi_UstavkaPump_4,
  wifi_UstavkaHeat_4,
  wifi_UstavkaWin_4,
  wifi_setWindow_4,
  wifi_mode_4,
  wifi_pump_4,
  wifi_heat_4,
  wifi_hysteresis_4,
  wifi_time_open_windows_4,
  res48,
  wifi_flag_edit_5,
  wifi_UstavkaPump_5,
  wifi_UstavkaHeat_5,
  wifi_UstavkaWin_5,
  wifi_setWindow_5,
  wifi_mode_5,
  wifi_pump_5,
  wifi_heat_5,
  wifi_hysteresis_5,
  wifi_time_open_windows_5,
  res58,
  wifi_flag_edit_6,
  wifi_UstavkaPump_6,
  wifi_UstavkaHeat_6,
  wifi_UstavkaWin_6,
  wifi_setWindow_6,
  wifi_mode_6,
  wifi_pump_6,
  wifi_heat_6,
  wifi_hysteresis_6,
  wifi_time_open_windows_6,
  res68,
  wifi_flag_edit_7,
  wifi_UstavkaPump_7,
  wifi_UstavkaHeat_7,
  wifi_UstavkaWin_7,
  wifi_setWindow_7,
  wifi_mode_7,
  wifi_pump_7,
  wifi_heat_7,
  wifi_hysteresis_7,
  wifi_time_open_windows_7,
  res78,
  wifi_HOLDING_REGS_SIZE //  leave this one
};

int modbusdateWiFi[WiFi_HOLDING_REGS_SIZE];
unsigned long timesendnextion, time_updateGreenHouse;
const unsigned long TIME_UPDATE_GREENOOUSE = 360000;
const unsigned long TIME_UPDATE_MODBUS = 1000;
long updateNextion;
String pageNextion = "p0";
int counterMBRead = 0;
int coun1 = 0 ;
int arr_adr[12];
int arr_set[8];

boolean counterMB = true;
TaskHandle_t Task_updateGreenHouse;
TaskHandle_t Task_updateDateSensor;
TaskHandle_t Task_webSerialSend;
ModbusRTU slave;
ModbusIP slaveWiFi;
ModbusRTU mb_master;
MB11016P_ESP mb11016p = MB11016P_ESP(&mb_master, 100, 2);
Sensor OutDoorTemperature = Sensor(10, 0, 0);
SensorRain Rain = SensorRain(1);
Sensor Tepl4Temperature = Sensor(4, 0, 0);
Sensor Tepl5Temperature = Sensor(5, 0, 0);
Sensor Tepl6Temperature = Sensor(6, 0, 0);
Sensor Tepl7Temperature = Sensor(7, 0, 0);
Teplica Tepl4 = Teplica(4, &Tepl4Temperature, 0, 1, 2, 3, 30, 20, 40, 60, &mb11016p);
Teplica Tepl5 = Teplica(5, &Tepl5Temperature, 4, 5, 6, 7, 30, 20, 40, 60, &mb11016p);
Teplica Tepl6 = Teplica(6, &Tepl6Temperature, 8, 9, 10, 11, 30, 20, 40, 60, &mb11016p);
// Teplica Tepl7 = Teplica(7, &Tepl7Temperature, 12, 13, 14, 15, 1, 30, 20, 40,  60, &mb11016p);
Teplica Tepl7 = Teplica(7, &Tepl7Temperature, 12, 13, 14, 30, 20, 40, &mb11016p); // теплица без окон

Sensor *sensor_SM200[5];
Teplica *arr_Tepl[4];

// 0 - адрес устройства в сети Modbus, 1 - адрес  регистра в таблице Modbus, 2 - статус, 3- резерв, 4 - температура, 5 - влажность
enum mb_Sensor
{
  adress,
  registr,
  status,
  firmware,
  temperature,
  humidity,
};
enum SensorSM200
{
  err,
  firm,
  temper,
  hum,
  SIZE,
  mberror,
};

uint16_t sensor[6] = {0, 0, 0, 0, 0, 0};
void sendNextion(String dev, String data);
void sendNextion(String dev, int data);
void sendNextion(String dev);
void readNextion();
void analyseString(String incStr);
void pageNextion_p0();
void pageNextion_p1(int i);
void pageNextion_p2();
void indiTepl4();
void indiTepl5();
void indiTepl6();
void indiTepl7();
void indiOutDoor();
int indiRain();
void pars_str_adr(String &str);
void pars_str_set(String &str);
void saveOutModBusArr();
void update_mbmaster();
void update_WiFiConnect();
void controlScada();
String calculateTimeWork();
void updateGreenHouse(void *pvParameters);
void updateDateSensor(void *pvParameters);

bool cbRead(Modbus::ResultCode event, uint16_t transactionId, void *data)
{
  // Serial.printf("result:\t0x%02X\n", event);
  sensor[SensorSM200::mberror] = event;
  return true;
}