#include "led-indicator.hpp"
#include <Arduino.h>

LEDIndicator::LEDIndicator(int red_pin, int green_pin, int blue_pin) {
    _red_pin = red_pin;
    _green_pin = green_pin;
    _blue_pin = blue_pin;

    pinMode(red_pin, OUTPUT);
    pinMode(green_pin, OUTPUT);
    pinMode(blue_pin, OUTPUT);

    off();
}

void LEDIndicator::on(int red, int green, int blue) {
    _on(red, green, blue);
    _blink = false;
}

void LEDIndicator::_on(int red, int green, int blue) {
    digitalWrite(_red_pin, red);
    digitalWrite(_green_pin, green);
    digitalWrite(_blue_pin, blue);
    _red_state = red;
    _green_state = green;
    _blue_state = blue;
    _on_state = true;
}

void LEDIndicator::_on() {
    _on(_red_state, _green_state, _blue_state);
}

void LEDIndicator::off() {
    _off();
    _blink = false;
}

void LEDIndicator::_off() {
    digitalWrite(_red_pin, LOW);
    digitalWrite(_green_pin, LOW);
    digitalWrite(_blue_pin, LOW);
    _on_state = false;
}

void LEDIndicator::blink(int red, int green, int blue, int cycles, int period_millis, bool return_to_last_state) {
    _blink_return_to_last_state = return_to_last_state;
    if (return_to_last_state) {
        _last_red_state = _red_state;
        _last_green_state = _green_state;
        _last_blue_state = _blue_state;
        if (!_blink) {
            _last_on_state = _on_state;
        }
        else {
            _last_on_state = false;
        }
    }
    
    _blink = true;
    _cycles = cycles;
    _blink_period = period_millis/2;
    
    _completed_cycles = 0;
    _on(red, green, blue);
    _last_timestamp = millis();
}

bool LEDIndicator::is_on() {
    return _on_state;
} 

void LEDIndicator::poll() {
    if (_blink && ((millis() - _last_timestamp) > (_blink_period)) && (_completed_cycles < _cycles)) {
        if (is_on()) {
            _off();
            _last_timestamp = millis();
        }
        else {
            if (_completed_cycles == (_cycles-1)) {
                _blink = false;
                if (_blink_return_to_last_state && _last_on_state) {
                    on(_last_red_state, _last_green_state, _last_blue_state);
                }
            }
            else {
                _on();
            }
            _completed_cycles++;
            _last_timestamp = millis();
        }
    }
}