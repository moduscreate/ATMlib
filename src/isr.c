
#include <stddef.h>
#include <string.h>
#include <avr/interrupt.h>
#include "isr.h"

static void osc_reset(void);
static void osc_setactive(const uint8_t active_flag);
/*
uint8_t osc_getactive(void);
void osc_toggleactive(void);
*/

/* oscillator structure */
struct osc {
	struct osc_params params;
	uint16_t phase_accumulator;
};

struct callback_info {
	uint8_t callback_prescaler_counter;
	uint8_t callback_prescaler_preset;
	osc_tick_callback cb;
	void *priv;
};

uint8_t int_count __attribute__((used));
struct osc osc[CH_COUNT];
struct callback_info osccb[OSC_TICK_CALLBACK_COUNT];

void osc_setup(void)
{
	osc_reset();
	TCCR4A = 0b01000010;    // Fast-PWM 8-bit
	TCCR4B = 0b00000001;    // currently 31250Hz
	OCR4C  = 0xFF;          // Resolution to 8-bit (TOP=0xFF)
}

static void osc_reset(void)
{
	TIMSK4 = 0b00000000;
	OCR4A  = OSC_DC_OFFSET;
	memset(osc, 0, sizeof(osc));
	memset(osccb, 0, sizeof(osccb));
	for (uint8_t i=0; i<CH_COUNT; i++) {
		/* set modulation to 50% duty cycle */
		osc[i].params.mod = 0x7F;
	}
	for (uint8_t i=0; i<OSC_TICK_CALLBACK_COUNT; i++) {
		osccb[i].callback_prescaler_preset = 255;
		osccb[i].callback_prescaler_counter = 255;
	}
	osc[CH_THREE].params.phase_increment = 0x0001; // Seed LFSR
}

static void osc_setactive(const uint8_t active_flag)
{
	TIMSK4 = active_flag ? 0b00000100 : 0b00000000;
}

#if 0
void osc_toggleactive(void)
{
	TIMSK4 = TIMSK4 ^ 0b00000100; /* toggle disable/enable interrupt */
}
#endif

void osc_set_tick_rate(const uint8_t callback_idx, const uint8_t rate_hz)
{
	const uint8_t div = OSC_SAMPLERATE/ISR_PRESCALER_DIV/rate_hz-1;
	osccb[callback_idx].callback_prescaler_preset = div;
}

void osc_set_tick_callback(const uint8_t callback_idx, const osc_tick_callback cb, const void *priv)
{
	osccb[callback_idx].cb = cb;
	osccb[callback_idx].priv = (void *)priv;
	/* Turn interrupts on/off as needed */
	osc_setactive(osccb[0].cb || osccb[1].cb);
}

void osc_get_tick_callback(const uint8_t callback_idx, osc_tick_callback *cb, void **priv)
{
	if (cb) {
		*cb = osccb[callback_idx].cb;
	}
	if (priv) {
		*priv = osccb[callback_idx].priv;
	}
}

/* if bit 0 in flags is set don't update phase increment */
void osc_update_osc(const uint8_t osc_idx, const struct osc_params *src, const uint8_t flags)
{
	if (flags & 0x1) {
		osc[osc_idx].params.vol = src->vol;
		osc[osc_idx].params.mod = src->mod;
	} else {
		osc[osc_idx].params = *src;
	}
}

void osc_save_osc(const uint8_t osc_idx, struct osc_params *dst)
{
	*dst = osc[osc_idx].params;
}

static __attribute__((used)) void osc_tick_handler(void)
{
	for (uint8_t n = 0; n < OSC_TICK_CALLBACK_COUNT; n++) {
		struct callback_info *cbi = &osccb[n];
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

#define ASM_EOL "\n\t"

ISR(TIMER4_OVF_vect, ISR_NAKED)
{
	asm volatile(
"	push r2                                          " ASM_EOL
"	in   r2,                    __SREG__             " ASM_EOL
"	push r18                                         " ASM_EOL
"	lds  r18,                   int_count            " ASM_EOL
"	dec  r18                                         " ASM_EOL
"	sts  int_count,             r18                  " ASM_EOL
"	;push a new sample at half the interrupt rate    " ASM_EOL
"	andi r18,                   1                    " ASM_EOL
"	breq continue                                    " ASM_EOL
"	pop  r18                                         " ASM_EOL
"	out  __SREG__,              r2                   " ASM_EOL
"	pop  r2                                          " ASM_EOL
"	reti                                             " ASM_EOL
"continue:                                           " ASM_EOL
"	push r27                                         " ASM_EOL
"	push r26                                         " ASM_EOL
"	push r0                                          " ASM_EOL
"	push r1                                          " ASM_EOL
"; OSC 2 advance phase accumulator                   " ASM_EOL
"	lds  r18,                   osc+2*%[osz]+%[phi]  " ASM_EOL
"	lds  r0,                    osc+2*%[osz]+%[pha]  " ASM_EOL
"	add  r0,                    r18                  " ASM_EOL
"	sts  osc+2*%[osz]+%[pha],   r0                   " ASM_EOL
"	lds  r18,                   osc+2*%[osz]+%[phi]+1" ASM_EOL
"	lds  r1,                    osc+2*%[osz]+%[pha]+1" ASM_EOL
"	adc  r1,                    r18                  " ASM_EOL
"	sts  osc+2*%[osz]+%[pha]+1, r1                   " ASM_EOL
"; OSC 2 square waveform                             " ASM_EOL
"	lds  r27,                   osc+2*%[osz]+%[mod]  " ASM_EOL
"	cp   r1,                    r27                  " ASM_EOL
"	lds  r26,                   osc+2*%[osz]+%[vol]  " ASM_EOL
"	brcs 1f                                          " ASM_EOL
"	neg  r26                                         " ASM_EOL
"1:                                                  " ASM_EOL
"; OSC 0 advance phase accumulator                   " ASM_EOL
"	lds  r18,                   osc+0*%[osz]+%[phi]  " ASM_EOL
"	lds  r0,                    osc+0*%[osz]+%[pha]  " ASM_EOL
"	add  r0,                    r18                  " ASM_EOL
"	sts  osc+0*%[osz]+%[pha],   r0                   " ASM_EOL
"	lds  r18,                   osc+0*%[osz]+%[phi]+1" ASM_EOL
"	lds  r1,                    osc+0*%[osz]+%[pha]+1" ASM_EOL
"	adc  r1,                    r18                  " ASM_EOL
"	sts  osc+0*%[osz]+%[pha]+1, r1                   " ASM_EOL
"; OSC 0 square waveform                             " ASM_EOL
"	lds  r27,                   osc+0*%[osz]+%[mod]  " ASM_EOL
"	cp   r1,                    r27                  " ASM_EOL
"	lds  r27,                   osc+0*%[osz]+%[vol]  " ASM_EOL
"	brcs 1f                                          " ASM_EOL
"	neg  r27                                         " ASM_EOL
"1:                                                  " ASM_EOL
"	add  r26,                   r27                  " ASM_EOL
"; OSC 1 advance phase accumulator                   " ASM_EOL
"	lds  r18,                   osc+1*%[osz]+%[phi]  " ASM_EOL
"	lds  r0,                    osc+1*%[osz]+%[pha]  " ASM_EOL
"	add  r0,                    r18                  " ASM_EOL
"	sts  osc+1*%[osz]+%[pha],   r0                   " ASM_EOL
"	lds  r18,                   osc+1*%[osz]+%[phi]+1" ASM_EOL
"	lds  r1,                    osc+1*%[osz]+%[pha]+1" ASM_EOL
"	adc  r1,                    r18                  " ASM_EOL
"	sts  osc+1*%[osz]+%[pha]+1, r1                   " ASM_EOL
"; OSC 1 square waveform                             " ASM_EOL
"	lds  r27,                   osc+1*%[osz]+%[mod]  " ASM_EOL
"	cp   r1,                    r27                  " ASM_EOL
"	lds  r27,                   osc+1*%[osz]+%[vol]  " ASM_EOL
"	brcs 1f                                          " ASM_EOL
"	neg  r27                                         " ASM_EOL
"1:                                                  " ASM_EOL
"	add  r26,                   r27                  " ASM_EOL
"; OSC 3 noise generator                             " ASM_EOL
"	ldi  r27,                   1                    " ASM_EOL
"	lds  r0,                    osc+3*%[osz]+%[phi]  " ASM_EOL
"	lds  r1,                    osc+3*%[osz]+%[phi]+1" ASM_EOL
"	add  r0,                    r0                   " ASM_EOL
"	adc  r1,                    r1                   " ASM_EOL
"	sbrc r1,                    7                    " ASM_EOL
"	eor  r0,                    r27                  " ASM_EOL
"	sbrc r1,                    6                    " ASM_EOL
"	eor  r0,                    r27                  " ASM_EOL
"	sts  osc+3*%[osz]+%[phi],   r0                   " ASM_EOL
"	sts  osc+3*%[osz]+%[phi]+1, r1                   " ASM_EOL
"; OSC 3 continued                                   " ASM_EOL
"	lds  r27,                   osc+3*%[osz]+%[vol]  " ASM_EOL
"	sbrc r1,                    7                    " ASM_EOL
"	neg  r27                                         " ASM_EOL
"	add  r26,                   r27                  " ASM_EOL
"; add DC offset                                     " ASM_EOL
"	ldi  r27,                   %[dc]                " ASM_EOL
"	add  r26,                   r27                  " ASM_EOL
"	sts  %[reg],                r26                  " ASM_EOL
"; tick handler prescaler                            " ASM_EOL
"	lds  r18,                   int_count            " ASM_EOL
"	and  r18,                   r18                  " ASM_EOL
"	;check if a channel tick is due only once every  " ASM_EOL
"	;8 interrupts                                    " ASM_EOL
"	brne isr_done                                    " ASM_EOL
"	ldi  r18,                   %[div]               " ASM_EOL
"	sts  int_count,             r18                  " ASM_EOL
"; check if a tick is due for each OSC               " ASM_EOL
"	clr  r27                                         " ASM_EOL
"	lds  r26,                   osccb+0*%[csz]+%[pre]" ASM_EOL
"	subi r26,                   1                    " ASM_EOL
"	;Use the carry from r26-1 to make r27 undeflow   " ASM_EOL
"	;instead of branching. (saves 9 cycles in total) " ASM_EOL
"	sbci r27,                   0                    " ASM_EOL
"	sts  osccb+0*%[csz]+%[pre], r26                  " ASM_EOL
"	lds  r26,                   osccb+1*%[csz]+%[pre]" ASM_EOL
"	subi r26,                   1                    " ASM_EOL
"	sbci r27,                   0                    " ASM_EOL
"	sts  osccb+1*%[csz]+%[pre], r26                  " ASM_EOL
"	;r27 underflow means at least one channel's      " ASM_EOL
"	;tick is due.                                    " ASM_EOL
"	brne call_playroutine                            " ASM_EOL
"isr_done:                                           " ASM_EOL
"	pop  r1                                          " ASM_EOL
"	pop  r0                                          " ASM_EOL
"	pop  r26                                         " ASM_EOL
"	pop  r27                                         " ASM_EOL
"	pop  r18                                         " ASM_EOL
"	out  __SREG__,              r2                   " ASM_EOL
"	pop  r2                                          " ASM_EOL
"	reti                                             " ASM_EOL
"call_playroutine:                                   " ASM_EOL

"	sei                                              " ASM_EOL
"	push r19                                         " ASM_EOL
"	push r20                                         " ASM_EOL
"	push r21                                         " ASM_EOL
"	push r22                                         " ASM_EOL
"	push r23                                         " ASM_EOL
"	push r24                                         " ASM_EOL
"	push r25                                         " ASM_EOL
"	push r30                                         " ASM_EOL
"	push r31                                         " ASM_EOL

"	clr  r1                                          " ASM_EOL
"	call osc_tick_handler                            " ASM_EOL

"	pop  r31                                         " ASM_EOL
"	pop  r30                                         " ASM_EOL
"	pop  r25                                         " ASM_EOL
"	pop  r24                                         " ASM_EOL
"	pop  r23                                         " ASM_EOL
"	pop  r22                                         " ASM_EOL
"	pop  r21                                         " ASM_EOL
"	pop  r20                                         " ASM_EOL
"	pop  r19                                         " ASM_EOL

"	pop  r1                                          " ASM_EOL
"	pop  r0                                          " ASM_EOL
"	pop  r26                                         " ASM_EOL
"	pop  r27                                         " ASM_EOL
"	pop  r18                                         " ASM_EOL
"	out  __SREG__,              r2                   " ASM_EOL
"	pop  r2                                          " ASM_EOL
"	reti                                             " ASM_EOL
	::
	[reg] "M" _SFR_MEM_ADDR(OCR4A),
	[div] "M" (ISR_PRESCALER_DIV*2),
	[dc]  "M" OSC_DC_OFFSET,
	[osz] "M" (sizeof(struct osc)),
	[csz] "M" (sizeof(struct callback_info)),
	[pre] "M" (offsetof(struct callback_info, callback_prescaler_counter)),
	[pha] "M" (offsetof(struct osc, phase_accumulator)),
	[phi] "M" (offsetof(struct osc_params, phase_increment)),
	[mod] "M" (offsetof(struct osc_params, mod)),
	[vol] "M" (offsetof(struct osc_params, vol))
	);
}
