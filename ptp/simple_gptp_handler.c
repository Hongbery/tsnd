/*
 * simple_gptp_handler.c
 *
 *  Created on: 24.04.2014
 *      Author: jasper
 */

#include "simple_gptp_handler.h"

#include <stdlib.h>
//#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "headers/ethernet.h"
#include "headers/ptp.h"


static struct HandlerTable_filterEntry gptpFilter[] =
{
        {HandlerTable_packetField_ETH_DST, 0, ETHERNET_MAC_LEN, PTP_ETH_MAC_P2P, ETHERNET_MAC_MASK},
        {HandlerTable_packetField_ETH_TYPE, 0, ETHERNET_TYPE_LEN, PTP_ETH_TYPE, ETHERNET_TYPE_MASK},
        {HandlerTable_packetField_NONE, 0, 0, NULL, NULL}
};

static void packetHandler(const struct Packet_packet *packet, void *context);
static void handlePDelayReq(const struct PTP_pDelayReq *packet, struct SimpleGPTPHandler_state *state);


/*
 * Return values:
 *             0: success
 *            -1: pointer null
 *            -2: could not register handler
 *            -3: could not allocate memory
 */
int32_t SimpleGPTPHandler_init(struct HandlerTable_table *table, struct Port *ports, uint32_t portCnt /* TODO: config */)
{
    struct SimpleGPTPHandler_state *state;
    struct HandlerTable_tableEntry *entry;

    if(table == NULL)
        return -1;

    state = malloc(sizeof(struct SimpleGPTPHandler_state));
    entry = malloc(sizeof(struct HandlerTable_tableEntry));
    if(state == NULL || entry == NULL)
        goto fail;

    memset(state, 0, sizeof(struct SimpleGPTPHandler_state));
    memset(entry, 0, sizeof(struct HandlerTable_tableEntry));

    state->conf = malloc(sizeof(struct PTPConfig));
    if(state->conf == NULL)
        goto fail;
    memset(state->conf, 0, sizeof(struct PTPConfig));

    state->ports = ports;
    state->portCnt = portCnt;

    state->conf->isAVB = 1;
    state->conf->versionPTP = PTP_VERSIONPTP_0;
    // initialize static members defaultDS
    state->conf->defaultDS.twoStepFlag = 1;
    //    state->conf->defaultDS.clockId TODO
    state->conf->defaultDS.numberPorts = portCnt;

    // initialize dynamic members defaultDS
    state->conf->defaultDS.clockQuality.clockClass = 255; // slave only
    state->conf->defaultDS.clockQuality.clockAccuracy = 0xFE; // unknown
    state->conf->defaultDS.clockQuality.offsetScaledLogVariance = 0xFFFF; // max value, (for the monent TODO)
    state->conf->defaultDS.priority1 = 255; // we do not want to be grandmaster
    state->conf->defaultDS.priority2 = 255; // so get the worst priority
    state->conf->defaultDS.domainNumber = 0; // default domain
    state->conf->defaultDS.slaveOnly = 1;

    // initialize dynamic members of currentDS
    state->conf->currentDS.stepsRemoved = 0;
    state->conf->currentDS.offsetFromMaster = 0;
    state->conf->currentDS.meanPathDelay = 0;

    // initialize dynamic members of parentDS
    memcpy(state->conf->parentDS.parentPortId, state->conf->defaultDS.clockId, PTP_CLOCKID_LEN);
    state->conf->parentDS.partentStats = 0;
    state->conf->parentDS.observedParentOffsetScaledLogVariance = 0xFFFF;
    state->conf->parentDS.observedParentClockPhaseChangeRate = 0x7FFFFFFF;
    memcpy(state->conf->parentDS.grandmasterIdentity, state->conf->defaultDS.clockId, PTP_CLOCKID_LEN);
    state->conf->parentDS.grandmasterClockQuality = state->conf->defaultDS.clockQuality;
    state->conf->parentDS.grandmasterPriority1 = state->conf->defaultDS.priority1;
    state->conf->parentDS.grandmasterPriority2 = state->conf->defaultDS.priority2;

    // initialize dynamic members of timePropertiesDS
    state->conf->timePropertiesDS.currentUtcOffset = PTP_CURRUTCOFFSET;
    state->conf->timePropertiesDS.currentUtcOffsetValid = 1;
    state->conf->timePropertiesDS.leap59 = 0;
    state->conf->timePropertiesDS.leap61 = 0;
    state->conf->timePropertiesDS.timeTracable = 0;
    state->conf->timePropertiesDS.frequencyTracable = 0;
    state->conf->timePropertiesDS.ptpTimescale = 1;
    state->conf->timePropertiesDS.timeSource = 0xA0; // internal oscillator

    // TODO: setup state

    // okay, register handler
    entry->context = state;
    entry->filters = gptpFilter;
    entry->handler = &packetHandler;
    if(HandlerTable_registerHandler(table, entry) != 0)
        goto fail;

    return 0;

fail:
    if(state != NULL)
    {
        if(state->conf != NULL)
            free(state->conf);
        free(state);
    }
    if(entry != NULL)
        free(entry);
    return -3;
}

static void packetHandler(const struct Packet_packet *packet, void *context)
{
    struct SimpleGPTPHandler_state *state = (struct SimpleGPTPHandler_state*)context;
    const struct PTP_header *ptpPacket;
    uint32_t ethHdrLen, len;

    puts("recd msg");

    if(packet == NULL || context == NULL || packet->packet == NULL)
        return;

    ethHdrLen = Ethernet_getHeaderLength(packet->packet, packet->len);
    if(ethHdrLen == 0)
        return;

    len = packet->len - ethHdrLen;

    ptpPacket = (struct PTP_header*)(packet->packet + ethHdrLen);

    puts("got ptp =)");

    if(PTP_isPacketValid((uint8_t*)ptpPacket, len, state->conf) != 0)
        return;

    puts("ptp message okay");

    switch(PTP_GET_MESSAGETYPE(ptpPacket->transportSpecific_messageType))
    {
    case PTP_MESSAGE_TYPE_PDELAY_REQ:
        handlePDelayReq((struct PTP_pDelayReq*)ptpPacket, state);
        break;
    case PTP_MESSAGE_TYPE_SYNC:
    case PTP_MESSAGE_TYPE_DELAY_REQ:
    case PTP_MESSAGE_TYPE_PDELAY_RESP:
    case PTP_MESSAGE_TYPE_PDELAY_RESP_FOLLOW_UP:
    case PTP_MESSAGE_TYPE_FOLLOW_UP:
    case PTP_MESSAGE_TYPE_DELAY_RESP:
    case PTP_MESSAGE_TYPE_ANNOUNCE:
    case PTP_MESSAGE_TYPE_SIGNALING:
    case PTP_MESSAGE_TYPE_MANAGEMENT:
        // currently not handled
        break;
    }
}

static void handlePDelayReq(const struct PTP_pDelayReq *packet, struct SimpleGPTPHandler_state *state)
{
    puts("got PDelayReq");
}
