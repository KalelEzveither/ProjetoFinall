#ifndef MUSIC_H
#define MUSIC_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

void pwm_init_buzzer(uint pin);
void play_Megalovania(uint pin);
void play_Sonic(uint pin);
void play_Zelda(uint pin);
void play_Pokemon(uint pin);
void play_Pacman(uint pin);
void tone(uint pin, uint frequency, uint duration_ms);

#endif
