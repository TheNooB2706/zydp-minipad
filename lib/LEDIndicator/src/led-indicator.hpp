/// @brief Class for common cathode RGB LED
class LEDIndicator {
    public:

        /// @param red_pin Pin number connected to red lead
        /// @param green_pin Pin number connected to green lead
        /// @param blue_pin Pin number connected to blue lead
        LEDIndicator(int red_pin, int green_pin, int blue_pin);

        /// @brief Turn on the indicator LED with the provided red, green and blue color state
        void on(int red, int green, int blue);

        /// @brief  Turn off the indicator LED
        void off();

        /// @brief Start blinking the indicator LED
        /// @param red The color of blinking LED
        /// @param green The color of blinking LED
        /// @param blue The color of blinking LED
        /// @param cycles Number of cycles
        /// @param period_millis Period of the blink
        /// @param return_to_last_state Whether or not to return to the original state after finish blinking
        void blink(int red, int green, int blue, int cycles, int period_millis, bool return_to_last_state);

        /// @brief Check if the LED is currently on or not
        bool is_on();

        /// @brief This function must be ran if blink is to be used
        void poll();

    private:

        int _red_pin;
        int _green_pin;
        int _blue_pin;

        int _red_state;
        int _green_state;
        int _blue_state;
        bool _on_state;

        int _last_red_state;
        int _last_green_state;
        int _last_blue_state;
        bool _last_on_state;

        bool _blink;
        bool _blink_return_to_last_state;
        int _blink_period;
        int _completed_cycles;
        int _cycles;
        unsigned long _last_timestamp;
        
        void _on(int red, int green, int blue);
        void _on();

        void _off();

};