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



#include "tnrc_g2rsvpte_api.h"
#include "tnrc_corba.h"
#include "tnrc_trace.h"

//Forward declaration
g2mplsTypes::tnrcResult
getResponse(tnrcap_result_t    result,
	    tnrc_action_type_t action_type);

void
action_response(tnrcap_cookie_t    cookie,
		tnrc_action_type_t action_type,
		tnrcap_result_t    result,
		bool               executed,
		long               ctxt)
{
	TNRC_DBG("Sending actionResult %s for request %d to G2.RSVP-TE",
		 SHOW_TNRCSP_RESULT(result), cookie);
#if HAVE_OMNIORB
	try {
		Types::uint32           id;
		CORBA::Long             responseCtxt;
		g2mplsTypes::tnrcResult response;

		id           = cookie;
		responseCtxt = ctxt;
		response     = getResponse(result, action_type);

		g2rsvpte_proxy->actionResponse(id, response, responseCtxt);

	} catch (Types::InternalProblems()) {

		TNRC_WRN("Problems in comunications with G2.RSVP-TE");

	} catch (...) {

		TNRC_WRN("Problems with CORBA");

	}
#endif // HAVE_OMNIORB
	//
}

void
async_notification(tnrcap_cookie_t cookie,
		   void **         notify_list,
		   long            ctxt)
{
	//VERY TMP
	TNRC_DBG("Sending actionNotify for request %d to G2.RSVP-TE",
		 cookie);
#if HAVE_OMNIORB
	try {
		Types::uint32  id;
		CORBA::Long    notifyCtxt;

		id           = cookie;
		notifyCtxt = ctxt;

		g2rsvpte_proxy->actionNotify(id, notifyCtxt);

	} catch (Types::InternalProblems()) {

		TNRC_WRN("Problems in comunications with G2.RSVP-TE");

	} catch (...) {

		TNRC_WRN("Problems with CORBA");

	}

#endif // HAVE_OMNIORB
}

#if HAVE_OMNIORB

using namespace std;


g2mplsTypes::tnrcResult
getResponse(tnrcap_result_t    result,
	    tnrc_action_type_t action_type)
{
	g2mplsTypes::tnrcResult res;

	switch (action_type) {
		case MAKE_XC:
			switch (result) {
				case TNRCSP_RESULT_NOERROR:
					res = g2mplsTypes::TNRC_RESULT_MAKEXC_NOERROR;
					break;
				case TNRCSP_RESULT_EQPTLINKDOWN:
					res = g2mplsTypes::TNRC_RESULT_MAKEXC_EQPTDOWN;
					break;
				case TNRCSP_RESULT_PARAMERROR:
					res = g2mplsTypes::TNRC_RESULT_MAKEXC_PARAMERROR;
					break;
				case TNRCSP_RESULT_NOTCAPABLE:
					res = g2mplsTypes::TNRC_RESULT_MAKEXC_NOTCAPABLE;
					break;
				case TNRCSP_RESULT_BUSYRESOURCES:
					res = g2mplsTypes::TNRC_RESULT_MAKEXC_BUSYRESOURCES;
					break;
				case TNRCSP_RESULT_INTERNALERROR:
					res = g2mplsTypes::TNRC_RESULT_MAKEXC_INTERNALERROR;
					break;
				case TNRCSP_RESULT_GENERICERROR:
					res = g2mplsTypes::TNRC_RESULT_MAKEXC_GENERICERROR;
					break;
				default:
					throw out_of_range("tnrcResult type out-of-range");
					break;
			}
			break;
		case DESTROY_XC:
			switch (result) {
				case TNRCSP_RESULT_NOERROR:
					res = g2mplsTypes::TNRC_RESULT_DESTROYXC_NOERROR;
					break;
				case TNRCSP_RESULT_EQPTLINKDOWN:
					res = g2mplsTypes::TNRC_RESULT_DESTROYXC_EQPTDOWN;
					break;
				case TNRCSP_RESULT_PARAMERROR:
					res = g2mplsTypes::TNRC_RESULT_DESTROYXC_PARAMERROR;
					break;
				case TNRCSP_RESULT_NOTCAPABLE:
					res = g2mplsTypes::TNRC_RESULT_DESTROYXC_NOTCAPABLE;
					break;
				case TNRCSP_RESULT_BUSYRESOURCES:
					res = g2mplsTypes::TNRC_RESULT_DESTROYXC_BUSYRESOURCES;
					break;
				case TNRCSP_RESULT_INTERNALERROR:
					res = g2mplsTypes::TNRC_RESULT_DESTROYXC_INTERNALERROR;
					break;
				case TNRCSP_RESULT_GENERICERROR:
					res = g2mplsTypes::TNRC_RESULT_DESTROYXC_GENERICERROR;
					break;
				default:
					throw out_of_range("tnrcResult type out-of-range");
					break;
			}
			break;
		case RESERVE_XC:
			switch (result) {
				case TNRCSP_RESULT_NOERROR:
					res = g2mplsTypes::TNRC_RESULT_RESERVEXC_NOERROR;
					break;
				case TNRCSP_RESULT_EQPTLINKDOWN:
					res = g2mplsTypes::TNRC_RESULT_RESERVEXC_EQPTDOWN;
					break;
				case TNRCSP_RESULT_PARAMERROR:
					res = g2mplsTypes::TNRC_RESULT_RESERVEXC_PARAMERROR;
					break;
				case TNRCSP_RESULT_NOTCAPABLE:
					res = g2mplsTypes::TNRC_RESULT_RESERVEXC_NOTCAPABLE;
					break;
				case TNRCSP_RESULT_BUSYRESOURCES:
					res = g2mplsTypes::TNRC_RESULT_RESERVEXC_BUSYRESOURCES;
					break;
				case TNRCSP_RESULT_INTERNALERROR:
					res = g2mplsTypes::TNRC_RESULT_RESERVEXC_INTERNALERROR;
					break;
				case TNRCSP_RESULT_GENERICERROR:
					res = g2mplsTypes::TNRC_RESULT_RESERVEXC_GENERICERROR;
					break;
				default:
					throw out_of_range("tnrcResult type out-of-range");
					break;
			}
			break;
		case UNRESERVE_XC:
			switch (result) {
				case TNRCSP_RESULT_NOERROR:
					res = g2mplsTypes::TNRC_RESULT_UNRESERVEXC_NOERROR;
					break;
				case TNRCSP_RESULT_EQPTLINKDOWN:
					res = g2mplsTypes::TNRC_RESULT_UNRESERVEXC_EQPTDOWN;
					break;
				case TNRCSP_RESULT_PARAMERROR:
					res = g2mplsTypes::TNRC_RESULT_UNRESERVEXC_PARAMERROR;
					break;
				case TNRCSP_RESULT_NOTCAPABLE:
					res = g2mplsTypes::TNRC_RESULT_UNRESERVEXC_NOTCAPABLE;
					break;
				case TNRCSP_RESULT_BUSYRESOURCES:
					res = g2mplsTypes::TNRC_RESULT_UNRESERVEXC_BUSYRESOURCES;
					break;
				case TNRCSP_RESULT_INTERNALERROR:
					res = g2mplsTypes::TNRC_RESULT_UNRESERVEXC_INTERNALERROR;
					break;
				case TNRCSP_RESULT_GENERICERROR:
					res = g2mplsTypes::TNRC_RESULT_UNRESERVEXC_GENERICERROR;
					break;
				default:
					throw out_of_range("tnrcResult type out-of-range");
					break;
			}
			break;
	}

	return res;
}

#endif // HAVE_OMNIORB
