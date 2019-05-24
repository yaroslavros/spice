/*
   Copyright (C) 2009-2015 Red Hat, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#include <config.h>

#include "inputs-channel-client.h"
#include "migration-protocol.h"
#include "red-channel-client.h"

uint8_t *InputsChannelClient::alloc_recv_buf(uint16_t type, uint32_t size)
{
    if (size > sizeof(recv_buf)) {
        red_channel_warning(get_channel(), "error: too large incoming message");
        return NULL;
    }

    return recv_buf;
}

void InputsChannelClient::release_recv_buf(uint16_t type, uint32_t size, uint8_t *msg)
{
}

void InputsChannelClient::on_disconnect()
{
    inputs_release_keys(INPUTS_CHANNEL(get_channel()));
}

RedChannelClient* inputs_channel_client_create(RedChannel *channel,
                                               RedClient *client,
                                               RedStream *stream,
                                               RedChannelCapabilities *caps)
{
    auto rcc = new InputsChannelClient(channel, client, stream, caps);
    if (!rcc->init()) {
        delete rcc;
        rcc = nullptr;
    }
    return rcc;
}

void InputsChannelClient::send_migrate_data(SpiceMarshaller *m, RedPipeItem *item)
{
    init_send_data(SPICE_MSG_MIGRATE_DATA);

    spice_marshaller_add_uint32(m, SPICE_MIGRATE_DATA_INPUTS_MAGIC);
    spice_marshaller_add_uint32(m, SPICE_MIGRATE_DATA_INPUTS_VERSION);
    spice_marshaller_add_uint16(m, motion_count);
}

void InputsChannelClient::handle_migrate_data(uint16_t motion_count)
{
    motion_count = motion_count;

    for (; motion_count >= SPICE_INPUT_MOTION_ACK_BUNCH;
           motion_count -= SPICE_INPUT_MOTION_ACK_BUNCH) {
        pipe_add_type(RED_PIPE_ITEM_MOUSE_MOTION_ACK);
    }
}

void InputsChannelClient::on_mouse_motion()
{
    InputsChannel *inputs_channel = INPUTS_CHANNEL(get_channel());

    if (++motion_count % SPICE_INPUT_MOTION_ACK_BUNCH == 0 &&
        !inputs_channel_is_src_during_migrate(inputs_channel)) {
        pipe_add_type(RED_PIPE_ITEM_MOUSE_MOTION_ACK);
        motion_count = 0;
    }
}