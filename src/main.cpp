
#include "main.h"
void setup()
{
  pinMode(GPIO_NUM_2, OUTPUT);
  Serial.begin(115200);
  Serial.print(VER);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setHostname("Tepl_4_5_6_7");
  Serial.printf("\nConnecting to WiFi..\n");
  int counter_WiFi = 0;
  while (WiFi.status() != WL_CONNECTED && counter_WiFi < 10)
  {
    delay(1000);
    counter_WiFi++;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    char *ssid = "yastrebovka";
    char *password = "zerNo32_";
    WiFi.begin(ssid, password);
    counter_WiFi = 0;
    while (WiFi.status() != WL_CONNECTED && counter_WiFi < 10)
    {
      delay(1000);
      counter_WiFi++;
    }
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Greenhaus #4, #5, #6 and #7.\n" + VER + calculateTimeWork() + "\nRSSI: " + String(WiFi.RSSI()) + " db\nRain: " + String(Rain.getRaiLevel())); });

  AsyncElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
#ifdef USE_WEB_SERIAL
  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);
#endif

#ifdef DEBUG_WIFI
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.printf("Connect to:\t");
    Serial.println(ssid);
    Serial.printf("IP address:\t");
    Serial.println(WiFi.localIP());
    Serial.printf("Hostname:\t");
    Serial.println(WiFi.getHostname());
    Serial.printf("Mac Address:\t");
    Serial.println(WiFi.macAddress());
    Serial.printf("Subnet Mask:\t");
    Serial.println(WiFi.subnetMask());
    Serial.printf("Gateway IP:\t");
    Serial.println(WiFi.gatewayIP());
    Serial.printf("DNS:t\t\t");
    Serial.println(WiFi.dnsIP());
    Serial.println("HTTP server started");
  }
  else
    Serial.printf("No connect WiFi\n");
#endif
#ifdef SCAN_WIFI
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  for (;;)
  {
    Serial.println("scan start");
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
    {
      Serial.println("no networks found");
    }
    else
    {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i)
      {
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        delay(10);
      }
    }
    Serial.println("");
    // Wait a bit before scanning again
    delay(5000);
  }
#endif

  flash.begin("eerom", false);
  Serial1.begin(flash.getInt("mspeed", 2400), SERIAL_8N1, RXDMASTER, TXDMASTER, false); // Modbus Master
  Serial2.begin(flash.getInt("sspeed", 19200));                                         // Modbus Slave
  SerialNextion.begin(19200, SWSERIAL_8N1, RXDNEX, TXDNEX, false, 256);
  sendNextion("rest");
  String incStr;
  slave.begin(&Serial2);
  slave.slave(IDSLAVE);
  slaveWiFi.slave(IDSLAVE);
  slaveWiFi.begin();
  mb_master.begin(&Serial1);
  mb_master.master();
  for (int i = rs485mode4; i < rs485_HOLDING_REGS_SIZE; i++)
    slave.addHreg(i);
  for (int i = WiFimode4; i < WiFi_HOLDING_REGS_SIZE; i++)
    slave.addIreg(i);
  for (int i = wifi_flag_edit_4; i < wifi_HOLDING_REGS_SIZE; i++)
    slave.addHreg(i);
  sensor_SM200[0] = &Tepl4Temperature;
  sensor_SM200[1] = &Tepl5Temperature;
  sensor_SM200[2] = &Tepl6Temperature;
  sensor_SM200[3] = &Tepl7Temperature;
  sensor_SM200[4] = &OutDoorTemperature;
  arr_Tepl[0] = &Tepl4;
  arr_Tepl[1] = &Tepl5;
  arr_Tepl[2] = &Tepl6;
  arr_Tepl[3] = &Tepl7;
  //считывание параметров установок теплиц из памяти
  for (int i = 0; i < 4; i++)
  {
    uint t = flash.getUInt(String("SetPump" + String(arr_Tepl[i]->getId())).c_str(), 500);
    arr_Tepl[i]->setSetPump(t);
    t = flash.getUInt(String("SetHeat" + String(arr_Tepl[i]->getId())).c_str(), 300);
    arr_Tepl[i]->setSetHeat(t);
    t = flash.getUInt(String("SetSetWindow" + String(arr_Tepl[i]->getId())).c_str(), 600);
    arr_Tepl[i]->setSetWindow(t);
    t = flash.getUInt(String("Hyster" + String(arr_Tepl[i]->getId())).c_str(), 20);
    arr_Tepl[i]->setHysteresis(t);
    t = flash.getUInt(String("Opentwin" + String(arr_Tepl[i]->getId())).c_str(), 60);
    arr_Tepl[i]->setOpenTimeWindow(t);
  }

  incStr = flash.getString("adr", "");
  pars_str_adr(incStr);
  // первоначальное закрытие окон
  Tepl4.setWindowlevel(-100);
  Tepl5.setWindowlevel(-100);
  Tepl6.setWindowlevel(-100);
  Tepl7.setWindowlevel(-100);
  mb11016p.write();
  tickerWiFiConnect.attach_ms(600000, update_WiFiConnect); // таймер проверки соединения WiFi (раз в 10 минут)
  xTaskCreatePinnedToCore(
      updateDateSensor,        /* Запрос по Modbus по  сети RS485 */
      "Task_updateDateSensor", /* Название задачи */
      10000,                   /* Размер стека задачи */
      NULL,                    /* Параметр задачи */
      1,                       /* Приоритет задачи */
      &Task_updateDateSensor,  /* Идентификатор задачи, чтобы ее можно было отслеживать */
      1);                      /* Ядро для выполнения задачи (0) */

  updateNextion = millis();
  xTaskCreatePinnedToCore(
      updateGreenHouse,        /* Регулировка окон*/
      "Task_updateGreenHouse", /* Название задачи */
      10000,                   /* Размер стека задачи */
      NULL,                    /* Параметр задачи */
      2,                       /* Приоритет задачи */
      &Task_updateGreenHouse,  /* Идентификатор задачи, чтобы ее можно было отслеживать */
      0);
#ifdef USE_WEB_SERIAL
  xTaskCreatePinnedToCore(
      webSerialSend,        /* */
      "Task_wedSerialSend", /* Название задачи */
      10000,                /* Размер стека задачи */
      NULL,                 /* Параметр задачи */
      4,                    /* Приоритет задачи */
      &Task_webSerialSend,  /* Идентификатор задачи, чтобы ее можно было отслеживать */
      1);
#endif
}
void loop()
{
  saveOutModBusArr();
  slave.task();
  slaveWiFi.task();
  for (Teplica *t : arr_Tepl)
  {
    t->updateWorkWindows();
    if (1 == t->getSensorStatus())
      t->regulationPump(t->getTemperature());

    if ((Rain.getAdress() && Rain.getRaiLevel() <0x400) && Rain.getRaiLevel() > RAIN_MAX) //  если идет сильный дождь - закрываем окна
    {
      if (t->getLevel() > 50)
        t->setWindowlevel(40);
    }
  }
  if (SerialNextion.available())
    readNextion();
  if (millis() > updateNextion)
  {
    if (pageNextion == "p0")
      pageNextion_p0();
    else if (pageNextion == "p1_0")
      pageNextion_p1(0);
    else if (pageNextion == "p1_1")
      pageNextion_p1(1);
    else if (pageNextion == "p1_2")
      pageNextion_p1(2);
    else if (pageNextion == "p1_3")
      pageNextion_p1(3);
    else if (pageNextion == "p2")
      pageNextion_p2();
    updateNextion = millis() + 300;
    digitalWrite(GPIO_NUM_2, !digitalRead(GPIO_NUM_2));
  }
  controlScada();
}
void pageNextion_p0()
{
  coun1 = 0;
  // вывод данных теплицы 4
  indiTepl4();
  // вывод данных теплицы 5
  indiTepl5();
  // вывод данных теплицы 6
  indiTepl6();
  // вывод данных теплицы 7
  indiTepl7();
  // вывод температура на улице
  indiOutDoor();
  // вывод символа дождя/солнца
  switch (indiRain())
  {
  case 0:
    sendNextion("p0.pic", 11); // нет датчика
    break;
  case 1:
    sendNextion("p0.pic", 4); // солнце
    break;
  case 2:
    sendNextion("p0.pic", 7); // влажно
    break;
  case 3:
    sendNextion("p0.pic", 3); // сильный дождь
    break;
  default:
    sendNextion("p0.pic", 8); // ошибка датчика
    break;
  }
}
//окно установок теплиц
void pageNextion_p1(int i)
{
  sendNextion("g0.txt", "\nRSSI: " + String(WiFi.RSSI()) + " | " + WiFi.localIP().toString());
  if (coun1 < 3)
  {
    sendNextion("x0.val", arr_Tepl[i]->getSetPump() / 10);
    sendNextion("x4.val", arr_Tepl[i]->getSetHeat() / 10);
    sendNextion("x3.val", arr_Tepl[i]->getSetWindow() / 10);
    sendNextion("x1.val", arr_Tepl[i]->getHysteresis() / 10);
    sendNextion("h1.val", arr_Tepl[i]->getHysteresis());
    sendNextion("x2.val", arr_Tepl[i]->getOpenTimeWindow());
    sendNextion("h2.val", arr_Tepl[i]->getOpenTimeWindow());
    sendNextion("n0.val", arr_Tepl[i]->getLevel());
    sendNextion("h0.val", arr_Tepl[i]->getLevel());
    coun1++;
  }
  arr_Tepl[i]->getPump() ? sendNextion("b3.picc", 2) : sendNextion("b3.picc", 1);
  arr_Tepl[i]->getHeat() ? sendNextion("b2.picc", 2) : sendNextion("b2.picc", 1);
  switch (arr_Tepl[i]->getMode())
  {
  case Teplica::MANUAL:
    sendNextion("b0.picc", 2);
    break;
  case Teplica::AUTO:
    sendNextion("b0.picc", 1);
    sendNextion("b2.picc", 1);
    sendNextion("b1.picc", 1);
    break;
  case Teplica::AIR:
    sendNextion("b0.picc", 1);
    sendNextion("b1.picc", 2);
    break;
  case Teplica::DECREASE_IN_HUMIDITY:
    sendNextion("b2.picc", 2);
    break;  
  default:
    break;
  }
}
void pageNextion_p2()
{
  if (coun1 < 3)
  {
    String incStr = flash.getString("adr", "");
    pars_str_adr(incStr);
  }
  coun1++;
  switch (flash.getInt("sspeed", 2400))
  {
  case 2400:
    sendNextion("r4.val=1");
    break;
  case 9600:
    sendNextion("r3.val=1");
    break;
  case 19200:
    sendNextion("r5.val=1");
    break;
  }
  switch (flash.getInt("mspeed", 2400))
  {
  case 2400:
    sendNextion("r0.val=1");
    break;
  case 9600:
    sendNextion("r1.val=1");
    break;
  case 19200:
    sendNextion("r2.val=1");
    break;
  }
}

// вывод данных датчика дождя
int indiRain()
{
  if (Rain.getAdress())
  {
    if (Rain.getRaiLevel() < RAIN_MIN)
    {
      return 1; // солнце
    }
    else if (Rain.getRaiLevel() < RAIN_MAX)
    {
      return 2; // влажно
    }
    else if (Rain.getRaiLevel() < 0x400)
    {
      return 3; // сильный дождь
    }
    else if (Rain.getRaiLevel() == 0xffff)
    {
      return 0xffff; // ошибка датчика
    }
  }
  else
    return 0; //  датчик не установлен
}

// вывод температура на улице
void indiOutDoor()
{
  if (OutDoorTemperature.getAdress())
  {
    if (1 == OutDoorTemperature.getStatus())
    {
      sendNextion("page0.t1.txt", String(OutDoorTemperature.getTemperature() / 100.0, 1));
      sendNextion("page0.t2.txt", String(OutDoorTemperature.getHumidity() / 100.0, 1));
    }
    else
    {
      sendNextion("page0.t1.txt", String(OutDoorTemperature.getStatus(), HEX));
      sendNextion("page0.t1.pco", LIGHT);
      sendNextion("page0.t2.txt", "---");
      sendNextion("page0.t2.pco", LIGHT);
    }
  }
  else
  {
    sendNextion("x12.val", "");
    sendNextion("x13.val", "");
  }
}

// вывод данных теплицы 4
void indiTepl4()
{
 if (Tepl4Temperature.getAdress())
  { //ввывод температуры и ошибок датчика температуры
    if (1 == Tepl4.getSensorStatus())
    {
      sendNextion("page0.t0.font", 5);
      sendNextion("page0.t0.txt", String(Tepl4.getTemperature() / 100.0, 1));
      if (Tepl4.getTemperature() < Tepl4.getSetHeat())
        sendNextion("page0.t0.pco", BLUE);
      else if (Tepl4.getTemperature() > Tepl4.getSetWindow())
        sendNextion("page0.t0.pco", RED);
      else
        sendNextion("page0.t0.pco", GREEN);
    }
    else
    {
      sendNextion("page0.t0.pco", LIGHT);
      sendNextion("page0.t0.font", 1);
      sendNextion("page0.t0.txt", "Er " + String(Tepl4.getSensorStatus(), HEX));
    }
  }
  else
  {
    sendNextion("page0.t0.pco", LIGHT);
    sendNextion("page0.t0.font", 1);
    sendNextion("page0.t0.txt", "---");
  }
  //ввывод уставки насос
  sendNextion("x1.val", Tepl4.getSetPump() / 10);
  //ввывод уставки дополнительный обогреватель
  sendNextion("x2.val", Tepl4.getSetHeat() / 10);
  //ввывод уставки окно
  sendNextion("x3.val", Tepl4.getSetWindow() / 10);
  //ввывод уровня открытия окна
  sendNextion("h0.val", Tepl4.getLevel());
  //ввывод режима работы
  if (Tepl4.getMode() == Teplica::MANUAL)
    sendNextion("page0.t3.txt", "M");
  else if (Tepl4.getMode() == Teplica::AUTO)
    sendNextion("page0.t3.txt", "A");
  else if (Tepl4.getMode() == Teplica::AIR)
    sendNextion("page0.t3.txt", "W");
  else if (Tepl4.getMode() == Teplica::DECREASE_IN_HUMIDITY)
    sendNextion("page0.t3.txt", "H");
  //идикация состояния насоса
  Tepl4.getPump() ? sendNextion("p1.pic", 10) : sendNextion("p1.pic", 9);
  //идикация состояния дополнительного обогревателя
  Tepl4.getHeat() ? sendNextion("p2.pic", 10) : sendNextion("p2.pic", 9);
}

// вывод данных теплицы 5
void indiTepl5()
{
  if (Tepl4Temperature.getAdress())
  { //ввывод температуры и ошибок датчика температуры
    if (1 == Tepl5.getSensorStatus())
    {
      sendNextion("t7.font", 5);
      sendNextion("t7.txt", String(Tepl5.getTemperature() / 100.0, 1));
      if (Tepl5.getTemperature() < Tepl5.getSetHeat())
        sendNextion("t7.pco", BLUE);
      else if (Tepl5.getTemperature() > Tepl5.getSetWindow())
        sendNextion("t7.pco", RED);
      else
        sendNextion("t7.pco", GREEN);
    }
    else
    {
      sendNextion("t7.pco", LIGHT);
      sendNextion("t7.font", 1);
      sendNextion("t7.txt", "Er " + String(Tepl5.getSensorStatus(), HEX));
    }
  }
  else
  {
    sendNextion("t7.pco", LIGHT);
    sendNextion("t7.font", 1);
    sendNextion("t7.txt", "---");
  }
  //ввывод уставки насос
  sendNextion("x4.val", Tepl5.getSetPump() / 10);
  //ввывод уставки дополнительный обогреватель
  sendNextion("x6.val", Tepl5.getSetHeat() / 10);
  //ввывод уставки окно
  sendNextion("x7.val", Tepl5.getSetWindow() / 10);
  //ввывод уровня открытия окна
  sendNextion("h1.val", Tepl5.getLevel());
  //ввывод режима работы
  if (Tepl5.getMode() == Teplica::MANUAL)
    sendNextion("page0.t4.txt", "M");
  if (Tepl5.getMode() == Teplica::AUTO)
    sendNextion("page0.t4.txt", "A");
    else if (Tepl5.getMode() == Teplica::AIR)
    sendNextion("page0.t4.txt", "W");
   else if (Tepl5.getMode() == Teplica::DECREASE_IN_HUMIDITY)
    sendNextion("page0.t4.txt", "H");  
  //идикация состояния насоса
  Tepl5.getPump() ? sendNextion("p3.pic", 10) : sendNextion("p3.pic", 9);
  //идикация состояния дополнительного обогревателя
  Tepl5.getHeat() ? sendNextion("p4.pic", 10) : sendNextion("p4.pic", 9);
}

// вывод данных теплицы 6
void indiTepl6()
{
  if (Tepl6Temperature.getAdress())
  { //ввывод температуры и ошибок датчика температуры
    if (1 == Tepl6.getSensorStatus())
    {
      sendNextion("t8.font", 5);
      sendNextion("t8.txt", String(Tepl6.getTemperature() / 100.0, 1));
      if (Tepl6.getTemperature() < Tepl6.getSetHeat())
        sendNextion("t8.pco", BLUE);
      else if (Tepl6.getTemperature() > Tepl6.getSetWindow())
        sendNextion("t8.pco", RED);
      else
        sendNextion("t8.pco", GREEN);
    }
    else
    {
      sendNextion("t8.pco", LIGHT);
      sendNextion("t8.font", 1);
      sendNextion("t8.txt", "Er " + String(Tepl6.getSensorStatus(), HEX));
    }
  }
  else
  {
    sendNextion("t8.pco", LIGHT);
    sendNextion("t8.font", 1);
    sendNextion("t8.txt", "---");
  }
  //ввывод уставки насос
  sendNextion("x8.val", Tepl6.getSetPump() / 10);
  //ввывод уставки дополнительный обогреватель
  sendNextion("x10.val", Tepl6.getSetHeat() / 10);
  //ввывод уставки окно
  sendNextion("x11.val", Tepl6.getSetWindow() / 10);
  //ввывод уровня открытия окна
  sendNextion("h2.val", Tepl6.getLevel());
  //ввывод режима работы
  if (Tepl6.getMode() == Teplica::MANUAL)
    sendNextion("page0.t5.txt", "M");
  if (Tepl6.getMode() == Teplica::AUTO)
    sendNextion("page0.t5.txt", "A");
    else if (Tepl6.getMode() == Teplica::AIR)
    sendNextion("page0.t5.txt", "W");
     else if (Tepl6.getMode() == Teplica::DECREASE_IN_HUMIDITY)
     sendNextion("page0.t5.txt", "H");
  //идикация состояния насоса
  Tepl6.getPump() ? sendNextion("p5.pic", 10) : sendNextion("p5.pic", 9);
  //идикация состояния дополнительного обогревателя
  Tepl6.getHeat() ? sendNextion("p6.pic", 10) : sendNextion("p6.pic", 9);
}

// вывод данных теплицы 7
void indiTepl7()
{
  if (Tepl7Temperature.getAdress())
  { //ввывод температуры и ошибок датчика температуры
    if (1 == Tepl7.getSensorStatus())
    {
      sendNextion("t9.font", 5);
      sendNextion("t9.txt", String(Tepl7.getTemperature() / 100.0, 1));
      if (Tepl7.getTemperature() < Tepl7.getSetHeat())
        sendNextion("t9.pco", BLUE);
      else if (Tepl7.getTemperature() > Tepl7.getSetWindow())
        sendNextion("t9.pco", RED);
      else
        sendNextion("t9.pco", GREEN);
    }
    else
    {
      sendNextion("t9.pco", LIGHT);
      sendNextion("t9.font", 1);
      sendNextion("t9.txt", "Er " + String(Tepl7.getSensorStatus(), HEX));
    }
  }
  else
  {
    sendNextion("t9.pco", LIGHT);
    sendNextion("t9.font", 1);
    sendNextion("t9.txt", "---");
  }
  //вывод уставки насос
  sendNextion("x14.val", Tepl7.getSetPump() / 10);
  //вывод уставки дополнительный обогреватель
  sendNextion("x16.val", Tepl7.getSetHeat() / 10);
  //вывод уставки окно
  sendNextion("x17.val", Tepl7.getSetWindow() / 10);
  //вывод уровня открытия окна  
  sendNextion("page0.h3.val", Tepl7.getLevel());
  //вывод режима работы
  if (Tepl7.getMode() == Teplica::MANUAL)
    sendNextion("page0.t6.txt", "M");
  if (Tepl7.getMode() == Teplica::AUTO)
    sendNextion("page0.t6.txt", "A");
    else if (Tepl7.getMode() == Teplica::AIR)
    sendNextion("page0.t6.txt", "W");
     else if (Tepl7.getMode() == Teplica::DECREASE_IN_HUMIDITY)
    sendNextion("page0.t6.txt", "H");
  //идикация состояния насоса
  Tepl7.getPump() ? sendNextion("p7.pic", 10) : sendNextion("p7.pic", 9);
  //идикация состояния дополнительного обогревателя
  Tepl7.getHeat() ? sendNextion("p8.pic", 10) : sendNextion("p8.pic", 9);
}

// данные с Nextion
void readNextion()
{
  char inc;
  String incStr = "";
  while (SerialNextion.available())
  {
    inc = SerialNextion.read();
    // Serial.print(inc);
    incStr += inc;
    if (inc == 0x23)
      incStr = "";
    if (inc == '\n')
    {
      if (incStr != "" || incStr.length() > 2)
      {
        analyseString(incStr);
      }
      return;
    }
    delay(10);
  }
}

//получение данных от датчиков температуры и влажности, регулировка отопления
void updateDateSensor(void *pvParameters)
{
  for (;;)
  {
    mb11016p.write();
    update_mbmaster();
    switch (counterMBRead)
    {
    case 5:
      if (Rain.getAdress())
      {
        mb_master.readIreg(Rain.getAdress(), 0, sensor, 1, cbRead);
        update_mbmaster();
        if (!sensor[SensorSM200::mberror])
          Rain.setRain(sensor[0]);
        else
          Rain.setRain(0xffff);
      }
      break;
    default:
      if (OutDoorTemperature.getAdress() && counterMBRead == 4)
      {
        // получение данных от датчика температуры и влажности на улице
        if (sensor_SM200[counterMBRead]->getAdress())
          mb_master.readIreg(sensor_SM200[counterMBRead]->getAdress(), 30000, sensor, SensorSM200::SIZE, cbRead);
      }
      else
      {
        if (sensor_SM200[counterMBRead]->getAdress())
          mb_master.readIreg(sensor_SM200[counterMBRead]->getAdress(), 30000, sensor, SensorSM200::SIZE, cbRead);
      }
      if (sensor_SM200[counterMBRead]->getAdress())
      {
        update_mbmaster();
        if (!sensor[SensorSM200::mberror])
          switch (sensor[SensorSM200::err])
          {
          case 1:
            sensor_SM200[counterMBRead]->setTemperature(sensor[SensorSM200::temper]);
            sensor_SM200[counterMBRead]->setHumidity(sensor[SensorSM200::hum]);
            // Serial.printf("Температура %d - %d\n", counterMBRead, sensor[SensorSM200::temper]);
          default:
            sensor_SM200[counterMBRead]->setStatus(sensor[SensorSM200::err]);
            break;
          }
        else
          sensor_SM200[counterMBRead]->setStatus(sensor[SensorSM200::mberror]);
        break;
      }
    }
    counterMBRead > 4 ? counterMBRead = 0 : counterMBRead++;
    vTaskDelay(TIME_UPDATE_MODBUS / portTICK_PERIOD_MS);
  }
}

// регулировка окон
void updateGreenHouse(void *pvParameters)
{
  for (;;)
  {
    if (millis() > 100000)
    {
      if (!Rain.getAdress() || Rain.getRaiLevel() < RAIN_MAX) //  если не идет сильный дождь - регулируем  окна
      {
        int t_out_door = 0;
        if (OutDoorTemperature.getAdress())
        {
          if (OutDoorTemperature.getStatus())
            t_out_door = OutDoorTemperature.getTempVector();
        }
        for (Teplica *t : arr_Tepl)
          if (t->getSensorStatus() && t->getThereAreWindows())
            t->regulationWindow(t->getTemperature(), t_out_door);
      }
    }
    vTaskDelay(TIME_UPDATE_GREENOOUSE / portTICK_PERIOD_MS);
  }
}

//парсинг полученых данных от дисплея Nextion
void analyseString(String incStr)
{
  // Serial.printf("Nextion send - %s\n", incStr);
  for (int i = 0; i < incStr.length(); i++)
  {
    if (incStr.substring(i).startsWith("page0")) //
      pageNextion = "p0";
    if (incStr.substring(i).startsWith("pageSet")) //
    {
      uint16_t temp = uint16_t(incStr.substring(i + 7, i + 8).toInt());
      if (temp == Tepl4.getId())
        pageNextion = "p1_0";
      if (temp == Tepl5.getId())
        pageNextion = "p1_1";
      if (temp == Tepl6.getId())
        pageNextion = "p1_2";
      if (temp == Tepl7.getId())
        pageNextion = "p1_3";
      // Serial.println(pageNextion);
    }
    if (incStr.substring(i).startsWith("page2")) //
      pageNextion = "p2";

    if (incStr.substring(i).startsWith("m4on")) //  переключение режим теплица 4 автомат - ручной
    {
      Tepl4.getMode() ? Tepl4.setMode(Teplica::AUTO) : Tepl4.setMode(Teplica::MANUAL);
      slave.Hreg(wifi_mode_4, Tepl4.getMode());
      return;
    }
    if (incStr.substring(i).startsWith("m5on")) //
    {
      Tepl5.getMode() ? Tepl5.setMode(Teplica::AUTO) : Tepl5.setMode(Teplica::MANUAL);
      return;
    }
    if (incStr.substring(i).startsWith("m6on")) //
    {
      Tepl6.getMode() ? Tepl6.setMode(Teplica::AUTO) : Tepl6.setMode(Teplica::MANUAL);
      return;
    }
    if (incStr.substring(i).startsWith("m7on")) //  переключение режим теплица 7 автомат - ручной
    {
      Tepl7.getMode() ? Tepl7.setMode(Teplica::AUTO) : Tepl7.setMode(Teplica::MANUAL);
      return;
    }
    if (incStr.substring(i).startsWith("w4on")) //  переключение режим теплица 4 вентиляция - автомат
    {
      Tepl4.getMode() == Teplica::AIR ? Tepl4.setMode(Teplica::AUTO) : Tepl4.setMode(Teplica::AIR);
      if (Teplica::AIR == Tepl4.getMode())
      Tepl4.air(AIRTIME, 0);
      return;
    }
    if (incStr.substring(i).startsWith("w5on")) //  переключение режим теплица 5 вентиляция - автомат
    {
      Tepl5.getMode() == Teplica::AIR ? Tepl5.setMode(Teplica::AUTO) : Tepl5.setMode(Teplica::AIR);
      if (Teplica::AIR == Tepl5.getMode())
      Tepl5.air(AIRTIME, 0);
      return;
    }
    if (incStr.substring(i).startsWith("w6on")) //  переключение режим теплица 6 вентиляция - автомат
    {
      Tepl6.getMode() == Teplica::AIR ? Tepl6.setMode(Teplica::AUTO) : Tepl6.setMode(Teplica::AIR);
      if (Teplica::AIR == Tepl6.getMode())
      Tepl6.air(AIRTIME, 0);
      return;
    }
    if (incStr.substring(i).startsWith("w7on")) //  переключение режим теплица 7 вентиляция - автомат
    {
      Tepl7.getMode() == Teplica::AIR ? Tepl7.setMode(Teplica::AIR) : Tepl7.setMode(Teplica::AIR);
      if (Teplica::AIR == Tepl7.getMode())
      Tepl7.air(AIRTIME, 0);
      return;
    }

    if (incStr.substring(i).startsWith("pump4")) //
    {
      Tepl4.getPump() ? Tepl4.setPump(OFF) : Tepl4.setPump(ON);
      return;
    }
    if (incStr.substring(i).startsWith("pump5")) //
    {
      Tepl5.getPump() ? Tepl5.setPump(OFF) : Tepl5.setPump(ON);
      return;
    }
    if (incStr.substring(i).startsWith("pump6")) //
    {
      Tepl6.getPump() ? Tepl6.setPump(OFF) : Tepl6.setPump(ON);
      return;
    }
    if (incStr.substring(i).startsWith("pump7")) //
    {
      Tepl7.getPump() ? Tepl7.setPump(OFF) : Tepl7.setPump(ON);
      return;
    }
    if (incStr.substring(i).startsWith("dh4")) //
    {
      //Tepl4.getHeat() ? Tepl4.setHeat(OFF) : Tepl4.setHeat(ON);
      Tepl4.setMode(Teplica::DECREASE_IN_HUMIDITY);
      Tepl4.decrease_in_humidity(AIRTIME, 0);
      return;
    }
    if (incStr.substring(i).startsWith("dh5")) //
    {
      //Tepl5.getHeat() ? Tepl5.setHeat(OFF) : Tepl5.setHeat(ON);
      Tepl5.setMode(Teplica::DECREASE_IN_HUMIDITY);
      Tepl5.decrease_in_humidity(AIRTIME, 0);
      return;
    }
    if (incStr.substring(i).startsWith("dh")) //
    {
      //Tepl6.getHeat() ? Tepl6.setHeat(OFF) : Tepl6.setHeat(ON);
      Tepl6.setMode(Teplica::DECREASE_IN_HUMIDITY);
      Tepl6.decrease_in_humidity(AIRTIME, 0);
      return;
    }
    if (incStr.substring(i).startsWith("dh")) //
    {
      //Tepl7.getHeat() ? Tepl7.setHeat(OFF) : Tepl7.setHeat(ON);
      Tepl7.setMode(Teplica::DECREASE_IN_HUMIDITY);
      Tepl7.decrease_in_humidity(AIRTIME, 0);
      return;
    }
    if (incStr.substring(i).startsWith("set")) //
    {
      pars_str_set(incStr);
      return;
    }
    if (incStr.substring(i).startsWith("adr")) //
    {
      pars_str_adr(incStr);
      flash.putString("adr", incStr);
    }
    if (incStr.substring(i).startsWith("ss")) // изменение скорости шины связи с датчиками температуры
    {
      uint16_t temp = uint16_t(incStr.substring(i + 2, i + 7).toInt());
      flash.putInt("sspeed", temp);
      Serial2.end();
      Serial2.begin(temp); // Modbus Master
    }
    if (incStr.substring(i).startsWith("ms")) // изменение скорости шины связи с терминалом
    {
      uint16_t temp = uint16_t(incStr.substring(i + 2, i + 7).toInt());
      flash.putInt("mspeed", temp);
      Serial1.end();
      Serial1.begin(temp, SERIAL_8N1, RXDMASTER, TXDMASTER, false); // Modbus Slave
    }
    if (incStr.substring(i).startsWith("wvol")) //
    {
      uint16_t tepl = uint16_t(incStr.substring(i + 4, i + 5).toInt());
      uint16_t level = uint16_t(incStr.substring(i + 5, i + 8).toInt());
      switch (tepl)
      {
      case 4:
        slave.Hreg(wifi_setWindow_4, level);
        Tepl4.setWindowlevel(level);
        break;
      case 5:
        slave.Hreg(wifi_setWindow_5, level);
        Tepl5.setWindowlevel(level);
        break;
      case 6:
        slave.Hreg(wifi_setWindow_6, level);
        Tepl6.setWindowlevel(level);
        break;
      case 7:
        slave.Hreg(wifi_setWindow_7, level);
        Tepl7.setWindowlevel(level);
        break;
      default:
        break;
      }
      return;
    }
  }
}

// отправка на Nextion
void sendNextion(String dev, String data)
{
  dev += "=\"" + data + "\"";
  SerialNextion.print(dev);
  uint8_t arj[] = {0xff, 0xff, 0xff};
  SerialNextion.write(arj, 3);
}

// отправка на Nextion
void sendNextion(String dev, int data)
{
  dev += "=" + String(data);
  SerialNextion.print(dev);
  uint8_t arj[] = {0xff, 0xff, 0xff};
  SerialNextion.write(arj, 3);
}

// отправка на Nextion
void sendNextion(String dev)
{
  SerialNextion.print(dev);
  uint8_t arj[] = {0xff, 0xff, 0xff};
  SerialNextion.write(arj, 3);
}

void pars_str_set(String &str)
{
  int j = 0;
  String st = "";
  for (int i = 3; i < str.length(); i++)
  {
    if (str.charAt(i) == '.' || str.charAt(i) == '\n')
    {
      arr_set[j] = st.toInt();
      // Serial.println(arr_set[j]);
      j++;
      st = "";
    }
    else
      st += str.charAt(i);
  }
  arr_Tepl[arr_set[0] - 4]->setSetPump(arr_set[1] * 10);
  flash.putUInt(String("SetPump" + String(arr_set[0])).c_str(), arr_set[1] * 10);
  arr_Tepl[arr_set[0] - 4]->setSetHeat(arr_set[2] * 10);
  flash.putUInt(String("SetHeat" + String(arr_set[0])).c_str(), arr_set[2] * 10);

  arr_Tepl[arr_set[0] - 4]->setSetWindow(arr_set[3] * 10);
  flash.putUInt(String("SetSetWindow" + String(arr_set[0])).c_str(), arr_set[3] * 10);

  arr_Tepl[arr_set[0] - 4]->setWindowlevel(arr_set[4]);
  arr_Tepl[arr_set[0] - 4]->setHysteresis(arr_set[5]);
  flash.putUInt(String("Hyster" + String(arr_set[0])).c_str(), arr_set[5]);

  arr_Tepl[arr_set[0] - 4]->setOpenTimeWindow(arr_set[6]);
  flash.putUInt(String("Opentwin" + String(arr_set[0])).c_str(), arr_set[6]);
}

void pars_str_adr(String &str)
{
  int j = 0;
  String st = "";
  for (int i = 3; i < str.length(); i++)
  {
    if (str.charAt(i) == '.' || str.charAt(i) == '\n')
    {
      arr_adr[j] = st.toInt();
      // Serial.println(arr_adr[j]);
      j++;
      st = "";
    }
    else
      st += str.charAt(i);
  }
  Tepl4.setAdress(arr_adr[0]);
  sendNextion("page2.n0.val", arr_adr[0]);
  Tepl4.setCorrectionTemp(10 * arr_adr[1]);
  sendNextion("page2.t0.txt", String(arr_adr[1]));
  Tepl5.setAdress(arr_adr[2]);
  sendNextion("page2.n1.val", arr_adr[2]);
  Tepl5.setCorrectionTemp(10 * arr_adr[3]);
  sendNextion("page2.t1.txt", String(arr_adr[3]));
  Tepl6.setAdress(arr_adr[4]);
  sendNextion("page2.n2.val", arr_adr[4]);
  Tepl6.setCorrectionTemp(10 * arr_adr[5]);
  sendNextion("page2.t2.txt", String(arr_adr[5]));
  Tepl7.setAdress(arr_adr[6]);
  sendNextion("page2.n3.val", arr_adr[6]);
  Tepl7.setCorrectionTemp(10 * arr_adr[7]);
  sendNextion("page2.t3.txt", String(arr_adr[7]));
  OutDoorTemperature.setAdress(arr_adr[8]);
  sendNextion("page2.n6.val", arr_adr[8]);
  OutDoorTemperature.setCorrectionTemp(10 * arr_adr[9]);
  sendNextion("page2.t4.txt", String(arr_adr[9]));
  Rain.setAdress(arr_adr[10]);
  sendNextion("page2.n4.val", arr_adr[10]);
  mb11016p.setAdress(arr_adr[11]);
  sendNextion("page2.n5.val", arr_adr[11]);
}

//заполнение таблицы для передачи (протокол Modbus)
void saveOutModBusArr()
{
  // для терминала
  slave.Hreg(rs485mode4, Tepl4.getMode() | Tepl4.getPump() << 2 | Tepl4.getHeat() << 3 | Tepl4.getSetPump() / 10 << 8);
  slave.Hreg(rs485mode41, Tepl4.getLevel() | (Tepl4.getSetWindow() - Tepl4.getSetPump()) / 100 << 8);
  slave.Hreg(rs485mode5, Tepl5.getMode() | Tepl5.getPump() << 2 | Tepl5.getHeat() << 3 | Tepl5.getSetPump() / 10 << 8);
  slave.Hreg(rs485mode51, Tepl5.getLevel() | (Tepl5.getSetWindow() - Tepl5.getSetPump()) / 100 << 8);
  slave.Hreg(rs485mode6, Tepl6.getMode() | Tepl6.getPump() << 2 | Tepl6.getHeat() << 3 | Tepl6.getSetPump() / 10 << 8);
  slave.Hreg(rs485mode61, Tepl6.getLevel() | (Tepl6.getSetWindow() - Tepl6.getSetPump()) / 100 << 8);
  slave.Hreg(rs485mode7, Tepl7.getMode() | Tepl7.getPump() << 2 | Tepl7.getHeat() << 3 | Tepl7.getSetPump() / 10 << 8);
  slave.Hreg(rs485mode71, Tepl7.getLevel() | (Tepl7.getSetWindow() - Tepl7.getSetPump()) / 100 << 8);
  slave.Hreg(rs485settings, Rain.getAdress() | OutDoorTemperature.getAdress() << 1 | Tepl4.getOpenTimeWindow() << 5);
  slave.Hreg(rs485temperature4, Tepl4.getTemperature() / 10);
  slave.Hreg(rs485temperature5, Tepl5.getTemperature() / 10);
  slave.Hreg(rs485temperature6, Tepl6.getTemperature() / 10);
  slave.Hreg(rs485temperature7, Tepl7.getTemperature() / 10);
  slave.Hreg(rs485temperatureoutdoor, OutDoorTemperature.getTemperature() / 10);
  slave.Hreg(rs485humidity45, Tepl4.getHumidity() / 100 | Tepl5.getHumidity() / 100 << 8);
  slave.Hreg(rs485humidity67, Tepl6.getHumidity() / 100 | Tepl7.getHumidity() / 100 << 8);
  slave.Hreg(rs485humidityoutdoor, OutDoorTemperature.getHumidity() / 100);
  slave.Hreg(rs485error45, Tepl4Temperature.getStatus() | Tepl5Temperature.getStatus() << 8);
  slave.Hreg(rs485error67, Tepl6Temperature.getStatus() | Tepl7Temperature.getStatus() << 8);
  slave.Hreg(rs485error8, OutDoorTemperature.getStatus() | Tepl4.getHysteresis() / 10 << 8);
  slave.Hreg(rs485rain, Rain.getRaiLevel());

  // для SCADA
  int i = 0;
  for (Teplica *t : arr_Tepl)
  {
    slaveWiFi.Ireg(WiFimode4 + i, t->getMode());
    slaveWiFi.Ireg(WiFipump4 + i, t->getPump());
    slaveWiFi.Ireg(WiFiheat4 + i, t->getHeat());
    slaveWiFi.Ireg(WiFisetpump4 + i, t->getSetPump());
    slaveWiFi.Ireg(WiFitemperature4 + i, t->getTemperature());
    slaveWiFi.Ireg(WiFihumidity4 + i, t->getHumidity());
    slaveWiFi.Ireg(WiFierror4 + i, t->getSensorStatus());
    slaveWiFi.Ireg(WiFiLevel4 + i, t->getLevel());
    slaveWiFi.Ireg(WiFisetwindow4 + i, t->getSetWindow());
    slaveWiFi.Ireg(WiFisetheat4 + i, t->getSetHeat());
    slaveWiFi.Ireg(WiFiHysteresis4 + i, t->getHysteresis());
    slaveWiFi.Ireg(WiFiOpenTimeWindow4 + i, t->getOpenTimeWindow());
    i += 12;
  }
  if (OutDoorTemperature.getAdress())
  {
    slaveWiFi.Ireg(WiFitemperatureoutdoor, OutDoorTemperature.getTemperature());
    slaveWiFi.Ireg(WiFihumidityoutdoor, OutDoorTemperature.getHumidity());
    slaveWiFi.Ireg(WiFierroroutdoor, OutDoorTemperature.getStatus());
  }
  else
  {
    slaveWiFi.Ireg(WiFitemperatureoutdoor, 0xffff);
    slaveWiFi.Ireg(WiFihumidityoutdoor, 0xffff);
    slaveWiFi.Ireg(WiFierroroutdoor, 0);
  }
  if (Rain.getAdress())
    slaveWiFi.Ireg(WiFirain, Rain.getRaiLevel());
  else
    slaveWiFi.Ireg(WiFirain, 0xfffa);
}

void update_mbmaster()
{
  unsigned long t = millis() + 420;
  while (t > millis())
  {
    mb_master.task();
    yield();
    // delay(5);
  }
}
void update_WiFiConnect()
{
  /*----настройка Wi-Fi---------*/
  int counter_WiFi = 0;
  while (WiFi.status() != WL_CONNECTED && counter_WiFi < 10)
  {
    WiFi.disconnect();
    WiFi.reconnect();
    // Serial.println("Reconecting to WiFi..");
    counter_WiFi++;
    delay(1000);
  }
  // if (WiFi.status() == WL_CONNECTED)
  //   Serial.printf("Connect to:\t%s\n", ssid);
  // else
  //   Serial.printf("Dont connect to: %s\n", ssid);
}

/*--------------------------------------- изменения параметров с компьютера ------------------------------------*/
void controlScada()
{
  int number = 0xfff;
  if (slave.Hreg(wifi_flag_edit_4))
  {
    number = 0;
    slave.Hreg(wifi_flag_edit_4, 0);
  }
  if (slave.Hreg(wifi_flag_edit_5))
  {
    number = 1;
    slave.Hreg(wifi_flag_edit_5, 0);
  }
  if (slave.Hreg(wifi_flag_edit_6))
  {
    number = 2;
    slave.Hreg(wifi_flag_edit_6, 0);
  }
  if (slave.Hreg(wifi_flag_edit_7))
  {
    number = 3;
    slave.Hreg(wifi_flag_edit_7, 0);
  }
  if (number == 0xfff)
    return;

  int k = (int(wifi_HOLDING_REGS_SIZE) - int(wifi_flag_edit_4)) / 4;
  k *= number;
  arr_Tepl[number]->setSetPump(slave.Hreg(wifi_UstavkaPump_4 + k));
  flash.putUInt(String("SetPump" + String(arr_Tepl[number]->getId())).c_str(), slave.Hreg(wifi_UstavkaPump_4 + k));
  arr_Tepl[number]->setSetHeat(slave.Hreg(wifi_UstavkaHeat_4 + k));
  flash.putUInt(String("SetHeat" + String(arr_Tepl[number]->getId())).c_str(), slave.Hreg(wifi_UstavkaHeat_4 + k));
  arr_Tepl[number]->setSetWindow(slave.Hreg(wifi_UstavkaWin_4 + k));
  flash.putUInt(String("SetSetWindow" + String(arr_Tepl[number]->getId())).c_str(), slave.Hreg(wifi_UstavkaWin_4 + k));
  arr_Tepl[number]->setMode(slave.Hreg(wifi_mode_4 + k));
  if (arr_Tepl[number]->getMode() == Teplica::MANUAL)
  {
    arr_Tepl[number]->setWindowlevel(slave.Hreg(wifi_setWindow_4 + k));
    arr_Tepl[number]->setPump(slave.Hreg(wifi_pump_4 + k));
    arr_Tepl[number]->setHeat(slave.Hreg(wifi_heat_4 + k));
  }
  if (arr_Tepl[number]->getMode() == Teplica::AIR)
    arr_Tepl[number]->air(AIRTIME, 0);
  if (arr_Tepl[number]->getMode() == Teplica::DECREASE_IN_HUMIDITY)
    arr_Tepl[number]->decrease_in_humidity(AIRTIME, 0);

  arr_Tepl[number]->setHysteresis(slave.Hreg(wifi_hysteresis_4 + k));
  flash.putUInt(String("Hyster" + String(arr_Tepl[number]->getId())).c_str(), slave.Hreg(wifi_hysteresis_4 + k));
  arr_Tepl[number]->setOpenTimeWindow(slave.Hreg(wifi_time_open_windows_4 + k));
  flash.putUInt(String("Opentwin" + String(arr_Tepl[number]->getId())).c_str(), slave.Hreg(wifi_time_open_windows_4 + k));
}

#ifdef USE_WEB_SERIAL
void webSerialSend(void *pvParameters)
{
  for (;;)
  {
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void recvMsg(uint8_t *data, size_t len)
{
  WebSerial.println("Received Data...");
  String d = "";
  for (int i = 0; i < len; i++)
  {
    d += char(data[i]);
  }
  WebSerial.println(d);
}
#endif

String calculateTimeWork()
{
  String str = "\nDuration of work: ";
  unsigned long currentMillis = millis();
  unsigned long seconds = currentMillis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  currentMillis %= 1000;
  seconds %= 60;
  minutes %= 60;
  hours %= 24;
  if (days > 0)
  {
    str += String(days) + " d ";
    str += String(hours) + " h ";
    str += String(minutes) + " min ";
  }
  else if (hours > 0)
  {
    str += String(hours) + " h ";
    str += String(minutes) + " min ";
  }
  else if (minutes > 0)
    str += String(minutes) + " min ";
  str += String(seconds) + " sec";
  return str;
}