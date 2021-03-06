/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host commands for TCPMv2 USB PD module
 */

#include <string.h>

#include "console.h"
#include "ec_commands.h"
#include "host_command.h"
#include "usb_pd.h"

#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ## args)
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ## args)

/* Retrieve all discovery results for the given port and transmit type */
static enum ec_status hc_typec_discovery(struct host_cmd_handler_args *args)
{
	const struct ec_params_typec_discovery *p = args->params;
	struct ec_response_typec_discovery *r = args->response;
	const struct pd_discovery *disc;
	enum tcpm_transmit_type type;

	/* Confirm the number of HC VDOs matches our stored VDOs */
	BUILD_ASSERT(sizeof(r->discovery_vdo) == sizeof(union disc_ident_ack));

	if (p->port >= board_get_usb_pd_port_count())
		return EC_RES_INVALID_PARAM;

	if (p->partner_type > TYPEC_PARTNER_SOP_PRIME)
		return EC_RES_INVALID_PARAM;

	type = p->partner_type == TYPEC_PARTNER_SOP ?
		TCPC_TX_SOP : TCPC_TX_SOP_PRIME;

	/*
	 * Clear out access mask so we can track if tasks have touched data
	 * since read started.
	 */
	pd_discovery_access_clear(p->port, type);

	disc = pd_get_am_discovery(p->port, type);

	/* Initialize return size to that of discovery with no SVIDs */
	args->response_size = sizeof(*r);

	if (pd_get_identity_discovery(p->port, type) == PD_DISC_COMPLETE) {
		r->identity_count = disc->identity_cnt;
		memcpy(r->discovery_vdo,
		       pd_get_identity_response(p->port, type)->raw_value,
		       sizeof(r->discovery_vdo));
	} else {
		r->identity_count = 0;
		return EC_RES_SUCCESS;
	}

	if (pd_get_modes_discovery(p->port, type) == PD_DISC_COMPLETE) {
		int svid_i;
		int max_resp_svids = (args->response_max - args->response_size)/
				      sizeof(struct svid_mode_info);

		if (disc->svid_cnt > max_resp_svids) {
			CPRINTS("Warn: SVIDS exceeded HC response");
			r->svid_count = max_resp_svids;
		} else {
			r->svid_count = disc->svid_cnt;
		}

		for (svid_i = 0; svid_i < r->svid_count; svid_i++) {
			r->svids[svid_i].svid = disc->svids[svid_i].svid;
			r->svids[svid_i].mode_count =
						disc->svids[svid_i].mode_cnt;
			memcpy(r->svids[svid_i].mode_vdo,
			       disc->svids[svid_i].mode_vdo,
			       sizeof(r->svids[svid_i].mode_vdo));
			args->response_size += sizeof(struct svid_mode_info);
		}
	} else {
		r->svid_count = 0;
	}

	/*
	 * Verify that another task did not access this data during the duration
	 * of the copy.  If the data was accessed, return BUSY so the AP will
	 * try retrieving again and get the updated data.
	 */
	if (!pd_discovery_access_validate(p->port, type))
		return EC_RES_BUSY;

	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_TYPEC_DISCOVERY,
		     hc_typec_discovery,
		     EC_VER_MASK(0));
