
#include <string.h>
#include <avr/pgmspace.h>

#include "ATMlib.h"

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

struct atmlib_state atmlib_state;

#define MAX_VOLUME (63)
#define MAX_OSC_PHASE_INC (9397)
#define LAST_NOTE (63)
const uint16_t noteTable[64] PROGMEM = {
	0,
	274,  291,  308,  326,  346,  366,  388,  411,  435,  461,  489,  518,
	549,  581,  616,  652,  691,  732,  776,  822,  871,  923,  978,  1036,
	1097, 1163, 1232, 1305, 1383, 1465, 1552, 1644, 1742, 1845, 1955, 2071,
	2195, 2325, 2463, 2610, 2765, 2930, 3104, 3288, 3484, 3691, 3910, 4143,
	4389, 4650, 4927, 5220, 5530, 5859, 6207, 6577, 6968, 7382, 7821, 8286,
	8779, 9301, 9854,
};
#define note_index_2_phase_inc(note_idx) (pgm_read_word(&noteTable[(note_idx) & 0x3F]))

static void atm_synth_tick_handler(uint8_t cb_index, void *priv);

struct slide_params {
	int8_t slide_amount;
	uint8_t slide_config;
	uint8_t slide_count;
};

/* flags: bit 7 = 0 clamp, 1 wraparound */
uint16_t slide_quantity(int8_t amount, int16_t value, int16_t bottom, int16_t top, uint8_t flags)
{
	const bool clamp = !(flags & 0x80);
	const int16_t res = value + amount;
	if (clamp) {
		if (res < bottom) {
			return bottom;
		} else if (res > top) {
			return top;
		}
	}
	return res;
}

static void slidefx(struct slide_params *slide_params, struct osc_params *osc_params)
{
	if (slide_params->slide_amount) {
		if ((slide_params->slide_count & 0x3F) >= (slide_params->slide_config & 0x3F)) {
			switch (slide_params->slide_count & 0xC0) {
				case 0:
					osc_params->vol = slide_quantity(
											slide_params->slide_amount,
											osc_params->vol,
											0,
											MAX_VOLUME,
											slide_params->slide_config & 0x80);
					break;
				case 0x40:
					osc_params->phase_increment = slide_quantity(
													slide_params->slide_amount,
													osc_params->phase_increment,
													0,
													MAX_OSC_PHASE_INC,
													slide_params->slide_config & 0x80);
					break;
				case 0x80:
					osc_params->mod = slide_quantity(
													slide_params->slide_amount,
													osc_params->mod,
													0,
													OSC_MOD_MAX,
													slide_params->slide_config & 0x80);
					break;
			}
			slide_params->slide_count &= 0xC0;
		} else {
			slide_params->slide_count++;
		}
	}
}

struct channel_state {
	const uint8_t *ptr;
	uint8_t note;

	// Nesting
	uint8_t *stackPointer[7];
	uint8_t stackCounter[7];
	uint8_t stackTrack[7]; // note 1
	uint8_t stackIndex;
	uint8_t repeatPoint;

	// Looping
	uint16_t delay;
	uint8_t counter;
	uint8_t track;

	// Shadow OSC params
	struct osc_params osc_params;

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

struct channel_state channels[CH_COUNT];

uint16_t read_vle(const uint8_t **pp) {
	uint16_t q = 0;
	uint8_t d;
	do {
		q <<= 7;
		d = pgm_read_byte(*pp++);
		q |= (d & 0x7F);
	} while (d & 0x80);
	return q;
}

static inline const uint8_t *get_track_start_ptr(const uint8_t track_index)
{
	return atmlib_state.tracks_base + pgm_read_word(&atmlib_state.track_list[track_index]);
}

void atm_synth_setup(void)
{
	osc_setup();
	atmlib_state.channel_active_mute = 0b11110000;
}

// Stop playing, unload melody
static void atm_synth_stop(void) {
	osc_set_tick_callback(0, NULL, NULL);
}

void atm_synth_play_score(const uint8_t *score)
{
	/* stop current score if any */
	atm_synth_stop();
	/* clean up */
	memset(channels, 0, sizeof(channels));
	/* mark the channel as stopped */
	for (uint8_t n = 0; n < ARRAY_SIZE(channels); n++) {
		channels[n].delay = 0xFFFF;
	}
	/* Set default score data */
	channels[CH_THREE].osc_params.phase_increment = 0x0001; // xFX
	atmlib_state.tick_rate = 25;
	osc_set_tick_rate(0, atmlib_state.tick_rate);
	/* Read track count */
	uint8_t tracks_count = pgm_read_byte(score++);
	/* Store track list pointer */
	atmlib_state.track_list = (uint16_t*)score;
	/* Store track pointer */
	score += tracks_count*sizeof(uint16_t);
	atmlib_state.tracks_base = score + CH_COUNT;
	/* Fetch starting points for each track */
	for (unsigned n = 0; n < ARRAY_SIZE(channels); n++) {
		channels[n].ptr = get_track_start_ptr(pgm_read_byte(score++));
		channels[n].delay = 0;
		channels[n].osc_params.mod = 0x7F;
	}
	/* start playaback */
	osc_set_tick_callback(0, atm_synth_tick_handler, NULL);
}

void ATMsynth::play(const uint8_t *score) {
	if (!setup_done) {
		atm_synth_setup();
		setup_done = true;
	}
	atm_synth_play_score(score);
}

// Stop playing, unload melody
void ATMsynth::stop() {
	atm_synth_stop();
}

// Start grinding samples or Pause playback
void ATMsynth::playPause() {
	osc_tick_callback cb;
	osc_get_tick_callback(0, &cb, NULL);
	if (*cb) {
		osc_set_tick_callback(0, NULL, NULL);
	} else {
		osc_set_tick_callback(0, atm_synth_tick_handler, NULL);
	}
}

void atm_synth_set_muted(const uint8_t channel_mask)
{
	atmlib_state.channel_active_mute ^= (atmlib_state.channel_active_mute ^ channel_mask) & 0x0F;
}

uint8_t atm_synth_get_muted(void)
{
	return atmlib_state.channel_active_mute & 0x0F;
}

// Toggle mute on/off on a channel, so it can be used for sound effects
// So you have to call it before and after the sound effect
void ATMsynth::muteChannel(uint8_t ch) {
	atm_synth_set_muted((1 << ch) | atm_synth_get_muted());
}

void ATMsynth::unMuteChannel(uint8_t ch) {
	atm_synth_set_muted((1 << ch) & atm_synth_get_muted());
}

static inline process_cmd(const uint8_t n, const uint8_t cmd, struct channel_state *ch)
{
	if (cmd < 64) {
		// 0 … 63 : NOTE ON/OFF
		ch->note = cmd;
		if (ch->note) {
			ch->note += ch->transConfig;
		}
		ch->osc_params.phase_increment = note_index_2_phase_inc(ch->note);
		if (!ch->vf_slide.slide_config) {
			ch->osc_params.vol = ch->reCount;
		}
		if (ch->arpTiming & 0x20) {
			ch->arpCount = 0; // ARP retriggering
		}
	} else if (cmd < 160) {
		// 64 … 159 : SETUP FX
		switch (cmd - 64) {
			case 0: // Set volume
				ch->osc_params.vol = pgm_read_byte(ch->ptr++);
				ch->reCount = ch->osc_params.vol;
				break;
			case 1: // Slide volume ON
				ch->vf_slide.slide_count = 0x00;
				goto slide_on;
			case 4: // Slide frequency ON
				ch->vf_slide.slide_count = 0x40;
				goto slide_on;
			case 23: // Slide modulation ON
				ch->vf_slide.slide_count = 0x80;
				goto slide_on;
			case 2: // Slide volume ON advanced
				ch->vf_slide.slide_count = 0x00;
				goto slide_on_adv;
			case 5: // Slide frequency ON advanced
				ch->vf_slide.slide_count = 0x40;
				goto slide_on_adv;
			case 24: // Slide modulation ON advanced
				ch->vf_slide.slide_count = 0x80;
				/* fall though to slide_on_adv label */
slide_on_adv:
				ch->vf_slide.slide_amount = pgm_read_byte(ch->ptr++);
				ch->vf_slide.slide_config = pgm_read_byte(ch->ptr++);
				break;
slide_on:
				ch->vf_slide.slide_amount = pgm_read_byte(ch->ptr++);
				ch->vf_slide.slide_config = 0;
				break;
			case 3: // Slide volume off
			case 6: // Slide frequency off
			case 25: // Modulation slide off
				ch->vf_slide.slide_amount = 0;
				break;
			case 7: // Set Arpeggio
				ch->arpNotes = pgm_read_byte(ch->ptr++);    // 0x40 + 0x03
				ch->arpTiming = pgm_read_byte(ch->ptr++);   // 0x40 (no third note) + 0x20 (toggle retrigger) + amount
				break;
			case 8: // Arpeggio OFF
				ch->arpNotes = 0;
				break;
			case 9: // Set Retriggering (noise)
				ch->reConfig = pgm_read_byte(ch->ptr++);    // RETRIG: point = 1 (*4), speed = 0 (0 = fastest, 1 = faster , 2 = fast)
				ch->reCount = 0;
				break;
			case 10: // Retriggering (noise) OFF
				ch->reConfig = 0;
				break;
			case 11: // ADD Transposition
				ch->transConfig += (int8_t)pgm_read_byte(ch->ptr++);
				break;
			case 12: // SET Transposition
				ch->transConfig = pgm_read_byte(ch->ptr++);
				break;
			case 13: // Transposition OFF
				ch->transConfig = 0;
				break;
			case 14:
			case 16: // SET Tremolo/Vibrato
				ch->treviDepth = pgm_read_word(ch->ptr++);
				ch->treviConfig = pgm_read_word(ch->ptr++) + (cmd == 78 ? 0x00 : 0x40);
				ch->treviCount = 0;
				break;
			case 15:
			case 17: // Tremolo/Vibrato OFF
				ch->treviDepth = 0;
				break;
			case 18: // Glissando
				ch->glisConfig = pgm_read_byte(ch->ptr++);
				ch->glisCount = 0;
				break;
			case 19: // Glissando OFF
				ch->glisConfig = 0;
				break;
			case 20: // SET Note Cut
				ch->arpNotes = 0xFF;                        // 0xFF use Note Cut
				ch->arpTiming = pgm_read_byte(ch->ptr++);   // tick amount
				break;
			case 21: // Note Cut OFF
				ch->arpNotes = 0;
				break;
			case 22: // Set modulation
				ch->osc_params.mod = pgm_read_byte(ch->ptr++);
				break;
			case 92: // ADD tempo
				atmlib_state.tick_rate += pgm_read_byte(ch->ptr++);
				osc_set_tick_rate(0, atmlib_state.tick_rate);
				break;
			case 93: // SET tempo
				atmlib_state.tick_rate = pgm_read_byte(ch->ptr++);
				osc_set_tick_rate(0, atmlib_state.tick_rate);
				break;
			case 94: // Goto advanced
				for (uint8_t i = 0; i < ARRAY_SIZE(channels); i++) {
					channels[i].repeatPoint = pgm_read_byte(ch->ptr++);
				}
				break;
			case 95: // Stop channel
				atmlib_state.channel_active_mute = atmlib_state.channel_active_mute ^ (1 << (n + CH_COUNT));
				ch->osc_params.vol = 0;
				ch->delay = 0xFFFF;
				break;
		}
	} else if (cmd < 224) {
		// 160 … 223 : DELAY
		ch->delay = cmd - 159;
	} else if (cmd == 224) {
		// 224: LONG DELAY
		ch->delay = read_vle(&ch->ptr) + 65;
	} else if (cmd < 252) {
		// 225 … 251 : RESERVED
	} else if (cmd == 252 || cmd == 253) {
		// 252 (253) : CALL (REPEATEDLY)
		uint8_t new_counter = cmd == 252 ? 0 : pgm_read_byte(ch->ptr++);
		uint8_t new_track = pgm_read_byte(ch->ptr++);

		if (new_track != ch->track) {
			// Stack PUSH
			ch->stackCounter[ch->stackIndex] = ch->counter;
			ch->stackTrack[ch->stackIndex] = ch->track; // note 1
			ch->stackPointer[ch->stackIndex] = ch->ptr;
			ch->stackIndex++;
			ch->track = new_track;
		}

		ch->counter = new_counter;
		ch->ptr = get_track_start_ptr(ch->track);
	} else if (cmd == 254) {
		// 254 : RETURN
		if (ch->counter > 0 || ch->stackIndex == 0) {
			// Repeat track
			if (ch->counter) {
				ch->counter--;
			}
			ch->ptr = get_track_start_ptr(ch->track);
			//asm volatile ("  jmp 0"); // reboot
		} else {
			// Check stack depth
			if (ch->stackIndex == 0) {
				// Stop the channel
				ch->delay = 0xFFFF;
			} else {
				// Stack POP
				ch->stackIndex--;
				ch->ptr = ch->stackPointer[ch->stackIndex];
				ch->counter = ch->stackCounter[ch->stackIndex];
				ch->track = ch->stackTrack[ch->stackIndex]; // note 1
			}
		}
	} else if (cmd == 255) {
		// 255 : EMBEDDED DATA
		ch->ptr += read_vle(&ch->ptr);
	}
}

static void atm_synth_tick_handler(uint8_t cb_index, void *priv) {
	// for every channel start working
	for (uint8_t ch_index = 0; ch_index < ARRAY_SIZE(channels); ch_index++)
	{
		struct channel_state *ch = &channels[ch_index];
		bool noise_retrigger = false;

		// Noise retriggering
		if (ch_index == CH_THREE && ch->reConfig && (ch->reCount++ >= (ch->reConfig & 0x03))) {
			ch->osc_params.phase_increment = note_index_2_phase_inc(ch->reConfig >> 2);
			noise_retrigger = true;
			ch->reCount = 0;
		}

		//Apply Glissando
		if (ch->glisConfig && (ch->glisCount++ >= (ch->glisConfig & 0x7F))) {
			const uint8_t amount = (ch->glisConfig & 0x80) ? -1 : 1;
			const uint8_t note = slide_quantity(amount, ch->note, 1, LAST_NOTE, 0);
			ch->note = note;
			ch->osc_params.phase_increment = note_index_2_phase_inc(note);
			ch->glisCount = 0;
		}

		// Apply volume/frequency slides
		slidefx(&ch->vf_slide, &ch->osc_params);

		// Apply Arpeggio or Note Cut
		if (ch->arpNotes && ch->note) {
			if ((ch->arpCount & 0x1F) < (ch->arpTiming & 0x1F)) {
				ch->arpCount++;
			} else {
				if ((ch->arpCount & 0xE0) == 0x00) {
					ch->arpCount = 0x20;
				} else if ((ch->arpCount & 0xE0) == 0x20 && !(ch->arpTiming & 0x40) && (ch->arpNotes != 0xFF)) {
					ch->arpCount = 0x40;
				} else {
					ch->arpCount = 0x00;
				}
				uint8_t arpNote = ch->note;
				if ((ch->arpCount & 0xE0) != 0x00) {
					if (ch->arpNotes == 0xFF) {
						arpNote = 0;
					} else {
						arpNote += (ch->arpNotes >> 4);
					}
				}
				if ((ch->arpCount & 0xE0) == 0x40) {
					arpNote += (ch->arpNotes & 0x0F);
				}
				// Is it correct to add ch->transConfig to arpNote? The existing note should
				// already be transposed.
				ch->osc_params.phase_increment = note_index_2_phase_inc(arpNote + ch->transConfig);
			}
		}

		// Apply Tremolo or Vibrato
		if (ch->treviDepth) {
			int16_t vt = ((ch->treviConfig & 0x40) ? ch->osc_params.phase_increment : ch->osc_params.vol);
			vt = (ch->treviCount & 0x80) ? (vt + ch->treviDepth) : (vt - ch->treviDepth);
			if (vt < 0) {
				vt = 0;
			} else if (ch->treviConfig & 0x40) {
				if (vt > MAX_OSC_PHASE_INC) {
					vt = MAX_OSC_PHASE_INC;
				}
			} else if (!(ch->treviConfig & 0x40)) {
				if (vt > MAX_VOLUME) {
					vt = MAX_VOLUME;
				}
			}
			if (ch->treviConfig & 0x40) {
				ch->osc_params.phase_increment = vt;
			} else {
				ch->osc_params.vol = vt;
			}
			if ((ch->treviCount & 0x1F) < (ch->treviConfig & 0x1F)) {
				ch->treviCount++;
			} else {
				if (ch->treviCount & 0x80) {
					ch->treviCount = 0;
				} else {
					ch->treviCount = 0x80;
				}
			}
		}

		while (ch->delay == 0) {
			const uint8_t cmd = pgm_read_byte(ch->ptr++);
			process_cmd(ch_index, cmd, ch);
		}

		if (ch->delay != 0xFFFF) {
			ch->delay--;
		}

		if (!(atmlib_state.channel_active_mute & (1 << ch_index))) {
			const uint8_t flags = (noise_retrigger || ch_index != CH_THREE) ? 0 : 0x1;
			osc_update_osc(ch_index, &ch->osc_params, flags);
		}
	}

	// if all channels are inactive, stop playing or check for repeat
	if (!(atmlib_state.channel_active_mute & 0xF0)) {
		uint8_t repeatSong = 0;
		for (uint8_t j = 0; j < ARRAY_SIZE(channels); j++) {
			repeatSong += channels[j].repeatPoint;
		}
		if (repeatSong) {
			for (uint8_t k = 0; k < ARRAY_SIZE(channels); k++) {
				channels[k].ptr = get_track_start_ptr(channels[k].repeatPoint);
				channels[k].delay = 0;
			}
			atmlib_state.channel_active_mute |= 0b11110000;
		} else {
			atm_synth_stop();
		}
	}
}
