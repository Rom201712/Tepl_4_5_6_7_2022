#pragma once

#define ON HIGH
#define OFF LOW

class Window
{
private:

    int _changelevel;         // количество шагов закрытия/открытия окна (>0 - открытие, <0 - закрытие)
    unsigned long _timeopen;  // время выключения открытия окон
    unsigned long _timeclose; // время выключения закрытия окон
    int _level = 0;           // уровень открытия
    int _opentimewindow;      // полное время  работы механизма, сек
    int _relayUp, _relayDown;
    MB11016P_ESP *__relay;

public:
    Window(MB11016P_ESP *mb11016p, int relayUp, int relayDown, int opentimewindow) : _opentimewindow(opentimewindow),  _relayUp(relayUp), _relayDown(relayDown)
    {
        __relay = mb11016p;
    }

    int getlevel() const
    {
        return _level;
    }
    //изменение времени работы механизма
    void setopentimewindow(int opentimewindow)
    {
        if (0 == _level && !getWindowDown())
            _opentimewindow = opentimewindow;
    }

    int getWindowUp() const
    {
        return __relay->getRelay(_relayUp);
    }

    int getWindowDown() const
    {
        return __relay->getRelay(_relayDown);
    }

    int getOpenTimeWindow() const
    {
        return _opentimewindow;
    }

    //включение механизма открытия окна
    void openWindow(int changelevel)
    {
        if (changelevel + _level > 100)
            changelevel = 100 - _level;
        if (__relay->getRelay(_relayUp) == OFF && __relay->getRelay(_relayDown) == OFF)
        {
            __relay->setOn(_relayUp);
            _timeopen = millis() + 10 * (static_cast<unsigned long>(changelevel * _opentimewindow));
            _level = constrain(_level + changelevel, 0, 100);
        }
    }
    //включение механизма закрытия окна
    void closeWindow(int changelevel)
    {
        if (__relay->getRelay(_relayUp) == OFF && __relay->getRelay(_relayDown) == OFF)
        {
            __relay->setOn(_relayDown);
            _timeclose = millis() + 10 * (static_cast<unsigned long>(changelevel * _opentimewindow));
            _level = constrain(_level - changelevel, 0, 100);
        }
    }

    //выключение механизма открытия и закрытия
    void off() const
    {
        if (_timeopen < millis())
            __relay->setOff(_relayUp);
        if (_timeclose < millis())
            __relay->setOff(_relayDown);
    }
    unsigned long getOpenTime() const
    {
        return _timeopen;
    }
    unsigned long getCloseTime() const
    {
        return _timeclose;
    }
};