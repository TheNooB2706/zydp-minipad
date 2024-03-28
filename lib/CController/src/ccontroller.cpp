#include "ccontroller.hpp"
#include <Arduino.h>
#include <cmath>

CController::CController(int pin_num, float threshold_percent, int midi_cc_num) {
    pin = pin_num;
    pinMode(pin, INPUT);

    _last_reading = analogRead(pin);

    _delta_threshold = round(4096*threshold_percent/100);

    _midi_cc_num = midi_cc_num;
}

CController::CController(int pin_num, float threshold_percent) : CController(pin_num, threshold_percent, 0) {}

int CController::poll() {
    int cur_reading = analogRead(pin);
    if (abs(cur_reading-_last_reading) > _delta_threshold) {
        on_change(cur_reading);
        _last_reading = cur_reading;
        return 1;
    }
    else {
        return 0;
    }
}

int CController::poll(unsigned int sample_period_micro) {
    if ((micros()-_last_sample_time) > sample_period_micro) {
        _last_sample_time = micros();
        return poll();
    }
    else {
        return 0;
    }    
}

int CController::get_cc_num() {
    return _midi_cc_num;
}

void CController::set_cc_num(int new_cc_num) {
    _midi_cc_num = new_cc_num;
}

void CController::on_change(int controller_input) {}