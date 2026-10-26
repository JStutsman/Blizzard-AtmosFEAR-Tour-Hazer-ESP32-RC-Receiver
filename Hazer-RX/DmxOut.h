// DmxOut.h  (Hazer-RX) - continuous DMX512 output on the RS485 port
#pragma once
#include <Arduino.h>

void    dmxBegin();                          // starts the DMX task (all channels at 0)
void    dmxSetChannel(uint16_t ch, uint8_t v); // ch = 1..512
uint8_t dmxGetChannel(uint16_t ch);
void    dmxZeroAll();
