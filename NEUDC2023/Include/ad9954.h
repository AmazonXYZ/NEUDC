#pragma once

#include <stdbool.h>
#include <stdint.h>

#define AD9954_MODE_DWELL   0x04
#define AD9954_MODE_NODWELL 0x00

void ad9954_init(void);
void ad9954_set_ps_mode(bool ps1, bool ps2);
void ad9954_set_freq(float freq);
void ad9954_set_amp(float amp_ratio);
void ad9954_set_phase(float phase_radio);
void ad9954_set_linearsweep(
    float start_freq, float stop_freq, float step_freq, uint8_t step_time_d10, uint8_t mode
);

inline void ad9964_update(void);