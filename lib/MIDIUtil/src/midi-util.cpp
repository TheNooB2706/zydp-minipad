#include "midi-util.hpp"
#include <math.h>

int midi_lin_vel_map(int input, int lower_input, int upper_input) {
    if (input <= lower_input) {
        return 0;
    }
    else if (input >= upper_input) {
        return 127;
    }
    else {
        int delta_range = upper_input - lower_input;
        int delta_input = input - lower_input;
        double result = 127.0*delta_input/delta_range;
        return round(result);
    }
}

int midi_lin_vel_map(int input) {
    return midi_lin_vel_map(input, 0, 4096);
}