
#include <string.h>
#include <avr/pgmspace.h>

#include "ATMlib.h"

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

uint8_t trackCount;
uint8_t tickRate;
const uint16_t *trackList;
const uint8_t *trackBase;

uint8_t ChannelActiveMute = 0b11110000;
//                            ||||||||
//                            |||||||└->  0  channel 0 is muted (0 = false / 1 = true)
//                            ||||||└-->  1  channel 1 is muted (0 = false / 1 = true)
//                            |||||└--->  2  channel 2 is muted (0 = false / 1 = true)
//                            ||||└---->  3  channel 3 is muted (0 = false / 1 = true)
//                            |||└----->  4  channel 0 is Active (0 = false / 1 = true)
//                            ||└------>  5  channel 1 is Active (0 = false / 1 = true)
//                            |└------->  6  channel 2 is Active (0 = false / 1 = true)
//                            └-------->  7  channel 3 is Active (0 = false / 1 = true)

const uint16_t noteTable[64] PROGMEM = {
	0,
	262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494,
	523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988,
	1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
	2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
	4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
	8372, 8870, 9397,
};

struct channel_state {
	const uint8_t *ptr;
	uint8_t note;

	// Nesting
	uint16_t stackPointer[7];
	uint8_t stackCounter[7];
	uint8_t stackTrack[7]; // note 1
	uint8_t stackIndex;
	uint8_t repeatPoint;

	// Looping
	uint16_t delay;
	uint8_t counter;
	uint8_t track;

	// External FX
	uint16_t phase_increment;
	uint8_t vol;

	// Volume & Frequency slide FX
	char volFreSlide;
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
	char transConfig;

	// Tremolo or Vibrato FX
	uint8_t treviDepth;
	uint8_t treviConfig;
	uint8_t treviCount;

	// Glissando FX
	char glisConfig;
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

static inline const uint8_t *getTrackPointer(uint8_t track) {
	return trackBase + pgm_read_word(&trackList[track]);
}

// Stop playing, unload melody
static void atmsynth_stop(void) {
	TIMSK4 = 0; // Disable interrupt
	memset(osc, 0, sizeof(osc));
	memset(channels, 0, sizeof(channels));
	/* mark the channel as stopped */
	for (uint8_t n = 0; n < ARRAY_SIZE(channels); n++) {
		channels[n].delay = 0xFFFF;
	}
	ChannelActiveMute = 0b11110000;
}

void ATMsynth::play(const uint8_t *song) {

	// cleanUp stuff first
	atmsynth_stop();

	// Initializes ATMsynth
	// Sets sample rate and tick rate
	tickRate = 25;
	cia = 15625 / tickRate;
	cia_count = cia;
	// Sets up the ports, and the sample grinding ISR
	osc[CH_THREE].phase_increment = 0x0001; // Seed LFSR
	channels[CH_THREE].phase_increment = 0x0001; // xFX

	TCCR4A = 0b01000010;    // Fast-PWM 8-bit
	TCCR4B = 0b00000001;    // 62500Hz
	OCR4C  = 0xFF;          // Resolution to 8-bit (TOP=0xFF)
	OCR4A  = 0x80;
	TIMSK4 = 0b00000100;


	// Load a melody stream and start grinding samples
	// Read track count
	trackCount = pgm_read_byte(song++);
	// Store track list pointer
	trackList = (uint16_t*)song;
	// Store track pointer
	trackBase = (song += (trackCount << 1)) + CH_COUNT;
	// Fetch starting points for each track
	for (unsigned n = 0; n < ARRAY_SIZE(channels); n++) {
		channels[n].ptr = getTrackPointer(pgm_read_byte(song++));
		channels[n].delay = 0;
	}
}

// Stop playing, unload melody
void ATMsynth::stop() {
	atmsynth_stop();
}

// Start grinding samples or Pause playback
void ATMsynth::playPause() {
	TIMSK4 = TIMSK4 ^ 0b00000100; // toggle disable/enable interrupt
}

// Toggle mute on/off on a channel, so it can be used for sound effects
// So you have to call it before and after the sound effect
void ATMsynth::muteChannel(uint8_t ch) {
	ChannelActiveMute += (1 << ch);
}

void ATMsynth::unMuteChannel(uint8_t ch) {
	ChannelActiveMute &= (~(1 << ch));
}

static inline process_cmd(const uint8_t n, const uint8_t cmd, struct channel_state *ch)
{
	if (cmd < 64) {
		// 0 … 63 : NOTE ON/OFF
		if (ch->note = cmd) {
			ch->note += ch->transConfig;
		}
		ch->phase_increment = pgm_read_word(&noteTable[ch->note]);
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
				ch->volFreConfig = (cmd - 64) == 1 ? 0x00 : 0x40;
				break;
			case 2:
			case 5: // Slide volume/frequency ON advanced
				ch->volFreSlide = pgm_read_byte(ch->ptr++);
				ch->volFreConfig = pgm_read_byte(ch->ptr++);
				break;
			case 3:
			case 6: // Slide volume/frequency OFF (same as 0x01 0x00)
				ch->volFreSlide = 0;
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
				ch->transConfig += (char)pgm_read_byte(ch->ptr++);
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
				ch->treviConfig = pgm_read_word(ch->ptr++) + ((cmd - 64) == 14 ? 0x00 : 0x40);
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
				break;
			case 20: // SET Note Cut
				ch->arpNotes = 0xFF;                        // 0xFF use Note Cut
				ch->arpTiming = pgm_read_byte(ch->ptr++);   // tick amount
				break;
			case 21: // Note Cut OFF
				ch->arpNotes = 0;
				break;
			case 92: // ADD tempo
				tickRate += pgm_read_byte(ch->ptr++);
				cia = 15625 / tickRate;
				break;
			case 93: // SET tempo
				tickRate = pgm_read_byte(ch->ptr++);
				cia = 15625 / tickRate;
				break;
			case 94: // Goto advanced
				for (uint8_t i = 0; i < ARRAY_SIZE(channels); i++) channels[i].repeatPoint = pgm_read_byte(ch->ptr++);
				break;
			case 95: // Stop channel
				ChannelActiveMute = ChannelActiveMute ^ (1 << (n + CH_COUNT));
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
			ch->stackPointer[ch->stackIndex] = ch->ptr - trackBase;
			ch->stackIndex++;
			ch->track = new_track;
		}

		ch->counter = new_counter;
		ch->ptr = getTrackPointer(ch->track);
	} else if (cmd == 254) {
		// 254 : RETURN
		if (ch->counter > 0 || ch->stackIndex == 0) {
			// Repeat track
			if (ch->counter) {
				ch->counter--;
			}
			ch->ptr = getTrackPointer(ch->track);
			//asm volatile ("  jmp 0"); // reboot
		} else {
			// Check stack depth
			if (ch->stackIndex == 0) {
				// Stop the channel
				ch->delay = 0xFFFF;
			} else {
				// Stack POP
				ch->stackIndex--;
				ch->ptr = ch->stackPointer[ch->stackIndex] + trackBase;
				ch->counter = ch->stackCounter[ch->stackIndex];
				ch->track = ch->stackTrack[ch->stackIndex]; // note 1
			}
		}
	} else if (cmd == 255) {
		// 255 : EMBEDDED DATA
		ch->ptr += read_vle(&ch->ptr);
	}
}

__attribute__((used))
void ATM_playroutine() {
	// for every channel start working
	for (uint8_t n = 0; n < ARRAY_SIZE(channels); n++)
	{
		struct channel_state *ch = &channels[n];

		// Noise retriggering
		if (ch->reConfig) {
			if (ch->reCount >= (ch->reConfig & 0x03)) {
				osc[n].phase_increment = pgm_read_word(&noteTable[ch->reConfig >> 2]);
				ch->reCount = 0;
			} else {
				ch->reCount++;
			}
		}

		//Apply Glissando
		if (ch->glisConfig) {
			if (ch->glisCount >= (ch->glisConfig & 0x7F)) {
				if (ch->glisConfig & 0x80) {
					ch->note -= 1;
				} else {
					ch->note += 1;
				}
				if (ch->note < 1) {
					ch->note = 1;
				} else if (ch->note > 63) {
					ch->note = 63;
				}
				ch->phase_increment = pgm_read_word(&noteTable[ch->note]);
				ch->glisCount = 0;
			} else {
				ch->glisCount++;
			}
		}

		// Apply volume/frequency slides
		if (ch->volFreSlide) {
			if (!ch->volFreCount) {
				int16_t vf = ((ch->volFreConfig & 0x40) ? ch->phase_increment : ch->vol);
				vf += (ch->volFreSlide);
				if (!(ch->volFreConfig & 0x80)) {
					if (vf < 0) {
						vf = 0;
					} else if (ch->volFreConfig & 0x40) {
						if (vf > 9397) {
							vf = 9397;
						}
					} else if (!(ch->volFreConfig & 0x40)) {
						if (vf > 63) {
							vf = 63;
						}
					}
				}
				if (ch->volFreConfig & 0x40) {
					ch->phase_increment = vf;
				} else {
					ch->vol = vf;
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
				ch->phase_increment = pgm_read_word(&noteTable[arpNote + ch->transConfig]);
			}
		}

		// Apply Tremolo or Vibrato
		if (ch->treviDepth) {
			int16_t vt = ((ch->treviConfig & 0x40) ? ch->phase_increment : ch->vol);
			vt = (ch->treviCount & 0x80) ? (vt + ch->treviDepth) : (vt - ch->treviDepth);
			if (vt < 0) {
				vt = 0;
			} else if (ch->treviConfig & 0x40) {
				if (vt > 9397) {
					vt = 9397;
				}
			} else if (!(ch->treviConfig & 0x40)) {
				if (vt > 63) {
					vt = 63;
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
			process_cmd(n, cmd, ch);
		}

		if (ch->delay != 0xFFFF) {
			ch->delay--;
		}

		if (!(ChannelActiveMute & (1 << n))) {
			if (n == CH_THREE) {
				// Half volume, no frequency for noise channel
				osc[n].vol = ch->vol >> 1;
			} else {
				osc[n].phase_increment = ch->phase_increment;
				osc[n].vol = ch->vol;
			}
		}
	}

	// if all channels are inactive, stop playing or check for repeat
	if (!(ChannelActiveMute & 0xF0)) {
		uint8_t repeatSong = 0;
		for (uint8_t j = 0; j < ARRAY_SIZE(channels); j++) {
			repeatSong += channels[j].repeatPoint;
		}
		if (repeatSong) {
			for (uint8_t k = 0; k < ARRAY_SIZE(channels); k++) {
				channels[k].ptr = getTrackPointer(channels[k].repeatPoint);
				channels[k].delay = 0;
			}
			ChannelActiveMute = 0b11110000;
		} else {
			atmsynth_stop();
		}
	}
}