#ifndef NODE_BUTTON_H
#define NODE_BUTTON_H

#include <Arduino.h>

class NodeButton {
public:
    NodeButton(uint8_t pin);
    void begin();
    void update();
    bool isPressed() const;
    bool isSingleClick();
    bool isDoubleClick();
    bool isLongPress();

private:
    uint8_t _pin;
    bool _lastState;
    bool _pressed;
    unsigned long _lastDebounceTime;
    unsigned long _lastPressTime;
    unsigned long _lastReleaseTime;
    int _clickCount;
    bool _singleClickFlag;
    bool _doubleClickFlag;
    bool _longPressFlag;
};

#endif // NODE_BUTTON_H
