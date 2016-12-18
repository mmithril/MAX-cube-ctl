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

#include "maxmsg.h"
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
struct MAX_message parsedMessage;
struct L_Data parsedLData[10];
int thermostatCount=0;

void addChar(char ch) {
    switch (state) {
        case 0:
            if (previousChar == '\r' && ch == '\n') {
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
    parsedMessage.type = 'L';
    size_t dataLength;
    parsedMessage.data = base64_to_hex(lMessageBuffer, lPosition - MSG_END_LEN, 0, 0, &dataLength);
    parsedMessage.dataLength = dataLength;
    
}

char* getOriginalLMessage() {
    return lMessageBuffer;
}

int getOriginalLMessageLength(){
    return lPosition;
}
 
struct MAX_message getLMessage() {
    return parsedMessage;
}
