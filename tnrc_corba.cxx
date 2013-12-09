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



#include "zebra.h"

#include "tnrcd.h"
#include "tnrc_trace.h"
#include "tnrc_config.h"

#include "log.h"

#if HAVE_OMNIORB
#include "tnrc_corba.h"
#include "lib/corba.h"
#include "g2mpls_corba_utils.h"
#include "idl/tnrc.hh"
#include "idl/g2rsvpte.hh"
#include "idl/lrm.hh"
#include <iostream>
#include <string>
#include <stdlib.h>

//using namespace std;

class TNRC_i : public POA_TNRC::NorthBound,
	       public PortableServer::RefCountServantBase
{
public:
	inline TNRC_i();
	virtual ~TNRC_i();

	CORBA::Boolean
	makeXC(Types::uint32 &              cookie,
	       const g2mplsTypes::DLinkId & dlinkIn,
	       const g2mplsTypes::labelId & labelIn,
	       const g2mplsTypes::DLinkId & dlinkOut,
	       const g2mplsTypes::labelId & labelOut,
	       CORBA::Boolean               isVirtual,
	       g2mplsTypes::xcDirection     direction,
	       Types::uint32                activate,
	       Types::uint32                rsrvCookie,
	       CORBA::Long                  responseCtxt,
	       CORBA::Long                  asyncCtxt);

	CORBA::Boolean
	destroyXC(Types::uint32 cookie,
		  Types::uint32 deactivate,
		  CORBA::Long   responseCtxt);

	CORBA::Boolean
	reserveXC(Types::uint32 &              cookie,
		  const g2mplsTypes::DLinkId & dlinkIn,
		  const g2mplsTypes::labelId & labelIn,
		  const g2mplsTypes::DLinkId & dlinkOut,
		  const g2mplsTypes::labelId & labelOut,
		  CORBA::Boolean               isVirtual,
		  g2mplsTypes::xcDirection     direction,
		  Types::uint32                advanceRsrv,
		  CORBA::Long                  startTime,
		  CORBA::Long                  endTime,
		  CORBA::Long                  responseCtxt);

	CORBA::Boolean
	unreserveXC(Types::uint32 cookie,
		    CORBA::Long   responseCtxt);

	CORBA::Boolean
	getDLinkDetails(const g2mplsTypes::DLinkId &       dataLink,
			g2mplsTypes::DLinkParameters_out   params);

	CORBA::Boolean
	getLabelStatus(const g2mplsTypes::DLinkId & localDataLink,
		       const g2mplsTypes::labelId & label,
		       g2mplsTypes::labelState &    labelStatus,
		       g2mplsTypes::operState &     operStatus);

	CORBA::Boolean
	getLabelFromDLink(const g2mplsTypes::DLinkId & dataLink,
			  g2mplsTypes::labelId &       label);

	g2mplsTypes::dataLinkCalendarSeq *
	getDLinkCalendar(const g2mplsTypes::DLinkId & dataLink,
			 Types::uint32                startTime,
			 Types::uint32                numOfEvents,
			 Types::uint32                quantum);

	void
	g2rsvpteIsUp(void);

	void
	lrmIsUp(void);

};

TNRC_i::TNRC_i()
{
}

TNRC_i::~TNRC_i()
{
}

CORBA::Boolean
TNRC_i::makeXC(Types::uint32 &              cookie,
	       const g2mplsTypes::DLinkId & dlinkIn,
	       const g2mplsTypes::labelId & labelIn,
	       const g2mplsTypes::DLinkId & dlinkOut,
	       const g2mplsTypes::labelId & labelOut,
	       CORBA::Boolean               isVirtual,
	       g2mplsTypes::xcDirection     direction,
	       Types::uint32                activate,
	       Types::uint32                rsrvCookie,
	       CORBA::Long                  responseCtxt,
	       CORBA::Long                  asyncCtxt)
{
	STACK_LOCK();

	g2mplsTypes::DLinkId_var dlinkInTmp;
	g2mplsTypes::DLinkId_var dlinkOutTmp;
	g2mplsTypes::labelId_var labelInTmp;
	g2mplsTypes::labelId_var labelOutTmp;
	tnrcapiErrorCode_t       res;
	g2mpls_addr_t            dlinkIdIn;
	g2mpls_addr_t            dlinkIdOut;
	label_t                  labelIdIn;
	label_t                  labelIdOut;
	xcdirection_t            dir;
	tnrcap_cookie_t          cookieTmp;
	tnrc_boolean_t           isVirt;

	cookieTmp   = cookie;
	isVirt      = isVirtual ? 1 : 0;

	dlinkInTmp  = dlinkIn;
	dlinkOutTmp = dlinkOut;
	labelInTmp  = labelIn;
	labelOutTmp = labelOut;

	dlinkIdIn  << dlinkInTmp;
	dlinkIdOut << dlinkOutTmp;
	labelIdIn  << labelInTmp;
	labelIdOut << labelOutTmp;

	dir << direction;

	TNRC_DBG("Requested a %s MAKE XC action",
		 isVirtual ? "virtual" : "");

	res = TNRC_OPEXT_API::tnrcap_make_xc(&cookieTmp,
					     dlinkIdIn,
					     labelIdIn,
					     dlinkIdOut,
					     labelIdOut,
					     dir,
					     isVirt,
					     activate,
					     rsrvCookie,
					     responseCtxt,
					     asyncCtxt);
	if (res != TNRC_API_ERROR_NONE) {
		TNRC_ERR("Cannot make XC: %s",
			 TNRCAPIERR2STRING(res));
		STACK_UNLOCK();
		throw (TNRC::ParamError());
	}

	cookie = cookieTmp;

	STACK_UNLOCK();
	return true;
}

CORBA::Boolean
TNRC_i::destroyXC(Types::uint32 cookie,
		  Types::uint32 deactivate,
		  CORBA::Long   responseCtxt)
{
	STACK_LOCK();

	TNRC_DBG("Requested a DESTROY XC action");

	tnrcapiErrorCode_t res;

	res = TNRC_OPEXT_API::tnrcap_destroy_xc(cookie,
						deactivate,
						responseCtxt);
	if (res != TNRC_API_ERROR_NONE) {
		TNRC_ERR("Cannot destroy XC: %s",
			 TNRCAPIERR2STRING(res));
		STACK_UNLOCK();
		throw (TNRC::ParamError());
	}

	STACK_UNLOCK();
	return true;
}

CORBA::Boolean
TNRC_i::reserveXC(Types::uint32 &              cookie,
		  const g2mplsTypes::DLinkId & dlinkIn,
		  const g2mplsTypes::labelId & labelIn,
		  const g2mplsTypes::DLinkId & dlinkOut,
		  const g2mplsTypes::labelId & labelOut,
		  CORBA::Boolean               isVirtual,
		  g2mplsTypes::xcDirection     direction,
		  Types::uint32                advanceRsrv,
		  CORBA::Long                  startTime,
		  CORBA::Long                  endTime,
		  CORBA::Long                  responseCtxt)
{
	STACK_LOCK();

	TNRC_DBG("Requested a %s RESERVE XC action",
		 isVirtual ? "virtual" : "");

	g2mplsTypes::DLinkId_var dlinkInTmp;
	g2mplsTypes::DLinkId_var dlinkOutTmp;
	g2mplsTypes::labelId_var labelInTmp;
	g2mplsTypes::labelId_var labelOutTmp;
	tnrcapiErrorCode_t       res;
	g2mpls_addr_t            dlinkIdIn;
	g2mpls_addr_t            dlinkIdOut;
	label_t                  labelIdIn;
	label_t                  labelIdOut;
	xcdirection_t            dir;
	tnrcap_cookie_t          cookieTmp;
	long                     start;
	long                     end;
	tnrc_boolean_t           isVirt;

	cookieTmp   = cookie;
	isVirt      = isVirtual ? 1 : 0;

	dlinkInTmp  = dlinkIn;
	dlinkOutTmp = dlinkOut;
	labelInTmp  = labelIn;
	labelOutTmp = labelOut;

	dlinkIdIn  << dlinkInTmp;
	dlinkIdOut << dlinkOutTmp;
	labelIdIn  << labelInTmp;
	labelIdOut << labelOutTmp;

	dir << direction;

	start = startTime;
	end   = endTime;

	res = TNRC_OPEXT_API::tnrcap_reserve_xc(&cookieTmp,
						dlinkIdIn,
						labelIdIn,
						dlinkIdOut,
						labelIdOut,
						dir,
						isVirt,
						advanceRsrv,
						start,
						end,
						responseCtxt);
	if (res != TNRC_API_ERROR_NONE) {
		TNRC_ERR("Cannot reserve XC: %s",
			 TNRCAPIERR2STRING(res));
		STACK_UNLOCK();
		throw (TNRC::ParamError());
	}

	cookie = cookieTmp;

	STACK_UNLOCK();
	return true;
}

CORBA::Boolean
TNRC_i::unreserveXC(Types::uint32 cookie,
		    CORBA::Long   responseCtxt)
{
	STACK_LOCK();

	TNRC_DBG("Requested an UNRESERVE XC action");

	tnrcapiErrorCode_t res;

	res = TNRC_OPEXT_API::tnrcap_unreserve_xc(cookie,
						  responseCtxt);
	if (res != TNRC_API_ERROR_NONE) {
		TNRC_ERR("Cannot unreserve XC: %s",
			 TNRCAPIERR2STRING(res));
		STACK_UNLOCK();
		throw (TNRC::ParamError());
	}

	STACK_UNLOCK();
	return true;
}

CORBA::Boolean
TNRC_i::getDLinkDetails(const g2mplsTypes::DLinkId &       dataLink,
			g2mplsTypes::DLinkParameters_out   params)
{
	STACK_LOCK();

	g2mplsTypes::DLinkId_var                dlinkTmp;
	g2mplsTypes::DLinkParameters *          paramsTmp;
	std::string                             dlString;
	tnrcapiErrorCode_t                      res;
	g2mpls_addr_t                           dlink;
	tnrc_api_dl_details_t                   details;
	std::map<long, bw_per_prio_t>::iterator iter;

	dlinkTmp  = dataLink;
	dlString << dlinkTmp;

	TNRC_DBG("Someone is requesting details of Data Link  %s",
		 dlString.c_str());

	dlink   << dlinkTmp;

	memset(&details, 0, sizeof(tnrc_api_dl_details_t));
	details.calendar.clear();

	res = TNRC_OPEXT_API::tnrcap_get_dl_details(dlink, details);
	if (res != TNRC_API_ERROR_NONE) {
		TNRC_ERR("Cannot send Data Link details: %s",
			 TNRCAPIERR2STRING(res));
		STACK_UNLOCK();
		throw (TNRC::CannotFetch());
	}
	paramsTmp = new g2mplsTypes::DLinkParameters;

	paramsTmp->states.opState    << details.opstate;
	//Set admState to a valid value (even if LRM doesn't matter the value)
	paramsTmp->states.admState    = g2mplsTypes::ADMINSTATE_DISABLED;
	paramsTmp->swCap             << details.sw_cap;
	paramsTmp->encType           << details.enc_type;
	paramsTmp->maxBandwidth       = details.tot_bw;
	paramsTmp->maxResBandwidth    = details.max_res_bw;
	for (int i = 0; i < 8; i++) {
		paramsTmp->availBandwidthPerPrio[i] = details.unres_bw.bw[i];
		paramsTmp->maxLSPbandwidth[i]       = details.max_lsp_bw.bw[i];
	}
	paramsTmp->minLSPbandwidth    = details.min_lsp_bw;
	if (details.sw_cap == SWCAP_LSC) {
		g2mplsTypes::DLinkWdmLambdasBitmap_var bitmapTmp;
		bitmapTmp                    << details.bitmap;
		paramsTmp->lambdasBitmap      = bitmapTmp;
	}

	g2mplsTypes::dataLinkCalendarSeq_var calendarSeq;
	{
		g2mplsTypes::dataLinkCalendarSeq * tmp;

		tmp = new g2mplsTypes::dataLinkCalendarSeq(details.calendar.size());
		if (!tmp) {
			STACK_UNLOCK();
			throw(TNRC::InternalProblems());
		}
		calendarSeq = tmp;
	}
	calendarSeq->length(details.calendar.size());

	int i = 0;
	for (iter = details.calendar.begin();
	     iter != details.calendar.end();
	     iter++) {
		calendarSeq[i].unixTime = (*iter).first;
		for (int j = 0; j < 8; j++) {
			calendarSeq[i].availBw[j] = (*iter).second.bw[j];
		}
		i++;
	}
	paramsTmp->advRsrvCalendar = calendarSeq;

	params = paramsTmp;

	TNRC_DBG("Sent details of Data Link");
	STACK_UNLOCK();
	return true;
}

CORBA::Boolean
TNRC_i::getLabelStatus(const g2mplsTypes::DLinkId & localDataLink,
		       const g2mplsTypes::labelId & label,
		       g2mplsTypes::labelState &    labelStatus,
		       g2mplsTypes::operState &     operStatus)
{
	STACK_LOCK();

	g2mplsTypes::DLinkId_var dlinkTmp;
	g2mplsTypes::labelId_var labelTmp;
	std::string              dlString;
	std::string              labelString;
	std::string              opStateString;
	std::string              labelStateString;
	tnrcapiErrorCode_t       res;
	g2mpls_addr_t            dlink;
	label_t                  labelId;
	opstate_t                opState;
	label_state_t            labelState;

	dlinkTmp = localDataLink;
	labelTmp = label;

	dlString    << dlinkTmp;
	labelString << labelTmp;

	TNRC_DBG("Someone is requesting the status of Label %s "
		 "from DataLink %s", labelString.c_str(), dlString.c_str());

	dlink   << dlinkTmp;
	labelId << labelTmp;

	res = TNRC_OPEXT_API::tnrcap_get_label_details(dlink,
						       labelId,
						       opState,
						       labelState);
	if (res != TNRC_API_ERROR_NONE) {
		TNRC_ERR("Cannot send Label Status: %s", TNRCAPIERR2STRING(res));
		STACK_UNLOCK();
		throw (TNRC::CannotFetch());
	}
	labelStatus << labelState;
	operStatus  << opState;

	opStateString    << operStatus;
	labelStateString << labelStatus;

	TNRC_DBG("Sent Label Status: opstate %s, label state %s",
		 opStateString.c_str(), labelStateString.c_str());

	STACK_UNLOCK();

	return true;
}

CORBA::Boolean
TNRC_i::getLabelFromDLink(const g2mplsTypes::DLinkId & dataLink,
			  g2mplsTypes::labelId &       label)
{
	STACK_LOCK();

	g2mplsTypes::DLinkId_var              dlinkTmp;
	g2mplsTypes::labelId_var              labelTmp;
	std::string                           dlString;
	std::string                           labelString;
	tnrcapiErrorCode_t                    res;
	g2mpls_addr_t                         dlink;
	label_t                               labelId;
	std::list<tnrc_api_label_t>           labelList;
	std::list<tnrc_api_label_t>::iterator iter;
	tnrc_api_label_t                      labelDetails;

	dlinkTmp = dataLink;

	dlString << dlinkTmp;

	TNRC_DBG("Someone is requesting Label from DataLink %s",
		 dlString.c_str());

	dlink   << dlinkTmp;

	res = TNRC_OPEXT_API::tnrcap_get_label_list(dlink, labelList);
	if (res != TNRC_API_ERROR_NONE) {
		TNRC_ERR("Cannot send Label from DLink: %s",
			 TNRCAPIERR2STRING(res));
		STACK_UNLOCK();
		throw (TNRC::InternalProblems());
	}

	for(iter = labelList.begin(); iter != labelList.end(); iter++) {
		labelDetails = *iter;
		if ((labelDetails.opstate != UP        ) ||
		    (labelDetails.state   != LABEL_FREE)) {
			continue;
		}
		labelTmp << labelDetails.label_id;
		label    =  labelTmp;

		labelString << labelTmp;

		TNRC_DBG("Sent Label %s",
			 labelString.c_str());
		STACK_UNLOCK();
		return true;
	}

	TNRC_DBG("Cannot send Label from DLink %s : no free Labels",
		 dlString.c_str());
	STACK_UNLOCK();
	return false;
}

g2mplsTypes::dataLinkCalendarSeq *
TNRC_i::getDLinkCalendar(const g2mplsTypes::DLinkId & dataLink,
			 Types::uint32                startTime,
			 Types::uint32                numOfEvents,
			 Types::uint32                quantum)
{
	STACK_LOCK();

	g2mplsTypes::DLinkId_var                      dlinkTmp;
	std::string                                   dlString;
	std::string                                   labelString;
	tnrcapiErrorCode_t                            res;
	g2mpls_addr_t                                 dlink;
	std::map<long, bw_per_prio_t>                 calendar;
	std::map<long, bw_per_prio_t>::iterator       iter;
	int                                           i;

	dlinkTmp = dataLink;

	dlString << dlinkTmp;

	TNRC_DBG("LRM is requesting Calendar for DataLink %s",
		 dlString.c_str());

	dlink   << dlinkTmp;

	res = TNRC_OPEXT_API::tnrcap_get_dl_calendar(dlink,
						     numOfEvents,
						     startTime,
						     quantum,
						     calendar);
	if (res != TNRC_API_ERROR_NONE) {
		switch (res) {
			case  TNRC_API_ERROR_GENERIC:
				TNRC_ERR("Cannot send Calendar: %s. "
					 "No transitions for DataLink",
					 TNRCAPIERR2STRING(res));
				break;
			case  TNRC_API_ERROR_INVALID_PARAMETER:
				TNRC_ERR("Cannot send Calendar: %s",
					 TNRCAPIERR2STRING(res));
				STACK_UNLOCK();
				throw(TNRC::CannotFetch());
				break;
			default:
				TNRC_ERR("Cannot send Calendar: %s",
					 TNRCAPIERR2STRING(res));
				break;
		}
		STACK_UNLOCK();
		throw (TNRC::InternalProblems());
	}

	g2mplsTypes::dataLinkCalendarSeq_var calendarSeq;
	{
		g2mplsTypes::dataLinkCalendarSeq * tmp;

		tmp = new g2mplsTypes::dataLinkCalendarSeq(calendar.size());
		if (!tmp) {
			STACK_UNLOCK();
			throw(TNRC::InternalProblems());
		}
		calendarSeq = tmp;
	}
	calendarSeq->length(calendar.size());

	i = 0;
	for (iter = calendar.begin(); iter != calendar.end(); iter++) {
		calendarSeq[i].unixTime = (*iter).first;
		for (int j = 0; j < 8; j++) {
			calendarSeq[i].availBw[j] = (*iter).second.bw[j];
		}
		i++;
	}

	STACK_UNLOCK();

	return calendarSeq._retn();
}


void
TNRC_i::g2rsvpteIsUp(void)
{
	STACK_LOCK();

	TNRC_DBG("G2.RSVP-TE is up");

	// corba_g2rsvpte_client_shutdown before setup ???
	if (!corba_client_setup(g2rsvpte_proxy)) {
		TNRC_ERR("Cannot setup G2.RSVP-TE client object");
		STACK_UNLOCK();
		throw (TNRC::InternalProblems());
	}
	STACK_UNLOCK();
}

void
TNRC_i::lrmIsUp(void)
{
	STACK_LOCK();

	TNRC_DBG("LRM is up");

	// corba_lrm_client_shutdown before setup ???
	if (!corba_client_setup(lrm_proxy)) {
		TNRC_ERR("Cannot setup LRM client object");
		STACK_UNLOCK();
		throw (TNRC::InternalProblems());
	}
	STACK_UNLOCK();
}

TNRC_i*                                   servant = 0;
g2rsvpte::TnrControl_var                  g2rsvpte_proxy;
LRM::Info_var                             lrm_proxy;
#endif // HAVE_OMNIORB

int corba_server_setup(void)
{
#if HAVE_OMNIORB
	TNRC_DBG("Setting up CORBA server side");

	try {
		servant = new TNRC_i();
		if (!servant) {
			throw string("Cannot create servant");
		}

		PortableServer::POA_var poa;
		poa = corba_poa();
		if (CORBA::is_nil(poa)) {
			throw string("Cannot get POA");
		}

		PortableServer::ObjectId_var servant_id;
		servant_id = poa->activate_object(servant);

		CORBA::Object_var obj;
		obj = servant->_this();
		if (CORBA::is_nil(obj)) {
			throw string("Cannot get object");
		}

		CORBA::ORB_var orb;
		orb = corba_orb();
		if (CORBA::is_nil(orb)) {
			throw string("Cannot get ORB");
		}

		CORBA::String_var sior(orb->object_to_string(obj));
		TNRC_DBG("IOR = '%s'", (char*) sior);

		if (!corba_dump_ior(CORBA_SERVANT_TNRC, string(sior))) {
			throw string("Cannot dump IOR");
		}

		servant->_remove_ref();

		PortableServer::POAManager_var poa_manager;
		poa_manager = corba_poa_manager();
		if (CORBA::is_nil(poa_manager)) {
			throw string("Cannot get POA Manager");
		}

		poa_manager->activate();
	} catch (CORBA::SystemException & e) {
		TNRC_ERR("Caught CORBA::SystemException");
		return 0;
	} catch (CORBA::Exception & e) {
		TNRC_ERR("Caught CORBA::Exception");
		return 0;
	} catch (omniORB::fatalException & e) {
		TNRC_ERR("Caught omniORB::fatalException:");
		TNRC_ERR("  file: %s", e.file());
		TNRC_ERR("  line: %d", e.line());
		TNRC_ERR("  mesg: %s", e.errmsg());
		return 0;
	} catch (string & e) {
		TNRC_ERR("Caught exception: %s", e.c_str());
		return 0;
	} catch (...) {
		TNRC_ERR("Caught unknown exception");
		return 0;
	}
#endif // HAVE_OMNIORB

	return 1;
}

int corba_server_shutdown(void)
{
#if HAVE_OMNIORB
	try {
		TNRC_DBG("Shutting down CORBA server side");

		if (!corba_remove_ior(CORBA_SERVANT_TNRC)) {
			throw string("Cannot remove IOR");
		}
	} catch (...) {
		TNRC_ERR("Caught unknown exception");
		return 0;
	}
#endif // HAVE_OMNIORB

	return 1;
}

int corba_client_setup(g2rsvpte::TnrControl_var & g)
{
#if HAVE_OMNIORB
	CORBA::ORB_var orb;
	orb = corba_orb();
	if (CORBA::is_nil(orb)) {
		fprintf(stderr, "Cannot get ORB\n");
		return 0;
	}

	string ior;
	if (!corba_fetch_ior(CORBA_SERVANT_TNRCONTROLLER_G2RSVPTE, ior)) {
		fprintf(stderr, "Cannot fetch IOR\n");
		return 0;
	}

	try {
		CORBA::Object_var obj;
		obj = orb->string_to_object(ior.c_str());
		if (CORBA::is_nil(obj)) {
			fprintf(stderr, "Cannot get object\n");
			return 0;
		}

		g = g2rsvpte::TnrControl::_narrow(obj);
		if (CORBA::is_nil(g)) {
			fprintf(stderr, "cannot invoke on a nil object "
				"reference\n");
			return 0;
		}
	} catch (CORBA::SystemException & e) {
		fprintf(stderr, "Caught CORBA::SystemException\n");
		return 0;
	} catch (CORBA::Exception & e) {
		fprintf(stderr, "Caught CORBA::Exception\n");
		return 0;
	} catch (...) {
		fprintf(stderr, "CLIENT_SETUP: caught unknown "
			"exception\n");
		return 0;
	}
#endif // HAVE_OMNIORB
	return 1;
}

int corba_client_setup(LRM::Info_var & l)
{
#if HAVE_OMNIORB
	CORBA::ORB_var orb;
	orb = corba_orb();
	if (CORBA::is_nil(orb)) {
		fprintf(stderr, "Cannot get ORB\n");
		return 0;
	}

	string ior;
	if (!corba_fetch_ior(CORBA_SERVANT_LRM, ior)) {
		fprintf(stderr, "Cannot fetch IOR\n");
		return 0;
	}

	try {
		CORBA::Object_var obj;
		obj = orb->string_to_object(ior.c_str());
		if (CORBA::is_nil(obj)) {
			fprintf(stderr, "Cannot get object\n");
			return 0;
		}

		l = LRM::Info::_narrow(obj);
		if (CORBA::is_nil(l)) {
			fprintf(stderr, "cannot invoke on a nil object "
				"reference\n");
			return 0;
		}
	} catch (CORBA::SystemException & e) {
		fprintf(stderr, "Caught CORBA::SystemException\n");
		return 0;
	} catch (CORBA::Exception & e) {
		fprintf(stderr, "Caught CORBA::Exception\n");
		return 0;
	} catch (...) {
		fprintf(stderr, "CLIENT_SETUP: caught unknown "
			"exception\n");
		return 0;
	}
#endif // HAVE_OMNIORB
	return 1;
}

int corba_client_shutdown(g2rsvpte::TnrControl_var & g)
{
#if HAVE_OMNIORB
#endif // HAVE_OMNIORB
	return 1;
}

int corba_client_shutdown(LRM::Info_var & l)
{
#if HAVE_OMNIORB
#endif // HAVE_OMNIORB
	return 1;
}
