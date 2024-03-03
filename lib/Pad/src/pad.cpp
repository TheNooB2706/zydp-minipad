#include <pad.hpp>

Pad::Pad(int pin_num, float threshold_high, float threshold_low, int buffer_size) : buffer(buffer_size) {
    pin = pin_num;
    pinMode(pin, INPUT);
    while (!buffer.bufferIsFull()) {
    buffer.add(analogRead(pin));
    }

    _threshold_high = threshold_high;
    _threshold_low = threshold_low;
}

int Pad::poll() {
    buffer.add(analogRead(pin));
    float current_avg = buffer.getAverage();
    if ((current_avg > _threshold_high) && (_cooldown == 0) && !_state) {
        _cooldown = _cooldown_time;
        _state = true;
        return 1;
    }
    else if ((current_avg < _threshold_low) && (_cooldown == 0) && _state) {
        _state = false;
        return 2;
    }
    else if ((current_avg < _threshold_low) && (_cooldown != 0)) {
        _cooldown -= 1;
        return 3;
    }
    else if (!(current_avg < _threshold_low) && (_cooldown != 0)) {
        _cooldown = _cooldown_time;
        return 0;
    }
    else {
        return 0;
    }
}

float Pad::get_max() {
    return buffer.getMaxInBuffer();
}

bool Pad::get_state() {
    return _state;
}