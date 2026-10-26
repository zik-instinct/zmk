#pragma once
#include <stdbool.h>
/* Persisted flag: when true, hold-taps that opt in act as plain taps. */
bool zmk_hrm_off(void);
int zmk_hrm_off_set(bool off); /* returns <0 if scheduling the flash save failed; RAM state is still set */
