#ifndef EVADE_SCORE_H
#define EVADE_SCORE_H

const PROGMEM struct evade_score_s {
  uint8_t num_patterns;
  uint16_t patterns_offset[20];
  uint8_t start_patterns[4];
  uint8_t pattern0[39];
  uint8_t pattern1[27];
  uint8_t pattern2[1];
  uint8_t pattern3[13];
  uint8_t pattern4[6];
  uint8_t pattern5[9];
  uint8_t pattern6[17];
  uint8_t pattern7[9];
  uint8_t pattern8[9];
  uint8_t pattern9[19];
  uint8_t pattern10[9];
  uint8_t pattern11[9];
  uint8_t pattern12[13];
  uint8_t pattern13[13];
  uint8_t pattern14[17];
  uint8_t pattern15[17];
  uint8_t pattern16[15];
  uint8_t pattern17[34];
  uint8_t pattern18[13];
  uint8_t pattern19[20];
} evade_score = {
  .num_patterns = NUM_PATTERNS(struct evade_score_s),
  .patterns_offset = {
    offsetof(struct evade_score_s, pattern0),
    offsetof(struct evade_score_s, pattern1),
    offsetof(struct evade_score_s, pattern2),
    offsetof(struct evade_score_s, pattern3),
    offsetof(struct evade_score_s, pattern4),
    offsetof(struct evade_score_s, pattern5),
    offsetof(struct evade_score_s, pattern6),
    offsetof(struct evade_score_s, pattern7),
    offsetof(struct evade_score_s, pattern8),
    offsetof(struct evade_score_s, pattern9),
    offsetof(struct evade_score_s, pattern10),
    offsetof(struct evade_score_s, pattern11),
    offsetof(struct evade_score_s, pattern12),
    offsetof(struct evade_score_s, pattern13),
    offsetof(struct evade_score_s, pattern14),
    offsetof(struct evade_score_s, pattern15),
    offsetof(struct evade_score_s, pattern16),
    offsetof(struct evade_score_s, pattern17),
    offsetof(struct evade_score_s, pattern18),
    offsetof(struct evade_score_s, pattern19),
  },
  .start_patterns = {
    0x00,                         // Channel 0 entry pattern (PULSE)
    0x01,                         // Channel 1 entry pattern (SQUARE)
    0x02,                         // Channel 2 entry pattern (TRIANGLE)
    0x03,                         // Channel 3 entry pattern (NOISE)
  },
  .pattern0 = {
    //"Track channel 0"
    ATM_CMD_M_SET_VOLUME(32),   // FX: SET VOLUME: volume = 64
    ATM_CMD_M_SET_TEMPO(22),   // FX: SET TEMPO: tempo = 22
    ATM_CMD_M_CALL_REPEAT(6, 3),   // REPEAT: count = 2 + 1 / track = 6
    ATM_CMD_M_CALL(7),    // GOTO pattern 7
    ATM_CMD_M_CALL_REPEAT(6, 3),   // REPEAT: count = 2 + 1 / track = 6
    ATM_CMD_M_CALL(8),    // GOTO pattern 8
    ATM_CMD_M_CALL(10),   // GOTO pattern 10
    ATM_CMD_M_CALL(11),   // GOTO pattern 11
    ATM_CMD_M_CALL(10),   // GOTO pattern 10
    ATM_CMD_M_CALL(11),   // GOTO pattern 11
    ATM_CMD_M_CALL(10),   // GOTO pattern 10
    ATM_CMD_M_CALL(12),   // GOTO pattern 12
    ATM_CMD_M_CALL(10),   // GOTO pattern 10
    ATM_CMD_M_CALL(11),   // GOTO pattern 11
    ATM_CMD_M_CALL(10),   // GOTO pattern 10
    ATM_CMD_M_CALL(11),   // GOTO pattern 11
    ATM_CMD_M_CALL(10),   // GOTO pattern 10
    ATM_CMD_M_CALL(13),   // GOTO pattern 13
    ATM_CMD_I_STOP,     // FX: STOP CURRENT CHANNEL
  },
  .pattern1 = {
    //"Track channel 1"
    ATM_CMD_M_SET_VOLUME(32),   // FX: SET VOLUME: volume = 50
    ATM_CMD_M_CALL_REPEAT(14, 12),
    ATM_CMD_M_CALL_REPEAT(15, 4),
    ATM_CMD_M_CALL_REPEAT(14, 12),
    ATM_CMD_M_CALL_REPEAT(16, 4),
    ATM_CMD_M_CALL_REPEAT(14, 10),
    ATM_CMD_M_CALL_REPEAT(15, 2),
    ATM_CMD_M_CALL_REPEAT(14, 10),
    ATM_CMD_M_CALL_REPEAT(16, 2),
    ATM_CMD_I_STOP,     // FX: STOP CURRENT CHANNEL
  },
  .pattern2 = {
    //"Track channel 2"
    ATM_CMD_I_STOP,     // FX: STOP CURRENT CHANNEL
  },
  .pattern3 = {
    //"Track channel 3"
    ATM_CMD_M_SET_VOLUME(32),   // FX: SET VOLUME: volume = 64
    ATM_CMD_M_CALL_REPEAT(9, 7),   // REPEAT: count = 6 + 1 / track = 9
    ATM_CMD_M_CALL(17),   // GOTO pattern 17
    ATM_CMD_M_CALL_REPEAT(18, 11),   // REPEAT: count = 10 + 1 / track = 18
    ATM_CMD_M_CALL(19),   // GOTO pattern 19
    ATM_CMD_I_STOP,     // FX: STOP CURRENT CHANNEL
  },
  .pattern4 = {
    //"Track tick"
    ATM_CMD_M_SET_VOLUME(32),   // FX: SET VOLUME: volume = 32
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_SET_VOLUME(0),    // FX: SET VOLUME: volume = 0
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern5 = {
    //"Track snare"
    ATM_CMD_M_SET_VOLUME(32),   // FX: SET VOLUME: volume = 32
    ATM_CMD_M_SLIDE_VOL_ON(-16),    // FX: VOLUME SLIDE ON: steps = -16
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_M_SLIDE_VOL_OFF,     // FX: VOLUME SLIDE OFF
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern6 = {
    //"Track ld 1"
    ATM_CMD_M_NOTE(25),    // NOTE ON: note = 25
    ATM_CMD_M_DELAY_TICKS(5),   // DELAY: ticks = 5
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(4),   // DELAY: ticks = 4
    ATM_CMD_M_NOTE(25),    // NOTE ON: note = 25
    ATM_CMD_M_DELAY_TICKS(3),   // DELAY: ticks = 3
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_NOTE(27),    // NOTE ON: note = 27
    ATM_CMD_M_DELAY_TICKS(4),   // DELAY: ticks = 4
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(4),   // DELAY: ticks = 4
    ATM_CMD_M_NOTE(28),    // NOTE ON: note = 28
    ATM_CMD_M_DELAY_TICKS(7),   // DELAY: ticks = 7
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(4),   // DELAY: ticks = 4
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern7 = {
    //"Track ld 2"
    ATM_CMD_M_NOTE(30),    // NOTE ON: note = 30
    ATM_CMD_M_DELAY_TICKS(8),   // DELAY: ticks = 8
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(4),   // DELAY: ticks = 4
    ATM_CMD_M_NOTE(32),    // NOTE ON: note = 32
    ATM_CMD_M_DELAY_TICKS(8),   // DELAY: ticks = 8
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(12),    // DELAY: ticks = 12
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern8 = {
    //"Track ld 3"
    ATM_CMD_M_NOTE(35),    // NOTE ON: note = 35
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 8
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(5),   // DELAY: ticks = 5
    ATM_CMD_M_NOTE(32),    // NOTE ON: note = 32
    ATM_CMD_M_DELAY_TICKS(11),    // DELAY: ticks = 11
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(8),   // DELAY: ticks = 8
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern9 = {
    //"Track perc"
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(7),   // DELAY: ticks = 7
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(7),   // DELAY: ticks = 7
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(7),   // DELAY: ticks = 7
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(3),   // DELAY: ticks = 3
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern10 = {
    //"Track ld 4"
    ATM_CMD_M_NOTE(25),    // NOTE ON: note = 25
    ATM_CMD_M_DELAY_TICKS(6),   // DELAY: ticks = 6
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_M_NOTE(35),    // NOTE ON: note = 35
    ATM_CMD_M_DELAY_TICKS(7),   // DELAY: ticks = 7
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern11 = {
    //"Track LD 5"
    ATM_CMD_M_NOTE(25),    // NOTE ON: note = 25
    ATM_CMD_M_DELAY_TICKS(7),   // DELAY: ticks = 7
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_NOTE(37),    // NOTE ON: note = 37
    ATM_CMD_M_DELAY_TICKS(7),   // DELAY: ticks = 7
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern12 = {
    //"Track ld 6"
    ATM_CMD_M_NOTE(23),    // NOTE ON: note = 23
    ATM_CMD_M_DELAY_TICKS(3),   // DELAY: ticks = 3
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_M_NOTE(28),    // NOTE ON: note = 28
    ATM_CMD_M_DELAY_TICKS(6),   // DELAY: ticks = 6
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_M_NOTE(27),    // NOTE ON: note = 27
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern13 = {
    //"Track LD 8"
    ATM_CMD_M_NOTE(23),    // NOTE ON: note = 23
    ATM_CMD_M_DELAY_TICKS(3),   // DELAY: ticks = 3
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_M_NOTE(40),    // NOTE ON: note = 40
    ATM_CMD_M_DELAY_TICKS(5),   // DELAY: ticks = 5
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_M_NOTE(37),    // NOTE ON: note = 39
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern14 = {
    //"Track Bass C"
    ATM_CMD_M_NOTE(13),    // NOTE ON: note = 13
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_NOTE(25),    // NOTE ON: note = 25
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_NOTE(13),    // NOTE ON: note = 13
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_NOTE(25),    // NOTE ON: note = 25
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern15 = {
    //"Track BASS A#"
    ATM_CMD_M_NOTE(11),    // NOTE ON: note = 11
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_NOTE(23),    // NOTE ON: note = 23
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_NOTE(11),    // NOTE ON: note = 11
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_NOTE(23),    // NOTE ON: note = 23
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern16 = {
    //"Track BASS D"
    ATM_CMD_M_NOTE(15),    // NOTE ON: note = 15
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_NOTE(27),    // NOTE ON: note = 27
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_M_NOTE(15),    // NOTE ON: note = 15
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_NOTE(27),    // NOTE ON: note = 27
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_NOTE_OFF,   // NOTE ON: note = 0
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern17 = {
    //"Track PERC2"
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(3),   // DELAY: ticks = 3
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(3),   // DELAY: ticks = 3
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(3),   // DELAY: ticks = 3
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(3),   // DELAY: ticks = 3
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(3),   // DELAY: ticks = 3
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern18 = {
    //"Track PERC3"
    ATM_CMD_M_CALL(5),    // GOTO pattern 5
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(3),   // DELAY: ticks = 3
    ATM_CMD_M_CALL(5),    // GOTO pattern 5
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(3),   // DELAY: ticks = 3
    ATM_CMD_I_RETURN,     // RETURN
  },
  .pattern19 = {
    //"Track PERC 4"
    ATM_CMD_M_CALL(5),    // GOTO pattern 5
    ATM_CMD_M_DELAY_TICKS(2),   // DELAY: ticks = 2
    ATM_CMD_M_CALL(5),    // GOTO pattern 5
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_CALL(5),    // GOTO pattern 5
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_DELAY_TICKS(1),   // DELAY: ticks = 1
    ATM_CMD_M_CALL(5),    // GOTO pattern 5
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_M_CALL(4),    // GOTO pattern 4
    ATM_CMD_I_RETURN,     // RETURN
  }
};


#endif
