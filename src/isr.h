
#include <stdbool.h>
#include <stdint.h>

#define OSC_SAMPLERATE (31250/2)
#define OSC_DC_OFFSET (128)
#define OSC_MOD_MIN (0)
#define OSC_MOD_MAX (255)
#define ISR_PRESCALER_DIV (4)
#define OSC_TICKRATE_MAX (OSC_SAMPLERATE/ISR_PRESCALER_DIV)
/*
OSC period is 2**16 so Nyquist corresponds to 2**16/2. Note that's way too high
for square waves to sound OK.
*/
#define MAX_OSC_PHASE_INC (2^16/2)
#define OSC_TICK_CALLBACK_COUNT (2)

enum osc_channels_e {
/* Not prefixing these enum constants to avoid breaking existing code */
	CH_ZERO = 0,
	CH_ONE,
	CH_TWO,
	CH_THREE,
	CH_COUNT,
};

struct osc_params {
	uint8_t  mod;
	uint8_t  vol;
	uint16_t phase_increment;
};

extern struct osc_params osc_params_array[CH_COUNT];

typedef void (*osc_tick_callback)(const uint8_t cb_index, void *priv);

void osc_setup(void);

void osc_set_tick_rate(const uint8_t callback_idx, const uint16_t rate_hz);
void osc_set_tick_callback(const uint8_t callback_idx, const osc_tick_callback cb, const void *priv);
void osc_get_tick_callback(const uint8_t callback_idx, osc_tick_callback *cb, void **priv);
