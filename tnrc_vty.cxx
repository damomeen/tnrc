//
//  This file is part of phosphorus-g2mpls.
//
//  Copyright (C) 2006, 2007, 2008, 2009 Nextworks s.r.l.
//  Copyright (C) 2006, 2007, 2008, 2009 PSNC
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//  Authors:
//
//  Giacomo Bernini       (Nextworks s.r.l.) <g.bernini_at_nextworks.it>
//  Gino Carrozzo         (Nextworks s.r.l.) <g.carrozzo_at_nextworks.it>
//  Nicola Ciulli         (Nextworks s.r.l.) <n.ciulli_at_nextworks.it>
//  Giodi Giorgi          (Nextworks s.r.l.) <g.giorgi_at_nextworks.it>
//  Francesco Salvestrini (Nextworks s.r.l.) <f.salvestrini_at_nextworks.it>
//  Adam Kaliszan         (PSNC)             <kaliszan_at_man.poznan.pl>

//



#include <zebra.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "thread.h"
#include "vty.h"
#include "command.h"

#include "tnrcd.h"
#include "tnrc_plugin_simulator.h"
#include "tnrc_plugin_tl1_adva.h"
#include "tnrc_plugin_tl1_calient.h"
#include "tnrc_plugin_ssh_foundry.h"
#include "tnrc_plugin_telnet_at_8000.h"
#include "tnrc_plugin_telnet_at_9424.h"
#include "tnrc_plugin_telnet_cls.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"

using namespace std;

struct cmd_node tnrc_node = {
	TNRC_NODE,
	"%s(config-tnrc)# ",
	1
};

static std::string
bitmap2string(wdm_link_lambdas_bitmap_t & bitmap)
{
	std::string        tmp;
	std::ostringstream Id;
	uint32_t *         mask;

	tmp  = std::string("0x");
	mask = bitmap.bitmap_word;
	assert(mask);

	for (size_t i = 0; i < bitmap.bitmap_size; i++) {
		Id << hex << setw(8) << setfill('0')
		   << *mask << dec;
		mask++;
	}
	tmp += Id.str();

	return tmp;
}
static std::string
label2string(const label_t & label_id)
{
	std::string        tmp;
	std::ostringstream Id;

	tmp = std::string("");

	switch (label_id.type) {
		case LABEL_PSC:
			Id << "PSC (0x"  << hex << setw(8) << setfill('0')
			   << label_id.value.psc.id << ")" << dec;
			break;
		case LABEL_L2SC:
			int i;

			Id << "L2SC(0x"  << hex << setw(16) << setfill('0')
			   << label_id.value.rawId << ")" << dec;

			Id << ": VLAN-ID = "  << label_id.value.l2sc.vlanId;
			Id << " / DST-MAC = ";
			for (i = 0; i < 6; i++) {
				Id << hex << setw(2) << setfill('0')
				   << label_id.value.l2sc.dstMac[i];
				if (i != 5) {
					Id << ":";
				}
			}
			break;
		case LABEL_TDM:
			Id << "TDM (0x"  << hex << setw(8) << setfill('0')
			   << label_id.value.rawId << ")" << dec;

			Id << ": S = "  << label_id.value.tdm.s;
			Id << " / U = "  << label_id.value.tdm.u;
			Id << " / K = "  << label_id.value.tdm.k;
			Id << " / L = "  << label_id.value.tdm.l;
			Id << " / M = "  << label_id.value.tdm.m;
			break;
		case LABEL_LSC:
		        uint32_t grid;
		        uint32_t label;

			label = label_id.value.lsc.wavelen;
			grid  = GMPLS_LABEL_TO_WDM_GRID(label);

			Id << "LSC (0x"  << hex << setw(8) << setfill('0')
			   << label << ")" << dec;
			Id <<": Grid = " << SHOW_WDM_GRID(grid);

			if (grid == WDM_GRID_ITUT_DWDM) {
				uint32_t cs, s, n;
				float    frequency;

				GMPLS_LABEL_TO_DWDM_CS_S_N(label, cs, s, n);
				Id << " / Spacing = "
				   << SHOW_DWDM_CHANNEL_SPACING(cs);
				Id << " / n = " << (s ? "-" :"") << n;

				GMPLS_LABEL_TO_DWDM_LAMBDA(label, frequency);

				Id << " -- " << frequency <<"THz";
			} else if (grid == WDM_GRID_ITUT_CWDM) {
				uint32_t wl;

				GMPLS_LABEL_TO_CWDM_WAVELENGTH(label, wl);
				Id << "/ wavelength = " << wl <<"nm";
			}
			break;
		case LABEL_FSC:
			Id << "FSC (0x" << hex << setw(8) << setfill('0')
			   << label_id.value.fsc.portId << ")" << dec;
			Id << ": port id = " << label_id.value.fsc.portId;
			break;
		default:
			Id << "0x"  << hex << setw(8) << setfill('0')
			   << label_id.value.rawId << dec;
			break;
	}

	tmp += Id.str();

	return tmp;
}

DEFUN(equipment,
      equipment_cmd,
      "equipment name WORD location WORD",
      "Specify the equipment type\n"
      "Equipment specification\n")
{
	std::string	   name;
	std::string        location;
	tnrcapiErrorCode_t res;

	name = argv[0];
	location = argv[1];

	res = TNRC_CONF_API::init_plugin(name, location);
	if (res != TNRC_API_ERROR_NONE) {
		vty_out(vty,
			"Error in initializing plugin: %s%s",
			TNRCAPIERR2STRING(res), VTY_NEWLINE);
		//exit (-1);
		return CMD_WARNING;
	}

	return CMD_SUCCESS;
}

DEFUN(eqpt,
      eqpt_cmd,
      "eqpt id WORD addr A.B.C.D type (simulator|adva_lsc|calient_fsc|foundry_l2sc|at8000_l2sc|at9424_l2sc|cls_fsc) opstate (up|down) \
       admstate (enabled|disabled) location WORD",
      "Create an equipment simulator\n"
      "Equipment simulation\n")
{
	eqpt_id_t          id;
	g2mpls_addr_t      address;
	eqpt_type_t        type;
	opstate_t          opstate;
	admstate_t         admstate;
	std::string        location;
	tnrcapiErrorCode_t res;

	id = strtoul(argv[0], NULL, 10);
	memset(&address, 0, sizeof(address));

	address.type    = IPv4;
	address.preflen = 32;
	if(!inet_aton(argv[1], &(address.value.ipv4))) {
		vty_out(vty,
			"Invalid IP address (%s) for equipment address%s",
			argv[1], VTY_NEWLINE);
                TNRC_ERR("Conf: Invalid IP address (%s) for equipment address",
                         argv[1]);
		return CMD_WARNING;
	}

	if (strcmp(argv[2],"simulator") == 0)
        {
            type = EQPT_SIMULATOR;
        }
        else if (strcmp(argv[2],"adva_lsc") == 0)
        {
            type = EQPT_ADVA_LSC;
        }
        else if (strcmp(argv[2],"calient_fsc") == 0)
        {
            type = EQPT_CALIENT_DW_PXC;
        }
        else if (strcmp(argv[2],"foundry_l2sc") == 0)
        {
            type = EQPT_FOUNDRY_L2SC;
        }
        else if (strcmp(argv[2],"at8000_l2sc") == 0)
        {
            type = EQPT_AT8000_L2SC;
        }
        else if (strcmp(argv[2],"at9424_l2sc") == 0)
        {
            type = EQPT_AT9424_L2SC;
        }
        else if (strcmp(argv[2],"cls_fsc") == 0)
        {
            type = EQPT_CLS_FSC;
        }
        else
        {
		vty_out(vty, "Invalid type (%s) of equipment%s", argv[2], VTY_NEWLINE);
                TNRC_ERR("Conf: Invilid type (%s) of equipment", argv[2]);
		return CMD_WARNING;
	}


	if(strcmp(argv[3],"up") == 0) {
		opstate = UP;
	}
	else if(strcmp(argv[3],"down") == 0) {
		opstate = DOWN;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s) for equipment%s",
			argv[3], VTY_NEWLINE);
                TNRC_ERR("Conf: Invalid opstate (%s) for equipment", argv[3]);

		return CMD_WARNING;
	}

	if(strcmp(argv[4],"enabled") == 0) {
		admstate = ENABLED;
	}
	else if(strcmp(argv[4],"disabled") == 0) {
		admstate = DISABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s) for equipment%s",
			argv[4], VTY_NEWLINE);
                TNRC_ERR("Conf: Invalid admstate (%s) for equipment", argv[4]);

		return CMD_WARNING;
	}
	location = std::string (argv[5]);

	//call TNRC_CONF_API function to create a new Eqpt in the data model
	res = TNRC_CONF_API::add_Eqpt(id,
				      address,
				      type,
				      opstate,
				      admstate,
				      location);
	if(res != TNRC_API_ERROR_NONE) {
		vty_out (vty,
			 "Add of equipment in the data model failed: %s%s",
			 TNRCAPIERR2STRING(res), VTY_NEWLINE);
                TNRC_ERR("Conf: Add of equipment in the data model failed: %s",
                         TNRCAPIERR2STRING(res));

		return CMD_WARNING;
	}

	vty_out(vty,
		"Equipment with id %d correctly registered%s",
		id, VTY_NEWLINE);
        TNRC_INF("Conf: Equipment with id %d correctly registered",id);

	return CMD_SUCCESS;

}

DEFUN(board,
      board_cmd,
      "board id WORD eqpt-id WORD sw-cap (psc1|psc2|psc3|psc4|l2sc|fsc|lsc|tdm) \
       enc-type (packet|ethernet|pdh|reserved1|sdh|reserved2|digitalwrap|\
       lambda|fiber|reserved3|fiberchan|g709_odu|g709_oc) opstate (up|down) \
       admstate (enabled|disabled)",
      "Create a board simulator\n"
      "Board simulation\n")
{
	eqpt_id_t          eqpt_id;
	board_id_t         id;
	sw_cap_t           sw_cap;
	enc_type_t         enc_type;
	opstate_t          opstate;
	admstate_t         admstate;
	tnrcapiErrorCode_t res;

	id      = strtoul(argv[0], NULL, 10);
	eqpt_id = strtoul(argv[1], NULL, 10);

	//Switching cap
	if(strcmp(argv[2],"psc1") == 0) {
		sw_cap = SWCAP_PSC_1;
	}
	else if(strcmp(argv[2],"psc2") == 0) {
		sw_cap = SWCAP_PSC_2;
	}
	else if(strcmp(argv[2],"psc3") == 0) {
		sw_cap = SWCAP_PSC_3;
	}
	else if(strcmp(argv[2],"psc4") == 0) {
		sw_cap = SWCAP_PSC_4;
	}
	else if(strcmp(argv[2],"l2sc") == 0) {
		sw_cap = SWCAP_L2SC;
	}
	else if(strcmp(argv[2],"fsc") == 0) {
		sw_cap = SWCAP_FSC;
	}
	else if(strcmp(argv[2],"lsc") == 0) {
		sw_cap = SWCAP_LSC;
	}
	else if(strcmp(argv[2],"tdm") == 0) {
		sw_cap = SWCAP_TDM;
	}
	else {
		sw_cap = SWCAP_UNKNOWN;
	}
	//Encoding type
	if(strcmp(argv[3],"packet") == 0) {
		enc_type = ENCT_PACKET;
	}
	else if(strcmp(argv[3],"ethernet") == 0) {
		enc_type = ENCT_ETHERNET;
	}
	else if(strcmp(argv[3],"pdh") == 0) {
		enc_type = ENCT_ANSI_ETSI_PDH;
	}
	else if(strcmp(argv[3],"reserved1") == 0) {
		enc_type = ENCT_RESERVED_1;
	}
	else if(strcmp(argv[3],"sdh") == 0) {
		enc_type = ENCT_SDH_SONET;
	}
	else if(strcmp(argv[3],"reserved2") == 0) {
		enc_type = ENCT_RESERVED_2;
	}
	else if(strcmp(argv[3],"digitalwrap") == 0) {
		enc_type = ENCT_DIGITAL_WRAPPER;
	}
	else if(strcmp(argv[3],"lambda") == 0) {
		enc_type = ENCT_LAMBDA;
	}
	else if(strcmp(argv[3],"fiber") == 0) {
		enc_type = ENCT_FIBER;
	}
	else if(strcmp(argv[3],"reserved3") == 0) {
		enc_type = ENCT_RESERVED_3;
	}
	else if(strcmp(argv[3],"fiberchan") == 0) {
		enc_type = ENCT_FIBERCHANNEL;
	}
	else if(strcmp(argv[3],"g709_odu") == 0) {
		enc_type = ENCT_G709_ODU;
	}
	else if(strcmp(argv[3],"g709_oc") == 0) {
		enc_type = ENCT_G709_OC;
	} else {
		enc_type = ENCT_UNKNOWN;
	}
	//Opstate
	if(strcmp(argv[4],"up") == 0) {
		opstate = UP;
	}
	else if(strcmp(argv[4],"down") == 0) {
		opstate = DOWN;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s) for board%s",
			argv[4], VTY_NEWLINE);
                TNRC_ERR("Conf: Invalid opstate (0x%x) for board",
                        argv[4]);
		return CMD_WARNING;
	}
	//Admstate
	if(strcmp(argv[5],"enabled") == 0) {
		admstate = ENABLED;
	}
	else if(strcmp(argv[5],"disabled") == 0) {
		admstate = DISABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s) for board%s",
			argv[5], VTY_NEWLINE);
                TNRC_ERR("Conf: Invalid admstate (0x%x) for board",
                        argv[5]);
		return CMD_WARNING;
	}

	//call TNRC_CONF_API function to create a new Board in the data model
	res = TNRC_CONF_API::add_Board(eqpt_id,
				       id,
				       sw_cap,
				       enc_type,
				       opstate,
				       admstate);
	if(res != TNRC_API_ERROR_NONE) {
		vty_out (vty,
			 "Add of board in the data model failed: %s%s",
			 TNRCAPIERR2STRING(res), VTY_NEWLINE);
                TNRC_ERR("Conf: Add of board in the data model failed: %s",
                         TNRCAPIERR2STRING(res));
		return CMD_WARNING;
	}

	vty_out(vty,
		"Board with id %d correctly registered on the equipment%s",
		id, VTY_NEWLINE);

        TNRC_INF("Conf: Board with id 0x%x correctly registered on the equipment",
                id);

	return CMD_SUCCESS;
}

DEFUN(port,
      port_cmd,
      "port id WORD board-id WORD eqpt-id WORD flags WORD rem-eq-addr A.B.C.D \
       rem-portid WORD opstate (up|down) admstate (enabled|disabled) \
       bw WORD \
       protection (none|extra|unprotected|shared|1to1|1plus1|enhanced) \
       lambdas-base WORD lambdas-count WORD",
      "Create a port simulator\n"
      "Port simulation\n")
{
	port_id_t          id;
	board_id_t         board_id;
	eqpt_id_t          eqpt_id;
	int                flags;
	g2mpls_addr_t      remote_eqpt_address;
	port_id_t          remote_port_id;
	opstate_t          opstate;
	admstate_t         admstate;
	uint32_t           dwdm_lambda_base;
	uint16_t           dwdm_lambda_count;
	uint32_t           bandwidth;
	gmpls_prottype_t   protection;
	tnrcapiErrorCode_t res;

	id       = strtoul(argv[0], NULL, 0);
	board_id = strtoul(argv[1], NULL, 10);
	eqpt_id  = strtoul(argv[2], NULL, 10);
	flags    = strtoul(argv[3], NULL, 10);

	memset(&remote_eqpt_address, 0, sizeof(remote_eqpt_address));

	remote_eqpt_address.type = IPv4;
	remote_eqpt_address.preflen = 32;
	if (!inet_aton(argv[4], &(remote_eqpt_address.value.ipv4))) {
		vty_out(vty,
			"Invalid IP address (%s) for remote "
			"equipment address%s",
			argv[4], VTY_NEWLINE);
                TNRC_ERR("Conf: Invalid IP address (%s) for remote equipment address",
                        argv[4]);
		return CMD_WARNING;
	}

	remote_port_id = strtoul(argv[5], NULL, 0);

	if(strcmp(argv[6],"up") == 0) {
		opstate = UP;
	}
	else if(strcmp(argv[6],"down") == 0) {
		opstate = DOWN;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s) for port%s",
			argv[6], VTY_NEWLINE);
                TNRC_ERR("Conf: Invalid opstate (0x%x) for port",
                        argv[6]);
		return CMD_WARNING;
	}

	if(strcmp(argv[7],"enabled") == 0) {
		admstate = ENABLED;
	}
	else if(strcmp(argv[7],"disabled") == 0) {
		admstate = DISABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s) for port%s",
			argv[7], VTY_NEWLINE);
                TNRC_ERR("Conf: Invalid admstate (0x%x) for port",
                        argv[7]);
		return CMD_WARNING;
	}

	bandwidth  = strtoul(argv[8], NULL, 16);

	//protection type
	if(strcmp(argv[9],"none") == 0) {
		protection = PROTTYPE_NONE;
	}
	else if(strcmp(argv[9],"extra") == 0) {
		protection = PROTTYPE_EXTRA;
	}
	else if(strcmp(argv[9],"unprotected") == 0) {
		protection = PROTTYPE_UNPROTECTED;
	}
	else if(strcmp(argv[9],"shared") == 0) {
		protection = PROTTYPE_SHARED;
	}
	else if(strcmp(argv[9],"1to1") == 0) {
		protection = PROTTYPE_DEDICATED_1TO1;
	}
	else if(strcmp(argv[9],"1plus1") == 0) {
		protection = PROTTYPE_DEDICATED_1PLUS1;
	}
	else if(strcmp(argv[9],"enhanced") == 0) {
		protection = PROTTYPE_ENHANCED;
	}
	else {
		protection = PROTTYPE_NONE;
	}


	dwdm_lambda_count = (uint16_t) strtoul(argv[11], NULL, 10);
	dwdm_lambda_base  = (uint32_t) strtoul(argv[10], NULL, 16);

	//call TNRC_CONF_API function to create a new Port in the data model
	res = TNRC_CONF_API::add_Port(eqpt_id,
				      board_id,
				      id,
				      flags,
				      remote_eqpt_address,
				      remote_port_id,
				      opstate,
				      admstate,
				      dwdm_lambda_base,
				      dwdm_lambda_count,
				      bandwidth,
				      protection);
	if(res != TNRC_API_ERROR_NONE) {
		vty_out (vty,
			 "Add of port in the data model failed: %s%s",
			 TNRCAPIERR2STRING(res), VTY_NEWLINE);
                TNRC_ERR("Conf: Add of port in the data model failed: %s",
                         TNRCAPIERR2STRING(res));
		return CMD_WARNING;
	}
	vty_out(vty,
		"Port with id %d correctly registered on the board%s",
		id, VTY_NEWLINE);
        TNRC_INF("Conf: Port with id 0x%x correctly registered on the board 0x%x",
                id, board_id);
	return CMD_SUCCESS;

}

DEFUN(resource,
      resource_cmd,
      "resource id WORD port-id WORD board-id WORD eqpt-id WORD type \
       (psc|l2sc|fsc|lsc|tdm) tp-flags WORD opstate (up|down) admstate \
       (enabled|disaled) state (free|booked|xconnected|busy)",
      "Create a resource simulator\n"
      "Resource simulation\n")
{
	uint32_t           id;
	port_id_t          port_id;
	board_id_t         board_id;
	eqpt_id_t          eqpt_id;
	label_type_t       type;
	int                tp_flags;
	label_t            label_id;
	opstate_t          opstate;
	admstate_t         admstate;
	label_state_t      state;
	tnrcapiErrorCode_t res;

	id       = strtoul(argv[0], NULL, 0);
	port_id  = strtoul(argv[1], NULL, 0);
	board_id = strtoul(argv[2], NULL, 10);
	eqpt_id  = strtoul(argv[3], NULL, 10);

	memset(&label_id, 0, sizeof(label_id));

	if(strcmp(argv[4],"psc") == 0) {
		label_id.type = LABEL_PSC;
		label_id.value.psc.id = id;
	}
	else if(strcmp(argv[4],"l2sc") == 0) {
		label_id.type = LABEL_L2SC;
		label_id.value.l2sc.vlanId = id;
		for(int i = 0; i < 6; i++) {
			label_id.value.l2sc.dstMac[i] = id;
		}
	}
	else if(strcmp(argv[4],"fsc") == 0) {
		label_id.type = LABEL_FSC;
		label_id.value.fsc.portId = id;
	}
	else if(strcmp(argv[4],"lsc") == 0) {
		TNRC::TNRC_AP           * t;
		TNRC::Port              * p;
		int                       bcs;
		int                       bsign, sign;
		int                       bno, no;
		wdm_link_lambdas_bitmap_t bm;
		uint32_t                  blambda;

		//Get instance
		t = TNRC_MASTER.getTNRC();
		if (t == NULL) {
			vty_out(vty,
				"There's no TNRC instance running.%s",
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		p = t->getPort(eqpt_id, board_id, port_id);
		if (p == NULL) {
			vty_out (vty, "No such Port id.%s", VTY_NEWLINE);
                        TNRC_ERR("Conf: No such Port id 0x%x", port_id);
			return CMD_WARNING;
		}
		bm = p->dwdm_lambdas_bitmap();
		blambda = bm.base_lambda_label;

		label_id.type = LABEL_LSC;

		GMPLS_LABEL_TO_DWDM_CS_S_N(blambda, bcs, bsign, bno);

		no = bno*((bsign == 1) ? -1 : 1) + id;

		if (no < 0) {
			DWDM_CS_S_N_TO_GMPLS_LABEL(bcs,
						   1,
						   -no,
						   label_id.value.lsc.wavelen);
		} else {
			DWDM_CS_S_N_TO_GMPLS_LABEL(bcs,
						   0,
						   no,
						   label_id.value.lsc.wavelen);
		}
	}
	else if(strcmp(argv[4],"tdm") == 0) {
		label_id.type = LABEL_TDM;
		label_id.value.tdm.s = id;
		label_id.value.tdm.u = id;
		label_id.value.tdm.k = id;
		label_id.value.tdm.l = id;
		label_id.value.tdm.m = id;
	}
	else {
		label_id.type = LABEL_UNSPECIFIED;
	}

	tp_flags = strtoul(argv[5], NULL, 10);

	if(strcmp(argv[6],"up") == 0) {
		opstate = UP;
	}
	else if(strcmp(argv[6],"down") == 0) {
		opstate = DOWN;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s) for resource%s",
			argv[6], VTY_NEWLINE);
                TNRC_ERR("Conf: Invalid opstate (%s) for resource",
                        argv[6]);
		return CMD_WARNING;
	}
	if(strcmp(argv[7],"enabled") == 0) {
		admstate = ENABLED;
	}
	else if(strcmp(argv[7],"disabled") == 0) {
		admstate = DISABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s) for port%s",
			argv[7], VTY_NEWLINE);
                TNRC_ERR("Conf: Invalid admstate (%s) for port",
                        argv[7]);
		return CMD_WARNING;
	}

	if(strcmp(argv[8],"free") == 0) {
		state = LABEL_FREE;
	}
	else if(strcmp(argv[8],"booked") == 0) {
		state = LABEL_BOOKED;
	}
	else if(strcmp(argv[8],"xconnected") == 0) {
		state = LABEL_XCONNECTED;
	}
	else if(strcmp(argv[8],"busy") == 0) {
		state = LABEL_BUSY;
	}
	else {
		state = LABEL_UNDEFINED;
	}

	//call TNRC_CONF_API function to create a new Resource in the data model
	res = TNRC_CONF_API::add_Resource(eqpt_id,
					  board_id,
					  port_id,
					  tp_flags,
					  label_id,
					  opstate,
					  admstate,
					  state);
	if(res != TNRC_API_ERROR_NONE) {
		vty_out (vty,
			 "Add of resource in the data model failed: %s%s",
			 TNRCAPIERR2STRING(res), VTY_NEWLINE);
                TNRC_ERR("Conf: Add of resource in the data model failed: %s",
                         TNRCAPIERR2STRING(res));
		return CMD_WARNING;
	}
	vty_out(vty,
		"Resource with id %d correctly registered on the port%s",
		id, VTY_NEWLINE);
        TNRC_INF("Conf: Resource with id 0x%x correctly registered on the port 0x%x",
                id, port_id);
	return CMD_SUCCESS;

}

DEFUN(notification_eqpt_sync_lost,
      notification_eqpt_sync_lost_cmd,
      "notification eqpt WORD sync-lost",
      "Simulate a notification of synchronization loss with equipment\n"
      "Notification simulation\n")
{
	TNRC::TNRC_AP  * t;
	Plugin         * p;
	Psimulator     * psim;
	eqpt_id_t        eqpt_id;
	tnrcsp_event_t * events;
	label_t          labelid;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	p = TNRC_MASTER.getPC()->getPlugin(SIMULATOR_STRING);
	if (p == NULL) {
		vty_out(vty,
			"There's no Simulator Equipment Plugin.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	psim = static_cast<Psimulator*>(p);

	eqpt_id = strtoul(argv[0], NULL, 10);

	if (t->getEqpt(eqpt_id) == NULL) {
		vty_out (vty, "No such Eqpt id.%s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	memset (&labelid, 0, sizeof(labelid));
	events = new tnrcsp_event_t;

	events->event_list  = list_new();
	events->portid      = TNRC_CONF_API::get_tnrc_port_id(eqpt_id, 0, 0);
	events->labelid     = labelid;
	events->oper_state  = UP;
	events->admin_state = ENABLED;
	events->events      = EVENT_EQPT_SYNC_LOST;

	//call the register async function for simulator plugin
	(psim->tnrcsp_register_async_cb) (events);

	//delete structures
	delete events;

	vty_out(vty,
		"Notification for equipment selected executed correctly%s",
		VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(notification_eqpt,
      notification_eqpt_cmd,
      "notification eqpt WORD opstate (up|down|down2up) \
       admstate (en|dis|dis2en)",
      "Simulate a notification on an equipment\n"
      "Notification simulation\n")
{
	TNRC::TNRC_AP  * t;
	Plugin         * p;
	Psimulator     * psim;
	eqpt_id_t        eqpt_id;
	tnrcsp_event_t * events;
	label_t          labelid;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if(t == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	p = TNRC_MASTER.getPC()->getPlugin(SIMULATOR_STRING);
	if(p == NULL) {
		vty_out(vty,
			"There's no Simulator Equipment Plugin.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	psim = static_cast<Psimulator*>(p);
	eqpt_id = strtoul(argv[0], NULL, 10);

	if (t->getEqpt(eqpt_id) == NULL) {
		vty_out (vty, "No such Eqpt id.%s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	memset (&labelid, 0, sizeof(labelid));
	events = new tnrcsp_event_t;

	events->event_list = list_new();
	events->portid     = TNRC_CONF_API::get_tnrc_port_id(eqpt_id, 0, 0);
	events->labelid    = labelid;

	if(strcmp(argv[1],"up") == 0) {
		events->oper_state = UP;
	}
	else if(strcmp(argv[1],"down") == 0) {
		events->oper_state = DOWN;
		events->events = EVENT_EQPT_OPSTATE_DOWN;
	}
	else if(strcmp(argv[1],"down2up") == 0) {
		events->oper_state = UP;
		events->events = EVENT_EQPT_OPSTATE_UP;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s)%s",
			argv[1], VTY_NEWLINE);
		delete events;
		return CMD_WARNING;
	}

	if(strcmp(argv[2],"en") == 0) {
		events->admin_state = ENABLED;
	}
	else if(strcmp(argv[2],"dis") == 0) {
		events->admin_state = DISABLED;
		events->events = EVENT_EQPT_ADMSTATE_DISABLED;
	}
	else if(strcmp(argv[2],"dis2en") == 0) {
		events->admin_state = ENABLED;
		events->events = EVENT_EQPT_ADMSTATE_ENABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s)%s",
			argv[2], VTY_NEWLINE);
		delete events;
		return CMD_WARNING;
	}

	//call the register async function for simulator plugin
	(psim->tnrcsp_register_async_cb) (events);

	//delete structures
	delete events;

	vty_out(vty,
		"Notification for equipment selected executed correctly%s",
		VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(notification_board,
      notification_board_cmd,
      "notification board WORD eqpt-id WORD opstate (up|down|down2up) \
       admstate (en|dis|dis2en)",
      "Simulate a notification on a board\n"
      "Notification simulation\n")
{
	TNRC::TNRC_AP  * t;
	Plugin         * p;
	Psimulator     * psim;
	board_id_t       board_id;
	eqpt_id_t        eqpt_id;
	tnrcsp_event_t * events;
	label_t          labelid;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	p = TNRC_MASTER.getPC()->getPlugin(SIMULATOR_STRING);
	if (p == NULL) {
		vty_out(vty,
			"There's no Simulator Equipment Plugin.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	psim = static_cast<Psimulator*>(p);

	board_id = strtoul(argv[0], NULL, 10);
	eqpt_id  = strtoul(argv[1], NULL, 10);

	if(t->getBoard(eqpt_id, board_id) == NULL) {
		vty_out (vty, "No such Board id.%s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	memset (&labelid, 0, sizeof(labelid));
	events = new tnrcsp_event_t;

	events->event_list = list_new();
	events->portid     = TNRC_CONF_API::get_tnrc_port_id(eqpt_id,
							     board_id,
							     0);
	events->labelid    = labelid;

	if(strcmp(argv[2],"up") == 0) {
		events->oper_state = UP;
	}
	else if(strcmp(argv[2],"down") == 0) {
		events->oper_state = DOWN;
		events->events = EVENT_BOARD_OPSTATE_DOWN;
	}
	else if(strcmp(argv[2],"down2up") == 0) {
		events->oper_state = UP;
		events->events = EVENT_BOARD_OPSTATE_UP;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s)%s",
			argv[2], VTY_NEWLINE);
		delete events;
		return CMD_WARNING;
	}

	if(strcmp(argv[3],"en") == 0) {
		events->admin_state = ENABLED;
	}
	else if(strcmp(argv[3],"dis") == 0) {
		events->admin_state = DISABLED;
		events->events = EVENT_BOARD_ADMSTATE_DISABLED;
	}
	else if(strcmp(argv[3],"dis2en") == 0) {
		events->admin_state = ENABLED;
		events->events = EVENT_BOARD_ADMSTATE_ENABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s)%s",
			argv[3], VTY_NEWLINE);
		delete events;
		return CMD_WARNING;
	}

	//call the register async function for simulator plugin
	(psim->tnrcsp_register_async_cb) (events);

	//delete structures
	delete events;

	vty_out(vty,
		"Notification for board selected executed correctly%s",
		VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(notification_port,
      notification_port_cmd,
      "notification port WORD board-id WORD eqpt-id WORD \
       opstate (up|down|down2up) admstate (en|dis|dis2en)",
      "Simulate a notification on a port\n"
      "Notification simulation\n")
{
	TNRC::TNRC_AP  * t;
	Plugin         * p;
	Psimulator     * psim;
	port_id_t        port_id;
	board_id_t       board_id;
	eqpt_id_t        eqpt_id;
	tnrcsp_event_t * events;
	label_t          labelid;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	p = TNRC_MASTER.getPC()->getPlugin(SIMULATOR_STRING);
	if (p == NULL) {
		vty_out(vty,
			"There's no Simulator Equipment Plugin.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	psim = static_cast<Psimulator*>(p);

	port_id  = strtoul(argv[0], NULL, 0);
	board_id = strtoul(argv[1], NULL, 10);
	eqpt_id  = strtoul(argv[2], NULL, 10);

	if (t->getPort(eqpt_id, board_id, port_id) == NULL) {
		vty_out (vty, "No such Port id.%s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	memset (&labelid, 0, sizeof(labelid));
	events = new tnrcsp_event_t;

	events->event_list = list_new();
	events->portid     = TNRC_CONF_API::get_tnrc_port_id(eqpt_id,
							     board_id,
							     port_id);
	events->labelid    = labelid;

	if(strcmp(argv[3],"up") == 0) {
		events->oper_state = UP;
	}
	else if(strcmp(argv[3],"down") == 0) {
		events->oper_state = DOWN;
		events->events = EVENT_PORT_OPSTATE_DOWN;
	}
	else if(strcmp(argv[3],"down2up") == 0) {
		events->oper_state = UP;
		events->events = EVENT_PORT_OPSTATE_UP;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s)%s",
			argv[3], VTY_NEWLINE);
		delete events;
		return CMD_WARNING;
	}

	if(strcmp(argv[4],"en") == 0) {
		events->admin_state = ENABLED;
	}
	else if(strcmp(argv[4],"dis") == 0) {
		events->admin_state = DISABLED;
		events->events      = EVENT_PORT_ADMSTATE_DISABLED;
	}
	else if(strcmp(argv[4],"dis2en") == 0) {
		events->admin_state = ENABLED;
		events->events      = EVENT_PORT_ADMSTATE_ENABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s)%s",
			argv[4], VTY_NEWLINE);
		delete events;
		return CMD_WARNING;
	}

	//call the register async function for simulator plugin
	(psim->tnrcsp_register_async_cb) (events);

	//delete structures
	delete events;

	vty_out(vty,
		"Notification for port selected executed correctly%s",
		VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(notification_resource,
      notification_resource_cmd,
      "notification resource WORD port-id WORD board-id WORD eqpt-id WORD \
       opstate (up|down|down2up) admstate (en|dis|dis2en)",
      "Simulate a notification on a resource\n"
      "Notification simulation\n")
{
	TNRC::TNRC_AP  * t;
	Plugin         * pl;
	Psimulator     * psim;
	TNRC::Port     * p;
	uint32_t         id;
	port_id_t        port_id;
	board_id_t       board_id;
	eqpt_id_t        eqpt_id;
	tnrcsp_event_t * events;
	label_t          labelid;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if(t == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	pl = TNRC_MASTER.getPC()->getPlugin(SIMULATOR_STRING);
	if(p == NULL) {
		vty_out(vty,
			"There's no Simulator Equipment Plugin.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	psim = static_cast<Psimulator*>(pl);

	id       = strtoul(argv[0], NULL, 0);
	port_id  = strtoul(argv[1], NULL, 0);
	board_id = strtoul(argv[2], NULL, 10);
	eqpt_id  = strtoul(argv[3], NULL, 10);

	p = t->getPort(eqpt_id, board_id, port_id);
	if(p == NULL) {
		vty_out (vty, "No such Port id.%s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	memset (&labelid, 0, sizeof(labelid));
	events = new tnrcsp_event_t;

	events->event_list = list_new();
	events->portid     = TNRC_CONF_API::get_tnrc_port_id(eqpt_id,
							     board_id,
							     port_id);
	switch (p->board()->sw_cap()) {
		case SWCAP_FSC:
			labelid.type             = LABEL_FSC;
			labelid.value.fsc.portId = id;
			events->labelid          = labelid;
			break;
		case SWCAP_LSC:
			labelid.type              = LABEL_LSC;
			labelid.value.lsc.wavelen = id;
			events->labelid           = labelid;
			break;
		default:
			delete events;
			vty_out (vty, "Simulator doesn't support this switch "
				 "cap%s", VTY_NEWLINE);
			return CMD_WARNING;
			break;
	}
	if(strcmp(argv[4],"up") == 0) {
		events->oper_state = UP;
	}
	else if(strcmp(argv[4],"down") == 0) {
		events->oper_state = DOWN;
		events->events     = EVENT_RESOURCE_OPSTATE_DOWN;
	}
	else if(strcmp(argv[4],"down2up") == 0) {
		events->oper_state = UP;
		events->events     = EVENT_RESOURCE_OPSTATE_UP;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s)%s",
			argv[4], VTY_NEWLINE);
		delete events;
		return CMD_WARNING;
	}

	if(strcmp(argv[5],"en") == 0) {
		events->admin_state = ENABLED;
	}
	else if(strcmp(argv[5],"dis") == 0) {
		events->admin_state = DISABLED;
		events->events      = EVENT_RESOURCE_ADMSTATE_DISABLED;
	}
	else if(strcmp(argv[5],"dis2en") == 0) {
		events->admin_state = ENABLED;
		events->events      = EVENT_RESOURCE_ADMSTATE_ENABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s)%s",
			argv[5], VTY_NEWLINE);
		delete events;
		return CMD_WARNING;
	}

	//call the register async function for simulator plugin
	(psim->tnrcsp_register_async_cb) (events);

	//delete structures
	delete events;

	vty_out(vty,
		"Notification for resource selected executed correctly%s",
		VTY_NEWLINE);

	return CMD_SUCCESS;
}


DEFUN(xc_notification_eqpt,
      xc_notification_eqpt_cmd,
      "notification xc WORD eqpt (in|out) opstate (up|down|down2up) \
       admstate (en|dis|dis2en)",
      "Simulate an XC notification from equipment\n"
      "Notification simulation\n")
{
	TNRC::TNRC_AP            * t;
	Plugin                   * p;
	Psimulator               * psim;
	int                        id;
	tnrcsp_sim_xc_t          * xc;
	tnrcsp_resource_id_t     * resource_id;
	tnrcsp_resource_detail_t * resource_det;
	label_t                    labelid;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if(t == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	p = TNRC_MASTER.getPC()->getPlugin(SIMULATOR_STRING);
	if(p == NULL) {
		vty_out(vty,
			"There's no Simulator Equipment Plugin.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	psim = static_cast<Psimulator*>(p);

	id = strtoul(argv[0], NULL, 0);

	xc = psim->getXC(id);
	if(xc == NULL) {
		vty_out(vty,
			"There's no such XC.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	memset (&labelid, 0, sizeof(labelid));
	//create new "tnrcsp_resource_id_t" structure
	resource_id = new tnrcsp_resource_id_t;
	resource_id->resource_list = list_new();
	//create new "tnrcsp_resource_detail_t" structure
	resource_det = new tnrcsp_resource_detail_t;
	resource_det->labelid = labelid;

	if(strcmp(argv[1],"in") == 0) {
		resource_id->portid = TNRC_CONF_API::
			get_tnrc_port_id(xc->eqpt_id_in,
					 xc->board_id_in,
					 xc->port_id_in);
	}
	else if(strcmp(argv[1],"out") == 0) {
		resource_id->portid = TNRC_CONF_API::
			get_tnrc_port_id(xc->eqpt_id_out,
					 xc->board_id_out,
					 xc->port_id_out);
	}
	else {
		vty_out(vty,
			"Invalid equipment type (%s). Possible values are: "
		        "'in', 'out'%s",
			argv[1], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	if(strcmp(argv[2],"up") == 0) {
		resource_det->oper_state = UP;
	}
	else if(strcmp(argv[2],"down") == 0) {
		resource_det->oper_state = DOWN;
		resource_det->last_event = EVENT_EQPT_OPSTATE_DOWN;
	}
	else if(strcmp(argv[2],"down2up") == 0) {
		resource_det->oper_state = UP;
		resource_det->last_event = EVENT_EQPT_OPSTATE_UP;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s)%s",
			argv[2], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	if(strcmp(argv[3],"en") == 0) {
		resource_det->admin_state = ENABLED;
	}
	else if(strcmp(argv[3],"dis") == 0) {
		resource_det->admin_state = DISABLED;
		resource_det->last_event = EVENT_EQPT_ADMSTATE_DISABLED;
	}
	else if(strcmp(argv[3],"dis2en") == 0) {
		resource_det->admin_state = ENABLED;
		resource_det->last_event = EVENT_EQPT_ADMSTATE_ENABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s)%s",
			argv[3], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	listnode_add(resource_id->resource_list, resource_det);
	//call the callback
	(xc->async_cb) (0, &resource_id, xc->async_ctxt);

	//delete structures
	delete resource_det;
	delete resource_id;

	vty_out(vty,
		"Notification for XC %d executed correctly%s",
		id, VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(xc_notification_board,
      xc_notification_board_cmd,
      "notification xc WORD board (in|out) opstate (up|down|down2up) \
       admstate (en|dis|dis2en)",
      "Simulate an XC notification from equipment\n"
      "Notification simulation\n")
{
	TNRC::TNRC_AP            * t;
	Plugin                   * p;
	Psimulator               * psim;
	int                        id;
	tnrcsp_sim_xc_t          * xc;
	tnrcsp_resource_id_t     * resource_id;
	tnrcsp_resource_detail_t * resource_det;
	label_t                    labelid;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if(t == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	p = TNRC_MASTER.getPC()->getPlugin(SIMULATOR_STRING);
	if(p == NULL) {
		vty_out(vty,
			"There's no Simulator Equipment Plugin.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	psim = static_cast<Psimulator*>(p);

	id = strtoul(argv[0], NULL, 0);

	xc = psim->getXC(id);
	if(xc == NULL) {
		vty_out(vty,
			"There's no such XC.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	memset (&labelid, 0, sizeof(labelid));
	//create new "tnrcsp_resource_id_t" structure
	resource_id = new tnrcsp_resource_id_t;
	resource_id->resource_list = list_new();
	//create new "tnrcsp_resource_detail_t" structure
	resource_det = new tnrcsp_resource_detail_t;
	resource_det->labelid = labelid;

	if(strcmp(argv[1],"in") == 0) {
		resource_id->portid = TNRC_CONF_API::get_tnrc_port_id(xc->eqpt_id_in,
								      xc->board_id_in,
								      xc->port_id_in);
	}
	else if(strcmp(argv[1],"out") == 0) {
		resource_id->portid = TNRC_CONF_API::get_tnrc_port_id(xc->eqpt_id_out,
								      xc->board_id_out,
								      xc->port_id_out);
	}
	else {
		vty_out(vty,
			"Invalid board type (%s). Possible values are: "
			"'in', 'out'%s",
			argv[1], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	if(strcmp(argv[2],"up") == 0) {
		resource_det->oper_state = UP;
	}
	else if(strcmp(argv[2],"down") == 0) {
		resource_det->oper_state = DOWN;
		resource_det->last_event = EVENT_BOARD_OPSTATE_DOWN;
	}
	else if(strcmp(argv[2],"down2up") == 0) {
		resource_det->oper_state = UP;
		resource_det->last_event = EVENT_BOARD_OPSTATE_UP;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s)%s",
			argv[2], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	if(strcmp(argv[3],"en") == 0) {
		resource_det->admin_state = ENABLED;
	}
	else if(strcmp(argv[3],"dis") == 0) {
		resource_det->admin_state = DISABLED;
		resource_det->last_event  = EVENT_BOARD_ADMSTATE_DISABLED;
	}
	else if(strcmp(argv[3],"dis2en") == 0) {
		resource_det->admin_state = ENABLED;
		resource_det->last_event  = EVENT_BOARD_ADMSTATE_ENABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s)%s",
			argv[3], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	listnode_add(resource_id->resource_list, resource_det);
	//call the callback
	(xc->async_cb) (0, &resource_id, xc->async_ctxt);

	//delete structures
	delete resource_det;
	delete resource_id;

	vty_out(vty,
		"Notification for XC %d executed correctly%s",
		id, VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(xc_notification_port,
      xc_notification_port_cmd,
      "notification xc WORD port (in|out) opstate (up|down|down2up) \
       admstate (en|dis|dis2en)",
      "Simulate an XC notification from equipment\n"
      "Notification simulation\n")
{
	TNRC::TNRC_AP            * t;
	Plugin                   * p;
	Psimulator               * psim;
	int                        id;
	tnrcsp_sim_xc_t          * xc;
	tnrcsp_resource_id_t     * resource_id;
	tnrcsp_resource_detail_t * resource_det;
	label_t                    labelid;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if(t == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	p = TNRC_MASTER.getPC()->getPlugin(SIMULATOR_STRING);
	if(p == NULL) {
		vty_out(vty,
			"There's no Simulator Equipment Plugin.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	psim = static_cast<Psimulator*>(p);

	id = strtoul(argv[0], NULL, 0);

	xc = psim->getXC(id);
	if (xc == NULL) {
		vty_out(vty,
			"There's no such XC.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	memset (&labelid, 0, sizeof(labelid));
	//create new "tnrcsp_resource_id_t" structure
	resource_id = new tnrcsp_resource_id_t;
	resource_id->resource_list = list_new();
	//create new "tnrcsp_resource_detail_t" structure
	resource_det = new tnrcsp_resource_detail_t;
	resource_det->labelid = labelid;

	if (strcmp(argv[1],"in") == 0) {
		resource_id->portid = TNRC_CONF_API::get_tnrc_port_id(xc->eqpt_id_in,
								      xc->board_id_in,
								      xc->port_id_in);
	}
	else if(strcmp(argv[1],"out") == 0) {
		resource_id->portid = TNRC_CONF_API::get_tnrc_port_id(xc->eqpt_id_out,
								      xc->board_id_out,
								      xc->port_id_out);
	}
	else {
		vty_out(vty,
			"Invalid port type (%s). Possible values are: "
			"'in', 'out'%s",
			argv[1], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	if(strcmp(argv[2],"up") == 0) {
		resource_det->oper_state = UP;
	}
	else if(strcmp(argv[2],"down") == 0) {
		resource_det->oper_state = DOWN;
		resource_det->last_event = EVENT_PORT_OPSTATE_DOWN;
	}
	else if(strcmp(argv[2],"down2up") == 0) {
		resource_det->oper_state = UP;
		resource_det->last_event = EVENT_PORT_OPSTATE_UP;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s)%s",
			argv[2], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	if(strcmp(argv[3],"en") == 0) {
		resource_det->admin_state = ENABLED;
	}
	else if(strcmp(argv[3],"dis") == 0) {
		resource_det->admin_state = DISABLED;
		resource_det->last_event  = EVENT_PORT_ADMSTATE_DISABLED;
	}
	else if(strcmp(argv[3],"dis2en") == 0) {
		resource_det->admin_state = ENABLED;
		resource_det->last_event  = EVENT_PORT_ADMSTATE_ENABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s)%s",
			argv[3], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	listnode_add(resource_id->resource_list, resource_det);
	//call the callback
	(xc->async_cb) (0, &resource_id, xc->async_ctxt);

	//delete structures
	delete resource_det;
	delete resource_id;

	vty_out(vty,
		"Notification for XC %d executed correctly%s",
		id, VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(xc_notification_resource,
      xc_notification_resource_cmd,
      "notification xc WORD resource (in|out) opstate (up|down|down2up) \
       admstate (en|dis|dis2en)",
      "Simulate an XC notification from equipment\n"
      "Notification simulation\n")
{
	TNRC::TNRC_AP            * t;
	Plugin                   * p;
	Psimulator               * psim;
	int                        id;
	tnrcsp_sim_xc_t          * xc;
	tnrcsp_resource_id_t     * resource_id;
	tnrcsp_resource_detail_t * resource_det;
	label_t                    labelid;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	p = TNRC_MASTER.getPC()->getPlugin(SIMULATOR_STRING);
	if (p == NULL) {
		vty_out(vty,
			"There's no Simulator Equipment Plugin.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	psim = static_cast<Psimulator*>(p);

	id = strtoul(argv[0], NULL, 0);

	xc = psim->getXC(id);
	if (xc == NULL) {
		vty_out(vty,
			"There's no such XC.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	memset (&labelid, 0, sizeof(labelid));
	//create new "tnrcsp_resource_id_t" structure
	resource_id = new tnrcsp_resource_id_t;
	resource_id->resource_list = list_new();
	//create new "tnrcsp_resource_detail_t" structure
	resource_det = new tnrcsp_resource_detail_t;

	if(strcmp(argv[1],"in") == 0) {
		resource_id->portid = TNRC_CONF_API::get_tnrc_port_id(xc->eqpt_id_in,
								      xc->board_id_in,
								      xc->port_id_in);
		resource_det->labelid = xc->labelid_in;
	}
	else if(strcmp(argv[1],"out") == 0) {
		resource_id->portid = TNRC_CONF_API::get_tnrc_port_id(xc->eqpt_id_out,
								      xc->board_id_out,
								      xc->port_id_out);
		resource_det->labelid = xc->labelid_out;
	}
	else {
		vty_out(vty,
			"Invalid port type (%s). Possible values are: "
			"'in', 'out'%s",
			argv[1], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	if(strcmp(argv[2],"up") == 0) {
		resource_det->oper_state = UP;
	}
	else if(strcmp(argv[2],"down") == 0) {
		resource_det->oper_state = DOWN;
		resource_det->last_event = EVENT_RESOURCE_OPSTATE_DOWN;
	}
	else if(strcmp(argv[2],"down2up") == 0) {
		resource_det->oper_state = UP;
		resource_det->last_event = EVENT_RESOURCE_OPSTATE_UP;
	}
	else {
		vty_out(vty,
			"Invalid opstate (%s)%s",
			argv[2], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	if(strcmp(argv[3],"en") == 0) {
		resource_det->admin_state = ENABLED;
	}
	else if(strcmp(argv[3],"dis") == 0) {
		resource_det->admin_state = DISABLED;
		resource_det->last_event = EVENT_RESOURCE_ADMSTATE_DISABLED;
	}
	else if(strcmp(argv[3],"dis2en") == 0) {
		resource_det->admin_state = ENABLED;
		resource_det->last_event = EVENT_RESOURCE_ADMSTATE_ENABLED;
	}
	else {
		vty_out(vty,
			"Invalid admstate (%s)%s",
			argv[3], VTY_NEWLINE);
		delete resource_id;
		delete resource_det;
		return CMD_WARNING;
	}

	listnode_add(resource_id->resource_list, resource_det);
	//call the callback
	(xc->async_cb) (0, &resource_id, xc->async_ctxt);

	//delete structures
	delete resource_det;
	delete resource_id;

	vty_out(vty,
		"Notification for XC %d executed correctly%s",
		id, VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(make_xc,
      make_xc_cmd,
      "make-xc lab-in WORD port-in WORD board-in WORD eqpt-in WORD lab-out \
       WORD port-out WORD board-out WORD eqpt-out WORD dir (unidir|bidir) \
       virtual (yes|no) activate (yes|no) rsrv-cookie WORD",
      "Simulate a Make XC request from a client\n")
{
	tnrcap_cookie_t    cookiep;
	tnrcap_cookie_t    rsrv_cookie;
	uint32_t           l_id_in;
	uint32_t           l_id_out;
	port_id_t          port_id_in;
	board_id_t         board_id_in;
	eqpt_id_t          eqpt_id_in;
	port_id_t          port_id_out;
	board_id_t         board_id_out;
	eqpt_id_t          eqpt_id_out;
	tnrc_port_id_t     tnrc_p_id_in;
	tnrc_port_id_t     tnrc_p_id_out;
	g2mpls_addr_t      dl_id_in;
	g2mpls_addr_t      dl_id_out;
	label_t            lab_id_in;
	label_t            lab_id_out;
	xcdirection_t      dir;
	tnrc_boolean_t     activate;
	tnrc_boolean_t     is_virtual;
	tnrcapiErrorCode_t res;
	TNRC::Board        *b_in;
	TNRC::Board        *b_out;

	// in parameters
	l_id_in     = strtoul(argv[0], NULL, 0);
	port_id_in  = strtoul(argv[1], NULL, 0);
	board_id_in = strtoul(argv[2], NULL, 10);
	eqpt_id_in  = strtoul(argv[3], NULL, 10);
	// out parameters
	l_id_out     = strtoul(argv[4], NULL, 0);
	port_id_out  = strtoul(argv[5], NULL, 0);
	board_id_out = strtoul(argv[6], NULL, 10);
	eqpt_id_out  = strtoul(argv[7], NULL, 10);

	//set parameters to call api function in the rigth way
	tnrc_p_id_in  = TNRC_CONF_API::get_tnrc_port_id(eqpt_id_in,
							board_id_in,
							port_id_in);
	dl_id_in      = TNRC_CONF_API::get_dl_id(tnrc_p_id_in);
	tnrc_p_id_out = TNRC_CONF_API::get_tnrc_port_id(eqpt_id_out,
							board_id_out,
							port_id_out);
	dl_id_out     = TNRC_CONF_API::get_dl_id(tnrc_p_id_out);

	memset(&lab_id_in, 0, sizeof(lab_id_in));
	memset(&lab_id_out, 0, sizeof(lab_id_out));

	b_in  = TNRC_MASTER.getTNRC()->getBoard(eqpt_id_in, board_id_in);
	b_out = TNRC_MASTER.getTNRC()->getBoard(eqpt_id_out, board_id_out);
	if((b_in != NULL) && (b_out != NULL)) {
		switch (b_in->sw_cap()) {
			case SWCAP_FSC:
				lab_id_in.type              = LABEL_FSC;
				lab_id_in.value.fsc.portId  = l_id_in;
				break;
			case SWCAP_LSC:
				lab_id_in.type              = LABEL_LSC;
				lab_id_in.value.lsc.wavelen = l_id_in;
				break;
			default:
				break;
		}
		switch (b_out->sw_cap()) {
			case SWCAP_FSC:
				lab_id_out.type               = LABEL_FSC;
				lab_id_out.value.fsc.portId   = l_id_out;
				break;
			case SWCAP_LSC:
				lab_id_out.type               = LABEL_LSC;
				lab_id_out.value.lsc.wavelen  = l_id_out;
				break;
			default:
				break;
		}

	}
	if(strcmp(argv[8],"unidir") == 0) {
		dir = XCDIRECTION_UNIDIR;
	}
	else if(strcmp(argv[8],"bidir") == 0) {
		dir = XCDIRECTION_BIDIR;
	}
	else {
		vty_out(vty,
			"Invalid direction type (%s). Possible values are: "
			"'unidir', 'bidir'%s",
			argv[8], VTY_NEWLINE);
		return CMD_WARNING;
	}

	if(strcmp(argv[9],"yes") == 0) {
		is_virtual = 1;
	}
	else if(strcmp(argv[9],"no") == 0) {
		is_virtual = 0;
	}

	else {
		vty_out(vty,
			"Invalid is_virtual value (%s). Possible values are: "
			"'yes', 'no'%s",
			argv[9], VTY_NEWLINE);
		return CMD_WARNING;
	}

	if(strcmp(argv[10],"yes") == 0) {
		activate = 1;
	}
	else if(strcmp(argv[10],"no") == 0) {
		activate = 0;
	}
	else {
		vty_out(vty,
			"Invalid activate value (%s). Possible values are: "
			"'yes', 'no'%s",
			argv[10], VTY_NEWLINE);
		return CMD_WARNING;
	}
	rsrv_cookie = strtoul(argv[11], NULL, 10);

	//call api function
	res = TNRC_OPEXT_API::tnrcap_make_xc(&cookiep,
					     dl_id_in,
					     lab_id_in,
					     dl_id_out,
					     lab_id_out,
					     dir,
					     is_virtual,
					     activate,
					     rsrv_cookie,
					     0,
					     0);

	TNRC_DBG("Preliminary TNRC response: %s", TNRCAPIERR2STRING(res));

	if (res != TNRC_API_ERROR_NONE) {
		vty_out(vty,
			"Preliminary TNRC response failed. Check TNRC "
			"log file%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	vty_out(vty,
		"Action in execution. Cookie value is: %d%s",
		cookiep, VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(destroy_xc,
      destroy_xc_cmd,
      "destroy-xc cookie WORD deactivate (yes|no)",
      "Simulate a Destroy XC request from a client\n")
{
	tnrcap_cookie_t    cookie;
	tnrc_boolean_t     deactivate;
	tnrcapiErrorCode_t res;

	// in parameters
	cookie = strtoul(argv[0], NULL, 10);

	if(strcmp(argv[1],"yes") == 0) {
		deactivate = 1;
	}
	else if(strcmp(argv[1],"no") == 0) {
		deactivate = 0;
	}
	else {
		vty_out(vty,
			"Invalid deactivate value (%s). Possible values are: "
			"'yes', 'no'%s",
			argv[1], VTY_NEWLINE);
		return CMD_WARNING;
	}

	//call api function
	res = TNRC_OPEXT_API::tnrcap_destroy_xc(cookie,
						deactivate,
						0);

	TNRC_DBG("Preliminary TNRC response: %s", TNRCAPIERR2STRING(res));

	if (res != TNRC_API_ERROR_NONE) {
		vty_out(vty,
			"Preliminary TNRC response failed. Check TNRC "
			"log file%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	vty_out(vty, "Action in execution.%s", VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(reserve_xc,
      reserve_xc_cmd,
      "reserve-xc lab-in WORD port-in WORD board-in WORD eqpt-in WORD \
       lab-out WORD port-out WORD board-out WORD eqpt-out WORD dir \
       (unidir|bidir) virtual (yes|no) adv-rsrv (yes|no) start WORD end WORD",
      "Simulate a Reserve XC request from a client\n")
{
	tnrcap_cookie_t    cookiep;
	uint32_t           l_id_in;
	uint32_t           l_id_out;
	port_id_t          port_id_in;
	board_id_t         board_id_in;
	eqpt_id_t          eqpt_id_in;
	port_id_t          port_id_out;
	board_id_t         board_id_out;
	eqpt_id_t          eqpt_id_out;
	tnrc_port_id_t     tnrc_p_id_in;
	tnrc_port_id_t     tnrc_p_id_out;
	g2mpls_addr_t      dl_id_in;
	g2mpls_addr_t      dl_id_out;
	label_t            lab_id_in;
	label_t            lab_id_out;
	xcdirection_t      dir;
	tnrc_boolean_t     is_virtual;
	tnrc_boolean_t     advance_reservation;
	long               start;
	long               end;
	tnrcapiErrorCode_t res;
	TNRC::Board        *b_in;
	TNRC::Board        *b_out;

	// in parameters
	l_id_in     = strtoul(argv[0], NULL, 0);
	port_id_in  = strtoul(argv[1], NULL, 0);
	board_id_in = strtoul(argv[2], NULL, 10);
	eqpt_id_in  = strtoul(argv[3], NULL, 10);
	// out parameters
	l_id_out     = strtoul(argv[4], NULL, 0);
	port_id_out  = strtoul(argv[5], NULL, 0);
	board_id_out = strtoul(argv[6], NULL, 10);
	eqpt_id_out  = strtoul(argv[7], NULL, 10);

	//set parameters to call api function in the rigth way
	tnrc_p_id_in  = TNRC_CONF_API::get_tnrc_port_id(eqpt_id_in,
							board_id_in,
							port_id_in);
	dl_id_in      = TNRC_CONF_API::get_dl_id(tnrc_p_id_in);
	tnrc_p_id_out = TNRC_CONF_API::get_tnrc_port_id(eqpt_id_out,
							board_id_out,
							port_id_out);
	dl_id_out     = TNRC_CONF_API::get_dl_id(tnrc_p_id_out);

	memset(&lab_id_in, 0, sizeof(lab_id_in));
	memset(&lab_id_out, 0, sizeof(lab_id_out));

	b_in  = TNRC_MASTER.getTNRC()->getBoard(eqpt_id_in, board_id_in);
	b_out = TNRC_MASTER.getTNRC()->getBoard(eqpt_id_out, board_id_out);
	if((b_in != NULL) && (b_out != NULL)) {
		switch (b_in->sw_cap()) {
			case SWCAP_FSC:
				lab_id_in.type              = LABEL_FSC;
				lab_id_in.value.fsc.portId  = l_id_in;
				break;
			case SWCAP_LSC:
				lab_id_in.type              = LABEL_LSC;
				lab_id_in.value.lsc.wavelen = l_id_in;
				break;
			default:
				break;
		}
		switch (b_out->sw_cap()) {
			case SWCAP_FSC:
				lab_id_out.type               = LABEL_FSC;
				lab_id_out.value.fsc.portId   = l_id_out;
				break;
			case SWCAP_LSC:
				lab_id_out.type               = LABEL_LSC;
				lab_id_out.value.lsc.wavelen  = l_id_out;
				break;
			default:
				break;
		}

	}

	if(strcmp(argv[8],"unidir") == 0) {
		dir = XCDIRECTION_UNIDIR;
	}
	else if(strcmp(argv[8],"bidir") == 0) {
		dir = XCDIRECTION_BIDIR;
	}
	else {
		vty_out(vty,
			"Invalid direction type (%s). Possible values are: "
			"'unidir', 'bidir'%s",
			argv[8], VTY_NEWLINE);
		return CMD_WARNING;
	}

	if(strcmp(argv[9],"yes") == 0) {
		is_virtual = 1;
	}
	else if(strcmp(argv[9],"no") == 0) {
		is_virtual = 0;
	}

	else {
		vty_out(vty,
			"Invalid is_virtual value (%s). Possible values are: "
			"'yes', 'no'%s",
			argv[9], VTY_NEWLINE);
		return CMD_WARNING;
	}

	if(strcmp(argv[10],"yes") == 0) {
		advance_reservation = 1;
	}
	else if(strcmp(argv[10],"no") == 0) {
		advance_reservation = 0;
	}
	else {
		vty_out(vty,
			"Invalid adv-rsrv (%s). Possible values "
			"are: 'yes', 'no'%s",
			argv[10], VTY_NEWLINE);
		return CMD_WARNING;
	}

	start = strtoul(argv[11], NULL, 10);
	end   = strtoul(argv[12], NULL, 10);

	//call api function
	res = TNRC_OPEXT_API::tnrcap_reserve_xc (&cookiep,
						 dl_id_in,
						 lab_id_in,
						 dl_id_out,
						 lab_id_out,
						 dir,
						 is_virtual,
						 advance_reservation,
						 start,
						 end,
						 0);

	TNRC_DBG ("Preliminary TNRC response: %s", TNRCAPIERR2STRING(res));

	if (res != TNRC_API_ERROR_NONE) {
		vty_out(vty,
			"Preliminary TNRC response failed. Check TNRC "
			"log file%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	vty_out(vty,
		"Action in execution. Cookie value is: %d%s",
		cookiep, VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(unreserve_xc,
      unreserve_xc_cmd,
      "unreserve-xc cookie WORD",
      "Simulate a Unreserve XC request from a client\n")
{
	tnrcap_cookie_t    cookie;
	tnrcapiErrorCode_t res;

	// in parameters
	cookie = strtoul(argv[0], NULL, 10);

	//call api function
	res = TNRC_OPEXT_API::tnrcap_unreserve_xc(cookie,
						  0);

	TNRC_DBG("Preliminary TNRC response: %s", TNRCAPIERR2STRING(res));

	if (res != TNRC_API_ERROR_NONE) {
		vty_out(vty,
			"Preliminary TNRC response failed. Check TNRC "
			"log file%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	vty_out(vty, "Action in execution.%s", VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(load_xc,
      load_xc_cmd,
      "load-xc lab-in WORD port-in WORD board-in WORD eqpt-in WORD lab-out \
       WORD port-out WORD board-out WORD eqpt-out WORD status (active|booked)",
      "Load an XC in Simulator Plugin\n")
{
	uint32_t           l_id_in;
	uint32_t           l_id_out;
	port_id_t          port_id_in;
	board_id_t         board_id_in;
	eqpt_id_t          eqpt_id_in;
	port_id_t          port_id_out;
	board_id_t         board_id_out;
	eqpt_id_t          eqpt_id_out;
	label_t            lab_id_in;
	label_t            lab_id_out;
	TNRC::Board        *b_in;
	TNRC::Board        *b_out;
	Plugin             *p;
	Psimulator         *psim;
	xc_status_t        xc_status;
	tnrcsp_sim_xc_t    *xc;

	TNRC_DBG("Loading an XC in Simulator Plugin ...");

	p = TNRC_MASTER.getPC()->getPlugin(SIMULATOR_STRING);
	if (p == NULL) {
		vty_out(vty,
			"There's no Simulator Equipment Plugin.%s",
			VTY_NEWLINE);
		TNRC_ERR("There's no Simulator Equipment Plugin.");
		return CMD_WARNING;
	}
	psim = static_cast<Psimulator*>(p);

	// in parameters
	l_id_in     = strtoul(argv[0], NULL, 0);
	port_id_in  = strtoul(argv[1], NULL, 0);
	board_id_in = strtoul(argv[2], NULL, 0);
	eqpt_id_in  = strtoul(argv[3], NULL, 0);
	// out parameters
	l_id_out     = strtoul(argv[4], NULL, 0);
	port_id_out  = strtoul(argv[5], NULL, 0);
	board_id_out = strtoul(argv[6], NULL, 0);
	eqpt_id_out  = strtoul(argv[7], NULL, 0);

	memset(&lab_id_in, 0, sizeof(lab_id_in));
	memset(&lab_id_out, 0, sizeof(lab_id_out));

	b_in  = TNRC_MASTER.getTNRC()->getBoard(eqpt_id_in, board_id_in);
	b_out = TNRC_MASTER.getTNRC()->getBoard(eqpt_id_out, board_id_out);
	if((b_in != NULL) && (b_out != NULL)) {
		switch (b_in->sw_cap()) {
			case SWCAP_FSC:
				lab_id_in.type              = LABEL_FSC;
				lab_id_in.value.fsc.portId  = l_id_in;
				break;
			case SWCAP_LSC:
				lab_id_in.type              = LABEL_LSC;
				lab_id_in.value.lsc.wavelen = l_id_in;
				break;
			default:
				break;
		}
		switch (b_out->sw_cap()) {
			case SWCAP_FSC:
				lab_id_out.type               = LABEL_FSC;
				lab_id_out.value.fsc.portId   = l_id_out;
				break;
			case SWCAP_LSC:
				lab_id_out.type               = LABEL_LSC;
				lab_id_out.value.lsc.wavelen  = l_id_out;
				break;
			default:
				break;
		}

	}
	if(strcmp(argv[8],"active") == 0) {
		xc_status = XC_ACTIVE;
	}
	else if(strcmp(argv[8],"booked") == 0) {
		xc_status = XC_BOOKED;
	}
	else {
		vty_out(vty,
			"Invalid xc status (%s). Possible values are: "
			"'active', 'booked'%s",
			argv[8], VTY_NEWLINE);
		TNRC_ERR("Invalid xc status (%s). Possible values are: "
			"'active', 'booked'", argv[8]);
		return CMD_WARNING;
	}

	xc = new_tnrcsp_sim_xc(psim->new_xc_id(),
			       xc_status,
			       eqpt_id_in,
			       board_id_in,
			       port_id_in,
			       lab_id_in,
			       eqpt_id_out,
			       board_id_out,
			       port_id_out,
			       lab_id_out,
			       0,
			       0);
	//attach XC in the list
	psim->attach_xc(xc);

	vty_out(vty,
		"XC with id %d attached in Simulator Plugin XC list%s",
		xc->id, VTY_NEWLINE);
	TNRC_DBG("XC with id %d attached in Simulator Plugin XC list",
		 xc->id);

	return CMD_SUCCESS;
}

DEFUN(tnrc_instance_show,
      tnrc_instance_show_cmd,
      "show tnrc instance",
      "Show instance details\n")
{
	TNRC::TNRC_AP                 * t;
	TNRC::Eqpt                    * e;
	TNRC::TNRC_AP::iterator_eqpts   iter;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	vty_out (vty, "%s", VTY_NEWLINE);
	vty_out (vty, "\tTNRC instance id: %d.%s",
		 t->instance_id(), VTY_NEWLINE);
	vty_out (vty, "\tNumber of equipments: %d.%s",
		 t->n_eqpts(), VTY_NEWLINE);
	vty_out (vty, "%s", VTY_NEWLINE);
	for (iter = t->begin_eqpts(); iter != t->end_eqpts(); iter++) {
		e = (*iter).second;
		vty_out (vty, "\t\tequipment %d: %s ", e->eqpt_id(),
			 SHOW_EQPT_TYPE(e->type()));
		vty_out (vty, "(%s synchronized )",
			 ((t->eqpt_link_down()) ? " not" : ""));
		vty_out (vty, "%s", VTY_NEWLINE);
	}

	return CMD_SUCCESS;
}

static void
tnrc_eqpt_show_sub (struct vty *vty, TNRC::Eqpt *e)
{
	TNRC::Eqpt::iterator_boards iter;

	vty_out(vty, "Equipment     : id = %d%s", e->eqpt_id(), VTY_NEWLINE);
	vty_out(vty, "\tAddress     : %s%s", addr_ntoa(e->address()),
		VTY_NEWLINE);
	vty_out(vty, "\tType        : %s%s", SHOW_EQPT_TYPE(e->type()),
		VTY_NEWLINE);
	vty_out(vty, "\tOpstate     : %s%s", SHOW_OPSTATE(e->opstate()),
		VTY_NEWLINE);
	vty_out(vty, "\tAdmstate    : %s%s", SHOW_ADMSTATE(e->admstate()),
		VTY_NEWLINE);
	vty_out(vty, "\tLocation    : %s%s", e->location(), VTY_NEWLINE);
	vty_out(vty, "\tN. of boards: %d. Id's: ", e->n_boards());

	for(iter = e->begin_boards(); iter != e->end_boards(); iter++){
		vty_out(vty, "%d , ", (*iter).first);
	}
	vty_out(vty, "%s", VTY_NEWLINE);
	vty_out(vty, "%s", VTY_NEWLINE);
}

DEFUN(tnrc_eqpt_show,
      tnrc_eqpt_show_cmd,
      "show tnrc eqpt [ID]",
      "Show equipment details\n")
{
	TNRC::TNRC_AP                 * tnrc_ap;
	TNRC::Eqpt                    * e;
	eqpt_id_t                       id;
	TNRC::TNRC_AP::iterator_eqpts   iter;

	//Get instance
	tnrc_ap = TNRC_MASTER.getTNRC();
	if (tnrc_ap == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	// Show All Eqpts
	if (argc == 0) {
		vty_out (vty, "Number of Equipment: %d.%s", tnrc_ap->n_eqpts(),
			 VTY_NEWLINE);
		for(iter = tnrc_ap->begin_eqpts(); iter != tnrc_ap->end_eqpts(); iter++){
			tnrc_eqpt_show_sub (vty, (*iter).second);
		}
	}
	// Eqpt ID is specified
	else {
		id = strtoul (argv[0], NULL, 10);
		e = tnrc_ap->getEqpt (id);
		if (e == NULL) {
			vty_out (vty, "No such Equipment id.%s", VTY_NEWLINE);
		}
		else {
			tnrc_eqpt_show_sub (vty, e);
		}
	}
	return CMD_SUCCESS;
}

static void
tnrc_board_show_sub (struct vty *vty, TNRC::Board *b)
{
	TNRC::Board::iterator_ports iter;

	vty_out(vty, "Board : id = %d (eqpt id = %d)%s", b->board_id(),
		b->eqpt()->eqpt_id(), VTY_NEWLINE);
	vty_out(vty, "\tSw. cap    : %s%s", SHOW_SW_CAP(b->sw_cap()),
		VTY_NEWLINE);
	vty_out(vty, "\tOpstate    : %s%s", SHOW_OPSTATE(b->opstate()),
		VTY_NEWLINE);
	vty_out(vty, "\tAdmstate   : %s%s", SHOW_ADMSTATE(b->admstate()),
		VTY_NEWLINE);
	vty_out(vty, "\tN. of ports: %d. Id's: ", b->n_ports());

	for(iter = b->begin_ports(); iter != b->end_ports(); iter++){
		vty_out(vty, "%d , ", (*iter).first);
	}
	vty_out(vty, "%s", VTY_NEWLINE);
	vty_out(vty, "%s", VTY_NEWLINE);
}

DEFUN(tnrc_board_show,
      tnrc_board_show_cmd,
      "show tnrc board WORD eqpt-id WORD",
      "Show board details\n")
{
	TNRC::TNRC_AP * tnrc_ap;
	TNRC::Eqpt    * e;
	TNRC::Board   * b;
	eqpt_id_t       eqpt_id;
	board_id_t      board_id;

	//Get instance
	tnrc_ap = TNRC_MASTER.getTNRC();
	if (tnrc_ap == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	board_id = strtoul (argv[0], NULL, 10);
	eqpt_id  = strtoul (argv[1], NULL, 10);
	// Board ID is specified
	b = tnrc_ap->getBoard (eqpt_id, board_id);
	if (b == NULL) {
		vty_out (vty, "No such Board id.%s", VTY_NEWLINE);
	}
	else {
		tnrc_board_show_sub (vty, b);
	}

	return CMD_SUCCESS;
}

DEFUN(tnrc_all_board_show,
      tnrc_all_board_show_cmd,
      "show tnrc board eqpt-id WORD",
      "Show board details\n")
{
	TNRC::TNRC_AP               * tnrc_ap;
	TNRC::Eqpt                  * e;
	TNRC::Board                 * b;
	eqpt_id_t                     eqpt_id;
	board_id_t                    id;
	TNRC::Eqpt::iterator_boards   iter;

	//Get instance
	tnrc_ap = TNRC_MASTER.getTNRC();
	if (tnrc_ap == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	eqpt_id = strtoul (argv[0], NULL, 10);
	//Get right equipment
	e = tnrc_ap->getEqpt(eqpt_id);
	if (e == NULL) {
		vty_out(vty,
			"Equipment with id %d does not exist%s",
			eqpt_id, VTY_NEWLINE);
		return CMD_WARNING;
	}
	// Show All Boards
	vty_out (vty, "Number of Boards: %d.%s", e->n_boards(), VTY_NEWLINE);
	for(iter = e->begin_boards(); iter != e->end_boards(); iter++){
		tnrc_board_show_sub (vty, (*iter).second);
	}

	return CMD_SUCCESS;
}

static void
tnrc_port_show_sub(struct vty *vty, TNRC::Port *p)
{
	TNRC::Port::iterator_resources iter;
	label_t                        labelid;
	uint32_t                       bw;

	vty_out(vty, "Port : id = 0x%x (board id = %d, eqpt id = %d)%s",
		p->port_id(), p->board()->board_id(),
		p->board()->eqpt()->eqpt_id(), VTY_NEWLINE);

	vty_out(vty, "\tFlags          : 0x%x%s", p->port_flags(), VTY_NEWLINE);
	vty_out(vty, "\tRemote Eqpt    : %s%s",
		addr_ntoa(p->remote_eqpt_address()), VTY_NEWLINE);
	vty_out(vty, "\tRemote Port    : 0x%x%s", p->remote_port_id(),
		VTY_NEWLINE);
	vty_out(vty, "\tOpstate        : %s%s", SHOW_OPSTATE(p->opstate()),
		VTY_NEWLINE);
	vty_out(vty, "\tAdmstate       : %s%s", SHOW_ADMSTATE(p->admstate()),
		VTY_NEWLINE);
        vty_out(vty, "\tProtection     : %s%s", SHOW_GMPLS_PROTTYPE(p->prot_type()),
	        VTY_NEWLINE);
		
	bw = p->max_bw();
	vty_out(vty, "\tMax Bw         : %05.03f Mbps%s",
		BW_HEX2MBPS(bw), VTY_NEWLINE);
	bw = p->max_res_bw();
	vty_out(vty, "\tMax Res Bw     : %05.03f Mbps%s",
		BW_HEX2MBPS(bw), VTY_NEWLINE);
	bw = p->min_lsp_bw();
	vty_out(vty, "\tMin LSP Res Bw : %05.03f Mbps %s",
		BW_HEX2MBPS(bw), VTY_NEWLINE);

	vty_out(vty, "\tMax LSP Res Bw   prio. 0 : %05.03f Mbps %s",
		BW_HEX2MBPS(p->max_lsp_bw().bw[0]), VTY_NEWLINE);
	for(int j = 1; j <= (GMPLS_BW_PRIORITIES - 1); j++) {
		vty_out(vty, "\t                 prio. %d : %05.03f Mbps %s",
			j, BW_HEX2MBPS(p->max_lsp_bw().bw[j]),
			VTY_NEWLINE);
	}

	vty_out(vty, "\tAvailable Bw     prio. 0 : %05.03f Mbps %s",
		BW_HEX2MBPS(p->unres_bw().bw[0]), VTY_NEWLINE);
	for(int i = 1; i <= (GMPLS_BW_PRIORITIES - 1); i++) {
		vty_out(vty, "\t                 prio. %d : %05.03f Mbps %s",
			i, BW_HEX2MBPS(p->unres_bw().bw[i]),
			VTY_NEWLINE);
	}

	vty_out(vty, "\tN. of resources: %d.%s", p->n_resources(), VTY_NEWLINE);
	if(p->board()->sw_cap() == SWCAP_LSC) {
		wdm_link_lambdas_bitmap_t bm;
		uint32_t lbl;

		bm = p->dwdm_lambdas_bitmap();

		lbl = bm.base_lambda_label;

		vty_out(vty, "\tLambda bitmap:%s",
			VTY_NEWLINE);
		vty_out(vty, "\t\t- Base Lambda    : 0x%08x",
			lbl);
		if (GMPLS_LABEL_TO_WDM_GRID(lbl) == WDM_GRID_ITUT_DWDM) {
			float    lambda;
			GMPLS_LABEL_TO_DWDM_LAMBDA(lbl, lambda);

			vty_out(vty, " (%.2f Thz)", lambda);
		} else if (GMPLS_LABEL_TO_WDM_GRID(lbl) == WDM_GRID_ITUT_CWDM){
			uint32_t wave;
			GMPLS_LABEL_TO_CWDM_WAVELENGTH(lbl, wave);

			vty_out(vty, " (%d nm)", wave);
		}
		vty_out(vty, "%s", VTY_NEWLINE);

		vty_out(vty, "\t\t- Num of Lambdas : %d%s",
			bm.num_wavelengths, VTY_NEWLINE);
		vty_out(vty, "\t\t- Lambdas Bitmap : 0x");
		size_t i;
		for(i = 0; i < bm.bitmap_size; i++) {
			vty_out(vty, "%.8x | ",
				*(bm.bitmap_word + i));
		}
		vty_out(vty, "%s", VTY_NEWLINE);
	}
	vty_out(vty, "%s", VTY_NEWLINE);
}

DEFUN(tnrc_port_show,
      tnrc_port_show_cmd,
      "show tnrc port WORD board-id WORD eqpt-id WORD",
      "Show equipment details\n")
{
	TNRC::TNRC_AP * tnrc_ap;
	TNRC::Eqpt    * e;
	TNRC::Board   * b;
	TNRC::Port    * p;
	eqpt_id_t       eqpt_id;
	board_id_t      board_id;
	port_id_t       port_id;

	//Get instance
	tnrc_ap = TNRC_MASTER.getTNRC();
	if (tnrc_ap == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	port_id  = strtoul (argv[0], NULL, 0);
	board_id = strtoul (argv[1], NULL, 10);
	eqpt_id  = strtoul (argv[2], NULL, 10);
	// Port ID is specified
	p = tnrc_ap->getPort(eqpt_id, board_id, port_id);
	if (p == NULL) {
		vty_out (vty, "No such Port id.%s", VTY_NEWLINE);
	}
	else {
		tnrc_port_show_sub (vty, p);
	}

	return CMD_SUCCESS;
}

DEFUN(tnrc_all_port_show,
      tnrc_all_port_show_cmd,
      "show tnrc port board-id WORD eqpt-id WORD",
      "Show equipment details\n")
{
	TNRC::TNRC_AP * tnrc_ap;
	TNRC::Eqpt    * e;
	TNRC::Board   * b;
	TNRC::Port    * p;
	eqpt_id_t       eqpt_id;
	board_id_t      board_id;
	TNRC::Board::iterator_ports iter;

	//Get instance
	tnrc_ap = TNRC_MASTER.getTNRC();
	if (tnrc_ap == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	board_id = strtoul (argv[0], NULL, 10);
	eqpt_id  = strtoul (argv[1], NULL, 10);
	//Get right Board
	b = tnrc_ap->getBoard(eqpt_id, board_id);
	if (b == NULL) {
		vty_out(vty,
			"Board with id %d does not exist on Equipment %d%s",
			board_id, eqpt_id, VTY_NEWLINE);
		return CMD_WARNING;
	}

	// Show all Ports
	vty_out (vty, "Number of Ports: %d.%s", b->n_ports(), VTY_NEWLINE);
	for(iter = b->begin_ports(); iter != b->end_ports(); iter++){
		tnrc_port_show_sub (vty, (*iter).second);
	}

	return CMD_SUCCESS;
}

static void
tnrc_resource_show_sub (struct vty *vty, TNRC::Resource *r)
{
	label_t                               id;
	TNRC::Resource::iterator_reservations it;
	tm *                                  curTime;
	const char *                          timeStringFormat = "%a, %d %b %Y %H:%M:%S %zGMT";
	const int                             timeStringLength = 40;
	char                                  timeString[timeStringLength];

	id = r->label_id();

	vty_out(vty, "\t%s%s", label2string(id).c_str(), VTY_NEWLINE);

	vty_out(vty, "\tTP Flags             : %d%s", r->tp_flags(), VTY_NEWLINE);
	vty_out(vty, "\tOpstate              : %s%s", SHOW_OPSTATE(r->opstate()),
		VTY_NEWLINE);
	vty_out(vty, "\tAdmstate             : %s%s", SHOW_ADMSTATE(r->admstate()),
		VTY_NEWLINE);
	vty_out(vty, "\tState                : %s%s", SHOW_LABEL_STATE(r->state()),
		VTY_NEWLINE);
	vty_out(vty, "%s", VTY_NEWLINE);
	vty_out(vty, "\tNum of Advance Rsrv  : %d%s", r->n_reservations(),
		VTY_NEWLINE);
	vty_out(vty, "\t\t------------------------------------------%s", VTY_NEWLINE);
	for (it = r->begin_reservations(); it != r->end_reservations(); it++) {
		curTime = localtime((time_t *) &(*it).first);
		strftime(timeString, timeStringLength, timeStringFormat, curTime);
		vty_out(vty, "\t\tstart : %s", timeString);
		vty_out(vty, "%s", VTY_NEWLINE);
		curTime = localtime((time_t *) &(*it).second);
		strftime(timeString, timeStringLength, timeStringFormat, curTime);
		vty_out(vty, "\t\tend   : %s", timeString);
		vty_out(vty, "%s", VTY_NEWLINE);
		vty_out(vty, "\t\t------------------------------------------%s", VTY_NEWLINE);
	}

	vty_out(vty, "%s", VTY_NEWLINE);
}

DEFUN(tnrc_resource_show,
      tnrc_resource_show_cmd,
      "show tnrc resource port-id WORD board-id WORD eqpt-id WORD",
      "Show equipment details\n")
{
	TNRC::TNRC_AP  * tnrc_ap;
	TNRC::Eqpt     * e;
	TNRC::Board    * b;
	TNRC::Port     * p;
	TNRC::Resource * r;
	eqpt_id_t        eqpt_id;
	board_id_t       board_id;
	port_id_t        port_id;
	int              i = 1;
	TNRC::Port::iterator_resources iter;

	//Get instance
	tnrc_ap = TNRC_MASTER.getTNRC();
	if (tnrc_ap == NULL) {
		vty_out(vty,
			"There's no TNRC instance running.%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}
	port_id  = strtoul (argv[0], NULL, 0);
	board_id = strtoul (argv[1], NULL, 10);
	eqpt_id  = strtoul (argv[2], NULL, 10);
	//Get right Port
	p = tnrc_ap->getPort(eqpt_id, board_id, port_id);
	if (p == NULL) {
		vty_out(vty,
			"Port with id %d does not exist on Board %d, "
			"on Equipment %d%s",
			port_id, board_id, eqpt_id, VTY_NEWLINE);
		return CMD_WARNING;
	}
	// Show all Resources
	vty_out (vty, "Number of Resources: %d.%s%s", p->n_resources(),
		 VTY_NEWLINE, VTY_NEWLINE);
	for(iter = p->begin_resources(); iter != p->end_resources(); iter++) {
		vty_out (vty, "Resource %d :%s", i, VTY_NEWLINE);
		tnrc_resource_show_sub (vty, (*iter).second);
		i++;
	}
	return CMD_SUCCESS;
}

static void
tnrc_xc_show_sub (struct vty *vty, XC *xc)
{
	eqpt_id_t     e_id_in;
	eqpt_id_t     e_id_out;
	board_id_t    b_id_in;
	board_id_t    b_id_out;
	port_id_t     p_id_in;
	port_id_t     p_id_out;
	label_t       labelid_in;
	label_t       labelid_out;

	//get eqpt, board and port id
	TNRC_CONF_API::convert_tnrc_port_id(&e_id_in, &b_id_in,
					    &p_id_in, xc->portid_in());
	TNRC_CONF_API::convert_tnrc_port_id(&e_id_out, &b_id_out,
					    &p_id_out, xc->portid_out());

	labelid_in  = xc->labelid_in();
	labelid_out = xc->labelid_out();

	vty_out(vty, "XC : id = %d%s", xc->id(), VTY_NEWLINE);
	vty_out(vty, "\tCookie            : %d%s%s", xc->cookie(),
		VTY_NEWLINE, VTY_NEWLINE);
	vty_out(vty, "\tState             : %s%s",
		SHOW_XC_STATE(xc->state()), VTY_NEWLINE);
	vty_out(vty, "\tDirection         : %s%s%s",
		SHOW_XCDIRECTION(xc->direction()), VTY_NEWLINE, VTY_NEWLINE);
	vty_out(vty, "\tData Link IN      : 0x%x", xc->portid_in());
	vty_out(vty, "\t(%d-%d-0x%x)%s", e_id_in, b_id_in, p_id_in, VTY_NEWLINE);
	vty_out(vty, "\tLabel     IN      : %s%s%s",
		label2string(labelid_in).c_str(), VTY_NEWLINE, VTY_NEWLINE);
	vty_out(vty, "\tData Link OUT     : 0x%x", xc->portid_out());
	vty_out(vty, "\t(%d-%d-0x%x)%s", e_id_out, b_id_out, p_id_out,
		VTY_NEWLINE);
	vty_out(vty, "\tLabel     OUT     : %s%s%s",
		label2string(labelid_out).c_str(), VTY_NEWLINE, VTY_NEWLINE);

	vty_out(vty, "%s", VTY_NEWLINE);
}

DEFUN(tnrc_xc_show,
      tnrc_xc_show_cmd,
      "show tnrc xc [WORD]",
      "Show XC's details\n")
{
	XC    * xc;
	u_int   id;
	TNRC_Master::iterator_xcs  iter;

	// Show All XCs
	if (argc == 0) {
		vty_out (vty, "Number of XCs: %d.%s", TNRC_MASTER.n_xcs(),
			 VTY_NEWLINE);
		for(iter = TNRC_MASTER.begin_xcs();
		    iter != TNRC_MASTER.end_xcs();
		    iter++){
			tnrc_xc_show_sub (vty, (*iter).second);
		}
	}
	// XC ID is specified
	else {
		id = strtoul (argv[0], NULL, 0);
		xc = TNRC_MASTER.getXC (id);
		if (xc == NULL) {
			vty_out (vty, "No such XC id.%s", VTY_NEWLINE);
		}
		else {
			tnrc_xc_show_sub (vty, xc);
		}
	}
	return CMD_SUCCESS;
}

DEFUN(tnrc,
      tnrc_cmd,
      "tnrc",
      "Enable a TNRC instance\n"
      "Start TNRC configuration\n")
{
	std::string        err;
	tnrcapiErrorCode_t ret;

	vty->node  = TNRC_NODE;
	vty->index = NULL;

	ret = TNRC_CONF_API::tnrcStart(err);

	if (ret == TNRC_API_ERROR_INSTANCE_ALREADY_EXISTS) {
		vty_out(vty,
			"Got existing TNRC instance%s",
			VTY_NEWLINE);
		return CMD_SUCCESS;
	}

	if (ret != TNRC_API_ERROR_NONE) {
		vty_out(vty,
			"Cannot start TNRC instance "
			"(err: code=%s, reason='%s')%s",
			TNRCAPIERR2STRING(ret), err.c_str(),
			VTY_NEWLINE);
		return CMD_WARNING;
	}


	vty_out(vty,
		"TNRC instance started !%s",
		VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(no_tnrc,
      no_tnrc_cmd,
      "no tnrc",
      "Stop/disable a TNRC instance\n")
{
	std::string        err;
	tnrcapiErrorCode_t ret;

	vty->node  = TNRC_NODE;
	vty->index = NULL;

	ret = TNRC_CONF_API::tnrcStop(err);

	if (ret != TNRC_API_ERROR_NONE) {
		vty_out(vty,
			"Cannot stop G2.PCE-RA instance "
			"(err: code=%s, reason='%s')%s",
			TNRCAPIERR2STRING(ret), err.c_str(),
			VTY_NEWLINE);
		return CMD_SUCCESS;
	}

	vty_out(vty,
		"TNRC instance stopped !%s",
		VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(get_eqpt_details,
      get_eqpt_details_cmd,
      "get-eqpt-details",
      "Get details of equipment\n")
{
	g2mpls_addr_t      addr;
	eqpt_type_t        type;
	tnrcapiErrorCode_t ret;

	bzero(&addr, sizeof(g2mpls_addr_t));
	bzero(&type, sizeof(eqpt_type_t));

	ret = TNRC_OPEXT_API::tnrcap_get_eqpt_details(addr, type);
	if (ret != TNRC_API_ERROR_NONE) {
		vty_out(vty, "Error: %s%s", TNRCAPIERR2STRING(ret), VTY_NEWLINE);
		return CMD_SUCCESS;
	}
	vty_out(vty, "Equipment address : %s%s", addr_ntoa(addr), VTY_NEWLINE);
	vty_out(vty, "Equipment type    : %s%s", SHOW_EQPT_TYPE(type),
		VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(get_dl_list,
      get_dl_list_cmd,
      "get-dl-list",
      "Get list of data link id\n")
{
	std::list<g2mpls_addr_t>           dl_list;
	std::list<g2mpls_addr_t>::iterator iter;
	g2mpls_addr_t                      dl_id;
	tnrc_port_id_t                     tnrc_port_id;
	eqpt_id_t                          e_id;
	board_id_t                         b_id;
	port_id_t                          p_id;
	tnrcapiErrorCode_t                 ret;
	int                                i = 1;

	ret = TNRC_OPEXT_API::tnrcap_get_dl_list(dl_list);
	if (ret != TNRC_API_ERROR_NONE) {
		vty_out(vty, "Error: %s%s", TNRCAPIERR2STRING(ret),
			VTY_NEWLINE);
		return CMD_SUCCESS;
	}

	vty_out(vty, "Number of Data link: %d%s", dl_list.size(), VTY_NEWLINE);

	for(iter = dl_list.begin(); iter != dl_list.end(); iter++) {
		TNRC_CONF_API::convert_dl_id(&tnrc_port_id, *iter);
		vty_out(vty, "Data link id : \t0x%x", tnrc_port_id);
		TNRC_CONF_API::convert_tnrc_port_id(&e_id,
						    &b_id,
						    &p_id,
						    tnrc_port_id);
		vty_out(vty,
			"\t--->\teqpt id %d ; board id %d ; port id %d%s",
			e_id, b_id, p_id, VTY_NEWLINE);
		i++;
	}

	return CMD_SUCCESS;
}

DEFUN(get_dl_details,
      get_dl_details_cmd,
      "get-dl-details dl-id WORD",
      "Get details of data link\n")
{
	tnrc_api_dl_details_t details;
	tnrc_port_id_t        tnrc_port_id;
	g2mpls_addr_t         dl_id;
	tnrcapiErrorCode_t    ret;

	bzero(&details, sizeof(tnrc_api_dl_details_t));

	tnrc_port_id = strtoul(argv[0], NULL, 16);

	dl_id = TNRC_CONF_API::get_dl_id(tnrc_port_id);

	ret = TNRC_OPEXT_API::tnrcap_get_dl_details(dl_id, details);
	if (ret != TNRC_API_ERROR_NONE) {
		vty_out(vty, "Error: %s%s", TNRCAPIERR2STRING(ret), VTY_NEWLINE);
		return CMD_SUCCESS;
	}

	vty_out(vty, "Details of data link with id: 0x%x%s",
		tnrc_port_id, VTY_NEWLINE);

	vty_out(vty, "\tEqpt           : %s%s",
		addr_ntoa(details.eqpt_addr), VTY_NEWLINE);
	vty_out(vty, "\tRemote Eqpt    : %s%s",
		addr_ntoa(details.rem_eqpt_addr), VTY_NEWLINE);
	vty_out(vty, "\tRemote Port    : %d%s", details.rem_port_id,
		VTY_NEWLINE);
	vty_out(vty, "\tOpstate        : %s%s", SHOW_OPSTATE(details.opstate),
		VTY_NEWLINE);
	vty_out(vty, "\tSwitch. cap.   : %s%s", SHOW_SW_CAP(details.sw_cap),
		VTY_NEWLINE);
	vty_out(vty, "\tEnc. type      : %s%s",
		SHOW_ENC_TYPE(details.enc_type), VTY_NEWLINE);

	vty_out(vty, "\tMax Bw         : %05.03f Mbps %s",
		BW_HEX2MBPS(details.tot_bw), VTY_NEWLINE);

	vty_out(vty, "\tMax Res Bw     : %05.03f Mbps %s",
		BW_HEX2MBPS(details.max_res_bw), VTY_NEWLINE);

	vty_out(vty, "\tMin LSP Res Bw : %05.03f Mbps %s",
		BW_HEX2MBPS(details.min_lsp_bw), VTY_NEWLINE);

	vty_out(vty, "\tMax LSP Res Bw   prio. 0 : %05.03f Mbps %s",
		BW_HEX2MBPS(details.max_lsp_bw.bw[0]), VTY_NEWLINE);
	for(int j = 1; j <= (GMPLS_BW_PRIORITIES - 1); j++) {
		vty_out(vty, "\t                 prio. %d : %05.03f Mbps %s",
			j, BW_HEX2MBPS(details.max_lsp_bw.bw[j]),
			VTY_NEWLINE);
	}

	vty_out(vty, "\tAvailable Bw     prio. 0 : %05.03f Mbps %s",
		BW_HEX2MBPS(details.unres_bw.bw[0]), VTY_NEWLINE);
	for(int i = 1; i <= (GMPLS_BW_PRIORITIES - 1); i++) {
		vty_out(vty, "\t                 prio. %d : %05.03f Mbps %s",
			i, BW_HEX2MBPS(details.unres_bw.bw[i]),
			VTY_NEWLINE);
	}

	vty_out(vty, "\tProt. type     : %s%s",
		SHOW_GMPLS_PROTTYPE(details.prot_type), VTY_NEWLINE);

	if(details.sw_cap == SWCAP_LSC) {
		uint32_t lbl;

		lbl = details.bitmap.base_lambda_label;

		vty_out(vty, "\tLambda bitmap:%s",
			VTY_NEWLINE);
		vty_out(vty, "\t\t- Base Lambda    : 0x%08x",
			lbl);
		if (GMPLS_LABEL_TO_WDM_GRID(lbl) == WDM_GRID_ITUT_DWDM) {
			float    lambda;
			GMPLS_LABEL_TO_DWDM_LAMBDA(lbl, lambda);

			vty_out(vty, " (%.2f Thz)", lambda);
		} else if (GMPLS_LABEL_TO_WDM_GRID(lbl) == WDM_GRID_ITUT_CWDM){
			uint32_t wave;
			GMPLS_LABEL_TO_CWDM_WAVELENGTH(lbl, wave);

			vty_out(vty, " (%d nm)", wave);
		}
		vty_out(vty, "%s", VTY_NEWLINE);

		vty_out(vty, "\t\t- Num of Lambdas : %d%s",
			details.bitmap.num_wavelengths, VTY_NEWLINE);
		vty_out(vty, "\t\t- Lambdas Bitmap : 0x");
		size_t i;
		for(i = 0; i < details.bitmap.bitmap_size; i++) {
			vty_out(vty, "%.8x | ",
				*(details.bitmap.bitmap_word + i));
		}
		vty_out(vty, "%s", VTY_NEWLINE);
	}
	return CMD_SUCCESS;
}

DEFUN(get_dl_calendar,
      get_dl_calendar_cmd,
      "get-dl-calendar dl-id WORD num-events WORD quantum WORD",
      "Get advance reservations calendar of data link\n")
{
	std::map<long, bw_per_prio_t>                 calendar;
	std::map<long, bw_per_prio_t>::iterator       iter;
	tnrc_port_id_t                                tnrc_port_id;
	uint32_t                                      num_events;
	uint32_t                                      quantum;
	g2mpls_addr_t                                 dl_id;
	tnrcapiErrorCode_t                            ret;
	struct timeval                                current_time;
	tm *                                          curTime;
	const char *                                  timeStringFormat = "%a, %d %b %Y %H:%M:%S %zGMT";
	const int                                     timeStringLength = 40;
	char                                          timeString[timeStringLength];

	gettimeofday(&current_time, NULL);

	tnrc_port_id = strtoul(argv[0], NULL, 16);
	num_events   = strtoul(argv[1], NULL, 10);
	quantum      = strtoul(argv[2], NULL, 10);

	dl_id = TNRC_CONF_API::get_dl_id(tnrc_port_id);

	ret = TNRC_OPEXT_API::tnrcap_get_dl_calendar(dl_id,
						     current_time.tv_sec,
						     num_events,
						     quantum,
						     calendar);
	if (ret != TNRC_API_ERROR_NONE) {
		vty_out(vty, "Error: %s. ", TNRCAPIERR2STRING(ret));
		switch (ret) {
			case  TNRC_API_ERROR_GENERIC:
				vty_out(vty, "No transitions for specified "
					"data link");
				break;
			default:
				break;
		}
		vty_out(vty, "%s", VTY_NEWLINE);
		return CMD_SUCCESS;
	}

	if (!calendar.size()) {
		vty_out(vty, "No events for specified data link%s",
			VTY_NEWLINE);
		return CMD_SUCCESS;
	}

	vty_out(vty, "Number of events: %d%s",
		calendar.size(), VTY_NEWLINE);

	for (iter = calendar.begin(); iter != calendar.end(); iter++) {
		curTime = localtime((time_t *) &(*iter).first);
		strftime(timeString, timeStringLength, timeStringFormat, curTime);
		vty_out(vty, "%s", VTY_NEWLINE);
		vty_out(vty, "%s", timeString);
		vty_out(vty, "%s", VTY_NEWLINE);
		vty_out(vty, "Available Bandwidth prio 0: %05.03f Mbps %s",
			BW_HEX2MBPS((*iter).second.bw[0]), VTY_NEWLINE);
		vty_out(vty, "-------------------------------------------------%s",
			VTY_NEWLINE);
	}

	return CMD_SUCCESS;
}

DEFUN(get_label_list,
      get_label_list_cmd,
      "get-label-list dl-id WORD",
      "Get list of label for a data link\n")
{
	tnrc_port_id_t     tnrc_port_id;
	g2mpls_addr_t      dl_id;
	eqpt_id_t          e_id;
	board_id_t         b_id;
	port_id_t          p_id;
	tnrcapiErrorCode_t ret;
	int                i = 1;
	std::list<tnrc_api_label_t>           label_list;
	std::list<tnrc_api_label_t>::iterator iter;
	tnrc_api_label_t                      label_details;

	bzero(&label_details, sizeof(tnrc_api_label_t));

	tnrc_port_id = strtoul(argv[0], NULL, 16);

	dl_id = TNRC_CONF_API::get_dl_id(tnrc_port_id);

	ret = TNRC_OPEXT_API::tnrcap_get_label_list(dl_id, label_list);
	if (ret != TNRC_API_ERROR_NONE) {
		vty_out(vty, "Error: %s%s", TNRCAPIERR2STRING(ret),
			VTY_NEWLINE);
		return CMD_SUCCESS;
	}

	vty_out(vty, "N. Label: %d%s", label_list.size(), VTY_NEWLINE);
	vty_out(vty, "%s", VTY_NEWLINE);

	for(iter = label_list.begin(); iter != label_list.end(); iter++) {
		label_details = *iter;
		vty_out(vty, "Label %d%s", i, VTY_NEWLINE);
		vty_out(vty, "\t%s%s",
			label2string(label_details.label_id).c_str(),
			VTY_NEWLINE);
		vty_out(vty, "\tOpstate \t: %s%s",
			SHOW_OPSTATE(label_details.opstate), VTY_NEWLINE);
		vty_out(vty, "\tState   \t: %s%s",
			SHOW_LABEL_STATE(label_details.state), VTY_NEWLINE);
		i++;
	}

	return CMD_SUCCESS;
}

DEFUN(get_label_details,
      get_label_details_cmd,
      "get-label-details dl-id WORD label-type (psc|l2sc|fsc|lsc|tdm|raw) \
       label-id WORD",
      "Get list of label for a data link\n")
{
	tnrc_port_id_t     tnrc_port_id;
	g2mpls_addr_t      dl_id;
	label_t            label_id;
	uint32_t           id;
	label_state_t      label_state;
	eqpt_id_t          e_id;
	board_id_t         b_id;
	port_id_t          p_id;
	tnrcapiErrorCode_t ret;
	opstate_t          opstate;
	admstate_t         admstate;

	memset(&label_id, 0, sizeof(label_t));

	tnrc_port_id = strtoul(argv[0], NULL, 16);

	dl_id =  TNRC_CONF_API::get_dl_id(tnrc_port_id);

	id    = strtoul(argv[2], NULL, 0);

	if(strcmp(argv[1],"psc") == 0) {
		label_id.type = LABEL_PSC;
		label_id.value.psc.id = id;
	}
	else if(strcmp(argv[1],"l2sc") == 0) {
		label_id.type = LABEL_L2SC;
		label_id.value.l2sc.vlanId = id;
		for(int i = 0; i < 6; i++) {
			label_id.value.l2sc.dstMac[i] = id;
		}
	}
	else if(strcmp(argv[1],"fsc") == 0) {
		label_id.type = LABEL_FSC;
		label_id.value.fsc.portId = id;
	}
	else if(strcmp(argv[1],"lsc") == 0) {
		label_id.type = LABEL_LSC;
		label_id.value.lsc.wavelen = id;
	}
	else if(strcmp(argv[1],"tdm") == 0) {
		label_id.type = LABEL_TDM;
		label_id.value.tdm.s = id;
		label_id.value.tdm.u = id;
		label_id.value.tdm.k = id;
		label_id.value.tdm.l = id;
		label_id.value.tdm.m = id;
	}
	else if(strcmp(argv[1],"raw") == 0) {
		label_id.type = LABEL_UNSPECIFIED;
		label_id.value.rawId = (uint64_t) id;
	}
	else {
		vty_out(vty, "label type (%s)%s",
			argv[1], VTY_NEWLINE);
		return CMD_WARNING;
	}

	ret = TNRC_OPEXT_API::tnrcap_get_label_details(dl_id,
						       label_id,
						       opstate,
						       label_state);
	if (ret != TNRC_API_ERROR_NONE) {
		vty_out(vty, "Error: %s%s", TNRCAPIERR2STRING(ret),
			VTY_NEWLINE);
		return CMD_SUCCESS;
	}

	vty_out(vty, "Label details: %s", VTY_NEWLINE);
	vty_out(vty, "%s", VTY_NEWLINE);

	vty_out(vty, "\tOpstate : %s%s",
		SHOW_OPSTATE(opstate), VTY_NEWLINE);
	vty_out(vty, "\tState   : %s%s",
		SHOW_LABEL_STATE(label_state), VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(test_mode,
      test_mode_cmd,
      "testmode location WORD",
      "Enable test mode\n")
{
	std::string location;

	location = argv[0];

	//Set test mode flag
	TNRC_MASTER.test_mode(true);
	//Set test conf file location
	TNRC_MASTER.test_file(location);

	return CMD_SUCCESS;
}

DEFUN(cls_port_configure,
      cls_port_configure_cmd,
      "cls set port PORT_ID board BOARD flags FLAGS rem-eq-addr A.B.C.D rem-portid REM_PORT_ID wavelengths WAVELENGHTHS max-bandwidth MAX_BW protection (none|extra|unprotected|shared|1to1|1plus1|enhanced)",
      "CzechLight Plugin command\n"
      "set port parameters\n"
      "set fixed port parameters\n"
      "PORT ID for example 0x0109\n"
      "choose port's board\n"
      "device ID, i.e. 4\n"
      "set flags\n"
      "FLAG uint32\n"
      "set remote address\n"
      "IPv4 format\n"
      "set remote port id\n"
      "REMOTE PORT ID for example 0x110D\n"
      "set number of wavelength\n"
      "Int32\n"
      "set maximum bandwidth of a single lambda\n"
      "Int32\n"
      "set protection\n"
      "no protection\n"
      "extra protection\n"
      "unprotected\n"
      "shared\n"
      "ltol\n"
      "lplus 1\n"
      "enhanced\n")
{
  TNRC_SP_TELNET::PluginClsFSC *p;
  p = static_cast<TNRC_SP_TELNET::PluginClsFSC *> (TNRC_MASTER.getPC()->getPlugin(CLS_STRING));

  if (p == NULL)
  {
    vty_out (vty, "Plugin %s not found %s", FOUNDRY_L2SC_STRING, VTY_NEWLINE);
    return CMD_WARNING;
  }
  port_id_t         port           = strtoul(argv[0], NULL, 0);
  int               board          = strtoul(argv[1], NULL, 10);
  int               flags          = strtoul(argv[2], NULL, 10);
  g2mpls_addr_t     rem_eq_addr;
  memset(&rem_eq_addr, 0, sizeof(rem_eq_addr));
  rem_eq_addr.type = IPv4;
  rem_eq_addr.preflen = 32;
  if (!inet_aton(argv[3], &(rem_eq_addr.value.ipv4)))
  {
    vty_out(vty, "Invalid IP address (%s) for remote equipment address%s",
      argv[3], VTY_NEWLINE);
    return CMD_WARNING;
  }
  port_id_t         rem_port_id       = strtoul(argv[4], NULL, 0);
  uint32_t          no_of_wavelen    = strtoul(argv[5], NULL, 0);
  uint32_t          max_bandwidth_sl  = strtoul(argv[6], NULL, 0);

  gmpls_prottype_t  protection        =
    (strcmp(argv[7],"none")        == 0) ? PROTTYPE_NONE :
    (strcmp(argv[7],"extra")       == 0) ? PROTTYPE_EXTRA :
    (strcmp(argv[7],"unprotected") == 0) ? PROTTYPE_UNPROTECTED :
    (strcmp(argv[7],"shared")      == 0) ? PROTTYPE_SHARED :
    (strcmp(argv[7],"1to1")        == 0) ? PROTTYPE_DEDICATED_1TO1 :
    (strcmp(argv[7],"1plus1")      == 0) ? PROTTYPE_DEDICATED_1PLUS1 :
    (strcmp(argv[7],"enhanced")    == 0) ? PROTTYPE_ENHANCED : PROTTYPE_NONE;

  uint32_t key =  (0x0 |
				  ((1 & EQPTID_TO_DLID_MASK)	<< EQPTID_SHIFT) |
				  ((board & BOARDID_TO_DLID_MASK)	<< BOARDID_SHIFT) |
				  ((port & PORTID_TO_DLID_MASK)	<< 0)
				  );
  p->tnrcsp_set_fixed_port(key, flags, rem_eq_addr, rem_port_id, max_bandwidth_sl, protection, true);
  return CMD_SUCCESS;
}

DEFUN(cls_fixed_port_show,
      cls_fixed_port_show_cmd,
      "show tnrc cls fixed port",
      SHOW_STR
      "Transport Network Resource Controler information\n"
      "CzechLight plugin information\n"
      "Fixed settings\n"
      "Port settings\n")
{
  TNRC_SP_TELNET::PluginClsFSC *p;
  p = static_cast<TNRC_SP_TELNET::PluginClsFSC *> (TNRC_MASTER.getPC()->getPlugin(CLS_STRING));

  if (p == NULL)
  {
    vty_out (vty, "Plugin %s not found %s", FOUNDRY_L2SC_STRING, VTY_NEWLINE);
    return CMD_WARNING;
  }

  TNRC_SP_TELNET::tnrcsp_telnet_map_fixed_port_t::iterator it;
  for (it=p->fixed_port_map.begin(); it!=p->fixed_port_map.end(); it++)
    vty_out (vty, " board %d port ID 0x%x, rem. port ID 0x%x, rem. addr. %s, flags 0x%x, bandwidth %d, prot. type: %s %s",
    (it->first>>BOARDID_SHIFT)&BOARDID_TO_DLID_MASK, it->first&PORTID_TO_DLID_MASK, it->second.rem_port_id, inet_ntoa(it->second.rem_eq_addr.value.ipv4), it->second.flags, it->second.bandwidth, SHOW_GMPLS_PROTTYPE(it->second.protection), VTY_NEWLINE);
  return CMD_SUCCESS;
}

void TNRC_Master::init_vty (void)
{
	// Install tnrc top node
	install_node (&tnrc_node, NULL);

	install_element(CONFIG_NODE, &tnrc_cmd);
	install_element(CONFIG_NODE, &no_tnrc_cmd);

	install_default(TNRC_NODE);

	// Show commands
	install_element(VIEW_NODE,   &tnrc_instance_show_cmd);
	install_element(ENABLE_NODE, &tnrc_instance_show_cmd);
	install_element(TNRC_NODE,   &tnrc_instance_show_cmd);
	install_element(VIEW_NODE,   &tnrc_eqpt_show_cmd);
	install_element(ENABLE_NODE, &tnrc_eqpt_show_cmd);
	install_element(TNRC_NODE,   &tnrc_eqpt_show_cmd);
	install_element(VIEW_NODE,   &tnrc_board_show_cmd);
	install_element(ENABLE_NODE, &tnrc_board_show_cmd);
	install_element(TNRC_NODE,   &tnrc_board_show_cmd);
	install_element(VIEW_NODE,   &tnrc_all_board_show_cmd);
	install_element(ENABLE_NODE, &tnrc_all_board_show_cmd);
	install_element(TNRC_NODE,   &tnrc_all_board_show_cmd);
	install_element(VIEW_NODE,   &tnrc_port_show_cmd);
	install_element(ENABLE_NODE, &tnrc_port_show_cmd);
	install_element(TNRC_NODE,   &tnrc_port_show_cmd);
	install_element(VIEW_NODE,   &tnrc_all_port_show_cmd);
	install_element(ENABLE_NODE, &tnrc_all_port_show_cmd);
	install_element(TNRC_NODE,   &tnrc_all_port_show_cmd);
	install_element(VIEW_NODE,   &tnrc_resource_show_cmd);
	install_element(ENABLE_NODE, &tnrc_resource_show_cmd);
	install_element(TNRC_NODE,   &tnrc_resource_show_cmd);
	install_element(VIEW_NODE,   &tnrc_xc_show_cmd);
	install_element(ENABLE_NODE, &tnrc_xc_show_cmd);
	install_element(TNRC_NODE,   &tnrc_xc_show_cmd);
	// Operative commands
	install_element(CONFIG_NODE, &equipment_cmd);
	install_element(TNRC_NODE,   &equipment_cmd);

	install_element(TNRC_NODE, &eqpt_cmd);
	install_element(TNRC_NODE, &board_cmd);
	install_element(TNRC_NODE, &port_cmd);
	install_element(TNRC_NODE, &resource_cmd);
	install_element(TNRC_NODE, &xc_notification_eqpt_cmd);
	install_element(TNRC_NODE, &xc_notification_board_cmd);
	install_element(TNRC_NODE, &xc_notification_port_cmd);
	install_element(TNRC_NODE, &xc_notification_resource_cmd);
	install_element(TNRC_NODE, &notification_eqpt_sync_lost_cmd);
	install_element(TNRC_NODE, &notification_eqpt_cmd);
	install_element(TNRC_NODE, &notification_board_cmd);
	install_element(TNRC_NODE, &notification_port_cmd);
	install_element(TNRC_NODE, &notification_resource_cmd);
	// Test commands
	install_element(TNRC_NODE, &make_xc_cmd);
	install_element(TNRC_NODE, &destroy_xc_cmd);
	install_element(TNRC_NODE, &reserve_xc_cmd);
	install_element(TNRC_NODE, &unreserve_xc_cmd);
	install_element(TNRC_NODE, &load_xc_cmd);
	install_element(TNRC_NODE, &get_eqpt_details_cmd);
	install_element(TNRC_NODE, &get_dl_list_cmd);
	install_element(TNRC_NODE, &get_dl_details_cmd);
	install_element(TNRC_NODE, &get_dl_calendar_cmd);
	install_element(TNRC_NODE, &get_label_list_cmd);
	install_element(TNRC_NODE, &get_label_details_cmd);
	install_element(TNRC_NODE, &test_mode_cmd);

  //CzechLight Switch plugin CLI functions
  install_element(VIEW_NODE,   &cls_fixed_port_show_cmd);
  install_element(ENABLE_NODE, &cls_fixed_port_show_cmd);
  install_element(TNRC_NODE,   &cls_fixed_port_show_cmd);

  install_element(ENABLE_NODE, &cls_port_configure_cmd);
  install_element(TNRC_NODE,   &cls_port_configure_cmd);
}
