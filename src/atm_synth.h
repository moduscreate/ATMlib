
#include <stddef.h>
#include <stdint.h>

#include "isr.h"

struct atmlib_state {
	const uint16_t *track_list;
	const uint8_t *tracks_base;
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

struct slide_params {
	int8_t slide_amount;
	uint8_t slide_config;
	uint8_t slide_count;
};

struct channel_state {
	const uint8_t *ptr;
	uint8_t note;

	// Nesting
	const uint8_t *stackPointer[7];
	uint8_t stackCounter[7];
	uint8_t stackTrack[7]; // note 1
	uint8_t stackIndex;
	uint8_t repeatPoint;

	// Looping
	uint16_t delay;
	uint8_t counter;
	uint8_t track;

	struct osc_params *osc_params;

	// Volume & Frequency slide FX
	struct slide_params vf_slide;

	// Arpeggio or Note Cut FX
	uint8_t arpNotes;       // notes: base, base+[7:4], base+[7:4]+[3:0], if FF => note cut ON
	uint8_t arpTiming;      // [7] = reserved, [6] = not third note ,[5] = retrigger, [4:0] = tick count
	uint8_t arpCount;

	// Retrig FX
	uint8_t reConfig;       // [7:2] = , [1:0] = speed // used for the noise channel
	uint8_t reCount;        // also using this as a buffer for volume retrig on all channels

	// Transposition FX
	int8_t transConfig;

	// Tremolo or Vibrato FX
	uint8_t treviDepth;
	uint8_t treviConfig;
	uint8_t treviCount;

	// Glissando FX
	int8_t glisConfig;
	uint8_t glisCount;

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
