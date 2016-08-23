/*
  Arduboy Tracker Music Library for Arduino Leonardo "Arduboy"

  2015
  TEAM a.r.g.
  Davey Taylor
*/

#ifndef _ATMLIB_H_
#define _ATMLIB_H_
#include <stddef.h>
#include <inttypes.h>
#include "Arduino.h"

#define Melody const uint8_t PROGMEM


class SquawkStream {
  public:
    virtual ~SquawkStream() = 0;
    virtual uint8_t read() = 0;
    virtual void seek(size_t offset) = 0;
};

inline SquawkStream::~SquawkStream() { }

class SquawkSynth {

  protected:
    // Load and play specified melody
    void play(SquawkStream *melody);

  public:
    SquawkSynth() {};

    // Initialize Squawk to generate samples at sample_rate Hz
    void begin(uint16_t sample_rate);

    // Load and play specified melody
    // melody needs to point to PROGMEM data
    void play(const uint8_t *melody);

    // Tune Squawk to a different frequency - default is 1.0
    void tune(float tuning);

    // Change the tempo - default is 50
    void tempo(uint16_t tempo);
};

extern SquawkSynth Squawk;

// oscillator structure
typedef struct {
  uint8_t  vol;
  uint16_t freq;
  uint16_t phase;
} osc_t;

typedef osc_t Oscillator;

// oscillator memory
extern osc_t osc[4];
extern uint8_t pcm;
// channel 0 is pulse wave @ 25% duty
// channel 1 is square wave
// channel 2 is triangle wave
// channel 3 is noise

extern void tick() asm("squawk_playroutine");


extern uint16_t cia, cia_count;
extern bool half;
ISR(TIMER4_OVF_vect, ISR_NAKED) {
  asm volatile(
    "push r2                                          " "\n\t"
    "in   r2,                    __SREG__             " "\n\t"
    "push r18                                         " "\n\t"
    "lds  r18, half \n\t"
    "com  r18 \n\t"
    "sts  half, r18 \n\t"
    "breq continue \n\t"
    "pop  r18                                         " "\n\t"
    "out  __SREG__,              r2                   " "\n\t"
    "pop  r2                                          " "\n\t"
    "reti                                             " "\n\t"
    "continue: \n\t"
    "push r27                                         " "\n\t"
    "push r26                                         " "\n\t"
    "push r0                                          " "\n\t"
    "push r1                                          " "\n\t"

    "lds  r18,                   osc+2*%[mul]+%[fre]  " "\n\t"
    "lds  r0,                    osc+2*%[mul]+%[pha]  " "\n\t"
    "add  r0,                    r18                  " "\n\t"
    "sts  osc+2*%[mul]+%[pha],   r0                   " "\n\t"
    "lds  r18,                   osc+2*%[mul]+%[fre]+1" "\n\t"
    "lds  r1,                    osc+2*%[mul]+%[pha]+1" "\n\t"
    "adc  r1,                    r18                  " "\n\t"
    "sts  osc+2*%[mul]+%[pha]+1, r1                   " "\n\t"

    "mov  r27,                   r1                   " "\n\t"
    "sbrc r27,                   7                    " "\n\t"
    "com  r27                                         " "\n\t"
    "lsl  r27                                         " "\n\t"
    "lds  r26,                   osc+2*%[mul]+%[vol]  " "\n\t"
    "subi r27,                   128                  " "\n\t"
    "muls r27,                   r26                  " "\n\t"
    "lsl  r1                                          " "\n\t"
    "mov  r26,                   r1                   " "\n\t"

    "lds  r18,                   osc+0*%[mul]+%[fre]  " "\n\t"
    "lds  r0,                    osc+0*%[mul]+%[pha]  " "\n\t"
    "add  r0,                    r18                  " "\n\t"
    "sts  osc+0*%[mul]+%[pha],   r0                   " "\n\t"
    "lds  r18,                   osc+0*%[mul]+%[fre]+1" "\n\t"
    "lds  r1,                    osc+0*%[mul]+%[pha]+1" "\n\t"
    "adc  r1,                    r18                  " "\n\t"
    "sts  osc+0*%[mul]+%[pha]+1, r1                   " "\n\t"

    "mov  r18,                   r1                   " "\n\t"
    "lsl  r18                                         " "\n\t"
    "and  r18,                   r1                   " "\n\t"
    "lds  r27,                   osc+0*%[mul]+%[vol]  " "\n\t"
    "sbrc r18,                   7                    " "\n\t"
    "neg  r27                                         " "\n\t"
    "add  r26,                   r27                  " "\n\t"

    "lds  r18,                   osc+1*%[mul]+%[fre]  " "\n\t"
    "lds  r0,                    osc+1*%[mul]+%[pha]  " "\n\t"
    "add  r0,                    r18                  " "\n\t"
    "sts  osc+1*%[mul]+%[pha],   r0                   " "\n\t"
    "lds  r18,                   osc+1*%[mul]+%[fre]+1" "\n\t"
    "lds  r1,                    osc+1*%[mul]+%[pha]+1" "\n\t"
    "adc  r1,                    r18                  " "\n\t"
    "sts  osc+1*%[mul]+%[pha]+1, r1                   " "\n\t"

    "lds  r27,                   osc+1*%[mul]+%[vol]  " "\n\t"
    "sbrc r1,                    7                    " "\n\t"
    "neg  r27                                         " "\n\t"
    "add  r26,                   r27                  " "\n\t"

    "ldi  r27,                   1                    " "\n\t"
    "lds  r0,                    osc+3*%[mul]+%[fre]  " "\n\t"
    "lds  r1,                    osc+3*%[mul]+%[fre]+1" "\n\t"
    "add  r0,                    r0                   " "\n\t"
    "adc  r1,                    r1                   " "\n\t"
    "sbrc r1,                    7                    " "\n\t"
    "eor  r0,                    r27                  " "\n\t"
    "sbrc r1,                    6                    " "\n\t"
    "eor  r0,                    r27                  " "\n\t"
    "sts  osc+3*%[mul]+%[fre],   r0                   " "\n\t"
    "sts  osc+3*%[mul]+%[fre]+1, r1                   " "\n\t"

    "lds  r27,                   osc+3*%[mul]+%[vol]  " "\n\t"
    "sbrc r1,                    7                    " "\n\t"
    "neg  r27                                         " "\n\t"
    "add  r26,                   r27                  " "\n\t"

    "lds  r27,                   pcm                  " "\n\t"
    "add  r26,                   r27                  " "\n\t"
    "sts  %[reg],                 r26                  " "\n\t"

    "lds  r27,                   cia_count+1          " "\n\t"
    "lds  r26,                   cia_count            " "\n\t"
    "sbiw r26,                   1                    " "\n\t"
    "breq call_playroutine                            " "\n\t"
    "sts  cia_count+1,           r27                  " "\n\t"
    "sts  cia_count,             r26                  " "\n\t"
    "pop  r1                                          " "\n\t"
    "pop  r0                                          " "\n\t"
    "pop  r26                                         " "\n\t"
    "pop  r27                                         " "\n\t"
    "pop  r18                                         " "\n\t"
    "out  __SREG__,              r2                   " "\n\t"
    "pop  r2                                          " "\n\t"
    "reti                                             " "\n\t"
    "call_playroutine:                                " "\n\t"

    "lds  r27, cia+1                                  " "\n\t"
    "lds  r26, cia                                    " "\n\t"
    "sts  cia_count+1,           r27                  " "\n\t"
    "sts  cia_count,             r26                  " "\n\t"

    "sei                                              " "\n\t"
    "push r19                                         " "\n\t"
    "push r20                                         " "\n\t"
    "push r21                                         " "\n\t"
    "push r22                                         " "\n\t"
    "push r23                                         " "\n\t"
    "push r24                                         " "\n\t"
    "push r25                                         " "\n\t"
    "push r30                                         " "\n\t"
    "push r31                                         " "\n\t"

    "clr  r1                                          " "\n\t"
    "call squawk_playroutine                          " "\n\t"

    "pop  r31                                         " "\n\t"
    "pop  r30                                         " "\n\t"
    "pop  r25                                         " "\n\t"
    "pop  r24                                         " "\n\t"
    "pop  r23                                         " "\n\t"
    "pop  r22                                         " "\n\t"
    "pop  r21                                         " "\n\t"
    "pop  r20                                         " "\n\t"
    "pop  r19                                         " "\n\t"

    "pop  r1                                          " "\n\t"
    "pop  r0                                          " "\n\t"
    "pop  r26                                         " "\n\t"
    "pop  r27                                         " "\n\t"
    "pop  r18                                         " "\n\t"
    "out  __SREG__,              r2                   " "\n\t"
    "pop  r2                                          " "\n\t"
    "reti                                             " "\n\t"
    :
    : [reg] "M" _SFR_MEM_ADDR(OCR4A),
    [mul] "M" (sizeof(Oscillator)),
    [pha] "M" (offsetof(Oscillator, phase)),
    [fre] "M" (offsetof(Oscillator, freq)),
    [vol] "M" (offsetof(Oscillator, vol))
  );
}


#endif