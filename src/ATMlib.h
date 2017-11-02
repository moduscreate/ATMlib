#ifndef _ATMLIB_H_
#define _ATMLIB_H_

#include "atm_synth.h"

class ATMsynth {

public:
	ATMsynth() {};

	// Load and play specified song
	void play(const uint8_t *song);

	// Play or Pause playback
	void playPause();

	// Stop playback (unloads song)
	void stop();

	void muteChannel(uint8_t ch);

	void unMuteChannel(uint8_t ch);
};

#endif
