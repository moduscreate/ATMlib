#ifndef SONG_H
#define SONG_H

#define Song const uint8_t PROGMEM

Song music[] = {                // total song in bytes = 58
  //                            // setup bytes 15
  0x05,                         // Number of tracks
  0x00, 0x00,                   // Address of track 0
  0x03, 0x00,                   // Address of track 1
  0x07, 0x00,                   // Address of track 2
  0x1C, 0x00,                   // Address of track 3
  0x24, 0x00,                   // Address of track 4
  0x00,                         // Channel 0 entry track (PULSE)
  0x00,                         // Channel 1 entry track (SQUARE)
  0x00,                         // Channel 2 entry track (TRIANGLE)
  0x01,                         // Channel 3 entry track (NOISE)

  //"Track 0"                   // ticks = 0, bytes = 3
  0x40, 0,                      // FX: SET VOLUME: volume = 0
  0xFE,                         // RETURN

  //"Track 1"                   // ticks = 32 * 64 = 2048, bytes = 4
  0xFD, 31, 2,                  // REPEAT: count = 32 - track = 4
  0xFE,                         // RETURN

  //"Track 2"                   // ticks = 64, bytes = 21
  0xFD, 0, 3,                   // REPEAT: count = 1 - track = 3 ( 8 ticks)
  0xFD, 2, 4,                   // REPEAT: count = 3 - track = 4 (12 ticks)
  0x9F + 4,                     // DELAY: ticks = 4              ( 4 ticks)
  0xFD, 0, 3,                   // REPEAT: count = 1 - track = 3 ( 8 ticks)
  0xFD, 0, 3,                   // REPEAT: count = 1 - track = 3 ( 8 ticks)
  0xFD, 0, 3,                   // REPEAT: count = 1 - track = 3 ( 8 ticks)
  0xFD, 0, 3,                   // REPEAT: count = 1 - track = 3 ( 8 ticks)
  0x9F + 8,                     // DELAY: ticks = 8              ( 8 ticks)
  0xFE,                         // RETURN

  //"Track 3"                   // ticks = 8, bytes = 8
  0x40, 32,                     // FX: SET VOLUME: volume = 32
  0x41, -8,                     // FX: VOLUME SLIDE ON: steps = -8
  0x9F + 4,                     // DELAY: ticks = 4
  0x43,                         // FX: VOLUME SLIDE OFF
  0x9F + 4,                     // DELAY: ticks = 4
  0xFE,                         // RETURN

  //"Track 4"                   // ticks = 4, bytes = 7
  0x40,  32,                    // FX: SET VOLUME: volume = 32
  0x9F + 2,                     // DELAY: ticks = 2
  0x40,  0,                     // FX: SET VOLUME: volume = 0
  0x9F + 2,                     // DELAY: ticks = 2
  0xFE,                         // RETURN
};

#endif
