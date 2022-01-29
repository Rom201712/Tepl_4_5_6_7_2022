
#include <vector>

class SensorRain
{
    enum _Sensor
    {
        _level,
        _SIZE
    };

    int _adress;         // id устройства в массиве Modbus
    int _adrreg = 30000; //адрес  регистра в таблице Modbus
    int _datesensor[1] = {0};
    std::vector<int> _rainvec;

public:
    SensorRain(int adress) : _adress(adress)
    {
    }
    void setRain(uint16_t r)
    {
        _datesensor[_level] = r;
    }
    void setAdress(int adr)
    {
        _adress = adr;
    }
    int getRaiLevel()
    {
        return _datesensor[_level];
    }

    int getAdress()
    {
        return _adress;
    }
};