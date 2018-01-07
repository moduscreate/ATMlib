
#include <stddef.h>
#include <string.h>
#include <avr/interrupt.h>
#include "osc.h"

/*
  Mixing 4 8bit channels requires 10 bits but by setting ENHC4 in TCCR4E
  bit 0 of OCR4A selects the timer clock edge so while the
  resolution is 10 bits the number of timer bits used to count
  (set in OCR4C) is 9.

  See section "15.6.2 Enhanced Compare/PWM mode" in the ATmega16U4/ATmega32U4
  datasheet. This means the PWM frequency can be double of what it would be
  if we used 10 timer bits for counting.

  Also, 8 8bit channels can be mixed (11bits resolution, 10bits
  timer counter) if the PWM rate is halved (or fast PWM mode is used
  instead of dual slope PWM).
*/

#define OSC_COMPARE_RESOLUTION_BITS (10)
#define OSC_DC_OFFSET (1<<(OSC_COMPARE_RESOLUTION_BITS-1))

#define OSC_TIMER_BITS (9)
#define OSC_PWM_TOP ((1<<OSC_TIMER_BITS)-1)
#define OSC_HI(v) ((v)>>8)
#define OSC_LO(v) ((v)&0xFF)

static void osc_reset(void);
static void osc_setactive(const uint8_t active_flag);

struct callback_info {
	uint8_t callback_prescaler_counter;
	uint8_t callback_prescaler_preset;
	osc_tick_callback cb;
	void *priv;
};

uint8_t __attribute__((used)) osc_isr_reenter = 0;
uint8_t osc_int_count __attribute__((used));
struct osc_params osc_params_array[OSC_CH_COUNT] __attribute__((used));
uint16_t osc_pha_acc_array[OSC_CH_COUNT] __attribute__((used));
struct callback_info osc_cb[OSC_TICK_CALLBACK_COUNT];

void osc_setup(void)
{
	osc_reset();
	/* PWM setup using timer 4 */
	PLLFRQ = 0b01011010;    /* PINMUX:16MHz XTAL, PLLUSB:48MHz, PLLTM:1, PDIV:96MHz */
	PLLCSR = 0b00010010;
	/* Wait for PLL lock */
	while (!(PLLCSR & 0x01)) {}
	TCCR4A = 0b01000010;    /* PWM mode */
	/* TCCR4B will be se to 0b00000001 for clock source/1, 96MHz/(OCR4C+1)/2 ~ 95703Hz */
	TCCR4D = 0b00000001;    /* Dual Slope PWM (the /2 in the eqn. above is because of dual slope PWM) */
	TCCR4E = 0b01000000;    /* Enhanced mode (bit 0 in OCR4C selects clock edge) */
	TC4H   = OSC_HI(OSC_PWM_TOP);
	OCR4C  = OSC_LO(OSC_PWM_TOP); /* Use 9-bits for counting (TOP=0x1FF) */

	TCCR3A = 0b00000000;
	TCCR3B = 0b00001001;    /* Mode CTC, clock source 16MHz */
	OCR3A  = (16E6/OSC_SAMPLERATE)-1; /* 16MHz/1k = 16kHz */
}

static void osc_reset(void)
{
	osc_setactive(0);
	memset(osc_params_array, 0, sizeof(osc_params_array));
	memset(osc_cb, 0, sizeof(osc_cb));
	for (uint8_t i=0; i<OSC_CH_COUNT; i++) {
		/* set modulation to 50% duty cycle */
		osc_params_array[i].mod = 0x7F;
	}
	osc_params_array[OSC_CH_THREE].phase_increment = 0x0001; // Seed LFSR
}

static void osc_setactive(const uint8_t active_flag)
{
	if (active_flag) {
		TC4H   = OSC_HI(OSC_DC_OFFSET);
		OCR4A  = OSC_LO(OSC_DC_OFFSET);
		TCCR4B = 0b00000001;    /* clock source/1, 96MHz/(OCR4C+1)/2 ~ 95703Hz */
		TIMSK3 = 0b00000010;    /* interrupts on */
	} else {
		TIMSK3 = 0b00000000;    /* interrupts off */
		TCCR4B = 0b00000000;    /* PWM = off */
	}
}

void osc_set_tick_rate(const uint8_t callback_idx, const uint16_t rate_hz)
{
	const uint8_t div = OSC_SAMPLERATE/OSC_ISR_PRESCALER_DIV/rate_hz-1;
	osc_cb[callback_idx].callback_prescaler_preset = div;
}

void osc_set_tick_callback(const uint8_t callback_idx, const osc_tick_callback cb, const void *priv)
{
	osc_cb[callback_idx].cb = cb;
	osc_cb[callback_idx].priv = (void *)priv;
	if (cb) {
		/* trigger callback ASAP */
		osc_cb[callback_idx].callback_prescaler_counter = 0;
	}
	/* Turn interrupts on/off as needed */
	osc_setactive(osc_cb[0].cb || osc_cb[1].cb);
}

void osc_get_tick_callback(const uint8_t callback_idx, osc_tick_callback *cb, void **priv)
{
	if (cb) {
		*cb = osc_cb[callback_idx].cb;
	}
	if (priv) {
		*priv = osc_cb[callback_idx].priv;
	}
}

static __attribute__((used)) void osc_tick_handler(void)
{
	for (uint8_t n = 0; n < OSC_TICK_CALLBACK_COUNT; n++) {
		struct callback_info *cbi = &osc_cb[n];
		/* channel tick is due when callback_prescaler_counter underflows */
		if (cbi->callback_prescaler_counter != 255) {
			if (!cbi->cb) {
				/*
				Reset the counter to the highest value when the
				callback is disabled, this way when there is only
				one callback registered (99% of the time) the disabled
				callback is never going to trigger a tick handler.
				*/
				cbi->callback_prescaler_counter = 255;
			}
			continue;
		}
		if (cbi->cb) {
			cbi->cb(n, cbi->priv);
		}
		cbi->callback_prescaler_counter = cbi->callback_prescaler_preset;
	}
}

ISR(TIMER3_COMPA_vect)
{
	uint16_t pcm = OSC_DC_OFFSET;
	struct osc_params *p = osc_params_array;
	for (uint8_t i=0;i<3;i++,p++) {
		const uint8_t vol = p->vol;
		if (!vol) {
			/* skip if volume is zero and save some cycles */
			continue;
		}
		const uint16_t pha = osc_pha_acc_array[i] + p->phase_increment;
		osc_pha_acc_array[i] = pha;
		if (OSC_HI(pha) > p->mod) {
			pcm += vol;
		} else {
			pcm -= vol;
		}
	}

	/* p == &osc_params_array[3] when leaving the loop above */
	{
		const uint8_t vol = p->vol;
		/* skip if volume is zero and save some cycles */
		if (vol) {
			uint16_t shift_reg = p->phase_increment;
			const uint8_t msb = OSC_HI(shift_reg) & 0x80;
			shift_reg += shift_reg;
			if (msb) {
				pcm += vol;
				shift_reg ^= 0x002D;
			} else {
				pcm -= vol;
			}
			p->phase_increment = shift_reg;
		}
	}

	TC4H = OSC_HI(pcm);
	OCR4A = OSC_LO(pcm);

	if (!(--osc_int_count)) {
		osc_int_count = OSC_ISR_PRESCALER_DIV;
		const uint8_t tick0_due = --osc_cb[0].callback_prescaler_counter == 255;
		const uint8_t tick1_due = --osc_cb[1].callback_prescaler_counter == 255;
		if (tick0_due || tick1_due) {
			if (!osc_isr_reenter) {
				osc_isr_reenter = 1;
				sei();
				osc_tick_handler();
				osc_isr_reenter = 0;
			}
		}
	}
}
