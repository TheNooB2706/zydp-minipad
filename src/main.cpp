#include <Arduino.h>
#include <USBComposite.h>
#include <MIDI.h>
#include <AceButton.h>

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

const int NUM_BUTTONS = 5;
const int BUTTON1_PIN = PB5;
const int BUTTON2_PIN = PB6;
const int BUTTON3_PIN = PB7;
const int BUTTON4_PIN = PB8;
const int BUTTON5_PIN = PB9;
const int LED_RED_PIN = PB4;
const int LED_GREEN_PIN = PB3;
const int LED_BLUE_PIN = PA15;

const int LED_BLINK_FAST_PERIOD = 100;
const int LED_BLINK_SLOW_PERIOD = 200;
const int LED_SLOT_COLOR[4][3] = {{1,0,0},{0,1,0},{0,0,1},{1,0,1}}; 

const int PADS_TYPE[12] = {0,1,0,1,0,0,2,0,0,0,0,0};
const double VEL_MAP_COEFF_BIG[3] = {0.0048, 0.0025, 0.0012};
const double VEL_MAP_COEFF_SMALL[3] = {0.0025, 0.0012, 0.0006};
const double VEL_MAP_COEFF_SNARE[3] = {0.006, 0.0048, 0.0025};
const double VEL_MAP_COEFF_KICK[3] = {0.0048, 0.0025, 0.0012};

const int INTERFACE_MAIN = 0;
const int INTERFACE_SETTINGS = 1;
const int SETTINGS_CHANNEL = 0;
const int SETTINGS_UART_MIDI_ENABLE = 1;
const int SETTINGS_KICK_ENABLE = 2;
const int SETTINGS_CC_ENABLE = 3;
const int SETTINGS_VEL_CURVE = 4;
const int SETTINGS_KICK_VEL_CURVE = 5;

const int FLASH_SIGNATURE = 0xA07C9C9A;

// ===== Flags =====

bool f_uart_midi_enabled = true;
int f_midi_channel_num = 1; 
int f_vel_map_profile = 1;
int f_kick_vel_map_profile = 1;

bool f_cc_ped_enabled = false;
bool f_kick_ped_enabled = true;

int f_interface_level = 0; // Ranged from 0-1
int f_bank = 0; // Ranged from 0-3
int f_slot = 0; // Ranged from 0-3
int f_settings_index = 0; // Ranged from 0-5

// ===== Configuration storage struct =====

struct configStructure {
  bool uart_midi_enabled = f_uart_midi_enabled;
  int midi_channel_num = f_midi_channel_num;
  int vel_map_profile = f_vel_map_profile;
  int kick_vel_map_profile = f_kick_vel_map_profile;
  bool cc_ped_enabled = f_cc_ped_enabled;
  bool kick_ped_enabled = f_kick_ped_enabled;
  /// @brief Bank -> Slot -> Pads
  int mapping_bank[4][4][12] = {
    {
      {43, 41, 36, 41, 43, 47, 38, 47, 49, 46, 42, 51},
      {43, 41, 37, 41, 43, 47, 38, 47, 49, 46, 42, 51},
      {46, 37, 38, 41, 43, 42, 50, 45, 49, 54, 57, 51},
      {46, 37, 38, 41, 43, 42, 50, 45, 49, 55, 57, 51}
    }
  };
  int mapping_bank_kick[4][4] = {{36, 36, 36, 36}};
  int mapping_bank_cc[4][4] = {{4, 4, 4, 4}};
};

configStructure config;

// ===== Global functions declaration =====

void global_poll();
int global_poll_return();
void pads_triggered(bool is_triggered, int note_number, int channel_number, int raw_reading, int vel_map_profile, int pad_type);
void send_note_event(bool is_note_on, int note_number, int channel_number, int velocity);
void controller_changed(int cc_number, int channel_number, int raw_reading);
void send_cc_event(int cc_number, int channel_number, int cc_value);

int integer_shifter(int initial_val, int lower_bound, int offset, int modulus);
int integer_shifter(int initial_val, int offset, int modulus);
int integer_up(int initial_val, int modulus);
int integer_down(int initial_val, int modulus);

void handle_button_event(ace_button::AceButton* button, uint8_t eventType, uint8_t buttonState);
void button1_pressed();
void button1_long_pressed();
void button2_pressed();
void button3_pressed();
void button4_pressed();
void button5_pressed();

// ===== Pads initialization =====

class MIDIPad: public Pad {
  public:
    MIDIPad(int pin_num, float threshold_high, float threshold_low, int buffer_size, int midi_note_num) : Pad(pin_num, threshold_high, threshold_low, buffer_size, midi_note_num) {}

    void on_trigger(int pad_input) {
      pads_triggered(true, get_note_num(), f_midi_channel_num, pad_input, f_kick_vel_map_profile, 3);
      CompositeSerial.print("Triggered: ");
      CompositeSerial.println(get_max());
    }

    void on_cooldown() {
      pads_triggered(false, get_note_num(), f_midi_channel_num, 0, f_kick_vel_map_profile, 3);
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

// ===== Buttons initialization =====

ace_button::AceButton buttons[NUM_BUTTONS];

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

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].check();
  }

  led.poll();
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

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].check();
  }

  led.poll();

  return -1;
}

/// @brief Placeholder function called when pads are triggered/cooled down
/// @param is_triggered true:trigger, false:cooled down
/// @param note_number MIDI note number
/// @param channel_number MIDI channel number 1-16. If 0, send to all channel
/// @param raw_reading Raw analogRead value from the pad
/// @param vel_map_profile Index of velocity mapping coefficient array. 0:soft, 1:medium, 2:hard
/// @param pad_type 0:Big pad, 1:small pad, 2:snare pad, 3:kick pedal
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
      case 3:
        velocity = midi_exp_vel_map(raw_reading, VEL_MAP_COEFF_KICK[vel_map_profile]);
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

/// @brief Shift integer by offset while staying within certain range
/// @param initial_val Starting value
/// @param lower_bound Lowest number of the range
/// @param offset How much to shift the starting value
/// @param modulus Length of the range
/// @return Shifted integer
int integer_shifter(int initial_val, int lower_bound, int offset, int modulus) {
  return (initial_val - lower_bound + (offset % modulus) + modulus) % modulus + lower_bound;
}

/// @brief Overload where lower_bound is set to 0.
int integer_shifter(int initial_val, int offset, int modulus) {
  return integer_shifter(initial_val, 0, offset, modulus);
}

/// @brief Increase integer by one
int integer_up(int initial_val, int modulus) {
  return integer_shifter(initial_val, 1, modulus);
}

/// @brief Decrease integer by one
int integer_down(int initial_val, int modulus) {
  return integer_shifter(initial_val, -1, modulus);
}

void handle_button_event(ace_button::AceButton* button, uint8_t eventType, uint8_t buttonState) {
  uint8_t id = button->getId();

  switch (eventType) {
    case ace_button::AceButton::kEventReleased:
    case ace_button::AceButton::kEventClicked:
      CompositeSerial.print("Clicked button ");
      CompositeSerial.println(id);
      switch (id) {
        case 0:
          button1_pressed();
          break;
        case 1:
          button2_pressed();
          break;
        case 2:
          button3_pressed();
          break;
        case 3:
          button4_pressed();
          break;
        case 4:
          button5_pressed();
          break;
      }
      break;
    case ace_button::AceButton::kEventDoubleClicked:
      CompositeSerial.print("Double clicked button ");
      CompositeSerial.println(id);
      break;
    case ace_button::AceButton::kEventLongPressed:
      CompositeSerial.print("Long pressed button ");
      CompositeSerial.println(id);
      switch (id) {
        case 0:
          button1_long_pressed();
          break;
      }
      break;   
    case ace_button::AceButton::kEventLongReleased:
      CompositeSerial.print("Long released button ");
      CompositeSerial.println(id);
      break;   
  }
}

// ===== Button functions =====

void button1_pressed() {
  switch (f_interface_level) {
    case INTERFACE_MAIN:
      // bank change function
      f_bank = integer_up(f_bank, 4);
      led.on(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2]);
      led.blink(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2], f_bank+1, LED_BLINK_SLOW_PERIOD, true);
      break;
    case INTERFACE_SETTINGS:
      f_settings_index = integer_up(f_settings_index, 6);
      led.blink(1,1,1,f_settings_index+1, LED_BLINK_SLOW_PERIOD, true);
  }
}

void button1_long_pressed() {
  if (f_interface_level == INTERFACE_MAIN) {
    f_interface_level = INTERFACE_SETTINGS;
    led.on(1,1,1);
  }
}

void button2_pressed() {
  switch (f_interface_level) {
    case INTERFACE_MAIN:
      // slot change function
      f_slot = 0;
      led.on(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2]);
      led.blink(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2], f_bank+1, LED_BLINK_SLOW_PERIOD, true);
      break;
    case INTERFACE_SETTINGS:
      switch (f_settings_index) {
        case SETTINGS_CHANNEL:
          f_midi_channel_num = integer_up(f_midi_channel_num, 17);
          break;
        case SETTINGS_KICK_ENABLE:
          f_kick_ped_enabled = !f_kick_ped_enabled;
          break;
        case SETTINGS_UART_MIDI_ENABLE:
          f_uart_midi_enabled = !f_uart_midi_enabled;
          break;
        case SETTINGS_CC_ENABLE:
          f_cc_ped_enabled = !f_cc_ped_enabled;
          break;
        case SETTINGS_VEL_CURVE:
          f_vel_map_profile = integer_up(f_vel_map_profile, 3);
          break;
        case SETTINGS_KICK_VEL_CURVE:
          f_kick_vel_map_profile = integer_up(f_kick_vel_map_profile, 3);
          break;
      }
  }
}

void button3_pressed() {
  switch (f_interface_level) {
    case INTERFACE_MAIN:
      // slot change function
      f_slot = 1;
      led.on(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2]);
      led.blink(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2], f_bank+1, LED_BLINK_SLOW_PERIOD, true);
      break;
    case INTERFACE_SETTINGS:
      switch (f_settings_index) {
        case SETTINGS_CHANNEL:
          f_midi_channel_num = integer_down(f_midi_channel_num, 17);
          break;
        case SETTINGS_KICK_ENABLE:
          f_kick_ped_enabled = !f_kick_ped_enabled;
          break;
        case SETTINGS_UART_MIDI_ENABLE:
          f_uart_midi_enabled = !f_uart_midi_enabled;
          break;
        case SETTINGS_CC_ENABLE:
          f_cc_ped_enabled = !f_cc_ped_enabled;
          break;
        case SETTINGS_VEL_CURVE:
          f_vel_map_profile = integer_down(f_vel_map_profile, 3);
          break;
        case SETTINGS_KICK_VEL_CURVE:
          f_kick_vel_map_profile = integer_down(f_kick_vel_map_profile, 3);
          break;
      }
  }  
}

void button4_pressed() {
  switch (f_interface_level) {
    case INTERFACE_MAIN:
      // slot change function
      f_slot = 2;
      led.on(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2]);
      led.blink(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2], f_bank+1, LED_BLINK_SLOW_PERIOD, true);
      break;
    case INTERFACE_SETTINGS:
      f_interface_level = INTERFACE_MAIN;
      led.on(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2]);
      led.blink(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2], f_bank+1, LED_BLINK_SLOW_PERIOD, true);
  }  
}

void button5_pressed() {
  switch (f_interface_level) {
    case INTERFACE_MAIN:
      // slot change function
      f_slot = 3;
      led.on(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2]);
      led.blink(LED_SLOT_COLOR[f_slot][0], LED_SLOT_COLOR[f_slot][1], LED_SLOT_COLOR[f_slot][2], f_bank+1, LED_BLINK_SLOW_PERIOD, true);
      break;
    case INTERFACE_SETTINGS:
      // save settings to flash memory
      break;
  }  
}

// ===== Main program =====

void setup() {
  // LED setup
  led.on(1,0,0);

  // Button setup
  buttons[0].init(BUTTON1_PIN, HIGH, 0);
  buttons[1].init(BUTTON2_PIN, HIGH, 1);
  buttons[2].init(BUTTON3_PIN, HIGH, 2);
  buttons[3].init(BUTTON4_PIN, HIGH, 3);
  buttons[4].init(BUTTON5_PIN, HIGH, 4);

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);
  pinMode(BUTTON5_PIN, INPUT_PULLUP);

  ace_button::ButtonConfig* buttonConfig = ace_button::ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handle_button_event);
  buttonConfig->setClickDelay(150);
  buttonConfig->setFeature(ace_button::ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ace_button::ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ace_button::ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ace_button::ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);
  buttonConfig->setFeature(ace_button::ButtonConfig::kFeatureSuppressAfterClick);
  buttonConfig->setFeature(ace_button::ButtonConfig::kFeatureSuppressAfterDoubleClick);
  buttonConfig->setFeature(ace_button::ButtonConfig::kFeatureSuppressAfterLongPress);

  // USB setup

  USBComposite.clear();
  CompositeMIDI.registerComponent();
  CompositeSerial.registerComponent();

  USBComposite.setVendorId(0x88dc);
  USBComposite.setProductId(0xa8a5);
  USBComposite.setManufacturerString("ZYDP");
  USBComposite.setProductString("miniPad alpha");
  USBComposite.begin();

  // UARTMIDI setup

  UARTMIDI.begin();
}

void loop() {
  global_poll();
}