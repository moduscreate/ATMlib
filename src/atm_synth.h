
#include <stddef.h>
#include <Arduino.h>

#define CH_ZERO             0
#define CH_ONE              1
#define CH_TWO              2
#define CH_THREE            3

extern byte trackCount;
extern const word *trackList;
extern const byte *trackBase;
extern uint8_t pcm;
extern uint16_t cia_count;

extern bool half;

// oscillator structure
struct osc {
  uint8_t  vol;
  uint16_t phase_increment;
  uint16_t phase_accumulator;
};

extern struct osc osc[4];

extern void ATM_playroutine() asm("ATM_playroutine");

uint16_t read_vle(const byte **pp);
