// релейный модуль
#include <ModbusRTU.h>


bool _mb11016[17];
bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void *data)
{
    // Serial.printf("MB110-16P result:\t0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
    event ? _mb11016[16] = 1 : _mb11016[16] = 0;
    return true;
}
class MB11016P_ESP
{
private:
    uint8_t _netadress; // адрес устройства в сети Modbus)
    int _errmodbus, _quantity;
    ModbusRTU *__mb11016p;

public:
    MB11016P_ESP(ModbusRTU *master, uint8_t netadress, int quantity) : _netadress(netadress), _quantity(quantity), __mb11016p(master)
    {
        for (int i = 0; i < 17; i++)
            _mb11016[i] = 0;
    }

    // запись в блок МВ110-16Р (включение реле управления)
    void write()
    {
        __mb11016p->writeCoil(_netadress, 0, _mb11016, 16, cbWrite);
    }
    void setOn(int relay)
    {
        _mb11016[relay] = 1;
    }
    void setOff(int relay)
    {
        _mb11016[relay] = 0;
    }

    bool getRelay(int relay)
    {
        return _mb11016[relay];
    }
    bool getError()
    {
        return _mb11016[16];
    }
    void setAdress(int adr)
    {
        _netadress = adr;
    }
};