
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

	// Shadow OSC state
	uint16_t phase_increment;
	uint8_t vol;

	// Volume & Frequency slide FX
	int8_t volFreSlide;
	uint8_t volFreConfig;
	uint8_t volFreCount;

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

// Stop playing, unload melody
static void atmsynth_stop(void) {
	osc_reset();
	memset(channels, 0, sizeof(channels));
	/* mark the channel as stopped */
	for (uint8_t n = 0; n < ARRAY_SIZE(channels); n++) {
		channels[n].delay = 0xFFFF;
	}
	atmlib_state.channel_active_mute = 0b11110000;
}

void ATMsynth::play(const uint8_t *song) {
	// cleanUp stuff first
	atmsynth_stop();
	osc_setup();

	osc_set_tick_callback(0, atm_synth_tick_handler, NULL);

	// Initializes ATMsynth
	channels[CH_THREE].phase_increment = 0x0001; // xFX
	atmlib_state.channel_active_mute = 0b11110000;
	atmlib_state.tick_rate = 25;
	osc_set_tick_rate(0, atmlib_state.tick_rate);

	// Load a melody stream and start grinding samples
	// Read track count
	uint8_t tracks_count = pgm_read_byte(song++);
	// Store track list pointer
	atmlib_state.track_list = (uint16_t*)song;
	// Store track pointer
	song += tracks_count*sizeof(uint16_t);
	atmlib_state.tracks_base = song + CH_COUNT;
	// Fetch starting points for each track
	for (unsigned n = 0; n < ARRAY_SIZE(channels); n++) {
		channels[n].ptr = get_track_start_ptr(pgm_read_byte(song++));
		channels[n].delay = 0;
	}
	osc_setactive(true);
}

// Stop playing, unload melody
void ATMsynth::stop() {
	atmsynth_stop();
}

// Start grinding samples or Pause playback
void ATMsynth::playPause() {
	osc_toggleactive();
}

// Toggle mute on/off on a channel, so it can be used for sound effects
// So you have to call it before and after the sound effect
void ATMsynth::muteChannel(uint8_t ch) {
	atmlib_state.channel_active_mute += (1 << ch);
}

void ATMsynth::unMuteChannel(uint8_t ch) {
	atmlib_state.channel_active_mute &= (~(1 << ch));
}

static inline process_cmd(const uint8_t n, const uint8_t cmd, struct channel_state *ch)
{
	if (cmd < 64) {
		// 0 … 63 : NOTE ON/OFF
		ch->note = cmd;
		if (ch->note) {
			ch->note += ch->transConfig;
		}
		ch->phase_increment = note_index_2_phase_inc(ch->note);
		if (!ch->volFreConfig) {
			ch->vol = ch->reCount;
		}
		if (ch->arpTiming & 0x20) {
			ch->arpCount = 0; // ARP retriggering
		}
	} else if (cmd < 160) {
		// 64 … 159 : SETUP FX
		switch (cmd - 64) {
			case 0: // Set volume
				ch->vol = pgm_read_byte(ch->ptr++);
				ch->reCount = ch->vol;
				break;
			case 1:
			case 4: // Slide volume/frequency ON
				ch->volFreSlide = pgm_read_byte(ch->ptr++);
				ch->volFreConfig = (cmd == 65) ? 0x00 : 0x40;
				break;
			case 2:
			case 5: // Slide volume/frequency ON advanced
				ch->volFreSlide = pgm_read_byte(ch->ptr++);
				ch->volFreConfig = pgm_read_byte(ch->ptr++) & 0xBF;
				ch->volFreConfig |= (cmd == 66) ? 0x00 : 0x40;
				break;
			case 3:
			case 6: // Slide volume/frequency OFF
				ch->volFreSlide = 0;
				ch->volFreCount = 0;
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
				break;
			case 10: // Retriggering (noise) OFF
				ch->reConfig = 0;
				ch->reCount = 0;
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
				break;
			case 15:
			case 17: // Tremolo/Vibrato OFF
				ch->treviDepth = 0;
				break;
			case 18: // Glissando
				ch->glisConfig = pgm_read_byte(ch->ptr++);
				break;
			case 19: // Glissando OFF
				ch->glisConfig = 0;
				ch->glisCount = 0;
				break;
			case 20: // SET Note Cut
				ch->arpNotes = 0xFF;                        // 0xFF use Note Cut
				ch->arpTiming = pgm_read_byte(ch->ptr++);   // tick amount
				break;
			case 21: // Note Cut OFF
				ch->arpNotes = 0;
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
				ch->vol = 0;
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

		// Noise retriggering
		if (ch_index == CH_THREE && ch->reConfig && (ch->reCount++ >= (ch->reConfig & 0x03))) {
			osc_update_osc(CH_THREE | 0x80, note_index_2_phase_inc(ch->reConfig >> 2), ch->vol);
			ch->reCount = 0;
		}

		//Apply Glissando
		if (ch->glisConfig && (ch->glisCount++ >= (ch->glisConfig & 0x7F))) {
			const uint8_t n0 = ch->note + ((ch->glisConfig & 0x80) ? -1 : 1);
			// clamp note between 1 and 63
			const uint8_t n1 = !n0 ? 1 : n0;
			const uint8_t n2 = n1 > LAST_NOTE ? LAST_NOTE : n1;
			ch->note = n2;
			ch->phase_increment = note_index_2_phase_inc(n2);
			ch->glisCount = 0;
		}

		// Apply volume/frequency slides
		if (ch->volFreSlide) {
			// Triggered when zero so the slide applies immediately.
			// Should the slide apply after n ticks like glissando
			// and noise retrigger instead? Would be more consistent,
			// make code simpler with no loss of features.
			if (!ch->volFreCount) {
				const bool clamp = !(ch->volFreConfig & 0x80);
				if (ch->volFreConfig & 0x40) {
					// frequency slide
					int16_t ph_inc = ch->phase_increment + ch->volFreSlide;
					if (clamp) {
						ph_inc = ph_inc < 0 ? 0 : ph_inc;
						ph_inc = ph_inc > MAX_OSC_PHASE_INC ? MAX_OSC_PHASE_INC : ph_inc;
					}
					ch->phase_increment = ph_inc;
				} else {
					// volume slide
					int8_t vol = ch->vol + ch->volFreSlide;
					if (clamp) {
						vol = vol < 0 ? 0 : vol;
						vol = vol > MAX_VOLUME ? MAX_VOLUME : vol;
					}
					ch->vol = vol;
				}
			}
			if (ch->volFreCount++ >= (ch->volFreConfig & 0x3F)) {
				ch->volFreCount = 0;
			}
		}

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
				ch->phase_increment = note_index_2_phase_inc(arpNote + ch->transConfig);
			}
		}

		// Apply Tremolo or Vibrato
		if (ch->treviDepth) {
			int16_t vt = ((ch->treviConfig & 0x40) ? ch->phase_increment : ch->vol);
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
				ch->phase_increment = vt;
			} else {
				ch->vol = vt;
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
			osc_update_osc(ch_index, ch->phase_increment, ch->vol);
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
			atmsynth_stop();
		}
	}
}
