#include <RunningAverage.h>

class Pad {
    public:

        uint8_t pin;
        RunningAverage buffer;

        /// @param pin_num Analog pin for the pad.
        /// @param threshold_high High-going threshold. Used to decide if a trigger occured.
        /// @param threshold_low Low-going threshold. Used to determine if a pad is fully cool-down.
        /// @param buffer_size Buffer size of buffer storing the analogRead value from the pin_num.
        Pad(int pin_num, float threshold_high, float threshold_low, int buffer_size);
        Pad(int pin_num, float threshold_high, float threshold_low, int buffer_size, int midi_note_num);

        /// @brief Poll the pad by acquiring analogRead value and appending to the buffer.
        /// @retval 1: Trigger is detected
        /// @retval 2: Pad is fully cool-down
        /// @retval 0: Otherwise
        int poll();  

        /// @brief Same as poll function, but with a delay of sampling period between polling.
        /// @param sample_period_micro Sampling period in microseconds
        int poll(uint sample_period_micro);

        /// @brief Get the peak level of the signal in the buffer.
        int get_max();

        /// @brief The state of the pad will be true for the duration between trigger is detected to stable state is reached. Otherwise, it will be false.
        bool get_state();

        /// @brief Get MIDI note number assigned to this pad.
        int get_note_num();

        /// @brief Assign new MIDI note number to this pad.
        void set_note_num(int new_note_num);

        // Overload the following functions to be executed in poll when pad triggered and fully cool-down.
        virtual void on_trigger(int pad_input);
        virtual void on_cooldown();

    private:

        int _cooldown = 0;
        bool _state = false;
        float _threshold_high;
        float _threshold_low;
        int _midi_note_num;
        uint32 _last_sample_time = 0;
        const int _cooldown_time = 32;

};

class MuxPad: public Pad {
    public:

        MuxPad(int pin_num, float threshold_high, float threshold_low, int buffer_size, const int select_pins[4], int mux_address);
        MuxPad(int pin_num, float threshold_high, float threshold_low, int buffer_size, int midi_note_num, const int select_pins[4], int mux_address);

        int poll(); 

        int poll(uint sample_period_micro);

        void set_mux_address(int mux_address);

        void set_sel_pins(const int select_pins[4]);

    private:

        int _select_pins[4];
        int _mux_address;

        void _set_mux_address();

};