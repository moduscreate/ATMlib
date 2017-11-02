
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum channels_e {
	CH_ZERO = 0,
	CH_ONE,
	CH_TWO,
	CH_THREE,
	CH_COUNT,
};

extern uint8_t trackCount;
extern const uint16_t *trackList;
extern const uint8_t *trackBase;
extern uint8_t pcm;
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
