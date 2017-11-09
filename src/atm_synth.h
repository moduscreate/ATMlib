
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

extern struct atmlib_state atmlib_state;
extern struct osc_tick_callback_info osc_tick_dispatch_table[CH_COUNT];

uint16_t read_vle(const uint8_t **pp);

void osc_setup(void);
uint8_t osc_compute_callback_prescaler(uint8_t rate_hz);
