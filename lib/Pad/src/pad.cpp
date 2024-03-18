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

MuxPad::MuxPad(int pin_num, float threshold_high, float threshold_low, int buffer_size, int midi_note_num, const int select_pins[4], int mux_address) : Pad(pin_num, threshold_high, threshold_low, buffer_size, midi_note_num) {
    set_sel_pins(select_pins);
    _mux_address = mux_address;
}

MuxPad::MuxPad(int pin_num, float threshold_high, float threshold_low, int buffer_size, const int select_pins[4], int mux_address) : MuxPad(pin_num, threshold_high, threshold_low, buffer_size, 0, select_pins, mux_address) {}

void MuxPad::set_mux_address(int mux_address) {
    _mux_address = mux_address;
}

void MuxPad::set_sel_pins(const int select_pins[4]) {
    for (size_t i=0; i<4; i++) {
        _select_pins[i] = select_pins[i];
        pinMode(select_pins[i], OUTPUT);
    }
}

int MuxPad::poll() {
    _set_mux_address();
    return Pad::poll();
}

int MuxPad::poll(uint sample_period_micro) {
    _set_mux_address();
    return Pad::poll(sample_period_micro);
}

void MuxPad::_set_mux_address() {
    for (size_t i=0; i<4; i++) {
        digitalWrite(_select_pins[i], bitRead(_mux_address, i));
    }
}