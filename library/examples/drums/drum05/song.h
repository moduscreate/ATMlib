#ifndef SONG_H
#define SONG_H

#define Song const uint8_t PROGMEM

Song music[] = {
  0x05,           // Number of tracks
  0x00, 0x00,     // Address of track 0
  0x03, 0x00,     // Address of track 1
  0x0F, 0x00,     // Address of track 2
  0x1E, 0x00,     // Address of track 3
  0x29, 0x00,     // Address of track 4
  0x00,           // Channel 0 entry track (PULSE)
  0x00,           // Channel 1 entry track (SQUARE)
  0x00,           // Channel 2 entry track (TRIANGLE)
  0x01,           // Channel 3 entry track (NOISE)

  //"Track 0"
  0x40, 0,        // FX: SET VOLUME: volume = 0
  0xFE,           // RETURN (empty track used for silent channels)

  //"Track 1"
  0xFD, 0, 2,     // REPEAT: count = 1 - track = 2
  0x9F + 8,       // DELAY: ticks = 8
  0xFD, 0, 2,     // REPEAT: count = 1 - track = 2
  0x9F + 8,       // DELAY: ticks = 8
  0xFD, 0, 2,     // REPEAT: count = 1 - track = 2
  0xFE,           // RETURN

  //"Track 2"
  0xFD,  5, 3,    // REPEAT: count = 6 - track = 3
  0x40,  0,       // FX: SET VOLUME: volume = 0
  0x9F + 4,       // DELAY: ticks = 4

  0xFD, 0, 4,     // REPEAT: count = 1 - track = 4
  0x9F + 12,      // DELAY: ticks = 12

  0xFD, 0, 4,     // REPEAT: count = 1 - track = 4
  0x9F + 12,      // DELAY: ticks = 12

  0xFE,           // RETURN

  //"Track 3"
  0x40, 32,       // FX: SET VOLUME: volume = 32
  0x41, -8,       // FX: VOLUME SLIDE ON: steps = -8
  0x9F + 2,       // DELAY: ticks = 2
  0x43,           // FX: VOLUME SLIDE OFF
  0xFE,           // RETURN

  //"Track 4"
  0x40, 32,       // FX: SET VOLUME: volume = 32
  0x41, -8,       // FX: VOLUME SLIDE ON: steps = -8
  0x9F + 4,       // DELAY: ticks = 4
  0x43,           // FX: VOLUME SLIDE OFF
  0xFE,           // RETURN
  
};

#endif