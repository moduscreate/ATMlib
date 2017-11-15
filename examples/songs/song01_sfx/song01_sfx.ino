#include <Arduino.h>
#include <Arduboy2.h>
#include <ATMlib.h>
#include "bitmaps.h"
#include "song.h"


Arduboy2Base arduboy;
Sprites sprites;
ATMsynth ATM;

struct mod_sfx_state sfx_state;

const PROGMEM struct sfx1_data {
  uint8_t num_tracks;
  uint16_t tracks_offset[2];
  uint8_t start_patterns[4];
  uint8_t pattern1[8];
  uint8_t pattern2[9];
} sfx1 = {
  .num_tracks = 0x02,
  .tracks_offset = {
      offsetof(struct sfx1_data, pattern1),
      offsetof(struct sfx1_data, pattern2),
  },
  .start_patterns = {0,0,0,0},
  .pattern1 = {
    0x9D, 10,  
    0x40, 10,
    0xFD, 10, 1,
    0x9F,       
  },
  .pattern2 = {
    0x00 + 42,    // NOTE ON: note = 42
    0x9F + 20,   // DELAY: ticks = 20
    0x00 + 43,    // NOTE ON: note = 43
    0x9F + 20,   // DELAY: ticks = 20
    0x00 + 44,    // NOTE ON: note = 44
    0x9F + 20,   // DELAY: ticks = 20
    0x00 + 45,    // NOTE ON: note = 45
    0x9F + 20,   // DELAY: ticks = 20
    0xFE,         // RETURN
  }
};

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
  char buf[64];

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
 #if 0
  sprintf(buf, "channel_active_mute: 0x%02hhX\n", atmlib_state.channel_active_mute);
  Serial.write(buf);
 #endif
}
