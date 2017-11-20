#include <Arduino.h>
#include <MIDIUSB.h>
#include <Arduboy2.h>
#include <ATMlib.h>
#include <atm_cmd_constants.h>

Arduboy2Base arduboy;

void synth_ext_callback(const uint8_t channel_count, atm_synth_state *synth_state, struct atm_channel_state *ch, struct atm_synth_ext *synth_ext)
{
  (void)(synth_ext);
  midiEventPacket_t rx;
  char buf[64];

  rx = MidiUSB.read();
  const uint8_t midi_ch_idx = rx.byte1 & 0x0F;
  if (!rx.header || midi_ch_idx >= channel_count) {
    return;
  } else {
    sprintf(buf, "MIDI rx: %02hhX %02hhX %02hhX %02hhX\n", rx.header, rx.byte1, rx.byte2, rx.byte3);
    Serial.write(buf);
    if (rx.header == 0x08 || (rx.header == 0x09 && !rx.byte3)) {
      const uint8_t cmd[] = {ATM_CMD_I_NOTE_OFF};
      ext_synth_command(midi_ch_idx, (const atm_cmd_data *)cmd, synth_state, &ch[midi_ch_idx]);
    } else if (rx.header == 0x09 && rx.byte3) {
      {
        const uint8_t cmd[] = {ATM_CMD_M_SET_VOLUME(63)};
        ext_synth_command(midi_ch_idx, (const atm_cmd_data *)cmd, synth_state, &ch[midi_ch_idx]);
      }
      {
        const uint8_t cmd[] = {ATM_CMD_M_NOTE((rx.byte2-35) & 0x3F)};
        ext_synth_command(midi_ch_idx, (const atm_cmd_data *)cmd, synth_state, &ch[midi_ch_idx]);
      }
    }
  }
}

struct atm_synth_ext synth_ext = {synth_ext_callback, NULL};

void setup() {
  arduboy.begin();
  // set the framerate of the game at 60 fps
  arduboy.setFrameRate(15);
  // let's make sure the sound was not muted in a previous sketch
  arduboy.audio.on();

  Serial.begin(115200);
/*
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
*/
  atm_synth_setup();
  atm_synth_play_ext(&synth_ext);
}

void loop() {
  if (!(arduboy.nextFrame()))
    return;
}
