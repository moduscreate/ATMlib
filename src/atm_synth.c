
#include <alloca.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "atm_synth.h"

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

struct atmlib_state atmlib_state;

#define ATMLIB_TICKRATE_MAX (255)
#define MAX_VOLUME (63)
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

struct channel_state channels[OSC_CH_COUNT];

static void atm_synth_score_tick_handler(uint8_t cb_index, void *priv);
static void atm_synth_sfx_tick_handler(uint8_t cb_index, void *priv);

#define next_pattern_byte(ch_ptr) (pgm_read_byte((ch_ptr)->pstack[(ch_ptr)->pstack_index].next_cmd_ptr++))
#define pattern_index(ch_ptr) ((ch_ptr)->pstack[(ch_ptr)->pstack_index].pattern_index)
#define pattern_cmd_ptr(ch_ptr) ((ch_ptr)->pstack[(ch_ptr)->pstack_index].next_cmd_ptr)
#define pattern_repetition_counter(ch_ptr) ((ch_ptr)->pstack[(ch_ptr)->pstack_index].repetitions_counter)

/* flags: bit 7 = 0 clamp, 1 wraparound */
static uint16_t slide_quantity(int8_t amount, int16_t value, int16_t bottom, int16_t top, uint8_t flags)
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

static void osc_update_osc(const uint8_t osc_idx, const struct osc_params *src, const uint8_t flags)
{
	struct osc_params *dst = channels[osc_idx].osc_params;
	if (src == dst) {
		return;
	}

	if (flags & 0x1) {
		dst->vol = src->vol;
		dst->mod = src->mod;
	} else {
		*dst = *src;
	}
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
													OSC_PHASE_INC_MAX,
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

static inline const uint8_t *get_track_start_ptr(struct atmlib_state *score_state, const uint8_t track_index)
{
	const uint8_t offset = pgm_read_word(score_state->score_start+1+sizeof(uint16_t)*track_index);
	return score_state->score_start + offset;
}

void atm_synth_setup(void)
{
	osc_setup();
	atmlib_state.channel_active_mute = 0;
	for (unsigned n = 0; n < ARRAY_SIZE(channels); n++) {
		channels[n].osc_params = &osc_params_array[n];
	}
}

void atm_synth_grab_channel(const uint8_t channel_index, struct osc_params *save)
{
	atm_synth_set_muted(atm_synth_get_muted() | (1 << channel_index));
	*save = osc_params_array[channel_index];
	memset(&osc_params_array[channel_index], 0, sizeof(osc_params_array[0]));
	channels[channel_index].osc_params = save;
}

void atm_synth_release_channel(const uint8_t channel_index)
{
	osc_params_array[channel_index] = *channels[channel_index].osc_params;
	channels[channel_index].osc_params = &osc_params_array[channel_index];
	atm_synth_set_muted(atm_synth_get_muted() & ~(1 << channel_index));
}

void atm_synth_play_sfx_track(const uint8_t ch_index, const struct mod_sfx *sfx, struct mod_sfx_state *sfx_state)
{
	sfx_state->ch_index = ch_index;
	atm_synth_stop_sfx_track(ch_index);
	memset(&sfx_state->channel_state, 0, sizeof(sfx_state->channel_state));
	atm_synth_grab_channel(ch_index, &sfx_state->osc_params);
	sfx_state->channel_state.osc_params = &osc_params_array[ch_index];
	sfx_state->track_info.score_start = ((uint8_t*)sfx);
	sfx_state->track_info.channel_active_mute = 1 << (ch_index+OSC_CH_COUNT);
	osc_params_array[ch_index].mod = 0x7F;
	/* Set tick rate to ATMLIB_TICKRATE_MAX to trigger ASAP */
	sfx_state->track_info.tick_rate = 25;
	osc_set_tick_rate(1, 25);
	pattern_cmd_ptr(&sfx_state->channel_state) = get_track_start_ptr(&sfx_state->track_info, 0);
	/* Start SFX */
	osc_set_tick_callback(1, atm_synth_sfx_tick_handler, sfx_state);
}

void atm_synth_stop_sfx_track(const uint8_t ch_index)
{
	osc_set_tick_callback(1, NULL, NULL);
	atm_synth_release_channel(ch_index);
}

// stop playing
void atm_synth_stop_score(void) {
	osc_set_tick_callback(0, NULL, NULL);
}

void atm_synth_play_score(const uint8_t *score)
{
	/* stop current score if any */
	atm_synth_stop_score();
	/* Set default score data */
	//osc_params_array[OSC_CH_THREE].phase_increment = 0x0001; // xFX
	atmlib_state.channel_active_mute |= 0b11110000;
	atmlib_state.tick_rate = 25;
	osc_set_tick_rate(0, atmlib_state.tick_rate);
	/* Read track count */
	atmlib_state.score_start = score;
	uint8_t tracks_count = pgm_read_byte(score++);
	/* Store track pointer */
	score += tracks_count*sizeof(uint16_t);
	/* Fetch starting points for each track */
	for (unsigned n = 0; n < ARRAY_SIZE(channels); n++) {
		struct osc_params *o = channels[n].osc_params;
		memset(&channels[n], 0, sizeof(channels[0]));
		channels[n].osc_params = o;
		pattern_cmd_ptr(&channels[n]) = get_track_start_ptr(&atmlib_state, pgm_read_byte(score++));
		channels[n].delay = 0;
		/* this is not entirely correct because the channel may be in use by SFX */
		osc_params_array[n].mod = 0x7F;
	}
	/* Start playback */
	osc_set_tick_callback(0, atm_synth_score_tick_handler, NULL);
}

uint8_t atm_synth_is_score_stopped(void)
{
	return !(atmlib_state.channel_active_mute & 0xF0);
}

uint8_t atm_synth_get_score_paused(void)
{
	if (!(atmlib_state.channel_active_mute & 0xF0)) {
		/* paused means it can resume */
		return 0;
	}
	osc_tick_callback cb;
	osc_get_tick_callback(0, &cb, NULL);
	return cb == NULL;
}

void atm_synth_set_score_paused(const uint8_t paused)
{
	if (!(atmlib_state.channel_active_mute & 0xF0)) {
		/* if the score is not ready to play do nothing */
		return;
	}
	for (unsigned n = 0; n < ARRAY_SIZE(channels); n++) {
		if (paused) {
			channels[n].osc_params->vol = 0;
		} else {
			channels[n].osc_params->vol = channels[n].reCount;
		}
	}
	osc_set_tick_callback(0, paused ? NULL : atm_synth_score_tick_handler, NULL);
}

void atm_synth_set_muted(const uint8_t channel_mask)
{
	atmlib_state.channel_active_mute ^= (atmlib_state.channel_active_mute ^ channel_mask) & 0x0F;
}

uint8_t atm_synth_get_muted(void)
{
	return atmlib_state.channel_active_mute & 0x0F;
}

static inline void process_cmd(const uint8_t ch_index, const uint8_t cmd, struct atmlib_state *score_state, struct channel_state *ch)
{
	if (cmd < 64) {
		// 0 … 63 : NOTE ON/OFF
		ch->note = cmd;
		if (ch->note) {
			ch->note += ch->transConfig;
		}
		ch->osc_params->phase_increment = note_index_2_phase_inc(ch->note);
		if (!ch->vf_slide.slide_config) {
			ch->osc_params->vol = ch->reCount;
		}
		if (ch->arpTiming & 0x20) {
			ch->arpCount = 0; // ARP retriggering
		}
	} else if (cmd < 160) {
		// 64 … 159 : SETUP FX
		switch (cmd - 64) {
			case 0: // Set volume
				ch->osc_params->vol = next_pattern_byte(ch);
				ch->reCount = ch->osc_params->vol;
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
				ch->vf_slide.slide_amount = next_pattern_byte(ch);
				ch->vf_slide.slide_config = next_pattern_byte(ch);
				break;
slide_on:
				ch->vf_slide.slide_amount = next_pattern_byte(ch);
				ch->vf_slide.slide_config = 0;
				break;
			case 3: // Slide volume off
			case 6: // Slide frequency off
			case 25: // Modulation slide off
				ch->vf_slide.slide_amount = 0;
				break;
			case 7: // Set Arpeggio
				ch->arpNotes = next_pattern_byte(ch);    // 0x40 + 0x03
				ch->arpTiming = next_pattern_byte(ch);   // 0x40 (no third note) + 0x20 (toggle retrigger) + amount
				break;
			case 8: // Arpeggio OFF
				ch->arpNotes = 0;
				break;
			case 9: // Set Retriggering (noise)
				ch->reConfig = next_pattern_byte(ch);    // RETRIG: point = 1 (*4), speed = 0 (0 = fastest, 1 = faster , 2 = fast)
				ch->reCount = 0;
				break;
			case 10: // Retriggering (noise) OFF
				ch->reConfig = 0;
				break;
			case 11: // ADD Transposition
				ch->transConfig += (int8_t)next_pattern_byte(ch);
				break;
			case 12: // SET Transposition
				ch->transConfig = next_pattern_byte(ch);
				break;
			case 13: // Transposition OFF
				ch->transConfig = 0;
				break;
			case 14:
			case 16: // SET Tremolo/Vibrato
				ch->treviDepth = next_pattern_byte(ch);
				ch->treviConfig = next_pattern_byte(ch) + (cmd == 78 ? 0x00 : 0x40);
				ch->treviCount = 0;
				break;
			case 15:
			case 17: // Tremolo/Vibrato OFF
				ch->treviDepth = 0;
				break;
			case 18: // Glissando
				ch->glisConfig = next_pattern_byte(ch);
				ch->glisCount = 0;
				break;
			case 19: // Glissando OFF
				ch->glisConfig = 0;
				break;
			case 20: // SET Note Cut
				ch->arpNotes = 0xFF;                        // 0xFF use Note Cut
				ch->arpTiming = next_pattern_byte(ch);   // tick amount
				break;
			case 21: // Note Cut OFF
				ch->arpNotes = 0;
				break;
			case 22: // Set modulation
				ch->osc_params->mod = next_pattern_byte(ch);
				break;
			case 92: // ADD tempo
				score_state->tick_rate += next_pattern_byte(ch);
				break;
			case 93: // SET tempo
				score_state->tick_rate = next_pattern_byte(ch);
				break;
			case 94: // Goto advanced
				ch->repeat_point = next_pattern_byte(ch);
				break;
			case 95: // Stop channel
				goto stop_channel;
		}
	} else if (cmd < 224) {
		// 160 … 223 : DELAY
		ch->delay = cmd - 159;
	} else if (cmd == 224) {
		// 224: LONG DELAY
		ch->delay = read_vle(&pattern_cmd_ptr(ch)) + 65;
	} else if (cmd < 252) {
		// 225 … 251 : RESERVED
	} else if (cmd == 252 || cmd == 253) {
		// 252 (253) : CALL (REPEATEDLY)
		/* ignore call command if the stack is full */
		if (ch->pstack_index < ATM_PATTERN_STACK_DEPTH-1) {
			uint8_t new_counter = cmd == 252 ? 0 : next_pattern_byte(ch);
			uint8_t new_track = next_pattern_byte(ch);

			if (new_track != pattern_index(ch)) {
				// Stack PUSH
				ch->pstack_index++;
				pattern_index(ch) = new_track;
			}

			pattern_repetition_counter(ch) = new_counter;
			pattern_cmd_ptr(ch) = get_track_start_ptr(score_state, pattern_index(ch));
		}
	} else if (cmd == 254) {
		// 254 : RETURN
		if (pattern_repetition_counter(ch) > 0 || ch->pstack_index == 0) {
			// Repeat track
			if (pattern_repetition_counter(ch)) {
				pattern_repetition_counter(ch)--;
			}
			pattern_cmd_ptr(ch) = get_track_start_ptr(score_state, pattern_index(ch));
			//asm volatile ("  jmp 0"); // reboot
		} else {
			// Check stack depth
			if (ch->pstack_index == 0) {
				// Stop the channel
				ch->delay = 0xFFFF;
				//goto stop_channel;
			} else {
				// Stack POP
				ch->pstack_index--;
			}
		}
	} else if (cmd == 255) {
		// 255 : EMBEDDED DATA
		pattern_cmd_ptr(ch) += read_vle(&pattern_cmd_ptr(ch));
	}
	return;

stop_channel:
	score_state->channel_active_mute = score_state->channel_active_mute ^ (1 << (ch_index + OSC_CH_COUNT));
	ch->osc_params->vol = 0;
	ch->delay = 0xFFFF;
}

static inline void process_channel(const uint8_t ch_index, struct atmlib_state *score_state, struct channel_state *ch)
{
	bool noise_retrigger = false;

	// Noise retriggering
	if (ch_index == OSC_CH_THREE && ch->reConfig && (ch->reCount++ >= (ch->reConfig & 0x03))) {
		ch->osc_params->phase_increment = note_index_2_phase_inc(ch->reConfig >> 2);
		noise_retrigger = true;
		ch->reCount = 0;
	}

	//Apply Glissando
	if (ch->glisConfig && (ch->glisCount++ >= (ch->glisConfig & 0x7F))) {
		const uint8_t amount = (ch->glisConfig & 0x80) ? -1 : 1;
		const uint8_t note = slide_quantity(amount, ch->note, 1, LAST_NOTE, 0);
		ch->note = note;
		ch->osc_params->phase_increment = note_index_2_phase_inc(note);
		ch->glisCount = 0;
	}

	// Apply volume/frequency slides
	slidefx(&ch->vf_slide, ch->osc_params);

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
			ch->osc_params->phase_increment = note_index_2_phase_inc(arpNote + ch->transConfig);
		}
	}

	// Apply Tremolo or Vibrato
	if (ch->treviDepth) {
		int16_t vt = ((ch->treviConfig & 0x40) ? ch->osc_params->phase_increment : ch->osc_params->vol);
		vt = (ch->treviCount & 0x80) ? (vt + ch->treviDepth) : (vt - ch->treviDepth);
		if (vt < 0) {
			vt = 0;
		} else if (ch->treviConfig & 0x40) {
			if (vt > OSC_PHASE_INC_MAX) {
				vt = OSC_PHASE_INC_MAX;
			}
		} else if (!(ch->treviConfig & 0x40)) {
			if (vt > MAX_VOLUME) {
				vt = MAX_VOLUME;
			}
		}
		if (ch->treviConfig & 0x40) {
			ch->osc_params->phase_increment = vt;
		} else {
			ch->osc_params->vol = vt;
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
		const uint8_t cmd = next_pattern_byte(ch);
		process_cmd(ch_index, cmd, score_state, ch);
	}

	if (ch->delay != 0xFFFF) {
		ch->delay--;
	}

	if (!(score_state->channel_active_mute & (1 << ch_index))) {
		const uint8_t flags = (noise_retrigger || ch_index != OSC_CH_THREE) ? 0 : 0x1;
		osc_update_osc(ch_index, ch->osc_params, flags);
	}
}

static void atm_synth_sfx_tick_handler(uint8_t cb_index, void *priv) {
	(void)(cb_index);
	struct mod_sfx_state *sfx_state = (struct mod_sfx_state *)priv;

	const uint8_t sfx_ch_index = sfx_state->ch_index;
	process_channel(sfx_ch_index, &sfx_state->track_info, &sfx_state->channel_state);
	osc_set_tick_rate(1, sfx_state->track_info.tick_rate);
	if (!(sfx_state->track_info.channel_active_mute & 0xF0)) {
		/* sfx done */
		atm_synth_stop_sfx_track(sfx_ch_index);
	}
}

static void atm_synth_score_tick_handler(uint8_t cb_index, void *priv) {
	(void)(cb_index);
	(void)(priv);
	// for every channel start working
	for (uint8_t ch_index = 0; ch_index < ARRAY_SIZE(channels); ch_index++)
	{
		process_channel(ch_index, &atmlib_state, &channels[ch_index]);
		osc_set_tick_rate(0, atmlib_state.tick_rate);
	}

	/* if all channels are inactive, stop playing or check for repeat */
	if (!(atmlib_state.channel_active_mute & 0xF0)) {
		for (uint8_t k = 0; k < ARRAY_SIZE(channels); k++) {
			struct channel_state *const ch = &channels[k];
			/* a quirk in the original implementation does not allow to loop to pattern 0 */
			if (!ch->repeat_point) {
				continue;
			}
			pattern_cmd_ptr(ch) = get_track_start_ptr(&atmlib_state, ch->repeat_point);
			ch->delay = 0;
			atmlib_state.channel_active_mute |= (1<<(k+OSC_CH_COUNT));
		}
		if (!(atmlib_state.channel_active_mute & 0xF0)) {
			atm_synth_stop_score();
		}
	}
}
