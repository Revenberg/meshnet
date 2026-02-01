#include "NodeButton.h"

NodeButton::NodeButton(uint8_t pin)
    : _pin(pin), _lastState(false), _pressed(false), _lastDebounceTime(0),
      _lastPressTime(0), _lastReleaseTime(0), _clickCount(0),
      _singleClickFlag(false), _doubleClickFlag(false), _longPressFlag(false) {}

void NodeButton::begin() {
    pinMode(_pin, INPUT_PULLUP);
}

void NodeButton::update() {
    bool currentState = !digitalRead(_pin); // active low
    unsigned long now = millis();

    if (currentState != _lastState) {
        _lastDebounceTime = now;
    }

    if ((now - _lastDebounceTime) > 50) { // debounce
        if (currentState && !_pressed) {
            _pressed = true;
            _lastPressTime = now;
            _clickCount++;
        } else if (!currentState && _pressed) {
            _pressed = false;
            _lastReleaseTime = now;
            if ((now - _lastPressTime) > 800) {
                _longPressFlag = true;
            } else if (_clickCount == 1) {
                _singleClickFlag = true;
            } else if (_clickCount == 2) {
                _doubleClickFlag = true;
                _clickCount = 0;
            }
        }
    }
    _lastState = currentState;
}

bool NodeButton::isPressed() const {
    return _pressed;
}

bool NodeButton::isSingleClick() {
    if (_singleClickFlag) {
        _singleClickFlag = false;
        _clickCount = 0;
        return true;
    }
    return false;
}

bool NodeButton::isDoubleClick() {
    if (_doubleClickFlag) {
        _doubleClickFlag = false;
        return true;
    }
    return false;
}

bool NodeButton::isLongPress() {
    if (_longPressFlag) {
        _longPressFlag = false;
        return true;
    }
    return false;
}
