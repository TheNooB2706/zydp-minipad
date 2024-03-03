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

        /// @brief Poll the pad by acquiring analogRead value and appending to the buffer.
        /// @retval 1: Trigger is detected
        /// @retval 2: Pad is fully cool-down
        /// @retval 0: Otherwise
        int poll();  

        /// @brief Get the peak level of the signal in the buffer.
        float get_max();

        /// @brief The state of the pad will be true for the duration between trigger is detected to stable state is reached. Otherwise, it will be false.
        bool get_state();

    private:

        int _cooldown = 0;
        bool _state = false;
        float _threshold_high;
        float _threshold_low;
        const int _cooldown_time = 32;
};