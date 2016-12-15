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

int parseMAXData(char *MAXData, int size, MAX_msg_list** msg_list)
{
    char *pos = MAXData, *tmp;
    char *end = MAXData + size - 1;
    MAX_msg_list *new = NULL, *iter;
    int len;
    size_t outlen, off;

    if (MAXData == NULL)
    {
        return -1;
    }
    while (pos != NULL && pos < end)
    {
        if (*(pos + 1) != ':')
        {
            return -1;
        }
        tmp = strstr(pos, MSG_END);
        if (tmp == NULL)
        {
            return -1;
        }
        tmp += MSG_END_LEN;
        new = (MAX_msg_list*)malloc(sizeof(MAX_msg_list));
        if (*msg_list == NULL)
        {
            *msg_list = new;
            new->prev = NULL;
            new->next = NULL;
        }
        else
        {
            iter = *msg_list;
            while (iter->next != NULL) {
                iter = iter->next;
            }
            iter->next = new;
            new->prev = iter;
            new->next = NULL;
        }
        switch (*pos)
        {
            case 'H':
                {
                    int H_len = sizeof(struct MAX_message) - 1 +
                                            sizeof(struct H_Data);
                    new->MAX_msg = malloc(H_len);
                    memcpy(new->MAX_msg, pos, H_len);
                    new->MAX_msg_len = H_len;
                    break;
                }
            case 'C':
                /* Calculate offset of second field (C_Data_Device) */
                off = sizeof(struct MAX_message) - 1 + sizeof(struct C_Data);
                /* Move to second field */
                /* Calculate length of second field */
                len = tmp - MSG_END_LEN - pos - off;
                new->MAX_msg = (struct MAX_message*)base64_to_hex(pos + off,
                                             len, off, 0, &outlen);
                if (new->MAX_msg == NULL)
                {
                    return -1;
                }
                memcpy(new->MAX_msg, pos, off);
                new->MAX_msg_len = off + outlen;
                break;
            case 'L':
                /* Calculate offset of payload */
                off = sizeof(struct MAX_message) - 1;
                /* Calculate length of data */
                len = tmp - MSG_END_LEN - pos - off;
                new->MAX_msg = (struct MAX_message*)base64_to_hex(pos + off,
                                             len, off, 0, &outlen);
                if (new->MAX_msg == NULL)
                {
                    return -1;
                }
                memcpy(new->MAX_msg, pos, off);
                new->MAX_msg_len = off + outlen;
                break;
            case 'M':
            case 'Q':
                new->MAX_msg = malloc(tmp - pos);
                memcpy(new->MAX_msg, pos, tmp - pos);
                new->MAX_msg_len = tmp - pos;
                break;
            case 'S':
                {
                    int S_len = sizeof(struct MAX_message) - 1 +
                                            sizeof(struct S_Data);
                    new->MAX_msg = malloc(S_len);
                    memcpy(new->MAX_msg, pos, S_len);
                    new->MAX_msg_len = S_len;
                    break;
                }
            case 's':
                /* Calculate offset of payload */
                off = sizeof(struct MAX_message) - 1;
                /* Calculate length of data */
                len = tmp - MSG_END_LEN - pos - off;
                new->MAX_msg = (struct MAX_message*)base64_to_hex(pos + off,
                                             len, off, 0, &outlen);
                if (new->MAX_msg == NULL)
                {
                    return -1;
                }
                memcpy(new->MAX_msg, pos, off);
                new->MAX_msg_len = off + outlen;
                break;
            default:
                new->MAX_msg = malloc(tmp - pos);
                memcpy(new->MAX_msg, pos, tmp - pos);
                break;
        }
        pos = tmp;
    }
    return 0;
}
