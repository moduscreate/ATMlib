
#include <stddef.h>
#include <stdint.h>

#include "isr.h"

#define ATM_PATTERN_STACK_DEPTH (3)

/* Disable all FX to save space */
/* #define ATM_HAS_FX_NONE (1) */

#if ATM_HAS_FX_NONE
#define ATM_HAS_FX_ARPEGGIO (0)
#define ATM_HAS_FX_NOTCUT (0)
#define ATM_HAS_FX_NOISE_RETRIG (0)
#define ATM_HAS_FX_VOL_SLIDE (0)
#define ATM_HAS_FX_FREQ_SLIDE (0)
#define ATM_HAS_FX_MOD_SLIDE (0)
#define ATM_HAS_FX_VIBRATO (0)
#define ATM_HAS_FX_TREMOLO (0)
#define ATM_HAS_FX_GLISSANDO (0)
#else
#define ATM_HAS_FX_ARPEGGIO (1)
#define ATM_HAS_FX_NOTCUT (1)
#define ATM_HAS_FX_NOISE_RETRIG (1)
#define ATM_HAS_FX_VOL_SLIDE (1)
#define ATM_HAS_FX_FREQ_SLIDE (1)
#define ATM_HAS_FX_MOD_SLIDE (1)
#define ATM_HAS_FX_VIBRATO (1)
#define ATM_HAS_FX_TREMOLO (1)
#define ATM_HAS_FX_GLISSANDO (1)
#endif

#define ATM_HAS_FX_NOTE_RETRIG (ATM_HAS_FX_ARPEGGIO || ATM_HAS_FX_NOTCUT)
#define ATM_HAS_FX_SLIDE (ATM_HAS_FX_VOL_SLIDE || ATM_HAS_FX_FREQ_SLIDE || ATM_HAS_FX_MOD_SLIDE)
#define ATM_HAS_FX_LFO (ATM_HAS_FX_TREMOLO || ATM_HAS_FX_VIBRATO)


struct atmlib_state {
	const uint8_t *score_start;
	uint8_t tick_rate;
	uint8_t channel_active_mute; //0b11110000;
	//                               ||||||||
	//                               |||||||└->  0  channel 0 is muted (0 = false / 1 = true)
	//                               ||||||└-->  1  channel 1 is muted (0 = false / 1 = true)
	//                               |||||└--->  2  channel 2 is muted (0 = false / 1 = true)
	//                               ||||└---->  3  channel 3 is muted (0 = false / 1 = true)
	//                               |||└----->  4  channel 0 is Active (0 = false / 1 = true)
	//                               ||└------>  5  channel 1 is Active (0 = false / 1 = true)
	//                               |└------->  6  channel 2 is Active (0 = false / 1 = true)
	//                               └-------->  7  channel 3 is Active (0 = false / 1 = true)
};

struct mod_sfx {
	uint8_t num_tracks;
	uint16_t tracks_offset[];
};

#if ATM_HAS_FX_SLIDE

struct slide_params {
	int8_t slide_amount;
	uint8_t slide_config;
	uint8_t slide_count;
};

#endif

struct pattern_state {
	const uint8_t *next_cmd_ptr;
	uint8_t pattern_index; /* TODO: look into removing this */
	uint8_t repetitions_counter;
};

struct channel_state {
	uint8_t note;
	uint8_t vol;
	uint16_t delay;

	// Nesting
	struct pattern_state pstack[ATM_PATTERN_STACK_DEPTH];
	uint8_t pstack_index;
	uint8_t repeat_point;

	struct osc_params *osc_params;

#if ATM_HAS_FX_SLIDE
	// Volume & Frequency slide FX
	struct slide_params vf_slide;
#endif

#if ATM_HAS_FX_NOTE_RETRIG
	// Arpeggio or Note Cut FX
	uint8_t arpNotes;       // notes: base, base+[7:4], base+[7:4]+[3:0], if FF => note cut ON
	uint8_t arpTiming;      // [7] = reserved, [6] = not third note ,[5] = retrigger, [4:0] = tick count
	uint8_t arpCount;
#endif

#if ATM_HAS_FX_NOISE_RETRIG
	// Retrig FX
	uint8_t reConfig;       // [7:2] = , [1:0] = speed // used for the noise channel
	uint8_t reCount;
#endif

	// Transposition FX
	int8_t transConfig;

#if ATM_HAS_FX_LFO
	// Tremolo or Vibrato FX
	uint8_t treviDepth;
	uint8_t treviConfig;
	uint8_t treviCount;
#endif

#if ATM_HAS_FX_GLISSANDO
	// Glissando FX
	int8_t glisConfig;
	uint8_t glisCount;
#endif
};

struct mod_sfx_state {
	uint8_t ch_index;
	struct atmlib_state track_info;
	struct channel_state channel_state;
	struct osc_params osc_params;
};

extern struct atmlib_state atmlib_state;

uint16_t read_vle(const uint8_t **pp);

void atm_synth_setup(void);

void atm_synth_play_score(const uint8_t *score);
void atm_synth_stop_score(void);
uint8_t atm_synth_is_score_stopped(void);

void atm_synth_set_score_paused(const uint8_t paused);
uint8_t atm_synth_get_score_paused(void);

void atm_synth_set_muted(const uint8_t channel_mask);
uint8_t atm_synth_get_muted(void);

void atm_synth_grab_channel(const uint8_t channel_index, struct osc_params *save);
void atm_synth_release_channel(const uint8_t channel_index);

void atm_synth_play_sfx_track(const uint8_t channel_index, const struct mod_sfx *sfx, struct mod_sfx_state *sfx_state);
void atm_synth_stop_sfx_track(const uint8_t channel_index);
