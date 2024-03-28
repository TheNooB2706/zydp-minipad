#include <stdint.h>

class CController {
    public:

        uint8_t pin;

        CController(int pin_num, float threshold_percent);
        CController(int pin_num, float threshold_percent, int midi_cc_num);

        int poll();

        int poll(unsigned int sample_period_micro);

        /// @brief Get MIDI CC number assigned to this pad.
        int get_cc_num();

        /// @brief Assign new MIDI CC number to this pad.
        void set_cc_num(int new_cc_num);

        // Overload the following function to be executed in poll when controller changed.
        virtual void on_change(int controller_input);

    private:

        int _last_reading;
        int _delta_threshold;
        uint32_t _last_sample_time = 0;
        int _midi_cc_num;

};