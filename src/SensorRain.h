#pragma once

#include <vector>

class SensorRain
{
    

public:
static const uint NO_ERROR = 1;
static const uint ERROR = 0xffff;
enum SensorRainData
    {
        level,
        temperature,
        size
    };
    SensorRain(int adress) : _adress(adress)
    {
    }
    void setRain(uint16_t r)
    {
        _datesensor[level] = r;
    }
    void setTemperature(uint16_t r)
    {
        _datesensor[temperature] = r;
    }
    void setAdress(int adr)
    {
        _adress = adr;
    }
    int getRaiLevel()
    {
        return _datesensor[level];
    }
    int getTemperature()
    {
        return _datesensor[temperature] + _correctiontemp;
    }

    int getAdress()
    {
        return _adress;
    }
    void setStatus( int r) {
        _status = r;
    }
    
    int getStatus() {
        return _status;
    }
    void setCorrectionTemp(int correctiontemp)
    {
        _correctiontemp = correctiontemp;
    }
    private:
    int _adress;         // id устройства в массиве Modbus
    int _adrreg = 30000; //адрес  регистра в таблице Modbus
    int _datesensor[size] = {0};
    int _status = ERROR;
    int _correctiontemp = 0;
    std::vector<int> _rainvec;
};