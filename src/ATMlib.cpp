
#include <ATMlib.h>


void ATMsynth::setup(void) {
	if (!setup_done) {
		atm_synth_setup();
		setup_done = true;
	}
}

void ATMsynth::play(const uint8_t *score) {
	setup();
	atm_synth_play_score(score);
}

// Stop playing, unload melody
void ATMsynth::stop() {
	atm_synth_stop_score();
}

// Start grinding samples or Pause playback
void ATMsynth::playPause() {
	atm_synth_set_score_paused(!atm_synth_get_score_paused());
}

// Toggle mute on/off on a channel, so it can be used for sound effects
// So you have to call it before and after the sound effect
void ATMsynth::muteChannel(uint8_t ch) {
	atm_synth_set_muted((1 << ch) | atm_synth_get_muted());
}

void ATMsynth::unMuteChannel(uint8_t ch) {
	atm_synth_set_muted(~(1 << ch) & atm_synth_get_muted());
}
