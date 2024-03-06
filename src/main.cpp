#include <Arduino.h>
#include <USBComposite.h>

#include <pad.hpp>
#include <midi-util.hpp>

USBMIDI CompositeMIDI;
USBCompositeSerial CompositeSerial;

class MIDIPad: public Pad {
  public:
    MIDIPad(int pin_num, float threshold_high, float threshold_low, int buffer_size, int midi_note_num) : Pad(pin_num, threshold_high, threshold_low, buffer_size, midi_note_num) {}

    void on_trigger(int pad_input) {
      CompositeMIDI.sendNoteOn(0, get_note_num(), midi_exp_vel_map(pad_input, 0.0025));
      CompositeSerial.print("Triggered: ");
      CompositeSerial.println(get_max());
    }

    void on_cooldown() {
      CompositeMIDI.sendNoteOff(0, get_note_num(), 0);
    }
};

void setup() {
  USBComposite.clear();
  CompositeMIDI.registerComponent();
  CompositeSerial.registerComponent();

  USBComposite.setVendorId(0x88dc);
  USBComposite.setProductId(0xa8a5);
  USBComposite.setManufacturerString("ZYDP");
  USBComposite.setProductString("miniPad alpha");
  USBComposite.begin();
}

MIDIPad pad1test(PA0, 100, 70, 10, SNARE_GM2);
MIDIPad pad2test(PA1, 100, 70, 10, BASS_DRUM_GM2);

void loop() {
  pad1test.poll(470);
  pad2test.poll(470);
  CompositeSerial.print(pad1test.buffer.getAverage());
  CompositeSerial.print(",");
  CompositeSerial.println(pad2test.buffer.getAverage());
}