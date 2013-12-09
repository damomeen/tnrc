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



#include "tnrcd.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_lrm_api.h"
#include "tnrc_g2rsvpte_api.h"

bool
virtual_xc_check_consistency(tnrc_port_id_t port_id,
			     label_t        labelid)
{
	XC *                       xc;
	TNRC_Master::iterator_xcs  iter;

	for(iter  = TNRC_MASTER.begin_xcs();
	    iter != TNRC_MASTER.end_xcs();
	    iter++){
		xc = static_cast<XC *> ((*iter).second);
		assert(xc);
		if ((xc->portid_in()  == port_id) &&
		    (xc->labelid_in() == labelid)) {
			TNRC_WRN("Port id 0x%x and label 0x%8x are busy",
				 port_id, labelid.value.rawId);
			return false;
		}
		if ((xc->portid_out()  == port_id) &&
		    (xc->labelid_out() == labelid)) {
			TNRC_WRN("Port id 0x%x and label 0x%8x are busy",
				 port_id, labelid.value.rawId);
			return false;
		}
	}

	return true;
}

bool
xc_check_consistency(g2mpls_addr_t      dl_id,
		     label_t            labelid,
		     tnrc_action_type_t act_type,
		     tnrc_boolean_t     activate,
		     tnrc_boolean_t     is_virtual)
{
	TNRC::TNRC_AP  * t;
	TNRC::Eqpt     * e;
	TNRC::Board    * b;
	TNRC::Port     * p;
	TNRC::Resource * r;
	tnrc_port_id_t   tnrc_port_id;
	eqpt_id_t        e_id;
	board_id_t       b_id;
	port_id_t        p_id;

	if (activate && is_virtual) {
		TNRC_ERR ("Parameter error: "
			  "cannot set both activate and is_virtual");
		return false;
	}

	//get instance
	t = TNRC_MASTER.getTNRC();

	//get equipment, board and port id's from datalink id
	TNRC_CONF_API::convert_dl_id(&tnrc_port_id, dl_id);
	TNRC_CONF_API::convert_tnrc_port_id(&e_id, &b_id, &p_id, tnrc_port_id);

	//check if equipment id exist in the data model
	e = t->getEqpt(e_id);
	if (e == NULL) {
		TNRC_ERR ("Data-link id uncorrect: no such equipment");
		return false;
	}
	//check if board id exist in the data model
	b = t->getBoard(e_id, b_id);
	if (b == NULL) {
		TNRC_ERR ("Data-link id uncorrect: no such board");
		return false;
	}
	//check if port id exist in the data model
	p = t->getPort(e_id, b_id, p_id);
	if (p == NULL) {
		TNRC_ERR ("Data-link id uncorrect: no such port");
		return false;
	}
	//check if label id exist in the data model
	r = t->getResource(e_id, b_id, p_id, labelid);
	if (r == NULL) {
		TNRC_ERR ("Label id uncorrect: no such resource");
		return false;
	}
	//check if the states of eqpt board and port are correct
	if (!CHECK_DL_STATE(e, b, p)) {
		TNRC_ERR ("Data-link state not valid");
		return false;
	}
	//check if the state of resource is correct
	if (!CHECK_RESOURCE_STATE(r)) {
		TNRC_ERR ("Resource state not valid");
		return false;
	}
	switch (act_type) {
		case MAKE_XC:
			if (is_virtual) {
				//check XCs
				if (!virtual_xc_check_consistency(tnrc_port_id,
								  labelid)) {
					TNRC_ERR ("Impossible to setup virtual MAKE XC action: "
						  "resources already xconnected");
					return false;
				}
			} else {
				if (activate) {
					//check if resource is booked
					if (!CHECK_RESOURCE_BOOKED(r)) {
						TNRC_ERR ("Resource is neither free "
							  "nor booked");
						return false;
					}
				} else {
					//check if resource is free
					if (!CHECK_RESOURCE_FREE(r)) {
						TNRC_ERR ("Resource is not free");
						return false;
					}
				}
			}
			break;
		case RESERVE_XC:
			if (is_virtual) {
				//check XCs
				if (!virtual_xc_check_consistency(tnrc_port_id,
								  labelid)) {
					TNRC_ERR ("Impossible to setup virtual RESERVE XC action: "
						  "resources busy");
					return false;
				}
			} else {
				//check if resource is free
				if (!CHECK_RESOURCE_FREE(r)) {
					TNRC_ERR ("Resource is not free");
					return false;
				}
			}
			break;
	}

	return true;
}

bool
advance_rsrv_check_consistency(g2mpls_addr_t     dl_id,
			       label_t           labelid,
			       long              start,
			       long              end,
			       tnrc_boolean_t    is_virtual)
{
	TNRC::TNRC_AP  * t;
	TNRC::Eqpt     * e;
	TNRC::Board    * b;
	TNRC::Port     * p;
	TNRC::Resource * r;
	tnrc_port_id_t   tnrc_port_id;
	eqpt_id_t        e_id;
	board_id_t       b_id;
	port_id_t        p_id;
	struct timeval   current_time;
	tm *             curTime;
	const char *     timeStringFormat = "%a, %d %b %Y %H:%M:%S %zGMT";
	const int        timeStringLength = 40;
	char             timeString[timeStringLength];

	//get instance
	t = TNRC_MASTER.getTNRC();

	//get equipment, board and port id's from datalink id
	TNRC_CONF_API::convert_dl_id(&tnrc_port_id, dl_id);
	TNRC_CONF_API::convert_tnrc_port_id(&e_id, &b_id, &p_id, tnrc_port_id);

	//check if equipment id exist in the data model
	e = t->getEqpt(e_id);
	if (e == NULL) {
		TNRC_ERR ("Data-link id uncorrect: no such equipment");
		return false;
	}
	//check if board id exist in the data model
	b = t->getBoard(e_id, b_id);
	if (b == NULL) {
		TNRC_ERR ("Data-link id uncorrect: no such board");
		return false;
	}
	//check if port id exist in the data model
	p = t->getPort(e_id, b_id, p_id);
	if (p == NULL) {
		TNRC_ERR ("Data-link id uncorrect: no such port");
		return false;
	}
	//check if label id exist in the data model
	r = t->getResource(e_id, b_id, p_id, labelid);
	if (r == NULL) {
		TNRC_ERR ("Data-link id uncorrect: no such resource");
		return false;
	}
	//check if end is bigger than start
	if (start >= end) {
		TNRC_ERR ("Start time for ADVANCE RESERVATION bigger "
			  "than end time");
		return false;
	}
	//check if start is in the future
	gettimeofday(&current_time, NULL);
	if (is_virtual) {
		if (end <= current_time.tv_sec) {
			TNRC_ERR ("End time for virtual ADVANCE "
				  "RESERVATION is in the past");
			return false;
		}
		if (start <= current_time.tv_sec) {
			TNRC_WRN ("Start time for virtual ADVANCE "
				  "RESERVATION is in the past. "
				  "Let it start now.");
		}
	} else {
		if (start <= current_time.tv_sec) {
			TNRC_ERR ("Start time for ADVANCE RESERVATION "
				  "is in the past");
			return false;
		}
	}
	//check if label is reserved in the time slot specified
	if (!r->check_label_availability(start, end)) {
		curTime = localtime((time_t *) &start);
		strftime(timeString, timeStringLength, timeStringFormat, curTime);
		TNRC_ERR ("Label is not available in the timeslot: %s",
			  timeString);
		curTime = localtime((time_t *) &end);
		strftime(timeString, timeStringLength, timeStringFormat, curTime);
		TNRC_ERR ("                                        %s",
			  timeString);
		return false;
	}
	//set advance reservation in data model
	r->attach(start, end);
	return true;
}

tnrcapiErrorCode_t
TNRC_OPEXT_API::tnrcap_make_xc(tnrcap_cookie_t *        cookiep,
			       g2mpls_addr_t            dl_id_in,
			       label_t                  labelid_in,
			       g2mpls_addr_t            dl_id_out,
			       label_t                  labelid_out,
			       xcdirection_t            direction,
			       tnrc_boolean_t           is_virtual,
			       tnrc_boolean_t           activate,
			       tnrcap_cookie_t          rsrv_cookie,
			       long                     response_ctxt,
			       long                     async_ctxt)
{
	TNRC::TNRC_AP       * t;
	make_xc_param_t     * param;
	api_queue_element_t * element;
	Plugin              * p;
	tnrc_port_id_t        tnrc_p_id_in;
	tnrc_port_id_t        tnrc_p_id_out;
	eqpt_id_t             e_id;
	board_id_t            b_id;
	port_id_t             p_id;
	eqpt_type_t           type;
	bool                  res;

	//check if a TNRC instance is running
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		TNRC_ERR ("No TNRC instance running. Discard request of "
			  "MAKE XC");
		return TNRC_API_ERROR_NO_INSTANCE;
	}
	//check if equipment link is down
	if (t->eqpt_link_down()) {
		TNRC_ERR ("Equipment link is down. Discard request of MAKE XC");
		return TNRC_API_ERROR_INTERNAL_ERROR;
	}
	//Check consistency for this request (IN&OUT parameters)
	if((!xc_check_consistency(dl_id_in,  labelid_in, MAKE_XC,  activate, is_virtual)) ||
	   (!xc_check_consistency(dl_id_out, labelid_out, MAKE_XC, activate, is_virtual))) {
		TNRC_ERR ("MAKE XC parameters' incompatible with data model");
		return TNRC_API_ERROR_INVALID_PARAMETER;
	}

	//Convert dl_ids and check if there's a Plugin for this operation
	TNRC_CONF_API::convert_dl_id(&tnrc_p_id_in, dl_id_in);
	TNRC_CONF_API::convert_dl_id(&tnrc_p_id_out, dl_id_out);
	TNRC_CONF_API::convert_tnrc_port_id(&e_id, &b_id, &p_id, tnrc_p_id_in);
	type = TNRC_MASTER.getEqpt_type(e_id);

	p = TNRC_MASTER.getPC()->getPlugin(EQPT_TYPE2NAME(type));
	if (p == NULL) {
		TNRC_ERR ("There's no plugin for the operation requested.");
		return TNRC_API_ERROR_NO_PLUGIN;
	}
	param = new_make_xc_element(cookiep,
				    tnrc_p_id_in,
				    labelid_in,
				    tnrc_p_id_out,
				    labelid_out,
				    direction,
				    is_virtual,
				    activate,
				    rsrv_cookie,
				    response_ctxt,
				    async_ctxt);

	//Set correct Plugin for operation requested
	param->plugin = p;

	element = new api_queue_element_t;

	element->type         = MAKE_XC;
	element->action_param = param;

	//Insert the element relative to this action in the queue of actions
	//to execute
	res = TNRC_MASTER.api_queue_insert (element);
	if (!res) {
		return TNRC_API_ERROR_INTERNAL_ERROR;
	}
	//check if this is the first action requested or if the queue is empty
	if ((TNRC_MASTER.api_queue_tot_request() == 1) ||
	    (TNRC_MASTER.api_queue_size()        == 1)) {
		//Add an event thread to the master of threads to process
		//the queue
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_OPEXT_API::tnrcap_destroy_xc(tnrcap_cookie_t      cookie,
				  tnrc_boolean_t       deactivate,
				  long                 response_ctxt)
{
	destroy_xc_param_t  * param;
	api_queue_element_t * element;
	bool                  res;

	if (TNRC_MASTER.getTNRC() == NULL) {
		TNRC_ERR ("No TNRC instance running. "
			  "Discard request of DESTROY XC");
		return TNRC_API_ERROR_NO_INSTANCE;
	}

	param = new_destroy_xc_element(cookie,
				       deactivate,
				       response_ctxt);

	element = new api_queue_element_t;

	element->type         = DESTROY_XC;
	element->action_param = param;

	//Insert the element relative to this action in the queue
	//of actions to execute
	res = TNRC_MASTER.api_queue_insert (element);
	if (!res) {
		return TNRC_API_ERROR_INTERNAL_ERROR;
	}
	//check if this is the first action requested or if the queue is empty
	if ((TNRC_MASTER.api_queue_tot_request() == 1) ||
	    (TNRC_MASTER.api_queue_size()        == 1)) {
		//Add an event thread to the master of threads
		//to process the queue
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_OPEXT_API::tnrcap_reserve_xc(tnrcap_cookie_t *    cookiep,
				  g2mpls_addr_t        dl_id_in,
				  label_t              labelid_in,
				  g2mpls_addr_t        dl_id_out,
				  label_t              labelid_out,
				  xcdirection_t        direction,
				  tnrc_boolean_t       is_virtual,
				  tnrc_boolean_t       advance_rsrv,
				  long                 start,
				  long                 end,
				  long                 response_ctxt)
{
	TNRC::TNRC_AP       * t;
	reserve_xc_param_t  * param;
	api_queue_element_t * element;
	Plugin              * p;
	tnrc_port_id_t        tnrc_p_id_in;
	tnrc_port_id_t        tnrc_p_id_out;
	eqpt_id_t             e_id;
	board_id_t            b_id;
	port_id_t             p_id;
	eqpt_type_t           type;
	bool                  res;
	tnrc_boolean_t        virt_flag_in = 0;
	tnrc_boolean_t        virt_flag_out = 0;

	//check if a TNRC instance is running
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		TNRC_ERR ("No TNRC instance running. Discard request of "
			  "RESERVE XC");
		return TNRC_API_ERROR_NO_INSTANCE;
	}
	//check if equipment link is down
	if (t->eqpt_link_down()) {
		TNRC_ERR ("Equipment link is down. Discard request of "
			  "RESERVE XC");
		return TNRC_API_ERROR_INTERNAL_ERROR;
	}
	//Check consistency for this request
	if (advance_rsrv) {
		if ((!advance_rsrv_check_consistency(dl_id_in,
						     labelid_in,
						     start,
						     end,
						     is_virtual)) ||
		    (!advance_rsrv_check_consistency(dl_id_out,
						     labelid_out,
						     start,
						     end,
						     is_virtual))) {
			    TNRC_ERR("ADVANCE RESERVATION parameters' "
				     "incompatible with data model");
			    return TNRC_API_ERROR_INVALID_PARAMETER;
		    }
	} else  {
		if ((!xc_check_consistency(dl_id_in,
					   labelid_in,
					   RESERVE_XC,
					   0,
					   is_virtual)) ||
		    (!xc_check_consistency(dl_id_out,
					   labelid_out,
					   RESERVE_XC,
					   0,
					   is_virtual))) {
			TNRC_ERR("RESERVE XC parameters' incompatible "
				 "with data model");
			return TNRC_API_ERROR_INVALID_PARAMETER;
		}
	}

	//Convert dl_ids and check if there's a Plugin for this operation
	TNRC_CONF_API::convert_dl_id(&tnrc_p_id_in, dl_id_in);
	TNRC_CONF_API::convert_dl_id(&tnrc_p_id_out, dl_id_out);
	TNRC_CONF_API::convert_tnrc_port_id(&e_id, &b_id, &p_id, tnrc_p_id_in);
	type = TNRC_MASTER.getEqpt_type(e_id);

	p = TNRC_MASTER.getPC()->getPlugin(EQPT_TYPE2NAME(type));
	if (p == NULL) {
		TNRC_ERR ("There's no plugin for the operation requested.");
		return TNRC_API_ERROR_NO_PLUGIN;
	}
	param = new_reserve_xc_element(cookiep,
				       tnrc_p_id_in,
				       labelid_in,
				       tnrc_p_id_out,
				       labelid_out,
				       direction,
				       is_virtual,
				       advance_rsrv,
				       start,
				       end,
				       response_ctxt);
	//Set correct Plugin for operation requested
	param->plugin = p;

	element = new api_queue_element_t;

	element->type = RESERVE_XC;
	element->action_param = param;

	//Insert the element relative to this action in the queue of actions
	//to execute
	res = TNRC_MASTER.api_queue_insert (element);
	if (!res) {
		return TNRC_API_ERROR_INTERNAL_ERROR;
	}
	//check if this is the first action requested or if the queue is empty
	if ((TNRC_MASTER.api_queue_tot_request() == 1) ||
	    (TNRC_MASTER.api_queue_size()        == 1)) {
		//Add an event thread to the master of threads to process
		//the queue
		thread_add_event (TNRC_MASTER.getMaster(),
			          process_api_queue, NULL, 0);
	}

	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_OPEXT_API::tnrcap_unreserve_xc(tnrcap_cookie_t      cookie,
				    long                 response_ctxt)
{
	unreserve_xc_param_t * param;
	api_queue_element_t  * element;
	bool                   res;

	if (TNRC_MASTER.getTNRC() == NULL) {
		TNRC_ERR ("No TNRC instance running. Discard request of "
			  "UNRESERVE XC");
		return TNRC_API_ERROR_NO_INSTANCE;
	}

	param = new_unreserve_xc_element(cookie,
					 response_ctxt);

	element = new api_queue_element_t;

	element->type = UNRESERVE_XC;
	element->action_param = param;

	//Insert the element relative to this action in the queue
	//of actions to execute
	res = TNRC_MASTER.api_queue_insert (element);
	if (!res) {
		return TNRC_API_ERROR_INTERNAL_ERROR;
	}
	//check if this is the first action requested or if the queue is empty
	if ((TNRC_MASTER.api_queue_tot_request() == 1) ||
	    (TNRC_MASTER.api_queue_size()        == 1)) {
		//Add an event thread to the master of threads
		//to process the queue
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_OPEXT_API::tnrcap_get_eqpt_details(g2mpls_addr_t & eqpt_addr,
					eqpt_type_t &   type)
{
	TNRC::TNRC_AP                 * t;
	TNRC::Eqpt                    * e;
	TNRC::TNRC_AP::iterator_eqpts   iter;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		return TNRC_API_ERROR_NO_INSTANCE;
	}
	//Check the presence of an EQPT object
	if (t->n_eqpts() == 0) {
		return TNRC_API_ERROR_GENERIC;
	}
	iter= t->begin_eqpts();
	e = (*iter).second;
	if (e == NULL) {
		return TNRC_API_ERROR_GENERIC;
	}
	eqpt_addr = e->address();
	type = e->type();

	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_OPEXT_API::tnrcap_get_dl_list(std::list<g2mpls_addr_t> & dl_list)
{
	TNRC::TNRC_AP                 * t;
	TNRC::Eqpt                    * e;
	TNRC::Board                   * b;
	TNRC::Port                    * p;
	tnrc_port_id_t                  tnrc_port_id;
	eqpt_id_t                       eqpt_id;
	board_id_t                      board_id;
	port_id_t                       port_id;
	g2mpls_addr_t                   dl_id;
	TNRC::TNRC_AP::iterator_eqpts   iter_e;
	TNRC::Eqpt::iterator_boards     iter_b;
	TNRC::Board::iterator_ports     iter_p;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		return TNRC_API_ERROR_NO_INSTANCE;
	}
	//Check the presence of an EQPT object
	if (t->n_eqpts() == 0) {
		return TNRC_API_ERROR_GENERIC;
	}
	//Fetch all data links in the data model
	iter_e = t->begin_eqpts();
	eqpt_id = (*iter_e).first;
	e = (*iter_e).second;
	if (e == NULL) {
		return TNRC_API_ERROR_GENERIC;
	}
	for(iter_b = e->begin_boards(); iter_b != e->end_boards(); iter_b++) {
		board_id = (*iter_b).first;
		b = (*iter_b).second;
		for(iter_p = b->begin_ports();
		    iter_p != b->end_ports();
		    iter_p++) {
			port_id = (*iter_p).first;
			p = (*iter_p).second;
			tnrc_port_id = TNRC_CONF_API::get_tnrc_port_id(eqpt_id,
								       board_id,
								       port_id);
			dl_id = TNRC_CONF_API::get_dl_id(tnrc_port_id);

			dl_list.push_back(dl_id);
		}
	}

	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_OPEXT_API::tnrcap_get_dl_details(g2mpls_addr_t           dl_id,
				      tnrc_api_dl_details_t & details)
{
	TNRC::TNRC_AP *               t;
	TNRC::Eqpt    *               e;
	TNRC::Board   *               b;
	TNRC::Port    *               p;
	tnrc_port_id_t                tnrc_port_id;
	eqpt_id_t                     eqpt_id;
	board_id_t                    board_id;
	port_id_t                     port_id;
	std::map<long, bw_per_prio_t> calendar;
	uint32_t                      start_time;
	struct timeval                current_time;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		return TNRC_API_ERROR_NO_INSTANCE;
	}
	TNRC_CONF_API::convert_dl_id(&tnrc_port_id, dl_id);
	TNRC_CONF_API::convert_tnrc_port_id(&eqpt_id,
					    &board_id,
					    &port_id,
					    tnrc_port_id);
	p = t->getPort(eqpt_id, board_id, port_id);
	if (p == NULL) {
		return TNRC_API_ERROR_INVALID_PARAMETER;
	}
	e = t->getEqpt(eqpt_id);
	b = t->getBoard(eqpt_id, board_id);

	//Fill datalink details structure
	details.eqpt_addr     = e->address();
	details.rem_eqpt_addr = p->remote_eqpt_address();
	details.rem_port_id   = p->remote_port_id();
	details.opstate       = p->opstate();
	details.sw_cap        = b->sw_cap();
	details.enc_type      = b->enc_type();
	details.tot_bw        = p->max_bw();
	details.max_res_bw    = p->max_res_bw();
	details.unres_bw      = p->unres_bw();
	details.min_lsp_bw    = p->min_lsp_bw();
	details.max_lsp_bw    = p->max_lsp_bw();
	details.prot_type     = p->prot_type();
	if (b->sw_cap() == SWCAP_LSC) {
		// Fill wavelength bitmask structure
		details.bitmap = p->dwdm_lambdas_bitmap();
	}
	calendar.clear();
	gettimeofday(&current_time, NULL);
	start_time = (uint32_t) current_time.tv_sec;

	p->get_calendar_map(TNRC_DEFAULT_NUM_EVENTS,
			    start_time,
			    TNRC_DEFAULT_QUANTUM,
			    calendar);
        if (calendar.size() == 0) {
	        TNRC_WRN("Empty advance reservation calendar");
	}

	details.calendar     = calendar;

	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_OPEXT_API::tnrcap_get_dl_calendar(g2mpls_addr_t          dl_id,
				       uint32_t               start_time,
				       uint32_t               num_events,
				       uint32_t               quantum,
				       std::map<long,
				       bw_per_prio_t> & calendar)
{
	TNRC::TNRC_AP                  * t;
	TNRC::Port                     * p;
	tnrc_port_id_t                   tnrc_port_id;
	eqpt_id_t                        eqpt_id;
	board_id_t                       board_id;
	port_id_t                        port_id;

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		return TNRC_API_ERROR_NO_INSTANCE;
	}
	TNRC_CONF_API::convert_dl_id(&tnrc_port_id, dl_id);
	TNRC_CONF_API::convert_tnrc_port_id(&eqpt_id,
					    &board_id,
					    &port_id,
					    tnrc_port_id);
	p = t->getPort(eqpt_id, board_id, port_id);
	if (p == NULL) {
		return TNRC_API_ERROR_INVALID_PARAMETER;
	}

	calendar.clear();
	p->get_calendar_map(num_events, start_time, quantum, calendar);
	if (calendar.size() == 0) {
		return TNRC_API_ERROR_GENERIC;
	}

	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_OPEXT_API::tnrcap_get_label_list(g2mpls_addr_t                 dl_id,
			              std::list<tnrc_api_label_t> & label_list)
{
	TNRC::TNRC_AP                  * t;
	TNRC::Port                     * p;
	TNRC::Resource                 * r;
	tnrc_port_id_t                   tnrc_port_id;
	eqpt_id_t                        eqpt_id;
	board_id_t                       board_id;
	port_id_t                        port_id;
	TNRC::Port::iterator_resources   iter;
	tnrc_api_label_t                 label_details;

	bzero(&label_details, sizeof(tnrc_api_label_t));

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		return TNRC_API_ERROR_NO_INSTANCE;
	}
	TNRC_CONF_API::convert_dl_id(&tnrc_port_id, dl_id);
	TNRC_CONF_API::convert_tnrc_port_id(&eqpt_id,
					    &board_id,
					    &port_id,
					    tnrc_port_id);
	p = t->getPort(eqpt_id, board_id, port_id);
	if (p == NULL) {
		return TNRC_API_ERROR_INVALID_PARAMETER;
	}
	for(iter = p->begin_resources(); iter != p->end_resources(); iter++) {
		r = (*iter).second;
		label_details.label_id = r->label_id();
		label_details.opstate  = r->opstate();
		label_details.state    = r->state();
		label_list.push_back(label_details);
	}
	if (label_list.size() == 0) {
		return TNRC_API_ERROR_INTERNAL_ERROR;
	}
	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_OPEXT_API::tnrcap_get_label_details(g2mpls_addr_t   dl_id,
			                 label_t         label_id,
			                 opstate_t     & opstate,
			                 label_state_t & label_state)
{
	TNRC::TNRC_AP                  * t;
	TNRC::Port                     * p;
	TNRC::Resource                 * r;
	tnrc_port_id_t                   tnrc_port_id;
	eqpt_id_t                        eqpt_id;
	board_id_t                       board_id;
	port_id_t                        port_id;
	TNRC::Port::iterator_resources   iter;
	tnrc_api_label_t                 label_details;

	bzero(&label_details, sizeof(tnrc_api_label_t));

	//Get instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		return TNRC_API_ERROR_NO_INSTANCE;
	}
	TNRC_CONF_API::convert_dl_id(&tnrc_port_id, dl_id);
	TNRC_CONF_API::convert_tnrc_port_id(&eqpt_id,
					    &board_id,
					    &port_id,
					    tnrc_port_id);
	p = t->getPort(eqpt_id, board_id, port_id);
	if (p == NULL) {
		return TNRC_API_ERROR_INVALID_PARAMETER;
	}
	for(iter = p->begin_resources(); iter != p->end_resources(); iter++) {
		r = (*iter).second;
		if(ARE_LABEL_EQUAL(r->label_id(), label_id)) {
			opstate     = r->opstate();
			label_state = r->state();
			return TNRC_API_ERROR_NONE;
		}
	}
	return TNRC_API_ERROR_INVALID_PARAMETER;
}


////////////////////////////////////////////////////////////
/////////////TNRC_OPSPEC_API////////////////////////////////
////////////////////////////////////////////////////////////

void
TNRC_OPSPEC_API::make_xc_resp_cb(tnrcsp_handle_t handle,
				 tnrcsp_result_t result,
				 void *          ctxt)
{
	Action * a;

	//Set correctly the equipment response in the relative action
	a = static_cast<Action*>(ctxt);
	a->atomic()->eqpt_resp (result);

	////TMP DEBUG LOG
	TNRC_DBG("make_xc_resp_cb: received pseudo-synch "
		 "notification for MAKE XC. "
		 "Result: %s", SHOW_TNRCSP_RESULT(result));
	////////////////

	switch (result) {
		case TNRCSP_RESULT_NOERROR:
			TNRC_DBG("Post an AtomicActionOk event to the MAKE XC "
				 "action FSM");
			a->fsm_post (fsm::TNRC::virtFsm::TNRC_AtomicActionOk,
				     ctxt);
			break;
		case TNRCSP_RESULT_EQPTLINKDOWN:
			TNRC_DBG("Post an EqptDown event to the MAKE XC "
				 "action FSM");
			a->fsm_post (fsm::TNRC::virtFsm::TNRC_EqptDown,
				     ctxt);
			break;
		default:
			TNRC_DBG("Post an AtomicActionKo event to the MAKE XC "
				 "action FSM");
			a->fsm_post (fsm::TNRC::virtFsm::TNRC_AtomicActionKo,
				     ctxt);
			break;
	}
}

void
TNRC_OPSPEC_API::destroy_xc_resp_cb(tnrcsp_handle_t handle,
				    tnrcsp_result_t result,
				    void *          ctxt)
{
	Action * a;

	//Set correctly the equipment response in the relative action
	a = static_cast<Action*>(ctxt);
	a->atomic()->eqpt_resp (result);

	////TMP DEBUG LOG
	TNRC_DBG("destroy_xc_resp_cb: received pseudo-synch "
		 "notification for DESTROY XC. "
		 "Result: %s", SHOW_TNRCSP_RESULT(result));
	////////////////

	switch (result) {
		case TNRCSP_RESULT_NOERROR:
			TNRC_DBG("Post an AtomicActionOk event to the DESTROY XC "
				 "action FSM");
			a->fsm_post (fsm::TNRC::virtFsm::TNRC_AtomicActionOk,
				     ctxt);
			break;
		default:
			TNRC_DBG("Post an AtomicActionKo event to the DESTROY XC "
				 "action FSM");
			a->fsm_post (fsm::TNRC::virtFsm::TNRC_AtomicActionKo,
				     ctxt);
			break;
	}

}

void
TNRC_OPSPEC_API::notification_xc_cb(tnrcsp_handle_t         handle,
				    tnrcsp_resource_id_t ** failed_resource_listp,
				    void *                  ctxt)

{
	Action * a;

	//Get the Action
	a = static_cast<Action*>(ctxt);

	TNRC_DBG ("Notification from equipment received. "
		  "Updating data model..");

	//Post an ActionNotification event
	a->fsm_post (fsm::TNRC::virtFsm::TNRC_ActionNotification,
		     * failed_resource_listp);

	//Call the client notification method
	//TODO --> fill a data structure to notify events occurred to the client
	void *notify;
	//
	TNRC_DBG ("Send notification to client");
	async_notification(a->ap_cookie(), &notify, a->async_ctxt());

}

void
TNRC_OPSPEC_API::reserve_xc_resp_cb(tnrcsp_handle_t handle,
				    tnrcsp_result_t result,
				    void *          ctxt)
{
	Action * a;

	//Set correctly the equipment response in the relative action
	a = static_cast<Action*>(ctxt);
	a->atomic()->eqpt_resp (result);

	////TMP DEBUG LOG
	TNRC_DBG("reserve_xc_resp_cb: received pseudo-synch "
		 "notification for RESERVE XC. "
		 "Result: %s", SHOW_TNRCSP_RESULT(result));
	////////////////

	switch (result) {
		case TNRCSP_RESULT_NOERROR:
			TNRC_DBG("Post an AtomicActionOk event to the RESERVE XC "
				 "action FSM");
			a->fsm_post (fsm::TNRC::virtFsm::TNRC_AtomicActionOk,
				     ctxt);
			break;
		case TNRCSP_RESULT_EQPTLINKDOWN:
			TNRC_DBG("Post an EqptDown  event to the RESERVE XC "
				 "action FSM");
			a->fsm_post (fsm::TNRC::virtFsm::TNRC_EqptDown, ctxt);
			break;
		default:
			TNRC_DBG("Post an AtomicActionKo event to the RESERVE XC "
				 "action FSM");
			a->fsm_post (fsm::TNRC::virtFsm::TNRC_AtomicActionKo,
				     ctxt);
			break;
	}

}

void
TNRC_OPSPEC_API::unreserve_xc_resp_cb(tnrcsp_handle_t handle,
				      tnrcsp_result_t result,
				      void *          ctxt)
{
	Action * a;

	//Set correctly the equipment response in the relative action
	a = static_cast<Action*>(ctxt);
	a->atomic()->eqpt_resp (result);

	////TMP DEBUG LOG
	TNRC_DBG("unreserve_xc_resp_cb: received pseudo-synch "
		 "notification for UNRESERVE XC. "
		 "Result: %s", SHOW_TNRCSP_RESULT(result));
	////////////////

	switch (result) {
		case TNRCSP_RESULT_NOERROR:
			TNRC_DBG("Post an AtomicActionOk event to the UNRESERVE XC "
				 "action FSM");
			a->fsm_post(fsm::TNRC::virtFsm::TNRC_AtomicActionOk,
				    ctxt);
			break;
		default:
			TNRC_DBG("Post an AtomicActionKo event to the UNRESERVE XC "
				 "action FSM");
			a->fsm_post(fsm::TNRC::virtFsm::TNRC_AtomicActionKo,
				    ctxt);
			break;
	}

}

void
TNRC_OPSPEC_API::update_dm_after_action(Action * action)
{
	Action * atomic;
	XC     * xc;
	bool     upd_unres_bw;

	assert(action);
	//Get atomic action
	atomic = action->atomic();
	assert(atomic);

	switch (atomic->action_type()) {
		case MAKE_XC: {
			//Get action context
			MakeDestroyxc * make_ctxt;
			make_ctxt = dynamic_cast<MakeDestroyxc*>(atomic);
			if (make_ctxt->rsrv_xc_flag() == NO_RSRV) {
				//create new XC object
				xc = new XC (make_ctxt->xc_id(),
					     make_ctxt->ap_cookie(),
					     XCONN,
					     make_ctxt->portid_in(),
					     make_ctxt->labelid_in(),
					     make_ctxt->portid_out(),
					     make_ctxt->labelid_out(),
					     make_ctxt->direction(),
					     make_ctxt->async_ctxt());
				//attach XC to the relative map
				TNRC_MASTER.attach_xc(xc->id(), xc);
				TNRC_DBG ("Attached a new XC object to the "
					  "map of XCs");
				upd_unres_bw = true;
			}
			else {
				//get correct XC object
				xc = TNRC_MASTER.getXC(make_ctxt->xc_id());
				if (xc == NULL) {
					TNRC_WRN("XC with id %d isn't present, "
						 "could not update it",
						 make_ctxt->xc_id());
				}
				else {
					//update XC status
					xc->state(XCONN);
					//update XC cookie
					xc->cookie(make_ctxt->ap_cookie());
					//update callback parameters
					xc->async_ctxt(make_ctxt->async_ctxt());
					TNRC_DBG ("Updated XC object in the "
						  "map of XCs");
					upd_unres_bw = false;
				}
			}
			//update data model
			update_resource	(make_ctxt->portid_in(),
					 make_ctxt->labelid_in(),
					 LABEL_XCONNECTED,
					 EVENT_NULL,
					 upd_unres_bw);
			update_resource	(make_ctxt->portid_out(),
					 make_ctxt->labelid_out(),
					 LABEL_XCONNECTED,
					 EVENT_NULL,
					 upd_unres_bw);
			break;
		}
		case DESTROY_XC: {
			//Get action context
			MakeDestroyxc      * dest_ctxt;
			label_state_t        label_state;
			dest_ctxt = dynamic_cast<MakeDestroyxc*>(atomic);
			//get correct XC object
			xc = TNRC_MASTER.getXC(dest_ctxt->xc_id());
			if (xc == NULL) {
				TNRC_WRN("XC with id %d isn't present, could "
					 "not destroy or update it",
					 dest_ctxt->xc_id());
			}
			if (dest_ctxt->rsrv_xc_flag() == NO_RSRV) {
				label_state = LABEL_FREE;
				if (xc) {
					//detach and delete XC objects
					TNRC_MASTER.detach_xc (xc->id());
					delete xc;
					TNRC_DBG ("Deleted XC object from the "
						  "map of XCs");
					upd_unres_bw = true;
				}
			}
			else {
				label_state = LABEL_BOOKED;
				if (xc) {
					//update XC status
					xc->state(RESERVED);
					//update XC cookie
					if (dest_ctxt->rsrv_cookie() != 0) {
						xc->cookie(dest_ctxt->
							   rsrv_cookie());
					}
					TNRC_DBG ("Updated XC object in the "
						  "map of XCs");
					upd_unres_bw = false;
				}
			}
			//update data model
			update_resource	(dest_ctxt->portid_in(),
					 dest_ctxt->labelid_in(),
					 label_state,
					 EVENT_NULL,
					 upd_unres_bw);
			update_resource	(dest_ctxt->portid_out(),
					 dest_ctxt->labelid_out(),
					 label_state,
					 EVENT_NULL,
					 upd_unres_bw);
			break;
		}
		case RESERVE_XC: {
			//Get action context
			ReserveUnreservexc * reserve_ctxt;
			reserve_ctxt = dynamic_cast<ReserveUnreservexc*>
				(atomic);
			//create new XC object, only if this reservation isn't
			//a deactivation
			if (!reserve_ctxt->deactivation()) {
				xc = new XC (reserve_ctxt->xc_id(),
					     reserve_ctxt->ap_cookie(),
					     RESERVED,
					     reserve_ctxt->portid_in(),
					     reserve_ctxt->labelid_in(),
					     reserve_ctxt->portid_out(),
					     reserve_ctxt->labelid_out(),
					     reserve_ctxt->direction(),
					     0);
				//attach XC to the relative map
				TNRC_MASTER.attach_xc (xc->id(), xc);
				TNRC_DBG ("Attached a new XC object to the "
				          "map of XCs");
			}
			upd_unres_bw = true;
			//update data model
			update_resource	(reserve_ctxt->portid_in(),
					 reserve_ctxt->labelid_in(),
					 LABEL_BOOKED,
					 EVENT_NULL,
					 upd_unres_bw);
			update_resource	(reserve_ctxt->portid_out(),
					 reserve_ctxt->labelid_out(),
					 LABEL_BOOKED,
					 EVENT_NULL,
					 upd_unres_bw);
			break;
		}
		case UNRESERVE_XC: {
			//Get action context
			ReserveUnreservexc * unreserve_ctxt;
			unreserve_ctxt = dynamic_cast<ReserveUnreservexc*>
				(atomic);
			//get correct XC object
			xc = TNRC_MASTER.getXC(unreserve_ctxt->xc_id());
			if (xc == NULL) {
				TNRC_WRN("XC with id %d isn't present, cannot "
					 "destroy or update it",
					 unreserve_ctxt->xc_id());
			}
			else {
				//detach and delete XC objects
				TNRC_MASTER.detach_xc(xc->id());
				delete xc;
				TNRC_DBG ("Deleted XC object from the "
					  "map of XCs");
			}
			upd_unres_bw = true;
			//update data model
			update_resource	(unreserve_ctxt->portid_in(),
					 unreserve_ctxt->labelid_in(),
					 LABEL_FREE,
					 EVENT_NULL,
					 upd_unres_bw);
			update_resource	(unreserve_ctxt->portid_out(),
					 unreserve_ctxt->labelid_out(),
					 LABEL_FREE,
					 EVENT_NULL,
					 upd_unres_bw);
			break;
		}
	}

}

void
TNRC_OPSPEC_API::update_dm_after_notify(tnrc_port_id_t  portid,
					label_t         labelid,
					tnrcsp_evmask_t event)
{
	switch (event) {
		case EVENT_EQPT_SYNC_LOST:
		case EVENT_EQPT_OPSTATE_UP:
		case EVENT_EQPT_OPSTATE_DOWN:
		case EVENT_EQPT_ADMSTATE_ENABLED:
		case EVENT_EQPT_ADMSTATE_DISABLED:
			update_eqpt (portid, event);
			break;
		case EVENT_BOARD_OPSTATE_UP:
		case EVENT_BOARD_OPSTATE_DOWN:
		case EVENT_BOARD_ADMSTATE_ENABLED:
		case EVENT_BOARD_ADMSTATE_DISABLED:
			update_board (portid, event);
			break;
		case EVENT_PORT_OPSTATE_UP:
		case EVENT_PORT_OPSTATE_DOWN:
		case EVENT_PORT_ADMSTATE_ENABLED:
		case EVENT_PORT_ADMSTATE_DISABLED:
		case EVENT_PORT_XCONNECTED:
		case EVENT_PORT_XDISCONNECTED:
			update_port (portid, event);
			break;
		case EVENT_RESOURCE_OPSTATE_UP:
		case EVENT_RESOURCE_OPSTATE_DOWN:
		case EVENT_RESOURCE_ADMSTATE_ENABLED:
		case EVENT_RESOURCE_ADMSTATE_DISABLED:
			update_resource (portid, labelid,
					 LABEL_UNDEFINED, event,
					 false);
			break;
		case EVENT_RESOURCE_XCONNECTED:
		case EVENT_RESOURCE_XDISCONNECTED:
			update_resource (portid, labelid,
					 LABEL_UNDEFINED, event,
					 true);
			break;
	}

}

void
TNRC_OPSPEC_API::update_eqpt(tnrc_port_id_t  portid,
			     tnrcsp_evmask_t event)
{
	eqpt_id_t                     e_id;
	board_id_t                    b_id;
	port_id_t                     p_id;
	TNRC::TNRC_AP               * t;
	TNRC::Eqpt                  * e;
	TNRC::Eqpt::iterator_boards   iter;
	TNRC::Board                 * b;

	TNRC_CONF_API::convert_tnrc_port_id(&e_id, &b_id, &p_id, portid);

	//get TNRC instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		TNRC_WRN ("update_eqpt: no TNRC instance is running. "
			  "Update failed.");
		return;
	}
	//get equipment
	e = t->getEqpt (e_id);
	if (e == NULL) {
		TNRC_WRN ("update_eqpt: equipment to update not found.");
		return;
	}
	//update equipment
	switch (event) {
		case EVENT_EQPT_SYNC_LOST:
			t->eqpt_link_down(true);
			TNRC_DBG ("EVENT_EQPT_SYNC_LOST on eqpt %d", e_id);
			break;
		case EVENT_EQPT_OPSTATE_UP:
			e->opstate(UP);
			TNRC_DBG ("EVENT_EQPT_OPSTATE_UP on eqpt %d", e_id);
			//update all associated boards
			TNRC_DBG ("Update all associated boards");
			for (iter  = e->begin_boards();
			     iter != e->end_boards();
			     iter++) {
				b = (*iter).second;
				portid = TNRC_CONF_API::get_tnrc_port_id(e_id,
									 b->board_id(),
									 0);
				update_board(portid, EVENT_BOARD_OPSTATE_UP);
			}
			break;
		case EVENT_EQPT_OPSTATE_DOWN:
			e->opstate(DOWN);
			TNRC_DBG ("EVENT_EQPT_OPSTATE_DOWN on eqpt %d", e_id);
			//update all associated boards
			TNRC_DBG ("Update all associated boards");
			for (iter = e->begin_boards();
			     iter != e->end_boards();
			     iter++) {
				b = (*iter).second;
				portid = TNRC_CONF_API::get_tnrc_port_id(e_id,
									 b->board_id(),
									 0);
				update_board(portid, EVENT_BOARD_OPSTATE_DOWN);
			}
			break;
		case EVENT_EQPT_ADMSTATE_ENABLED:
			e->admstate(ENABLED);
			TNRC_DBG ("EVENT_EQPT_ADMSTATE_ENABLED on eqpt %d",
				  e_id);
			//update all associated boards
			TNRC_DBG ("Update all associated boards");
			for (iter = e->begin_boards();
			     iter != e->end_boards();
			     iter++) {
				b = (*iter).second;
				portid = TNRC_CONF_API::
					get_tnrc_port_id(e_id,
							 b->board_id(),
							 0);
				update_board (portid,
					      EVENT_BOARD_ADMSTATE_ENABLED);
			}
			break;
		case EVENT_EQPT_ADMSTATE_DISABLED:
			e->admstate(DISABLED);
			TNRC_DBG ("EVENT_EQPT_ADMSTATE_DISABLED on eqpt %d",
				  e_id);
			//update all associated boards
			TNRC_DBG ("Update all associated boards");
			for (iter = e->begin_boards();
			     iter != e->end_boards();
			     iter++) {
				b = (*iter).second;
				portid = TNRC_CONF_API::
					get_tnrc_port_id(e_id,
							 b->board_id(),
							 0);
				update_board(portid,
					     EVENT_BOARD_ADMSTATE_DISABLED);
			}
			break;
	}

}

void
TNRC_OPSPEC_API::update_board(tnrc_port_id_t  portid,
			      tnrcsp_evmask_t event)
{
	eqpt_id_t                     e_id;
	board_id_t                    b_id;
	port_id_t                     p_id;
	TNRC::TNRC_AP               * t;
	TNRC::Board                 * b;
	TNRC::Board::iterator_ports   iter;
	TNRC::Port                  * p;

	TNRC_CONF_API::convert_tnrc_port_id(&e_id, &b_id, &p_id, portid);

	//get TNRC instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		TNRC_WRN ("update_board: no TNRC instance is running. "
			  "Update failed.");
		return;
	}
	//get equipment
	b = t->getBoard (e_id, b_id);
	if (b == NULL) {
		TNRC_WRN ("update_board: board to update not found.");
		return;
	}
	//update equipment
	switch (event) {
		case EVENT_BOARD_OPSTATE_UP:
			b->opstate(UP);
			TNRC_DBG ("EVENT_BOARD_OPSTATE_UP on board %d "
				  "(eqpt %d)", b_id, e_id);
			//update all associated ports
			TNRC_DBG ("Update all associated ports");
			for (iter = b->begin_ports();
			     iter != b->end_ports();
			     iter++) {
				p = (*iter).second;
				portid = TNRC_CONF_API::
					get_tnrc_port_id(e_id,
							 b_id,
							 p->port_id());
				update_port(portid,
					    EVENT_PORT_OPSTATE_UP);
			}
			break;
		case EVENT_BOARD_OPSTATE_DOWN:
			b->opstate(DOWN);
			TNRC_DBG ("EVENT_BOARD_OPSTATE_DOWN on board %d "
				  "(eqpt %d)", b_id, e_id);
			//update all associated ports
			TNRC_DBG ("Update all associated ports");
			for (iter = b->begin_ports();
			     iter != b->end_ports();
			     iter++) {
				p = (*iter).second;
				portid = TNRC_CONF_API::
					get_tnrc_port_id(e_id,
							 b_id,
							 p->port_id());
				update_port (portid,
					     EVENT_PORT_OPSTATE_DOWN);
			}
			break;
		case EVENT_BOARD_ADMSTATE_ENABLED:
			b->admstate(ENABLED);
			TNRC_DBG("EVENT_BOARD_ADMSTATE_ENABLED on board %d "
				 "(eqpt %d)", b_id, e_id);
			//update all associated ports
			TNRC_DBG ("Update all associated ports");
			for (iter = b->begin_ports();
			     iter != b->end_ports();
			     iter++) {
				p = (*iter).second;
				portid = TNRC_CONF_API::
					get_tnrc_port_id(e_id,
							 b_id,
							 p->port_id());
				update_port (portid,
					     EVENT_PORT_ADMSTATE_ENABLED);
			}
			break;
		case EVENT_BOARD_ADMSTATE_DISABLED:
			b->admstate(DISABLED);
			TNRC_DBG ("EVENT_BOARD_ADMSTATE_DISABLED on board %d "
				  "(eqpt %d)", b_id, e_id);
			//update all associated ports
			TNRC_DBG ("Update all associated ports");
			for (iter = b->begin_ports();
			     iter != b->end_ports();
			     iter++) {
				p = (*iter).second;
				portid = TNRC_CONF_API::
					get_tnrc_port_id(e_id,
							 b_id,
							 p->port_id());
				update_port (portid,
					     EVENT_PORT_ADMSTATE_DISABLED);
			}
			break;
	}

}

void
TNRC_OPSPEC_API::update_port(tnrc_port_id_t  portid,
			     tnrcsp_evmask_t event)
{
	eqpt_id_t                        e_id;
	board_id_t                       b_id;
	port_id_t                        p_id;
	TNRC::TNRC_AP                  * t;
	TNRC::Port                     * p;
	TNRC::Port::iterator_resources   iter;
	TNRC::Resource                 * r;

	TNRC_CONF_API::convert_tnrc_port_id(&e_id, &b_id, &p_id, portid);

	//get TNRC instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		TNRC_WRN ("update_port: no TNRC instance is running. "
			  "Update failed.");
		return;
	}
	//get equipment
	p = t->getPort (e_id, b_id, p_id);
	if (p == NULL) {
		TNRC_WRN ("update_port: board to update not found.");
		return;
	}
	//update equipment
	switch (event) {
		case EVENT_PORT_OPSTATE_UP:
			p->opstate(UP);
			TNRC_DBG ("EVENT_PORT_OPSTATE_UP on port %d "
				  "(board %d eqpt %d)", p_id, b_id, e_id);
			//advertise LRM of parameters changes
			update_datalink_details(p);
//			//update all associated resources
//			TNRC_DBG ("Update all associated resources");
//			for (iter = p->begin_resources();
//			     iter != p->end_resources();
//			     iter++) {
//				r = (*iter).second;
//				update_resource(portid,
//						r->label_id(),
//						LABEL_UNDEFINED,
//						EVENT_RESOURCE_OPSTATE_UP,
//						false);
//			}
			break;
		case EVENT_PORT_OPSTATE_DOWN:
			p->opstate(DOWN);
			TNRC_DBG ("EVENT_PORT_OPSTATE_DOWN on port %d "
			          "(board %d eqpt %d)", p_id, b_id, e_id);
			//advertise LRM of parameters changes
			update_datalink_details(p);
//			//update all associated resources
//			TNRC_DBG ("Update all associated resources");
//			for (iter = p->begin_resources();
//			     iter != p->end_resources();
//			     iter++) {
//				r = (*iter).second;
//				update_resource(portid,
//						r->label_id(),
//						LABEL_UNDEFINED,
//						EVENT_RESOURCE_OPSTATE_DOWN,
//						false);
//			}
			break;
		case EVENT_PORT_ADMSTATE_ENABLED:
			p->admstate(ENABLED);
			TNRC_DBG ("EVENT_PORT_ADMSTATE_ENABLED on port %d "
				  "(board %d eqpt %d)", p_id, b_id, e_id);
			//advertise LRM of parameters changes
			update_datalink_details(p);
//			//update all associated resources
//			TNRC_DBG ("Update all associated resources");
//			for (iter = p->begin_resources();
//			     iter != p->end_resources();
//			     iter++) {
//				r = (*iter).second;
//				update_resource(portid,
//						r->label_id(),
//						LABEL_UNDEFINED,
//						EVENT_RESOURCE_ADMSTATE_ENABLED,
//						false);
//			}

			break;
		case EVENT_PORT_ADMSTATE_DISABLED:
			p->admstate(DISABLED);
			TNRC_DBG ("EVENT_PORT_ADMSTATE_DISABLED on port %d "
			          "(board %d eqpt %d)", p_id, b_id, e_id);
			//advertise LRM of parameters changes
			update_datalink_details(p);
//			//update all associated resources
//			TNRC_DBG ("Update all associated resources");
//			for (iter = p->begin_resources();
//			     iter != p->end_resources();
//			     iter++) {
//				r = (*iter).second;
//				update_resource(portid,
//						r->label_id(),
//						LABEL_UNDEFINED,
//						EVENT_RESOURCE_ADMSTATE_DISABLED,
//						false);
//			}
			break;
		case EVENT_PORT_XCONNECTED:
			TNRC_DBG ("EVENT_PORT_XCONNECTED on port %d "
			          "(board %d eqpt %d)", p_id, b_id, e_id);
			if (p->board()->sw_cap() == SWCAP_FSC) {
				label_t id;
				memset(&id, 0, sizeof(label_t));
				id.type = LABEL_FSC;
				id.value.fsc.portId = 1;
				TNRC_DBG ("Update associated resource");
				update_resource(portid,
						id,
						LABEL_UNDEFINED,
						EVENT_RESOURCE_XCONNECTED,
						true);
				//advertise LRM of parameters changes
				update_datalink_details(p);
			} else {
				TNRC_WRN ("Switching capability is not FSC: this event "
					  "will not have effect");
			}
			break;
		case EVENT_PORT_XDISCONNECTED:
			TNRC_DBG ("EVENT_PORT_XDISCONNECTED on port %d "
			          "(board %d eqpt %d)", p_id, b_id, e_id);
			if (p->board()->sw_cap() == SWCAP_FSC) {
				label_t id;
				memset(&id, 0, sizeof(label_t));
				id.type = LABEL_FSC;
				id.value.fsc.portId = 1;
				TNRC_DBG ("Update associated resource");
				update_resource(portid,
						id,
						LABEL_UNDEFINED,
						EVENT_RESOURCE_XDISCONNECTED,
						true);
				//advertise LRM of parameters changes
				update_datalink_details(p);
			} else {
				TNRC_WRN ("Switching capability is not FSC: this event "
					  "will not have effect");
			}
			break;
	}

}

void
TNRC_OPSPEC_API::update_resource(tnrc_port_id_t     portid,
				 label_t            labelid,
				 label_state_t      label_state,
				 tnrcsp_evmask_t    event,
				 bool               upd_unres_bw)
{
	eqpt_id_t        e_id;
	board_id_t       b_id;
	port_id_t        p_id;
	TNRC::TNRC_AP  * t;
	TNRC::Port     * p;
	TNRC::Resource * r;

	TNRC_CONF_API::convert_tnrc_port_id(&e_id, &b_id, &p_id, portid);

	//get TNRC instance
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		TNRC_WRN ("update_resource: no TNRC instance is running. "
			  "Update failed.");
		return;
	}
	//get right resource to update
	r = t->getResource(e_id, b_id, p_id, labelid);
	if (r == NULL) {
		TNRC_WRN ("update_resource: resource to update not found.");
		return;
	}
	//update resource
	switch (label_state) {
		case LABEL_UNDEFINED:
			switch (event) {
				case EVENT_RESOURCE_OPSTATE_UP:
					r->opstate(UP);
					TNRC_DBG("EVENT_RESOURCE_OPSTATE_UP "
						 "on label (port %d board %d "
						 "eqpt %d)", p_id, b_id, e_id);
					break;
				case EVENT_RESOURCE_OPSTATE_DOWN:
					r->opstate(DOWN);
					TNRC_DBG("EVENT_RESOURCE_OPSTATE_DOWN "
						 "on label (port %d board %d "
						 "eqpt %d)", p_id, b_id, e_id);
					break;
				case EVENT_RESOURCE_ADMSTATE_ENABLED:
					r->admstate(ENABLED);
					TNRC_DBG("EVENT_RESOURCE_ADMSTATE_ENABLED "
						 "on label (port %d board %d "
						 "eqpt %d)", p_id, b_id, e_id);
					break;
				case EVENT_RESOURCE_ADMSTATE_DISABLED:
					r->admstate(DISABLED);
					TNRC_DBG("EVENT_RESOURCE_ADMSTATE_DISABLED "
						 "on label (port %d board %d "
						 "eqpt %d)", p_id, b_id, e_id);
					break;
				case EVENT_RESOURCE_XCONNECTED:
					r->state(LABEL_XCONNECTED);
					TNRC_DBG("EVENT_RESOURCE_XCONNECTED "
						 "on label (port %d board %d "
						 "eqpt %d)", p_id, b_id, e_id);
					if(upd_unres_bw) {
						r->port()->upd_parameters(r->label_id());
						//advertise LRM of parameters changes
						update_datalink_details(r->port());
					}
					break;
				case EVENT_RESOURCE_XDISCONNECTED:
					r->state(LABEL_FREE);
					TNRC_DBG("EVENT_RESOURCE_XDISCONNECTED "
						 "on label (port %d board %d "
						 "eqpt %d)", p_id, b_id, e_id);
					if(upd_unres_bw) {
						r->port()->upd_parameters(r->label_id());
						//advertise LRM of parameters changes
						update_datalink_details(r->port());
					}
					break;
			}
			break;
		case LABEL_FREE:
		case LABEL_BOOKED:
		case LABEL_XCONNECTED:
		case LABEL_BUSY:
			r->state(label_state);
			if(upd_unres_bw) {
				r->port()->upd_parameters(r->label_id());
				//advertise LRM of parameters changes
				update_datalink_details(r->port());
			}
			break;
		default:
			break;
	}
}

////////////////////////////////////////////////////////////
/////////////TNRC_CONF_API//////////////////////////////////
////////////////////////////////////////////////////////////
tnrcapiErrorCode_t
TNRC_CONF_API::tnrcStart(std::string & resp)
{
	METIN();

	TNRC::TNRC_AP * t;

	resp = "";

	t = TNRC_MASTER.getTNRC();

	if (t) {
		resp = "INSTANCE_ALREADY_EXISTS";
		return TNRC_API_ERROR_INSTANCE_ALREADY_EXISTS;
	}

	TNRC_DBG("Creating TNRC instance...");

	t = new TNRC::TNRC_AP;
	if (!t) {
		TNRC_ERR("Cannot allocate enough memory "
			 "for a TNRC instance");
		METOUT();
		return TNRC_API_ERROR_INTERNAL_ERROR;
	}

	if (!TNRC_MASTER.attach_instance(t)) {
		TNRC_ERR("Cannot attach TNRC instance "
			 "to its master manager");
		METOUT();
		return TNRC_API_ERROR_GENERIC;
	}

	TNRC_DBG("TNRC instance started");

	METOUT();
	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_CONF_API::tnrcStop(std::string & resp)
{
	METIN();

	TNRC::TNRC_AP * t;

	resp = "";

	t = TNRC_MASTER.getTNRC();

	if (!t) {
		resp = "INSTANCE NOT FOUND";
		return TNRC_API_ERROR_NO_INSTANCE;
	}

	if (!TNRC_MASTER.detach_instance(t)) {
		TNRC_ERR("Cannot detach TNRC instance "
			 "to its master manager");
		METOUT();
		return TNRC_API_ERROR_GENERIC;
	}

	delete t;
	t = 0;

	METOUT();
	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_CONF_API::init_plugin(std::string name, std::string loc)
{
	Pcontainer    * pc;
	Plugin        * p;

	//check if there's already an installed plugin
	if (TNRC_MASTER.getPlugin()) {
		printf ("TNRC_CONF_API::init_plugin: there's already "
			"an installed plugin\n");
		return TNRC_API_ERROR_INVALID_PLUGIN;
	}
	//Get plugin container
	pc = TNRC_MASTER.getPC();
	//Fetch the plugin specified in "name"
	p = pc->getPlugin (name);
	if (p == NULL) {
		TNRC_ERR ("TNRC_CONF_API::init_plugin: plugin %s not "
			  "registered", name.c_str());
		return TNRC_API_ERROR_INVALID_PLUGIN;
	}
	//Install plugin in TNRC_Master and set correspondent location
	TNRC_MASTER.installPlugin(p);
	TNRC_MASTER.plugin_location(loc);

	//schedule an EVENT thread to execute the probe of this plugin
	thread_add_event(TNRC_MASTER.getMaster(),
			 TNRC_CONF_API::plugin_probe, NULL, 0);

	return TNRC_API_ERROR_NONE;
}

int
TNRC_CONF_API::plugin_probe(struct thread *t)
{
	TNRC::TNRC_AP      * tnrc;
	Plugin             * p;
	tnrcapiErrorCode_t   res;

	//get installed plugin
	p = TNRC_MASTER.getPlugin();
	if (p == NULL) {
		TNRC_ERR ("There is no installed Plugin. Probe failed. "
			  "No data model loaded.");
		return -1;
	}
	//execute probe for the installed plugin
	res = p->probe(TNRC_MASTER.plugin_location());
	if (res != TNRC_API_ERROR_NONE) {
		TNRC_ERR ("Error in acquiring data model from equipment: %s",
			  TNRCAPIERR2STRING(res));
		return -1;
	}
	TNRC_DBG ("Data model loaded successfully. There are %d equipments "
		  "registered", TNRC_MASTER.getTNRC()->n_eqpts());

	tnrc = TNRC_MASTER.getTNRC();
	if (tnrc == NULL) {
		TNRC_ERR ("There is no instance running.");
		return -1;
	}
	//	tnrc->dwdm_lambdas_bitmaps_init();

	return 0;
}

//////////////////////////////////

tnrc_port_id_t
TNRC_CONF_API::get_tnrc_port_id(eqpt_id_t  eqpt_id,
				board_id_t board_id,
				port_id_t  port_id)
{
	tnrc_port_id_t tnrc_p_id;

	tnrc_p_id = ( 0x0						      |
		    ((eqpt_id    & EQPTID_TO_DLID_MASK  )  <<  EQPTID_SHIFT ) |
		    ((board_id   & BOARDID_TO_DLID_MASK )  <<  BOARDID_SHIFT) |
		    ((port_id    & PORTID_TO_DLID_MASK  )		      ));

	return tnrc_p_id;
}

g2mpls_addr_t
TNRC_CONF_API::get_dl_id(tnrc_port_id_t tnrc_port_id)
{
	g2mpls_addr_t dl_id;

	dl_id.type                = UNNUMBERED;
	dl_id.preflen             = 0;
	//dl_id.value.unnum.node_id = 0;
	//dl_id.value.unnum.addr    = tnrc_port_id;
	dl_id.value.unnum         = tnrc_port_id;

	return dl_id;
}

void
TNRC_CONF_API::convert_tnrc_port_id(eqpt_id_t *    eqpt_id,
				    board_id_t *   board_id,
				    port_id_t *    port_id,
				    tnrc_port_id_t tnrc_port_id)
{
	*eqpt_id  = (tnrc_port_id & DLID_TO_EQPTID_MASK ) >> EQPTID_SHIFT;
	*board_id = (tnrc_port_id & DLID_TO_BOARDID_MASK) >> BOARDID_SHIFT;
	*port_id  = (tnrc_port_id & DLID_TO_PORTID_MASK );
}


void
TNRC_CONF_API::convert_dl_id(tnrc_port_id_t * tnrc_port_id,
			     g2mpls_addr_t    dl_id)
{
	switch (dl_id.type) {
		case IPv4:
			*tnrc_port_id = dl_id.value.ipv4.s_addr;
			break;
		case UNNUMBERED:
			//*tnrc_port_id = dl_id.value.unnum.addr;
			*tnrc_port_id = dl_id.value.unnum;
			break;
		default:
			*tnrc_port_id = 0;
			break;
	}

	return;
}

tnrcapiErrorCode_t
TNRC_CONF_API::add_Eqpt(eqpt_id_t     id,
			g2mpls_addr_t address,
			eqpt_type_t   type,
			opstate_t     opst,
			admstate_t    admst,
			std::string   location)
{
	TNRC::TNRC_AP * tnrc_ap;
	TNRC::Eqpt    * e;
	std::string     name;
	Plugin        * p;

	//Get instance
	tnrc_ap = TNRC_MASTER.getTNRC();
	if (tnrc_ap == NULL) {
		TNRC_ERR ("There's no TNRC instance running. Equipment "
			  "cannot be registered");
		return TNRC_API_ERROR_NO_INSTANCE;
	}
	//fetch correct plugin for the equipment
	name = std::string (EQPT_TYPE2NAME(type));
	p = TNRC_MASTER.getPC()->getPlugin (name);
	if (p = NULL) {
		TNRC_ERR ("There's no plugin for equipment %s",
			  SHOW_EQPT_TYPE(type));
		return TNRC_API_ERROR_INVALID_PLUGIN;
	}
	//create a new Eqpt object
	e = new TNRC::Eqpt(tnrc_ap, id, address, type, opst, admst, location);
	//attach this Eqpt to the map of Eqpts in TNRC instance
	if (!tnrc_ap->attach (id, e)) {
		TNRC_ERR ("Equipment with id %d is already registered", id);
		delete e;
		return TNRC_API_ERROR_INVALID_EQPT_ID;
	}

	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_CONF_API::add_Board(eqpt_id_t  eqpt_id,
			 board_id_t id,
			 sw_cap_t   sw_cap,
			 enc_type_t enc_type,
			 opstate_t  opst,
			 admstate_t admst)
{
	TNRC::TNRC_AP * tnrc_ap;
	TNRC::Eqpt    * e;
	TNRC::Board   * b;

	//Get instance
	tnrc_ap = TNRC_MASTER.getTNRC();
	if (tnrc_ap == NULL) {
		TNRC_ERR ("There's no TNRC instance running. Equipment "
			  "cannot be registered");
		return TNRC_API_ERROR_NO_INSTANCE;
	}

	//Get right equipment
	e = tnrc_ap->getEqpt(eqpt_id);
	if (e == NULL) {
		TNRC_ERR ("Equipment with id %d does not exist", eqpt_id);
		return TNRC_API_ERROR_INVALID_EQPT_ID;
	}
	//create a new Board object
	b = new TNRC::Board(e, id, sw_cap, enc_type, opst, admst);

	if (!e->attach (id, b)) {
		TNRC_ERR ("Board with id %d is already registered", id);
		delete b;
		return TNRC_API_ERROR_INVALID_BOARD_ID;
	}

	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_CONF_API::add_Port(eqpt_id_t        eqpt_id,
			board_id_t       board_id,
			port_id_t        id,
			int              flags,
			g2mpls_addr_t    rem_eq_addr,
			port_id_t        rem_port_id,
			opstate_t        opst,
			admstate_t       admst,
			uint32_t         dwdm_lambda_base,
			uint16_t         dwdm_lambda_count,
			uint32_t         bandwidth,
			gmpls_prottype_t protection)
{
	TNRC::TNRC_AP  * tnrc_ap;
	TNRC::Board    * b;
	TNRC::Port     * p;
	TNRC::Resource * r;

	//Get instance
	tnrc_ap = TNRC_MASTER.getTNRC();
	if (tnrc_ap == NULL) {
		TNRC_ERR ("There's no TNRC instance running. Equipment "
			  "cannot be registered");
		return TNRC_API_ERROR_NO_INSTANCE;
	}
	//Get right Board
	b = tnrc_ap->getBoard(eqpt_id, board_id);
	if (b == NULL) {
		TNRC_ERR ("Board with id %d does not exist", board_id);
		return TNRC_API_ERROR_INVALID_BOARD_ID;
	}

	//create a new Port object
	p = new TNRC::Port(b, id, flags, rem_eq_addr, rem_port_id,
			   opst, admst,
			   dwdm_lambda_base,
			   dwdm_lambda_count,
			   bandwidth, protection);

	if (!b->attach (id, p)) {
		TNRC_ERR ("Port with id %d is already registered", id);
		delete p;
		return TNRC_API_ERROR_INVALID_PORT_ID;
	}
	//create a new Resource object if switching capability is PSC or FSC
	switch (b->sw_cap()) {
		case SWCAP_PSC_1:
		case SWCAP_PSC_2:
		case SWCAP_PSC_3:
		case SWCAP_PSC_4: {
			label_t id;
			memset(&id, 0, sizeof(label_t));
			id.type = LABEL_PSC;
			id.value.psc.id = 1;
			r = new TNRC::Resource(p, 0, id, UP, ENABLED,
					       LABEL_FREE);
			if (!p->attach (id, r)) {
				TNRC_ERR ("Impossible to attach Resource");
				delete r;
				return TNRC_API_ERROR_INTERNAL_ERROR;
			}
			p->upd_unres_bw();
			break;
		}
		case SWCAP_FSC: {
			label_t id;
			memset(&id, 0, sizeof(label_t));
			id.type = LABEL_FSC;
			id.value.fsc.portId = 1;
			r = new TNRC::Resource(p, 0, id, UP, ENABLED,
					       LABEL_FREE);
			if (!p->attach (id, r)) {
				TNRC_ERR ("Impossible to attach Resource");
				delete r;
				return TNRC_API_ERROR_INTERNAL_ERROR;
			}
			p->upd_unres_bw();
			break;
		}
		default:
			break;
	}

	// Send a notify to LRM 
	update_datalink_details(p);

	return TNRC_API_ERROR_NONE;
}

tnrcapiErrorCode_t
TNRC_CONF_API::add_Resource(eqpt_id_t     eqpt_id,
			    board_id_t    board_id,
			    port_id_t     port_id,
			    int           tp_fl,
			    label_t       id,
			    opstate_t     opst,
			    admstate_t    admst,
			    label_state_t st)
{
	TNRC::TNRC_AP  * tnrc_ap;
	TNRC::Port     * p;
	TNRC::Resource * r;

	//Get instance
	tnrc_ap = TNRC_MASTER.getTNRC();
	if (tnrc_ap == NULL) {
		TNRC_ERR ("There's no TNRC instance running. Equipment "
			  "cannot be registered");
		return TNRC_API_ERROR_NO_INSTANCE;
	}
	//Get right Port
	p = tnrc_ap->getPort(eqpt_id, board_id, port_id);
	if (p == NULL) {
		TNRC_ERR ("Port with id %d does not exist", port_id);
		return TNRC_API_ERROR_INVALID_PORT_ID;
	}
	switch (p->board()->sw_cap()) {
		case SWCAP_PSC_1:
		case SWCAP_PSC_2:
		case SWCAP_PSC_3:
		case SWCAP_PSC_4:
		case SWCAP_FSC:
			TNRC_ERR ("Add a Resource with FSC/PSC "
				  "switching capability is not allowed");
			return TNRC_API_ERROR_GENERIC;
			break;
		default:
			break;
	}
	//create a new instance of Resource objects
	r = new TNRC::Resource(p, tp_fl, id, opst, admst, st);

	if (!p->attach (id, r)) {
		TNRC_ERR ("Resource with specified id is already registered");
		delete r;
		return TNRC_API_ERROR_INVALID_RESOURCE_ID;
	}

	//update available bandwidth for the port
	p->upd_unres_bw();


	if (p->board()->sw_cap() == SWCAP_LSC) {
		//update wavelength bitmap for the port
		p->upd_dwdm_lambdas_bitmap(id);
	}

	return TNRC_API_ERROR_NONE;
}
