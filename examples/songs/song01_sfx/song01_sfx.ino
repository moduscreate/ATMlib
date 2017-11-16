#include <Arduino.h>
#include <Arduboy2.h>
#include <ATMlib.h>
#include "bitmaps.h"
#include "song.h"


Arduboy2Base arduboy;
Sprites sprites;
ATMsynth ATM;

struct mod_sfx_state sfx_state;

void setup() {
  arduboy.begin();
  // set the framerate of the game at 60 fps
  arduboy.setFrameRate(15);
  // let's make sure the sound was not muted in a previous sketch
  arduboy.audio.on();

  //Initialize serial and wait for port to open:
#if 0
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
 #endif
  // Begin playback of song.
  atm_synth_setup();
  atm_synth_play_score((uint8_t*)&score);
}

void loop() {

  if (!(arduboy.nextFrame()))
    return;

  arduboy.pollButtons();
  arduboy.clear();
  sprites.drawSelfMasked(34, 4, T_arg, 0);

  if (arduboy.justPressed(B_BUTTON)) {
    atm_synth_play_sfx_track(OSC_CH_ONE, (struct mod_sfx*)&sfx1, &sfx_state);
  }

  if (arduboy.justPressed(A_BUTTON)) {
    atm_synth_stop_sfx_track(OSC_CH_ONE);
  }

  if (arduboy.justPressed(LEFT_BUTTON)) {
    atm_synth_play_score((uint8_t*)&score);
  }

  if (arduboy.justPressed(RIGHT_BUTTON)) {
    atm_synth_set_score_paused(!atm_synth_get_score_paused());
  }
  arduboy.display();
}

#if 0
extern "C" {
void log_cmd(uint8_t chnum, uint8_t cmd, uint8_t sz, uint8_t *data);
}

#if 1
void log_cmd(uint8_t chnum, uint8_t cmd, uint8_t sz, uint8_t *data)
{
}
#else
void log_cmd(uint8_t chnum, uint8_t cmd, uint8_t sz, uint8_t *data)
{
  char buf[64];

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

  sprintf(buf, "\nch: 0x%02hhX sz: 0x%02hhX %03hhu cmd: 0x%02hhX 0b" BYTE_TO_BINARY_PATTERN "\n", chnum, sz, cmd, cmd, BYTE_TO_BINARY(cmd));
  Serial.write(buf);
  while (sz--) {
    uint8_t v = *data;
    sprintf(buf, "\t0x%02hhX %03hhu 0b" BYTE_TO_BINARY_PATTERN "\n", v, v, BYTE_TO_BINARY(v));
    Serial.write(buf);
    data++;
  }

}
#endif
#endif
