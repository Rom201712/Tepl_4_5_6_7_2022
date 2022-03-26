class Window
{
private:
#define ON HIGH
#define OFF LOW
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

    int getlevel()
    {
        return _level;
    }
    //изменение времени работы механизма
    void setopentimewindow(int opentimewindow)
    {
        if (0 == _level && !getWindowDown())
            _opentimewindow = opentimewindow;
    }

    int getWindowUp()
    {
        return __relay->getRelay(_relayUp);
    }

    int getWindowDown()
    {
        return __relay->getRelay(_relayDown);
    }

    int getOpenTimeWindow()
    {
        return _opentimewindow;
    }

    //включение механизма открытия окна
    void openWindow(int changelevel)
    {
        // if (changelevel)
        if (changelevel + _level > 100)
            changelevel = 100 - _level;
        // if (changelevel)
        if (__relay->getRelay(_relayUp) == 0 && __relay->getRelay(_relayDown) == 0)
        {
            __relay->setOn(_relayUp);
            _timeopen = millis() + 10 * (static_cast<unsigned long>(changelevel * _opentimewindow));
            _level = constrain(_level + changelevel, 0, 100);
        }
    }
    //включение механизма закрытия окна
    void closeWindow(int changelevel)
    {
        // if (changelevel)
        if (__relay->getRelay(_relayUp) == 0 && __relay->getRelay(_relayDown) == 0)
        {
            __relay->setOn(_relayDown);
            _timeclose = millis() + 10 * (static_cast<unsigned long>(changelevel * _opentimewindow));
            _level = constrain(_level - changelevel, 0, 100);
        }
    }

    //выключение механизма открытия и закрытия
    void off()
    {
        if (_timeopen < millis())
        {
            __relay->setOff(_relayUp);
        }
        if (_timeclose < millis())
        {
            __relay->setOff(_relayDown);
        }
    }
    unsigned long getOpenTime()
    {
        return _timeopen;
    }
    unsigned long getCloseTime()
    {
        return _timeclose;
    }
};