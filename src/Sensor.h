/* 
     SM-200 украинский датчик температуры и влажности (сетевой адрес датчика, коррекция температуры, коррекция влажности)
    
    ошибки
  0 - нет ошибок
  0xFFFF - нет питания
  4 - поиск датчика
  5 - другое
  6 - нет связи с блоком датчика
  
*/
// #include <Arduino.h>
#pragma once

#include <vector>

class Sensor
{
    int _adress;         // id устройства в массиве Modbus
    int _correctiontemp; // величина коррекции датчика температуры
    int _correctionhum;  // величина коррекции датчика влажности
    int _adrreg = 30000; //адрес  регистра в таблице Modbus
    // ModbusRTU *__sensor;
    int _datesensor[4] = {0, 0, 0, 0};
    std::vector<int> _tempvec;
    // std::vector<int> _humvec;

public:

static const uint NO_ERROR = 1;
static const uint  NO_POWER = 0xffff;

     Sensor(int adress, int correctiontemp, int correctionhum) : _adress(adress), _correctiontemp(correctiontemp),
                                                                _correctionhum(correctionhum)
    {
    }
 enum SensorSM200
    {
        status,
        firmware,
        temperature,
        humidity,
        size
    };
    int getStatus()
    {
        return _datesensor[status];
    }
    int getTemperature()
    {
        return _datesensor[temperature];
    }
    int getHumidity()
    {
        return _datesensor[humidity];
    }
    void setAdress(int adr)
    {
        _adress = adr;
    }
    void setTemperature(int16_t t)
    {
        _datesensor[temperature] = t + _correctiontemp;
        _tempvec.push_back(_datesensor[temperature]);
    }
    void setHumidity(int h)
    {
        _datesensor[humidity] = h;
    }
    void setStatus(int s)
    {
        _datesensor[status] = s;
    }

    int getAdress()
    {
        return _adress;
    }

    void setCorrectionTemp(int correctiontemp)
    {
        _correctiontemp = correctiontemp;
    }
    int getTempVector()
    {
        int32_t T = 0;
        for (int i = 0; i < _tempvec.size(); i++)
        {
            T += _tempvec[i];
           
        }
        if (!_tempvec.empty())
            T /= _tempvec.size();
        _tempvec.clear();
        return T;
    }
};