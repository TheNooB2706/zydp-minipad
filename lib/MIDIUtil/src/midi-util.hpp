#define SNARE_GM2 38
#define BASS_DRUM_GM2 36
#define HIHAT_CLOSED_GM2 42
#define PEDAL_HIHAT_GM2 44
#define HIHAT_OPEN_GM2 46

/// @brief Map raw pad input to MIDI velocity linearly
/// @param input Raw input from analogRead
/// @param lower_input Lower boundary of input value, anything lower than this will be treated as 0 velocity
/// @param upper_input Upper boundary of input value, anything higher than this will be treated as 127 velocity
/// @return MIDI velocity
int midi_lin_vel_map(int input, int lower_input, int upper_input);

/// @brief Map raw pad input to MIDI velocity linearly. This overload set lower_input=0 and upper_input=4096 as per full range of 12 bit ADC on Blue pill
/// @param input Raw input from analogRead 
/// @return MIDI velocity
int midi_lin_vel_map(int input);

/// @brief Map raw pad input to MIDI velocity using exponential equation -e^(-ax+ln(127))+127
/// @param input Raw input from analogRead
/// @param a Coefficient a in the equation
/// @return MIDI velocity
int midi_exp_vel_map(int input, double a);

/// @brief Map raw pad input to MIDI velocity using exponential equation -e^(-ax^b+ln(127))+127
/// @param input Raw input from analogRead
/// @param a Coefficient a in the equation
/// @param b Power that the input is raised to
/// @return MIDI velocity
int midi_exp_pow_vel_map(int input, double a, double b);