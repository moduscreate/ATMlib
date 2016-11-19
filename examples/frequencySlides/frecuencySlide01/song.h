#ifndef SONG_H
#define SONG_H

#define Song const uint8_t PROGMEM

Song music[] = {                // total song in bytes = 28 
                                // setup bytes 11
  0x03,                         // Number of tracks
  0x00, 0x00,                   // Address of track 0
  0x03, 0x00,                   // Address of track 1
  0x07, 0x00,                   // Address of track 2
  0x01,                         // Channel 0 entry track (PULSE)
  0x00,                         // Channel 1 entry track (SQUARE)
  0x00,                         // Channel 2 entry track (TRIANGLE)
  0x00,                         // Channel 3 entry track (NOISE)

  //"Track 0"                   // ticks = 0, bytes = 3
  0x40, 0,                      // FX: SET VOLUME: volume = 0
  0xFE,                         // RETURN (empty track used for silent channels)

  //"Track 1"                   // ticks = 2048, bytes = 4
  0xFD, 31, 2,                  // REPEAT: count = 32 - track = 2 (32 * 64 ticks)
  0xFE,                         // RETURN

  //"Track 2"                   // ticks = 64, bytes = 10
  0x00 + 37,                    // NOTE ON: note = 37 (delay 1 tick)
  0x40, 63,                     // FX: SET VOLUME: volume = 63
  0x44, -2,                     // FX: FREQUENCY SLIDE ON: steps = -2
  0x9F + 31,                    // DELAY: ticks = 31
  0x46,                         // FX: FREQUENCY SLIDE OFF
  0x00,                         // NOTE OFF (delay 1 tick)
  0x9F + 31,                    // DELAY: ticks = 31
  0xFE,                         // RETURN
};

#endif