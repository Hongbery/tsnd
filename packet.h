/*
 * packet.h
 *
 *  Created on: 24.04.2014
 *      Author: jasper
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <inttypes.h>

#include "common.h"


struct Packet_packet
{
    // timestamp?
    uint8_t port;
    uint8_t *packet;
    uint32_t len;
    struct Common_timestamp t;
};


#endif /* PACKET_H_ */
