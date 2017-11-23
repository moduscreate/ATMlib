# ATMlib2

ATMlib stands for **Arduboy Tracker Music** and is based on [_**Squawk**_](https://github.com/stg/Squawk "Squawk Github Page") a minimalistic 8-bit software synthesizer & playroutine library for Arduino, created by Davey Taylor aka STG.

While _Squawk_ provides a very nice synth, it wasn't optimized for a small footprint. Songs are not very efficient in size, so Joeri Gantois aka JO3RI asked Davey to help him work on a new score format and so ATMlib was born.

ATMlib2 is built on the work of JO3RI and Davey and adds a lot of new exciting features! ATMlib2 is not compatible with ATMlib music scores.

Contributors (Alphabetical order):

* Davey Taylor - ATMsynth - Effects
* Delio Brignoli
* Jay Garcia (Modus Create)
* Joeri Gantois - Effects

Thanks to [Modus Create](https://moduscreate.com) for sponsoring and participating in the development.

### Features

* Score format supports minimal scores (1 byte overhead)
* Extensible command encoding designed for simpler and smaller parser
* 4 channels: 3 square wave with independent programmable duty cycle + 1 noise
* LFO and slide effects can be applied to square wave duty cycle
* Asynchronous playback of 1 sound effect as a mono music score on an arbitrary channel with independent tempo. Music is muted on the channel used by sound effects and resumed when the sound effect is stopped or finishes playback.


### Music playback example


##### C
``` C
...
#include "atm_synth.h"

void setup() {
    arduboy.begin();
    arduboy.setFrameRate(15);
    arduboy.audio.on();
    
    /* Begin playback of song */
    atm_synth_setup();
    atm_synth_play_score(score);
}
```

##### C++
``` C++
...
#include "ATMlib.h"

ATMsynth ATM;

void setup() {
    arduboy.begin();
    arduboy.setFrameRate(15);
    arduboy.audio.on();

    /* Begin playback of song */
    ATM.play(score);
}
```


### Sound effect playback example

``` C
...
#include "atm_synth.h"

/* Temporary storage used by sound effect while playing back */
struct atm_sfx_state sfx_state;

void setup() {
    arduboy.begin();
    arduboy.setFrameRate(15);
    arduboy.audio.on();

    // Begin playback of song.
    atm_synth_setup();
    atm_synth_play_score(score);
}

void loop() {
    if (!(arduboy.nextFrame()))
        return;

    arduboy.pollButtons();

    if (arduboy.justPressed(B_BUTTON)) {
        /* Start playback of sfx1 on channel 1 */
        atm_synth_play_sfx_track(OSC_CH_ONE, &sfx1, &sfx_state);
    }

    if (arduboy.justPressed(A_BUTTON)) {
        atm_synth_stop_sfx_track(&sfx_state);
    }

    arduboy.display();
}
```

### Music score memory layout

Music scores have a single byte header describing the format of the following bytes. The smallest possible score layout is _single pattern, mono_ and has 1 byte header plus N bytes for the pattern. The shortest possible score that still produces audio looks like this (5 bytes total):

``` C
const PROGMEM struct sfx_data {
    uint8_t fmt;
    uint8_t pattern0[4];
} sfx1 = {
    .fmt = ATM_SCORE_FMT_MINIMAL_MONO,
    .pattern0 = {
        /* Use default tempo */
        /* Volume must be set because it defaults to 0 */
        ATM_CMD_M_SET_VOLUME(31),
        ATM_CMD_I_NOTE_C4,
        ATM_CMD_M_DELAY_TICKS(25),
        ATM_CMD_I_STOP,
    },
};
```

#### Overview

A score is made up of a common header followed by _chunks_ concatenated one after the other with no padding. The common header has a fixed size of one byte and is always present. Chunks are ordered as follows:

| Order  | Name            | Optional
|--------|-----------------|--------------
| 1      | Common header   | N
| 2      | Pattern info    | Y
| 3      | Channel info    | Y
| 4      | Extensions      | Y
| 5      | Patterns data   | N


#### Common header

```-``` means reserved bits. They should be ignored and written as zero.

| Offset | Size         | Name
|--------|--------------|--------------
| 0      | 1            | Format ID/Version

```
b------oc    :   Format ID/Version
 ||||||||
 |||||||└->  0   c: channel info chunk present flag
 ||||||└-->  1   o: pattern info chunk present flag
 └└└└└└--->  7:2 [reserved]

```

#### Pattern offsets information

This chunk is present if flag ```o``` is set in the common header. 

| Offset | Size              | Name
|--------|-------------------|--------------
| 0      | 1                 | Pattern count
| 1      | 2*[pattern count] | Pattern offsets


```
b--pppppp   : pattern count, p: number of patterns
-------------------------------------------------------------------
uint16_t[p] : pattern offsets, array of p elements, each element P
              is the offset in bytes of the start of pattern P
              from the beginning of the score data
              (including the common header).
```

#### Channel information

This chunk is present if flag ```c``` is set in the common header. 

| Offset | Size              | Name
|--------|-------------------|--------------
| 0      | 1                 | Channel count
| 1      | 2*[pattern count] | Entry patterns

```
0b------cc   : channel count, c: number of channels
uint8_t[c]   : entry patterns | array of c elements, each element
               uint8_t[c_i] is the index of the first pattern played by
               channel c_i.
```

#### Reserved for extensions

If the ```c``` flag is set in the header, extensions can be located immediately after pattern info (or immediately after channel info in case it is present) and occupy space up to the beginning of pattern data. When there are no extensions (none is defined for now) pattern data can immediately follow the previous block.

#### Pattern data

When the ```o``` flag is set in the header each pattern's start offset is specified in the pattern information chunk. When the ```o``` flag is not set pattern data follows immediately the end of the channel information chunk, if present, otherwise it follows the common header.


| ```o``` and ```c``` flags | Pattern data location
|---------------------------|-------------------
| o:0, c:0                  | Immediately after the header. Offset: 1 byte
| o:0, c:1                  | Immediately after channel information block: 2+[number of channels] bytes
| o:1, c:0                  | Specified by offset array
| o:1, c:1                  | Specified by offset array

### Commands encoding

Commands are of two types: immediate when they have no extra parameters and parametrised when they can be followed by parameter bytes. Command bit-space is partitioned as follows:

```
00nnnnnn : 1-63 note ON number, 0 note OFF (Immediate)
010ddddd : Delay d+1 ticks (max 32 ticks) (Immediate)
0110iiii : i = command ID (16 commands) (Immediate)
0111---- : [reserved]
1ssscccc : s+1 = parameters byte count (s=7 reserved), c = command ID (16 commands)
1111---- : [reserved]
```

#### Immediate command IDs

```
00 - Glissando/portamento OFF
01 - Arpeggio OFF
02 - Note Cut OFF
03 - Noise re-trigger OFF
04 - [reserved]
05 - [reserved]
06 - Return
07 - Stop (end pattern marker, stop playback on this channel)
08 - Transpose OFF
[09, 15] - [reserved]
```

Immediate command 1-6 have the same least significant bits as the corresponding parametrised commands below to make the implementation more compact.

#### Parametrised command IDs

Parametrised commands use the lower nibble to encode 16 command IDs and bits 6:4 to encode the number of parameter bytes which follow the command starting at 0 i.e. a value of ```b10000000``` means parametrised command ID 0 followed by 1 parameter byte.

```
00 - Glissando/portamento
01 - Arpeggio
02 - Note Cut
03 - Noise re-trigger
04 - Slide FX
05 - LFO FX
06 - Call
07 - Long delay
08 - Set transposition
09 - Add transposition
10 - Set tempo
11 - Add tempo
12 - Set Volume
13 - Set square wave duty cycle
14 - Setup pattern loop
15 - [reserved]
```

##### Glissando

```
Glissando - Raise or lower the current note by one semitone every N ticks

Parameter count: 1

P1
    Size   : 1 byte
    Name   : Effect configuration
    Format : bdnnnnnnn
              |└└└└└└└-> ticks between each note change minus one
              └--------> 1: shift pitch down, 0: shift pitch up
```

##### Arpeggio

```
Arpeggio - Play a second and optionally third note after each played note

Parameter count: 2

P1
    Size   : 1 byte
    Name   : chord
    Format : bkkkknnnn
              ||||└└└└-> 3nd note shift: semitones to add to the 2nd [0,14]
              └└└└-----> 2nd note shift: semitones to add to the 1st [0,14]

P2
    Size   : 1 byte
    Name   : Effect configuration
    Format : b-edttttt
              |||└└└└└-> ticks between note change minus one [0, 31]
              ||└------> 1: retrigger, 0: no retrigger
              |└-------> 1: skip 3rd note, 0: play 3rd note
              └--------> [reserved]
```

##### Note Cut

```
Note cut - Stop note automatically after N ticks

Parameter count: 1

P1
    Size  : 1 byte
    Name  : Note duration
    Range : [0,255] (u8)
```

##### Noise re-trigger

```
Noise re-trigger - Continuously reseed the noise PRNG (pseudo random number generator)

Parameter count: 1

P1
    Size   : 1 byte
    Name   : Effect configuration
    Format : baaaaaass
              ||||||└└-> ticks between reseeding minus one
              └└└└└└---> Seed value
```

##### Slide FX

```
Slide/Slide advanced - Ramp oscillator parameter up or down

Parameter count: 1/2/3

P1
    Size   : 1 byte
    Name   : Oscillator parameter to slide
    Note   : When only this parameter is present the effect is turned off
    Format : b------pp
              ||||||└└-> Oscillator parameter 0:vol, 1:freq, 3:mod
              └└└└└└---> [reserved]

P2
    Size   : 1 byte
    Name   : Amount to slide up or down per tick (or each N ticks)
    Range  : [-128:127] (i8)

P3
    Size   : 1 byte
    Name   : Configuration
    Note   : Defaults to 0 when not present i.e. update every tick, clamp and keep fading
    Format : bornnnnnn
              ||└└└└└└-> Ticks between update minus one
              |└-------> Retrigger flag 1: restart effect on note-on 0: keep sliding on note-on
              └--------> Overflow flag 1: let overflow, 0: clamp
```

##### LFO FX

```
LFO - Low frequency oscillator applied to one of the synth parameters

Parameter count: 1/2/3

P1
    Size   : 1 byte
    Name   : Oscillator parameter to modulate
    Note   : When only this parameter is present the effect is turned off
    Format : b------pp
              ||||||└└-> Oscillator parameter 0:vol, 1:freq, 3:mod
              └└└└└└---> [reserved]

P2
    Size   : 1 byte
    Name   : LFO depth
    Format : b-ddddddd
              |└└└└└└└-> Oscillator parameter delta per tick
              └--------> [reserved]

P3
    Size   : 1 byte
    Name   : Configuration
    Note   : Defaults to 0 when not present (update every tick)
    Format : b---nnnnn
              |||└└└└└-> Ticks between update minus one
              └└└------> [reserved]
```

##### Call

```
Call/Call Repeat - jump to a pattern index and optionally repeat it N times

Parameter count: 1/2

P1
    Size  : 1 byte
    Name  : Pattern index to jump to
    Range : [0:255] (u8)

P2
    Size  : 1 byte
    Name  : Repeat times - 1
    Range : [0:255] (u8)
    Note  : Default to 0 when not present (play once)
```


##### Long delay

```
Long Delay - delay any number of ticks between 1 and 65534

Parameter count: 1

P1
    Size  : 1/2 bytes
    Name  : Delay value
    Range : [0:255] (u8) or [0:65533] (u16)
    Note  : 1 <= delay <= 256 is enconded with one byte as (delay-1),
            delay > 256 is encoded as an uint16_t as (delay-1) using
            native CPU endiannes (i.e. high byte first)
```

##### Set transposition

```
Set transposition - set transposition in semitones

Parameter count: 1

P1
    Size  : 1 byte
    Name  : Semitones
    Range : [-128:127] (i8)
```

##### Add transposition

```
Add transposition - add to current transposition in semitones

Parameter count: 1

P1
    Size  : 1 byte
    Name  : Semitones
    Range : [-128:127] (i8)
```


##### Set tempo

```
Set tempo - set tick rate in Hz

Parameter count: 1

P1
    Size  : 1 byte
    Name  : Tick rate in Hz 
    Range : [8:255] (u8)
```

##### Add tempo

```
Add tempo - add to current tick rate in Hz

Parameter count: 1

P1
    Size  : 1 byte
    Name  : Tick rate in Hz to add or subtract
    Range : [-128:127] (i8)
```

##### Set Volume

```
Set volume - set volume

Parameter count: 1

P1
    Size  : 1 byte
    Name  : Volume
    Format : b-ddddddd
              |└└└└└└└-> Volume [0:127]
              └--------> [reserved]
```

##### Set square wave duty cycle

```
Set square wave duty cycle - set square wave duty cycle

Parameter count: 1

P1
    Size  : 1 byte
    Name  : Duty cycle
    Range : [0:255] (u8)
    Note  : Values from 0 to 255 map to 0%/100% duty cycle
```

##### Setup pattern loop

```
Setup pattern loop - set the pattern index to loop to

Parameter count: 1

P1
    Size  : 1 byte
    Name  : Pattern index to loop to when the score finishes
    Range : [0:255] (u8)
    Note  : The loop index takes effect when all channels have stopped
```
