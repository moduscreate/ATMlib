#ifndef SONG_H
#define SONG_H

#include "atm_cmd_constants.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))
#endif

#ifndef NUM_PATTERNS
#define NUM_PATTERNS(struct_) (ARRAY_SIZE( ((struct_ *)0)->patterns_offset))
#endif

#ifndef DEFINE_PATTERN
#define DEFINE_PATTERN(pattern_id, values) const uint8_t pattern_id[] = values;
#endif

/* Pattern 0 ticks = 64 / bytes = 4 */
#define pattern0_data { \
    ATM_CMD_M_SET_VOLUME(0), \
    ATM_CMD_M_DELAY_TICKS(32), \
    ATM_CMD_M_DELAY_TICKS(32), \
    ATM_CMD_I_STOP, \
}
DEFINE_PATTERN(pattern0_array, pattern0_data);

/* Pattern 1 ticks = 2048 / bytes = 6 */
#define pattern1_data { \
    ATM_CMD_M_SET_TEMPO(50), \
    ATM_CMD_M_CALL_REPEAT(4, 4), \
    ATM_CMD_I_STOP, \
}
DEFINE_PATTERN(pattern1_array, pattern1_data);

/* Pattern 2 ticks = 2048 / bytes = 5 */
#define pattern2_data { \
    ATM_CMD_M_CALL_REPEAT(6, 32), \
    ATM_CMD_I_NOTE_OFF, \
    ATM_CMD_I_STOP, \
}
DEFINE_PATTERN(pattern2_array, pattern2_data);

/* Pattern 3 ticks = 2048 / bytes = 7 */
#define pattern3_data { \
    ATM_CMD_M_CALL_REPEAT(0, 8), \
    ATM_CMD_M_CALL_REPEAT(7, 24), \
    ATM_CMD_I_STOP, \
}
DEFINE_PATTERN(pattern3_array, pattern3_data);

/* Pattern 4 ticks = 512 / bytes = 21 */
#define pattern4_data { \
    ATM_CMD_M_CALL_REPEAT(5, 2), \
    ATM_CMD_M_ADD_TRANSPOSITION(3), \
    ATM_CMD_M_CALL_REPEAT(5, 2), \
    ATM_CMD_M_ADD_TRANSPOSITION(-1), \
    ATM_CMD_M_CALL_REPEAT(5, 2), \
    ATM_CMD_M_ADD_TRANSPOSITION(3), \
    ATM_CMD_M_CALL_REPEAT(5, 2), \
    ATM_CMD_M_ADD_TRANSPOSITION(-5), \
    ATM_CMD_I_RETURN, \
}
DEFINE_PATTERN(pattern4_array, pattern4_data);

/* Pattern 5 ticks = 64 / bytes = 14 */
#define pattern5_data { \
    ATM_CMD_I_NOTE_B5, \
    ATM_CMD_M_SET_VOLUME(63), \
    ATM_CMD_M_SLIDE_VOL_ON(-16), \
    ATM_CMD_M_DELAY_TICKS(16), \
    ATM_CMD_M_SET_VOLUME(16), \
    ATM_CMD_M_SLIDE_VOL_ON(-4), \
    ATM_CMD_M_DELAY_TICKS(4), \
    ATM_CMD_M_SLIDE_VOL_OFF, \
    ATM_CMD_M_DELAY_TICKS(20), \
    ATM_CMD_M_DELAY_TICKS(24), \
    ATM_CMD_I_RETURN, \
}
DEFINE_PATTERN(pattern5_array, pattern5_data);

/* Pattern 6 ticks = 64 / bytes = 10 */
#define pattern6_data { \
    ATM_CMD_I_NOTE_B2, \
    ATM_CMD_M_SET_VOLUME(32), \
    ATM_CMD_M_TREMOLO_ON(1, 30), \
    ATM_CMD_M_DELAY_TICKS(32), \
    ATM_CMD_M_DELAY_TICKS(31), \
    ATM_CMD_M_TREMOLO_OFF, \
    ATM_CMD_M_DELAY_TICKS(3), \
    ATM_CMD_I_RETURN, \
}
DEFINE_PATTERN(pattern6_array, pattern6_data);

/* Pattern 7 ticks = 64 / bytes = 20 */
#define pattern7_data { \
    ATM_CMD_M_SET_VOLUME(32), \
    ATM_CMD_M_DELAY_TICKS(2), \
    ATM_CMD_M_SET_VOLUME(0), \
    ATM_CMD_M_DELAY_TICKS(16), \
\
    ATM_CMD_M_SET_VOLUME(32), \
    ATM_CMD_M_DELAY_TICKS(2), \
    ATM_CMD_M_SET_VOLUME(0), \
    ATM_CMD_M_DELAY_TICKS(16), \
\
    ATM_CMD_M_SET_VOLUME(32), \
    ATM_CMD_M_SLIDE_VOL_ON(-2), \
    ATM_CMD_M_DELAY_TICKS(17), \
    ATM_CMD_M_SLIDE_VOL_OFF, \
    ATM_CMD_M_DELAY_TICKS(17), \
    ATM_CMD_I_RETURN, \
}
DEFINE_PATTERN(pattern7_array, pattern7_data);

const PROGMEM struct score_data {
  uint8_t fmt;
  uint8_t num_patterns;
  uint16_t patterns_offset[8];
  uint8_t num_channels;
  uint8_t start_patterns[4];
  uint8_t pattern0[sizeof(pattern0_array)];
  uint8_t pattern1[sizeof(pattern1_array)];
  uint8_t pattern2[sizeof(pattern2_array)];
  uint8_t pattern3[sizeof(pattern3_array)];
  uint8_t pattern4[sizeof(pattern4_array)];
  uint8_t pattern5[sizeof(pattern5_array)];
  uint8_t pattern6[sizeof(pattern6_array)];
  uint8_t pattern7[sizeof(pattern7_array)];
} score = {
  .fmt = ATM_SCORE_FMT_FULL,
  .num_patterns = NUM_PATTERNS(struct score_data),
  .patterns_offset = {
      offsetof(struct score_data, pattern0),
      offsetof(struct score_data, pattern1),
      offsetof(struct score_data, pattern2),
      offsetof(struct score_data, pattern3),
      offsetof(struct score_data, pattern4),
      offsetof(struct score_data, pattern5),
      offsetof(struct score_data, pattern6),
      offsetof(struct score_data, pattern7),
  },
  .num_channels = 4,
  .start_patterns = {
    0x02,                         // Channel 0 entry track (PULSE)
    0x01,                         // Channel 1 entry track (SQUARE)
    0x00,                         // Channel 2 entry track (TRIANGLE)
    0x03,                         // Channel 3 entry track (NOISE)
  },
  .pattern0 = pattern0_data,
  .pattern1 = pattern1_data,
  .pattern2 = pattern2_data,
  .pattern3 = pattern3_data,
  .pattern4 = pattern4_data,
  .pattern5 = pattern5_data,
  .pattern6 = pattern6_data,
  .pattern7 = pattern7_data,
};

const PROGMEM struct sfx1_data {
  uint8_t fmt;
  uint8_t pattern0[9];
} sfx1 = {
  .fmt = ATM_SCORE_FMT_MINIMAL_MONO,
  .pattern0 = {
    ATM_CMD_M_SET_TEMPO(22),
    ATM_CMD_M_SET_VOLUME(63),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS(11),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_M_DELAY_TICKS(11),
    ATM_CMD_I_STOP,
  },
};

#endif

