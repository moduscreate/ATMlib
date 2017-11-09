
#include <stdbool.h>
#include <stdint.h>

#define OSC_DC_OFFSET (128)

enum channels_e {
/* Not prefixing these enum constants to avoid breaking existing code */
	CH_ZERO = 0,
	CH_ONE,
	CH_TWO,
	CH_THREE,
	CH_COUNT,
};

/* oscillator structure */
struct osc {
	uint8_t  vol;
	uint16_t phase_increment;
	uint16_t phase_accumulator;
};

extern uint16_t __attribute__((used)) cia;
extern uint16_t __attribute__((used)) cia_count;
extern struct osc osc[CH_COUNT];

extern void osc_tick_handler() asm("osc_tick_handler");
