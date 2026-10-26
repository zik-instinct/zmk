#pragma once
#include <stdbool.h>
/* Runtime flag: when true, hold-taps act as plain taps. Not persisted. */
bool zmk_hrm_off(void);
void zmk_hrm_off_set(bool off);
