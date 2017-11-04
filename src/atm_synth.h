
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define ATM_SYNTH_DC_OFFSET (128)

enum channels_e {
/* Not prefixing these enum constants to avoid breaking existing code */
	CH_ZERO = 0,
	CH_ONE,
	CH_TWO,
	CH_THREE,
	CH_COUNT,
};

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

extern struct atmlib_state atmlib_state;

extern uint16_t cia;
extern uint16_t cia_count;

extern bool half;

// oscillator structure
struct osc {
	uint8_t  vol;
	uint16_t phase_increment;
	uint16_t phase_accumulator;
};

extern struct osc osc[CH_COUNT];

extern void ATM_playroutine() asm("ATM_playroutine");

uint16_t read_vle(const uint8_t **pp);
