#include <pad.hpp>

Pad::Pad(int pin_num, float threshold_high, float threshold_low, int buffer_size, int midi_note_num) : buffer(buffer_size) {
    pin = pin_num;
    pinMode(pin, INPUT);
    while (!buffer.bufferIsFull()) {
        buffer.add(analogRead(pin));
    }

    _threshold_high = threshold_high;
    _threshold_low = threshold_low;
    _midi_note_num = midi_note_num;
}

Pad::Pad(int pin_num, float threshold_high, float threshold_low, int buffer_size) : Pad(pin_num, threshold_high, threshold_low, buffer_size, 0) {}

int Pad::poll() {
    buffer.add(analogRead(pin));
    float current_avg = buffer.getAverage();
    if ((current_avg > _threshold_high) && (_cooldown == 0) && !_state) {
        _cooldown = _cooldown_time;
        _state = true;
        on_trigger(get_max());
        return 1;
    }
    else if ((current_avg < _threshold_low) && (_cooldown == 0) && _state) {
        _state = false;
        on_cooldown();
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

int Pad::poll(uint sample_period_micro) {
    if ((micros()-_last_sample_time) > sample_period_micro) {
        _last_sample_time = micros();
        return poll();
    }
    else {
        return 0;
    }
}

int Pad::get_max() {
    return buffer.getMaxInBuffer();
}

bool Pad::get_state() {
    return _state;
}

int Pad::get_note_num() {
    return _midi_note_num;
}

void Pad::set_note_num(int new_note_num) {
    _midi_note_num = new_note_num;
}

void Pad::on_trigger(int pad_input) {}
void Pad::on_cooldown() {}