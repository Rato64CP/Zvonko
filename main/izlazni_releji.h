#pragma once

#include <stdint.h>

void inicijalizirajIzlazneReleje();
void postaviIzlazniRelej(uint8_t pin, bool ukljuceno);
void iskljuciSveIzlazneReleje();
void primijeniPolarnostIzlaznihReleja();

