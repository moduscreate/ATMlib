#include <Arduboy2.h>
#include <ATMlib.h>

#include "atm_cmd_constants.h"

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

const PROGMEM struct tempo_test_sfx {
  uint8_t fmt;
  uint8_t pattern0[22];
} tempo_test_sfx = {
  .fmt = ATM_SCORE_FMT_MINIMAL_MONO,
  .pattern0 = {
    /* don't set tempo, expect it to be 25 tick/s by default */
    ATM_CMD_M_SET_VOLUME(63),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_I_STOP,
  },
};

const PROGMEM struct delay_test_sfx {
  uint8_t fmt;
  uint8_t pattern0[40];
} delay_test_sfx = {
  .fmt = ATM_SCORE_FMT_MINIMAL_MONO,
  .pattern0 = {
    ATM_CMD_M_SET_TEMPO(16),
    ATM_CMD_M_SET_VOLUME(63),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS(32),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_M_DELAY_TICKS(32),
    ATM_CMD_M_SET_TEMPO(32),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS_1(64),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_M_DELAY_TICKS_1(64),
    ATM_CMD_M_SET_TEMPO(128),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS_1(256),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_M_DELAY_TICKS_1(256),
    ATM_CMD_M_SET_TEMPO(250),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS_2(500),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_M_DELAY_TICKS_2(500),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS_2(500),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_I_STOP,
  },
};

const PROGMEM struct volume_test_sfx {
  uint8_t fmt;
  uint8_t pattern0[21];
} volume_test_sfx = {
  .fmt = ATM_SCORE_FMT_MINIMAL_MONO,
  .pattern0 = {
    /* don't set tempo, expect it to be 25 tick/s by default */
    ATM_CMD_M_SET_VOLUME(2),
    ATM_CMD_I_NOTE_F5,
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_M_SET_VOLUME(4),
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_M_SET_VOLUME(8),
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_M_SET_VOLUME(16),
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_M_SET_VOLUME(32),
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_M_SET_VOLUME(64),
    ATM_CMD_M_DELAY_TICKS(25),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_I_STOP,
  },
};

const PROGMEM struct volume_slide_test_sfx {
  uint8_t fmt;
  uint8_t pattern0[45];
} volume_slide_test_sfx = {
  .fmt = ATM_SCORE_FMT_MINIMAL_MONO,
  .pattern0 = {
    ATM_CMD_M_SET_TEMPO(32),
    ATM_CMD_M_SET_VOLUME(32),
    ATM_CMD_I_NOTE_F5,
    /* sustain the note at volume 32 to create a baseline */
    ATM_CMD_M_DELAY_TICKS(16),
    ATM_CMD_M_SLIDE_VOL_ON(1),
    ATM_CMD_M_DELAY_TICKS(32),
    /* volume should be now 64 */
    ATM_CMD_M_SLIDE_VOL_ON(-1),
    ATM_CMD_M_DELAY_TICKS(32),
    /* volume should be now 32 */
    ATM_CMD_M_SET_TEMPO(128),
    ATM_CMD_M_SLIDE_VOL_ADV_ON(1, 4),
    ATM_CMD_M_DELAY_TICKS_1(128),
    /* volume should be now 64 */
    ATM_CMD_M_SLIDE_VOL_ADV_ON(-1, 4),
    ATM_CMD_M_DELAY_TICKS_1(128),
    /* volume should be now 32 */
    ATM_CMD_M_SET_TEMPO(32),
    ATM_CMD_M_SLIDE_VOL_ADV_ON(4, 4),
    ATM_CMD_M_DELAY_TICKS(32),
    /* volume should be now 64 */
    ATM_CMD_M_SLIDE_VOL_ADV_ON(-4, 4),
    ATM_CMD_M_DELAY_TICKS(32),
    /* volume should be now 32 */
    ATM_CMD_M_SLIDE_VOL_ADV_OFF,
    /* sustain the note at volume 32 to create a baseline */
    ATM_CMD_M_DELAY_TICKS(16),
    ATM_CMD_I_NOTE_OFF,
    ATM_CMD_I_STOP,
  },
};

struct test {
  const char *test_name;
  const char *test_exp_desc;
  const uint8_t *sfx_data;
};

/* Ruler                                       012345678901234567890 */
const char tempo_test_name[] PROGMEM =        "Tempo test";
const char tempo_test_desc[] PROGMEM =        "5 one second tones \n"
                                              "spaced by one second \n"
                                              "silence";

const char delay_test_name[] PROGMEM =        "Delay test";
const char delay_test_desc[] PROGMEM =        "5 two seconds tones \n"
                                              "spaced by two seconds\n"
                                              "silence";

const char volume_test_name[] PROGMEM =       "Volume test";
const char volume_test_desc[] PROGMEM =       "1 tone doubling its \n"
                                              "volume every second \n"
                                              "from 2 to 64 (6 \n"
                                              "seconds total)";

const char volume_slide_test_name[] PROGMEM = "Volume slide test";
const char volume_slide_test_desc[] PROGMEM = "Slide volume up in 1\n"
                                              "second then down in 1\n"
                                              "second for 3 times\n"
                                              "using different\n"
                                              "paramters and tempos";

struct test tests[] = {
  /* Tempo test: make sure tempo matches expected ticks/s and tempo defaults to 25 ticks/s */
  {tempo_test_name, tempo_test_desc, (const uint8_t*)&tempo_test_sfx},
  /* Delay test: make sure immediate, medium and long delays work as expected */
  {delay_test_name, delay_test_desc, (const uint8_t*)&delay_test_sfx},
  /* Volume test: make sure volume can be set while a note is being played */
  {volume_test_name, volume_test_desc, (const uint8_t*)&volume_test_sfx},
  /* Volume slide test: make sure volume volume slide effect behaves as expected */
  {volume_slide_test_name, volume_slide_test_desc, (const uint8_t*)&volume_slide_test_sfx},
};

Arduboy2 arduboy;
struct atm_sfx_state sfx_state;

uint8_t selected_test_idx = 0;
bool updated = true;

void setup() {
  arduboy.begin();
  // set the framerate of the game at 60 fps
  arduboy.setFrameRate(15);
  // let's make sure the sound was not muted in a previous sketch
  arduboy.audio.on();
  // setup ATMlib
  atm_synth_setup();
}

void loop() {
  if (!(arduboy.nextFrame()))
    return;

  arduboy.pollButtons();

  if (arduboy.justPressed(DOWN_BUTTON)) {
    selected_test_idx += selected_test_idx < ARRAY_SIZE(tests)-1 ? 1 : 0;
    updated = true;
  }
  
  if (arduboy.justPressed(UP_BUTTON)) {
    selected_test_idx -= selected_test_idx ? 1 : 0;
    updated = true;
  }

  if (arduboy.justPressed(B_BUTTON)) {
    atm_synth_play_sfx_track(OSC_CH_ONE, (const uint8_t*)tests[selected_test_idx].sfx_data, &sfx_state);
  }

  if (arduboy.justPressed(A_BUTTON)) {
    atm_synth_stop_sfx_track(&sfx_state);
  }

  if (updated) {
    char text_buffer[128];
    const struct test *sel_test = &tests[selected_test_idx];
    arduboy.clear();
    sprintf(text_buffer, "%02hhu: %S\n", selected_test_idx+1, (const PROGMEM wchar_t *)sel_test->test_name);
    arduboy.print(text_buffer);
    sprintf(text_buffer, "\n%S", (const PROGMEM wchar_t *)sel_test->test_exp_desc);
    arduboy.print(text_buffer);
    arduboy.display();
    updated = false;
  }
}
