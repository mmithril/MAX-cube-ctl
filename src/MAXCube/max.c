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

#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

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
        errno = EINVAL;
        return -1;
    }
    while (pos != NULL && pos < end)
    {
        if (*(pos + 1) != ':')
        {
            errno = EBADMSG;
            return -1;
        }
        tmp = strstr(pos, MSG_END);
        if (tmp == NULL)
        {
            errno = EBADMSG;
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
                    errno = EBADMSG;
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
                    errno = EBADMSG;
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
                    errno = EBADMSG;
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

static int addrinifaddrs(struct sockaddr *sa, struct ifaddrs *ifaddr)
{
    while (ifaddr)
    {
        if (ifaddr->ifa_addr->sa_family == sa->sa_family)
        {
            if (sa->sa_family == AF_INET)
            {
                struct sockaddr_in *sin, *ifsin;
                sin = (struct sockaddr_in*)sa;
                ifsin = (struct sockaddr_in*)ifaddr->ifa_addr;
                if (ifsin->sin_addr.s_addr == sin->sin_addr.s_addr)
                {
                    /* Found it! */
                    return 1;
                }
            }
            else
            {
                /* Only IPv4 supported */
                return 0;
            }
        }
        ifaddr = ifaddr->ifa_next;
    }
    return 0;
}

int MAXDiscover(struct sockaddr *sa, socklen_t sa_len,
    struct Discover_Data *D_Data, int tmo)
{
#ifdef __CYGWIN__
    fd_set fds;
#endif
    struct timeval tv;
    struct sockaddr_in sin_bcast, sin_mcast, sin;
    int i, pkt_len, n;
    char discover_pkt[] = "eQ3Max*\0**********I";
    int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int broadcast = 1;
    struct ifaddrs *ifaddr;

    if ((n = getifaddrs(&ifaddr)) != 0) {
        close(sd);
        return n;
    }

    tv.tv_sec = tmo / 1000;
    tv.tv_usec = (tmo % 1000) * 1000;

    sin_mcast.sin_family = AF_INET;
    sin_mcast.sin_port = htons(MAX_DISCOVER_PORT);
    inet_pton(AF_INET, MAX_MCAST_ADDR, &sin_mcast.sin_addr);

    sin_bcast.sin_family = AF_INET;
    sin_bcast.sin_port = htons(MAX_DISCOVER_PORT);
    inet_pton(AF_INET, MAX_BCAST_ADDR, &sin_bcast.sin_addr);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(MAX_DISCOVER_PORT);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    n = bind(sd, (struct sockaddr*)&sin, sizeof(sin));
    if (n < 0)
    {
        freeifaddrs(ifaddr);
        close(sd);
        return n;
    }

    pkt_len = sizeof(discover_pkt) - 1;

    /* Send to multicast address 3 times */
    for (i = 0; i < 3; i++)
    {
        n = sendto(sd, discover_pkt, pkt_len, 0, (struct sockaddr*)&sin_mcast,
                   sizeof(sin_mcast));
        if (n < 0)
        {
            freeifaddrs(ifaddr);
            close(sd);
            return n;
        }
    }

    if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof(broadcast)) == -1)
    {
        freeifaddrs(ifaddr);
        close(sd);
        return -1;
    }

    /* Send to broadcast address 3 times */
    for (i = 0; i < 3; i++)
    {
        n = sendto(sd, discover_pkt, pkt_len, 0, (struct sockaddr*)&sin_bcast,
                   sizeof(sin_bcast));
        if (n < 0)
        {
            freeifaddrs(ifaddr);
            close(sd);
            return n;
        }
    }

#ifdef __CYGWIN__
    FD_ZERO(&fds);
    FD_SET(sd, &fds);

    do {
        n = select(sd + 1, &fds, NULL, NULL, &tv);
        if (n == -1)
        {
            freeifaddrs(ifaddr);
            return -1;
        }
        else if (n > 0)
        {
            n = recvfrom(sd, D_Data, sizeof(struct Discover_Data), 0, sa,
                &sa_len);
            /* Ignore own packets */
            if (n > 0 && addrinifaddrs(sa, ifaddr) == 0)
            {
                /* This has to be the packet we were looking for */
                break;
            }
        }
    } while (n > 0);
#else
    if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
        sizeof(struct timeval)) < 0)
    {
        freeifaddrs(ifaddr);
        return -1;
    }

    do {
            n = recvfrom(sd, D_Data, sizeof(struct Discover_Data), 0, sa,
                &sa_len);
            /* Ignore own packets */
            if (n > 0 && addrinifaddrs(sa, ifaddr) == 0)
            {
                /* This has to be the packet we were looking for */
                break;
            }
    } while (n > 0);
#endif

    freeifaddrs(ifaddr);
    close(sd);

    return n;
}

int MAXConnect(struct sockaddr *sa)
{
    int sockfd;

    if((sockfd = socket(sa->sa_family, SOCK_STREAM, 0)) < 0)
    {
        return -1;
    }

    if(connect(sockfd, sa, sizeof(*sa)) < 0)
    {
        return -1;
    }

    return sockfd;
}

int MAXDisconnect(int connectionId)
{
    return close(connectionId);
}

int MAXMsgSend(int connectionId, MAX_msg_list *output_msg_list)
{
    int n, res;
    char *p;

    while (output_msg_list != NULL) {
        p = (char *)output_msg_list->MAX_msg;
        n = output_msg_list->MAX_msg_len;
        while (n > 0)
        {
            res = write(connectionId, p, n);
            if (res < 0)
            {
                return -1;
            }
            n -= res;
            p += res;
        }
        output_msg_list = output_msg_list->next;
    }
    return 0;
}

int MaxMsgRecv(int connectionId, MAX_msg_list **input_msg_list)
{
    char recvBuff[4096];
    int n;

    if ((n = read(connectionId, recvBuff, sizeof(recvBuff) - 1)) > 0)
    {
        parseMAXData(recvBuff, n, input_msg_list);
    }

    return 0;
}

int MaxMsgRecvTmo(int connectionId, MAX_msg_list **input_msg_list, int tmo)
{
#ifdef __CYGWIN__
    fd_set fds;
#endif
    char recvBuff[4096];
    struct timeval tv;
    int n;

    tv.tv_sec = tmo / 1000;
    tv.tv_usec = (tmo % 1000) * 1000;

#ifdef __CYGWIN__
    FD_ZERO(&fds);
    FD_SET(connectionId, &fds);

    do {
        n = select(connectionId + 1, &fds, NULL, NULL, &tv);
        if (n == -1)
        {
            return -1;
        }
        else if (n > 0)
        {
            n = read(connectionId, recvBuff, sizeof(recvBuff) - 1);
            parseMAXData(recvBuff, n, input_msg_list);
        }
    } while (n > 0);
#else
    if (setsockopt(connectionId, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
        sizeof(struct timeval)) < 0)
    {
        return -1;
    }

    while ((n = read(connectionId, recvBuff, sizeof(recvBuff) - 1)) > 0)
    {
        parseMAXData(recvBuff, n, input_msg_list);
    }
#endif

    return 0;
}

