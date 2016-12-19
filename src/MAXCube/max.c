/* Copyright (c) 2015, Costin Popescu
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "max.h"
#include "base64.h"


/*
 * 0 - other messages
 * 1 - /r/n parsed
 * 2 - L: parsed
 */
int state = 0;
char previousChar = 0;
char lMessageBuffer[200];
int lPosition = 0;
struct ThermostatData thermostatData[10];
int thermostatCount = 0;

void addChar(char ch) {
    switch (state) {
        case 0:
            if (previousChar == MSG_END[0] && ch == MSG_END[1]) {
                state = 1;
            }
            break;
        case 1:
            if (ch == ':') {
                if (previousChar == 'L') {
                    state = 2;
                    lPosition = 0;
                } else {
                    state = 0;
                }
            }
            break;
        case 2:
            if (previousChar == '\r' && ch == '\n') {
                state = 1;
            }
            lMessageBuffer[lPosition++] = ch;
    }
    previousChar = ch;
}

void finalizeParsing() {
    size_t dataLength;
    char* parsedMessage = base64_to_hex(lMessageBuffer, lPosition - MSG_END_LEN, 0, 0, &dataLength);
    thermostatCount = 0;
    for (int i = 0; i < dataLength;) {
        unsigned char submsgLength = (unsigned char) (parsedMessage[i]);

        if (submsgLength == 11 && thermostatCount < 10) {
            thermostatData[thermostatCount].RFAddress[0] = parsedMessage[i + 1];
            thermostatData[thermostatCount].RFAddress[1] = parsedMessage[i + 2];
            thermostatData[thermostatCount].RFAddress[2] = parsedMessage[i + 3];
            thermostatData[thermostatCount].valvePosition = parsedMessage[i + 7];
            thermostatData[thermostatCount].setpoint = (parsedMessage[i + 8] & 0b01111111) / 2;
            thermostatCount++;
        }

        i += submsgLength + 1;
    }
    
    // reset parser state
    free(parsedMessage);
    state = 0;
    previousChar = 0;
    lPosition = 0;
}

char* getRawLMessage() {
    return lMessageBuffer;
}

int getRawLMessageLength() {
    return lPosition;
}

int getThermostatCount() {
    return thermostatCount;
}

struct ThermostatData* getThermostatData() {
    return thermostatData;
} 