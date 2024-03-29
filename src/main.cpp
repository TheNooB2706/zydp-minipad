#include <Arduino.h>
#include <USBComposite.h>
#include <MIDI.h>

#include <pad.hpp>
#include <ccontroller.hpp>
#include <midi-util.hpp>
#include <led-indicator.hpp>

USBMIDI CompositeMIDI;
USBCompositeSerial CompositeSerial;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, UARTMIDI);

// ===== Constants =====

const int PADS_BUFFER_SIZE = 10;
const int PADS_THRESH_HIGH = 100;
const int PADS_THRESH_LOW = 70;
const int SNARE_THRESH_HIGH = 50;
const int SNARE_THRESH_LOW = 70;
const int PADS_SAMPLING_PERIOD = 470;

const int SELECT_PINS[4] = {PB1, PB0, PA7, PA6};
const int MUX_PADS_PIN = PA0;
const int PADS_NOTE_NUM[12] = {43, 41, 36, 41, 43, 47, 38, 47, 49, 46, 42, 51};

const int KICK_THRESH_HIGH = 100;
const int KICK_THRESH_LOW = 70;
const int KICK_PEDAL_PIN = PA1;
const int KICK_NOTE_NUM = BASS_DRUM_GM2;

const int CC_THRESH_CHANGE=1;
const int CC_PEDAL_PIN = PA2;

const int BUTTON1_PIN = PB5;
const int BUTTON2_PIN = PB6;
const int BUTTON3_PIN = PB7;
const int BUTTON4_PIN = PB8;
const int BUTTON5_PIN = PB9;
const int LED_RED_PIN = PB4;
const int LED_GREEN_PIN = PB3;
const int LED_BLUE_PIN = PA15;

const int PADS_TYPE[12] = {0,1,0,1,0,0,2,0,0,0,0,0};
const double VEL_MAP_COEFF_BIG[3] = {0.0048, 0.0025, 0.0012};
const double VEL_MAP_COEFF_SMALL[3] = {0.0025, 0.0012, 0.0006};
const double VEL_MAP_COEFF_SNARE[3] = {0.006, 0.0048, 0.0025};

// ===== Flags =====

bool f_uart_midi_enabled = true;
int f_midi_channel_num = 1;
int f_vel_map_profile = 1;

bool f_cc_ped_enabled = false;
bool f_kick_ped_enabled = true;

// ===== Global functions declaration =====

void global_poll();
int global_poll_return();
void pads_triggered(bool is_triggered, int note_number, int channel_number, int raw_reading, int vel_map_profile, int pad_type);
void send_note_event(bool is_note_on, int note_number, int channel_number, int velocity);
void controller_changed(int cc_number, int channel_number, int raw_reading);
void send_cc_event(int cc_number, int channel_number, int cc_value);

// ===== Pads initialization =====

class MIDIPad: public Pad {
  public:
    MIDIPad(int pin_num, float threshold_high, float threshold_low, int buffer_size, int midi_note_num) : Pad(pin_num, threshold_high, threshold_low, buffer_size, midi_note_num) {}

    void on_trigger(int pad_input) {
      pads_triggered(true, get_note_num(), f_midi_channel_num, pad_input, f_vel_map_profile, 0);
      CompositeSerial.print("Triggered: ");
      CompositeSerial.println(get_max());
    }

    void on_cooldown() {
      pads_triggered(false, get_note_num(), f_midi_channel_num, 0, f_vel_map_profile, 0);
    }
};

class MIDIMuxPad: public MuxPad {
  public:
    MIDIMuxPad(int pin_num, float threshold_high, float threshold_low, int buffer_size, int midi_note_num, const int select_pins[4], int mux_address) : MuxPad(pin_num, threshold_high, threshold_low, buffer_size, midi_note_num, select_pins, mux_address) {}

    void on_trigger(int pad_input) {
      pads_triggered(true, get_note_num(), f_midi_channel_num, pad_input, f_vel_map_profile, PADS_TYPE[get_mux_address()]);
      CompositeSerial.print("Triggered: ");
      CompositeSerial.println(get_max());
    }

    void on_cooldown() {
      pads_triggered(false, get_note_num(), f_midi_channel_num, 0, f_vel_map_profile, PADS_TYPE[get_mux_address()]);
    }

    int debug() {
    for (size_t i=0; i<4; i++) {
        digitalWrite(SELECT_PINS[i], bitRead(get_mux_address(), i));
    }
    delay_us(50);
      return analogRead(pin);
    }
};

MIDIMuxPad pads_array[] = {
  MIDIMuxPad(MUX_PADS_PIN, PADS_THRESH_HIGH, PADS_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[0], SELECT_PINS, 0),
  MIDIMuxPad(MUX_PADS_PIN, PADS_THRESH_HIGH, PADS_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[1], SELECT_PINS, 1),
  MIDIMuxPad(MUX_PADS_PIN, PADS_THRESH_HIGH, PADS_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[2], SELECT_PINS, 2),
  MIDIMuxPad(MUX_PADS_PIN, PADS_THRESH_HIGH, PADS_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[3], SELECT_PINS, 3),
  MIDIMuxPad(MUX_PADS_PIN, PADS_THRESH_HIGH, PADS_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[4], SELECT_PINS, 4),
  MIDIMuxPad(MUX_PADS_PIN, PADS_THRESH_HIGH, PADS_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[5], SELECT_PINS, 5),
  MIDIMuxPad(MUX_PADS_PIN, SNARE_THRESH_HIGH, SNARE_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[6], SELECT_PINS, 6),
  MIDIMuxPad(MUX_PADS_PIN, PADS_THRESH_HIGH, PADS_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[7], SELECT_PINS, 7),
  MIDIMuxPad(MUX_PADS_PIN, PADS_THRESH_HIGH, PADS_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[8], SELECT_PINS, 8),
  MIDIMuxPad(MUX_PADS_PIN, PADS_THRESH_HIGH, PADS_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[9], SELECT_PINS, 9),
  MIDIMuxPad(MUX_PADS_PIN, PADS_THRESH_HIGH, PADS_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[10], SELECT_PINS, 10),
  MIDIMuxPad(MUX_PADS_PIN, PADS_THRESH_HIGH, PADS_THRESH_LOW, PADS_BUFFER_SIZE, PADS_NOTE_NUM[11], SELECT_PINS, 11)
};

MIDIPad kick_pad(KICK_PEDAL_PIN, KICK_THRESH_HIGH, KICK_THRESH_LOW, PADS_BUFFER_SIZE, KICK_NOTE_NUM);

// ===== CC initialization =====

class MIDICController: public CController {
  public:
    MIDICController(int pin_num, float threshold_percent, int midi_cc_num): CController(pin_num, threshold_percent, midi_cc_num) {}

    void on_change(int controller_input) {
      controller_changed(get_cc_num(), f_midi_channel_num, controller_input);
    }
};

MIDICController cc_pedal(CC_PEDAL_PIN, CC_THRESH_CHANGE, 4);

// ===== LED initialization =====

LEDIndicator led(LED_RED_PIN, LED_GREEN_PIN, LED_BLUE_PIN);

// ===== Global functions =====

/// @brief Placeholder function for any polling call
void global_poll() {
  for (size_t i=0; i<12; i++) {
    pads_array[i].poll(PADS_SAMPLING_PERIOD);
  }

  if (f_kick_ped_enabled) {
    kick_pad.poll(PADS_SAMPLING_PERIOD);
  }
  
  if (f_cc_ped_enabled) {
    cc_pedal.poll();
  }
}

/// @brief Same as global_poll, but return the pad/pedal that is triggered/changed
/// @retval 0-11: Pad 1 to 12
/// @retval 12: Kick pedal
/// @retval 13: CC pedal
/// @retval -1: Nothing happened
int global_poll_return() {
  for (size_t i=0; i<12; i++) {
    if (pads_array[i].poll(PADS_SAMPLING_PERIOD) == 1) {
      return i;
    }
  }

  if (f_kick_ped_enabled) {
    if (kick_pad.poll(PADS_SAMPLING_PERIOD) == 1) {
      return 12;
    }
  }

  if (f_cc_ped_enabled) {
    if (cc_pedal.poll() == 1) {
      return 13;
    }
  }

  return -1;
}

/// @brief Placeholder function called when pads are triggered/cooled down
/// @param is_triggered true:trigger, false:cooled down
/// @param note_number MIDI note number
/// @param channel_number MIDI channel number 1-16. If 0, send to all channel
/// @param raw_reading Raw analogRead value from the pad
/// @param vel_map_profile Index of velocity mapping coefficient array. 0:soft, 1:medium, 2:hard
/// @param pad_type 0:Big pad, 1:small pad, 2:snare pad
void pads_triggered(bool is_triggered, int note_number, int channel_number, int raw_reading, int vel_map_profile, int pad_type) {
  int velocity;

  if (is_triggered) {
    switch (pad_type) {
      case 0:
        velocity = midi_exp_vel_map(raw_reading, VEL_MAP_COEFF_BIG[vel_map_profile]);
        break;
      case 1:
        velocity = midi_exp_vel_map(raw_reading, VEL_MAP_COEFF_SMALL[vel_map_profile]);
        break;
      case 2:
        velocity = midi_exp_vel_map(raw_reading, VEL_MAP_COEFF_SNARE[vel_map_profile]);
        break;
      default:
        velocity = 0;
    }  
  }
  else {
    velocity = 0;
  }

  switch (channel_number) {
    case 0:
      for (int i=1; i<=16; i++) {
        send_note_event(is_triggered, note_number, i, velocity);
      }
      break;
    default:
      send_note_event(is_triggered, note_number, channel_number, velocity);
  }
}

/// @brief Send note on or note off event while taking care of both USB and UART MIDI interface
/// @param is_note_on true: note on, false: note off
/// @param note_number MIDI note number
/// @param channel_number MIDI channel number
/// @param velocity MIDI note velocity
void send_note_event(bool is_note_on, int note_number, int channel_number, int velocity) {
  if (is_note_on) {
    CompositeMIDI.sendNoteOn(channel_number-1, note_number, velocity); //USBComposite library accepts channel 0-15 but MIDI library accepts 1-16
    if (f_uart_midi_enabled) {
      UARTMIDI.sendNoteOn(note_number, velocity, channel_number);
    }
  }
  else {
    CompositeMIDI.sendNoteOff(channel_number-1, note_number, 0);
    if (f_uart_midi_enabled) {
      UARTMIDI.sendNoteOff(note_number, 0, channel_number);
    }
  }
}

/// @brief Placeholder function called when controller changed
/// @param cc_number MIDI CC number
/// @param channel_number MIDI channel number 1-16. If 0, send to all channel
/// @param raw_reading Raw analogRead value from controller
void controller_changed(int cc_number, int channel_number, int raw_reading) {
  switch (channel_number) {
    case 0:
      for (int i=1; i<=16; i++) {
        send_cc_event(cc_number, i, midi_lin_vel_map(raw_reading));
      }
      break;
    default:
      send_cc_event(cc_number, channel_number, midi_lin_vel_map(raw_reading));
  }
}

/// @brief Send control change event while taking care of both USB and UART MIDI interface
/// @param cc_number MIDI CC number
/// @param channel_number MIDI channel number
/// @param cc_value MIDI CC value
void send_cc_event(int cc_number, int channel_number, int cc_value) {
  CompositeMIDI.sendControlChange(channel_number-1, cc_number, cc_value);
  if (f_uart_midi_enabled) {
    UARTMIDI.sendControlChange(cc_number, cc_value, channel_number);
  }
}

// ===== Main program =====

void setup() {
  USBComposite.clear();
  CompositeMIDI.registerComponent();
  CompositeSerial.registerComponent();

  USBComposite.setVendorId(0x88dc);
  USBComposite.setProductId(0xa8a5);
  USBComposite.setManufacturerString("ZYDP");
  USBComposite.setProductString("miniPad alpha");
  USBComposite.begin();

  UARTMIDI.begin();
}

void loop() {
  global_poll();

    // for (size_t i=0; i<12; i++) {
    //   CompositeSerial.print(pads_array[i].debug());
    //   CompositeSerial.print(",");
    // }
    // CompositeSerial.println();
}