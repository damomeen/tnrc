//
//  This file is part of phosphorus-g2mpls.
//
//  Copyright (C) 2006, 2007, 2008, 2009 Nextworks s.r.l.
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
//



#include "tnrc_lrm_api.h"
#include "tnrc_apis.h"
#include "tnrc_trace.h"
#include "tnrc_config.h"
#include "tnrc_corba.h"
#include "g2mpls_corba_utils.h"
#if HAVE_OMNIORB
#include "g2mpls_types.h"
#include "idl/types.hh"
#include "idl/g2mplsTypes.hh"
#include "idl/lrm.hh"
#include <iostream>
#endif // HAVE_OMNIORB

void
update_datalink_details(TNRC::Port * p)
{
#if HAVE_OMNIORB
	assert(p);

	TNRC_DBG("Updating new DataLink details in LRM");
	try {
		g2mplsTypes::DLinkId_var                dlink;
		g2mplsTypes::DLinkParameters_var        params;
		g2mplsTypes::DLinkWdmLambdasBitmap_var  bitmap;
		g2mpls_addr_t                           dlinkAddr;
		tnrc_port_id_t                          portId;
		std::map<long, bw_per_prio_t>           calendar;
		std::map<long, bw_per_prio_t>::iterator iter;
		uint32_t                                start_time;
		struct timeval                          current_time;

		portId = TNRC_CONF_API::
			get_tnrc_port_id(p->board()->eqpt()->eqpt_id(),
					 p->board()->board_id(),
					 p->port_id());
		dlinkAddr = TNRC_CONF_API::get_dl_id(portId);

		dlink << dlinkAddr;

		params = new g2mplsTypes::DLinkParameters;

		params->states.opState  << p->opstate();
		params->states.admState << p->admstate();
		params->swCap           << p->board()->sw_cap();
		params->encType         << p->board()->enc_type();
		params->maxBandwidth     = p->max_bw();
		params->maxResBandwidth  = p->max_res_bw();
		params->minLSPbandwidth  = p->min_lsp_bw();
		for (int i = 0; i < 8; i++) {
			params->availBandwidthPerPrio[i] = p->unres_bw().bw[i];
			params->maxLSPbandwidth[i]      = p->max_lsp_bw().bw[i];
		}

		if (p->board()->sw_cap() == SWCAP_LSC) {
			bitmap           << p->dwdm_lambdas_bitmap();
		} else {
			bitmap = new g2mplsTypes::DLinkWdmLambdasBitmap;
		}
		params->lambdasBitmap = bitmap;

		calendar.clear();
		gettimeofday(&current_time, NULL);
		start_time = (uint32_t) current_time.tv_sec;
		p->get_calendar_map(TNRC_DEFAULT_NUM_EVENTS,
				    start_time,
				    TNRC_DEFAULT_QUANTUM,
				    calendar);

		g2mplsTypes::dataLinkCalendarSeq_var calendarSeq;
		{
			g2mplsTypes::dataLinkCalendarSeq * tmp;

			tmp = new g2mplsTypes::dataLinkCalendarSeq(calendar.size());
			if (!tmp) {
				STACK_UNLOCK();
				TNRC_WRN("Impossible to create advance reservation "
					 "calendar sequence");
				return;
			}
			calendarSeq = tmp;
		}
		calendarSeq->length(calendar.size());

		int i = 0;
		for (iter  = calendar.begin();
		     iter != calendar.end();
		     iter++) {
			calendarSeq[i].unixTime = (*iter).first;
			for (int j = 0; j < 8; j++) {
				calendarSeq[i].availBw[j] = (*iter).second.bw[j];
			}
			i++;
		}
		params->advRsrvCalendar = calendarSeq;

		lrm_proxy->updateDLinkDetails(dlink, params);

	} catch (LRM::Info::InternalProblems) {
		TNRC_WRN("LRM Internal Problems");
	} catch (LRM::Info::UnknownDLink) {
		TNRC_WRN("Unknown DataLink in LRM");
	} catch (...) {
		TNRC_WRN("Problems in comunications with LRM");
	}
#endif // HAVE_OMNIORB
}
