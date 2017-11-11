
#include <stdbool.h>
#include <stdint.h>

#define OSC_SAMPLERATE (31250/2)
#define OSC_DC_OFFSET (128)
#define OSC_MOD_MIN (0)
#define OSC_MOD_MAX (255)
#define ISR_PRESCALER_DIV (4)
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


typedef void (*osc_tick_callback)(uint8_t cb_index, void *priv);

void osc_setup(void);
void osc_reset(void);
void osc_setactive(uint8_t active_flag);
uint8_t osc_getactive(void);
void osc_toggleactive(void);

void osc_update_osc(uint8_t osc_idx, struct osc_params *src, uint8_t flags);
void osc_set_tick_rate(uint8_t callback_idx, uint8_t rate_hz);
void osc_set_tick_callback(uint8_t callback_idx, osc_tick_callback cb, void *priv);
void osc_get_tick_callback(uint8_t callback_idx, osc_tick_callback *cb, void **priv);
