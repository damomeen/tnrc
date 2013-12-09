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



#include <zebra.h>
#include "vty.h"

#include "tnrc_plugin_simulator.h"
#include "tnrcd.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"

tnrcsp_sim_xc_t *
new_tnrcsp_sim_xc(int                      xc_id,
		  xc_status_t              status,
		  eqpt_id_t                eqpt_id_in,
		  board_id_t               board_id_in,
		  port_id_t                port_id_in,
		  label_t                  labelid_in,
		  eqpt_id_t                eqpt_id_out,
		  board_id_t               board_id_out,
		  port_id_t                port_id_out,
		  label_t                  labelid_out,
		  tnrcsp_notification_cb_t async_cb,
		  void *                   async_ctxt)
{
	tnrcsp_sim_xc_t * xc;

	xc = new tnrcsp_sim_xc_t;

	xc->id           = xc_id;
	xc->status       = status;
	xc->eqpt_id_in   = eqpt_id_in;
	xc->board_id_in  = board_id_in;
	xc->port_id_in   = port_id_in;
	xc->labelid_in   = labelid_in;
	xc->eqpt_id_out  = eqpt_id_out;
	xc->board_id_out = board_id_out;
	xc->port_id_out  = port_id_out;
	xc->labelid_out  = labelid_out;
	xc->async_cb     = async_cb;
	xc->async_ctxt   = async_ctxt;

	return xc;
}

tnrcsp_sim_maxe_xc_t *
new_tnrcsp_sim_maxe_xc(tnrcsp_handle_t          handle,
		       tnrc_port_id_t           portid_in,
		       label_t                  labelid_in,
		       tnrc_port_id_t           portid_out,
		       label_t                  labelid_out,
		       xcdirection_t            direction,
		       tnrc_boolean_t           isvirtual,
		       tnrc_boolean_t           activate,
		       tnrcsp_response_cb_t     response_cb,
		       void *                   response_ctxt,
		       tnrcsp_notification_cb_t async_cb,
		       void *                   async_ctxt)
{
	tnrcsp_sim_maxe_xc_t * det;
	eqpt_id_t              eqpt_id_in;
	eqpt_id_t              eqpt_id_out;
	board_id_t             board_id_in;
	board_id_t             board_id_out;
	port_id_t              port_id_in;
	port_id_t              port_id_out;

	TNRC_CONF_API::convert_tnrc_port_id(&eqpt_id_in, &board_id_in,
					    &port_id_in, portid_in);
	TNRC_CONF_API::convert_tnrc_port_id(&eqpt_id_out, &board_id_out,
					    &port_id_out, portid_out);

	det = new tnrcsp_sim_maxe_xc_t;

	det->handle       = handle;
	det->eqpt_id_in   = eqpt_id_in;
	det->board_id_in  = board_id_in;
	det->port_id_in   = port_id_in;
	det->labelid_in   = labelid_in;
	det->eqpt_id_out  = eqpt_id_out;
	det->board_id_out = board_id_out;
	det->port_id_out  = portid_out;
	det->labelid_out  = labelid_out;
	det->dir          = direction;
	det->isvirtual    = isvirtual;
	det->activate     = activate;
	det->resp_cb      = response_cb;
	det->resp_ctxt    = response_ctxt;
	det->async_cb     = async_cb;
	det->async_ctxt   = async_ctxt;

	return det;
}

tnrcsp_sim_destroy_xc_t *
new_tnrcsp_sim_destroy_xc(tnrcsp_handle_t      handle,
			  tnrc_port_id_t       portid_in,
			  label_t              labelid_in,
			  tnrc_port_id_t       portid_out,
			  label_t              labelid_out,
			  xcdirection_t        direction,
			  tnrc_boolean_t       isvirtual,
			  tnrc_boolean_t       deactivate,
			  tnrcsp_response_cb_t response_cb,
			  void *               response_ctxt)
{
	tnrcsp_sim_destroy_xc_t * det;
	eqpt_id_t                 eqpt_id_in;
	eqpt_id_t                 eqpt_id_out;
	board_id_t                board_id_in;
	board_id_t                board_id_out;
	port_id_t                 port_id_in;
	port_id_t                 port_id_out;

	TNRC_CONF_API::convert_tnrc_port_id (&eqpt_id_in, &board_id_in,
					     &port_id_in, portid_in);
	TNRC_CONF_API::convert_tnrc_port_id (&eqpt_id_out, &board_id_out,
					     &port_id_out, portid_out);

	det = new tnrcsp_sim_destroy_xc_t;

	det->handle       = handle;
	det->eqpt_id_in   = eqpt_id_in;
	det->board_id_in  = board_id_in;
	det->port_id_in   = port_id_in;
	det->labelid_in   = labelid_in;
	det->eqpt_id_out  = eqpt_id_out;
	det->board_id_out = board_id_out;
	det->port_id_out  = portid_out;
	det->labelid_out  = labelid_out;
	det->dir          = direction;
	det->isvirtual    = isvirtual;
	det->deactivate   = deactivate;
	det->resp_cb      = response_cb;
	det->resp_ctxt    = response_ctxt;

	return det;
}

tnrcsp_sim_reserve_xc_t *
new_tnrcsp_sim_reserve_xc(tnrcsp_handle_t      handle,
			  tnrc_port_id_t       portid_in,
			  label_t              labelid_in,
			  tnrc_port_id_t       portid_out,
			  label_t              labelid_out,
			  xcdirection_t        direction,
			  tnrc_boolean_t       isvirtual,
			  tnrcsp_response_cb_t response_cb,
			  void *               response_ctxt)
{
	tnrcsp_sim_reserve_xc_t * det;
	eqpt_id_t                 eqpt_id_in;
	eqpt_id_t                 eqpt_id_out;
	board_id_t                board_id_in;
	board_id_t                board_id_out;
	port_id_t                 port_id_in;
	port_id_t                 port_id_out;

	TNRC_CONF_API::convert_tnrc_port_id(&eqpt_id_in, &board_id_in,
					    &port_id_in, portid_in);
	TNRC_CONF_API::convert_tnrc_port_id(&eqpt_id_out, &board_id_out,
					    &port_id_out, portid_out);

	det = new tnrcsp_sim_reserve_xc_t;

	det->handle       = handle;
	det->eqpt_id_in   = eqpt_id_in;
	det->board_id_in  = board_id_in;
	det->port_id_in   = port_id_in;
	det->labelid_in   = labelid_in;
	det->eqpt_id_out  = eqpt_id_out;
	det->board_id_out = board_id_out;
	det->port_id_out  = portid_out;
	det->labelid_out  = labelid_out;
	det->dir          = direction;
	det->isvirtual    = isvirtual;
	det->resp_cb      = response_cb;
	det->resp_ctxt    = response_ctxt;

	return det;
}

tnrcsp_sim_unreserve_xc_t *
new_tnrcsp_sim_unreserve_xc(tnrcsp_handle_t      handle,
			    tnrc_port_id_t       portid_in,
			    label_t              labelid_in,
			    tnrc_port_id_t       portid_out,
			    label_t              labelid_out,
			    xcdirection_t        direction,
			    tnrc_boolean_t       isvirtual,
			    tnrcsp_response_cb_t response_cb,
			    void *               response_ctxt)
{
	//Reserve and Unreserve XC parameters structures are identical
	return new_tnrcsp_sim_reserve_xc(handle,
					 portid_in,
					 labelid_in,
					 portid_out,
					 labelid_out,
					 direction,
					 isvirtual,
					 response_cb,
					 response_ctxt);
}

///////////////////////////////////
///////Class Psimulator///////////
/////////////////////////////////

Psimulator::Psimulator(std::string name)
{
	name_   = name;
	handle_ = 1;
	xc_id_  = 1;
	wqueue_ = work_queue_new (TNRC_MASTER.getMaster(), SIMULATOR_WQ);

	wqueue_->spec.workfunc      = workfunction;
	wqueue_->spec.del_item_data = delete_item_data;
	wqueue_->spec.max_retries   = 5;
	//wqueue_->spec.hold = 500;

	xc_bidir_support_ = XC_BIDIR_SUPPORT;
}

Psimulator::~Psimulator(void)
{
	work_queue_free(wqueue_);
}

bool
Psimulator::attach_xc(tnrcsp_sim_xc_t *xc)
{
	int size;

	size = n_xconns();
	xconns_.push_back(xc);
	if (size == n_xconns()) {
		TNRC_WRN ("SIMULATOR EQPT: Cannot add XC with id %d from the list of XCs",
			  xc->id);
		return false;
	}

	return true;

}

bool
Psimulator::detach_xc(tnrcsp_sim_xc_t *xc)
{
	iterator_xconns iter;
	int             size;

	size = xconns_.size();
	//remove xc from the list
	xconns_.remove(xc);
	if (size == xconns_.size()) {
		TNRC_WRN ("SIMULATOR EQPT: Cannot remove XC with id %d from the list of XCs",
			  xc->id);
		return false;
	}

	return true;
}

tnrcsp_sim_xc_t *
Psimulator::getXC(eqpt_id_t  eqpt_id_in,
	          board_id_t board_id_in,
		  port_id_t  port_id_in,
		  label_t    labelid_in,
		  eqpt_id_t  eqpt_id_out,
		  board_id_t board_id_out,
		  port_id_t  port_id_out,
		  label_t    labelid_out)
{
	iterator_xconns   iter;
	tnrcsp_sim_xc_t * xc;

	for (iter = begin_xconns(); iter != end_xconns(); iter++) {
		xc = (*iter);
		if (matchXC (xc,
			     eqpt_id_in,
			     board_id_in,
			     port_id_in,
			     labelid_in,
			     eqpt_id_out,
			     board_id_out,
			     port_id_out,
			     labelid_out)) {
			return xc;
		}
	}
	TNRC_WRN ("SIMULATOR EQPT: Cannot get requested XC: that's not present in the list");
	return NULL;

}

tnrcsp_sim_xc_t *
Psimulator::getXC(int id)
{
	iterator_xconns iter;
	tnrcsp_sim_xc_t *xc;

	for (iter = begin_xconns(); iter != end_xconns(); iter++) {
		xc = (*iter);
		if (xc->id == id) {
			return xc;
		}
	}
	TNRC_WRN ("SIMULATOR EQPT: Cannot get requested XC: that's not present in the list");
	return NULL;
}

bool
Psimulator::matchXC(tnrcsp_sim_xc_t * xc,
		    eqpt_id_t         eqpt_id_in,
		    board_id_t        board_id_in,
		    port_id_t         port_id_in,
		    label_t           labelid_in,
		    eqpt_id_t         eqpt_id_out,
		    board_id_t        board_id_out,
		    port_id_t         port_id_out,
		    label_t           labelid_out)
{
	uint32_t first_id_in;
	uint32_t second_id_in;
	uint32_t first_id_out;
	uint32_t second_id_out;

	switch (labelid_in.type) {
		case LABEL_FSC:
			first_id_in   = xc->labelid_in.value.fsc.portId;
			second_id_in  = labelid_in.value.fsc.portId;
			first_id_out  = xc->labelid_out.value.fsc.portId;
			second_id_out = labelid_out.value.fsc.portId;
			break;
		case LABEL_LSC:
			first_id_in   = xc->labelid_in.value.lsc.wavelen;
			second_id_in  = labelid_in.value.lsc.wavelen;
			first_id_out  = xc->labelid_out.value.lsc.wavelen;
			second_id_out = labelid_out.value.lsc.wavelen;
			break;
		default:
			first_id_in   = xc->labelid_in.value.rawId;
			second_id_in  = labelid_in.value.rawId;
			first_id_out  = xc->labelid_out.value.rawId;
			second_id_out = labelid_out.value.rawId;
			break;
	}

	if((first_id_in  != second_id_in)  ||
	   (first_id_out != second_id_out) ||
	   (xc->port_id_in   != port_id_in)   ||
	   (xc->port_id_out  != port_id_out)  ||
	   (xc->board_id_in  != board_id_in)  ||
	   (xc->board_id_out != board_id_out) ||
	   (xc->eqpt_id_in   != eqpt_id_in)   ||
	   (xc->eqpt_id_out  != eqpt_id_out)) {
		return false;
	}

	return true;
}

int
Psimulator::new_xc_id(void)
{
	return xc_id_++;
}

int
Psimulator::n_xconns(void)
{
	return xconns_.size();
}

wq_item_status
Psimulator::wq_function(void *d)
{
	tnrcsp_action_t * data;
	Plugin          * p;
	tnrcsp_result_t   res;
	tnrcsp_sim_xc_t * xc;

	//get work item data
	data = (tnrcsp_action_t *) d;

	switch (data->type) {
		case MAKE_XC: {
			tnrcsp_sim_maxe_xc_t * make_xc_det;
			//get work item data details
			make_xc_det = (tnrcsp_sim_maxe_xc_t *) data->details;
			//This is an equipment simulator Plugin
			//so we return a pseudocasual answer
			//without "talking" with equipment

			//if response is OK create new XC structure to add
			//in the map of XC's
			if (make_xc_det->isvirtual) {
				xc = getXC(make_xc_det->eqpt_id_in,
					   make_xc_det->board_id_in,
					   make_xc_det->port_id_in,
					   make_xc_det->labelid_in,
					   make_xc_det->eqpt_id_out,
					   make_xc_det->board_id_out,
					   make_xc_det->port_id_out,
					   make_xc_det->labelid_out);
				if (xc == NULL) {
					res = get_response();
					if (res == TNRCSP_RESULT_NOERROR) {
						xc = new_tnrcsp_sim_xc(new_xc_id(),
								       XC_ACTIVE,
								       make_xc_det->eqpt_id_in,
								       make_xc_det->board_id_in,
								       make_xc_det->port_id_in,
								       make_xc_det->labelid_in,
								       make_xc_det->eqpt_id_out,
								       make_xc_det->board_id_out,
								       make_xc_det->port_id_out,
								       make_xc_det->labelid_out,
								       make_xc_det->async_cb,
								       make_xc_det->async_ctxt);
						//attach XC in the list
						attach_xc(xc);
						TNRC_DBG("SIMULATOR EQPT: Created a new XC for virtual "
							 "MAKE XC request");
					}
					make_xc_det->resp_cb(make_xc_det->handle, res,
							     make_xc_det->resp_ctxt);
				} else if (xc->status != XC_ACTIVE) {
					TNRC_ERR("SIMULATOR EQPT:  Requested a virtual MAKE XC but the "
						 "xconn is already present and not in state XC_ACTIVE");
					make_xc_det->resp_cb(make_xc_det->handle,
							     TNRCSP_RESULT_PARAMERROR,
							     make_xc_det->resp_ctxt);
				} else {
					xc->async_cb   = make_xc_det->
						async_cb;
					xc->async_ctxt = make_xc_det->
						async_ctxt;
					TNRC_DBG("SIMULATOR EQPT: Retrieved requested virtual MAKE XC");
					make_xc_det->resp_cb(make_xc_det->handle,
							     TNRCSP_RESULT_NOERROR,
							     make_xc_det->resp_ctxt);
				}
			} else {
				if (make_xc_det->activate == 0) {
					res = get_response();
					if (res == TNRCSP_RESULT_NOERROR) {
						xc = new_tnrcsp_sim_xc(new_xc_id(),
								       XC_ACTIVE,
								       make_xc_det->eqpt_id_in,
								       make_xc_det->board_id_in,
								       make_xc_det->port_id_in,
								       make_xc_det->labelid_in,
								       make_xc_det->eqpt_id_out,
								       make_xc_det->board_id_out,
								       make_xc_det->port_id_out,
								       make_xc_det->labelid_out,
								       make_xc_det->async_cb,
								       make_xc_det->async_ctxt);
						//attach XC in the list
						attach_xc(xc);
					}
					make_xc_det->resp_cb(make_xc_det->handle, res,
							     make_xc_det->resp_ctxt);
				} else {
					xc = getXC(make_xc_det->eqpt_id_in,
						   make_xc_det->board_id_in,
						   make_xc_det->port_id_in,
						   make_xc_det->labelid_in,
						   make_xc_det->eqpt_id_out,
						   make_xc_det->board_id_out,
						   make_xc_det->port_id_out,
						   make_xc_det->labelid_out);
					if (xc == NULL) {
						//error - there must be a reserved xconn
						TNRC_ERR ("SIMULATOR EQPT: Requested an activation "
							  "for an xconn not previuosly reserved");
						make_xc_det->resp_cb(make_xc_det->handle,
								     TNRCSP_RESULT_PARAMERROR,
								     make_xc_det->resp_ctxt);
					} else if (xc->status != XC_BOOKED) {
						//error - there must be a reserved xconn
						TNRC_ERR ("SIMULATOR EQPT: Requested an activation "
							  "for an xconn not XC_BOOKED state");
						make_xc_det->resp_cb(make_xc_det->handle,
								     TNRCSP_RESULT_PARAMERROR,
								     make_xc_det->resp_ctxt);
						return WQ_SUCCESS;
					} else {
						res = get_response();
						if (res == TNRCSP_RESULT_NOERROR) {
							xc->async_cb   = make_xc_det->
								async_cb;
							xc->async_ctxt = make_xc_det->
								async_ctxt;
							xc->status    = XC_ACTIVE;
						}
						make_xc_det->resp_cb(make_xc_det->handle, res,
								     make_xc_det->resp_ctxt);
					}
				}
			}
			break;
		}
		case DESTROY_XC: {
			tnrcsp_sim_destroy_xc_t * destroy_xc_det;
			//get work item data details
			destroy_xc_det = (tnrcsp_sim_destroy_xc_t *)
				data->details;
			//This is an equipment simulator Plugin
			//so we return a pseudocasual answer
			//without "talking" with equipment

			//if response is OK detach XC structure in the map
			//and delete it
			if (destroy_xc_det->deactivate == 0) {
				//detach XC in the list
				xc = getXC(destroy_xc_det->eqpt_id_in,
					   destroy_xc_det->board_id_in,
					   destroy_xc_det->port_id_in,
					   destroy_xc_det->labelid_in,
					   destroy_xc_det->eqpt_id_out,
					   destroy_xc_det->board_id_out,
					   destroy_xc_det->port_id_out,
					   destroy_xc_det->labelid_out);
				if (xc != NULL) {
					res = get_response();
					if (res == TNRCSP_RESULT_NOERROR) {
						detach_xc(xc);
						delete xc;
					}
					destroy_xc_det->resp_cb(destroy_xc_det->handle, res,
								destroy_xc_det->resp_ctxt);
				} else {
					TNRC_ERR ("SIMULATOR EQPT: Requested a DESTROY XC "
						  "for an xconn not existent");
					destroy_xc_det->resp_cb(destroy_xc_det->handle,
								TNRCSP_RESULT_PARAMERROR,
								destroy_xc_det->resp_ctxt);
				}
			} else {
				//detach XC in the list
				xc = getXC(destroy_xc_det->eqpt_id_in,
					   destroy_xc_det->board_id_in,
					   destroy_xc_det->port_id_in,
					   destroy_xc_det->labelid_in,
					   destroy_xc_det->eqpt_id_out,
					   destroy_xc_det->board_id_out,
					   destroy_xc_det->port_id_out,
					   destroy_xc_det->labelid_out);
				if (xc != NULL) {
					res = get_response();
					if (res == TNRCSP_RESULT_NOERROR) {
						xc->status = XC_BOOKED;
					}
					destroy_xc_det->resp_cb(destroy_xc_det->handle, res,
								destroy_xc_det->resp_ctxt);
				} else if (xc->status != XC_ACTIVE) {
							TNRC_ERR ("SIMULATOR EQPT: Requested a deactivation "
								  "for an xconn not in XC_ACTIVE state");
							destroy_xc_det->resp_cb(destroy_xc_det->handle,
										TNRCSP_RESULT_PARAMERROR,
										destroy_xc_det->resp_ctxt);
				} else {
					TNRC_ERR ("SIMULATOR EQPT: Requested a deactivation "
						  "for an xconn not existent");
					destroy_xc_det->resp_cb(destroy_xc_det->handle,
								TNRCSP_RESULT_PARAMERROR,
								destroy_xc_det->resp_ctxt);
				}
			}
			break;
		}
		case RESERVE_XC: {
			tnrcsp_sim_reserve_xc_t * reserve_xc_det;
			//get work item data details
			reserve_xc_det = (tnrcsp_sim_reserve_xc_t *)
				data->details;
			//This is an equipment simulator Plugin
			//so we return a pseudocasual answer
			//without "talking" with equipment
			if (reserve_xc_det->isvirtual) {
				xc = getXC(reserve_xc_det->eqpt_id_in,
					   reserve_xc_det->board_id_in,
					   reserve_xc_det->port_id_in,
					   reserve_xc_det->labelid_in,
					   reserve_xc_det->eqpt_id_out,
					   reserve_xc_det->board_id_out,
					   reserve_xc_det->port_id_out,
					   reserve_xc_det->labelid_out);
				if (xc == NULL) {
					res = get_response();
					if (res == TNRCSP_RESULT_NOERROR) {
						xc = new_tnrcsp_sim_xc(new_xc_id(),
								       XC_BOOKED,
								       reserve_xc_det->eqpt_id_in,
								       reserve_xc_det->board_id_in,
								       reserve_xc_det->port_id_in,
								       reserve_xc_det->labelid_in,
								       reserve_xc_det->eqpt_id_out,
								       reserve_xc_det->board_id_out,
								       reserve_xc_det->port_id_out,
								       reserve_xc_det->labelid_out,
								       0,
								       0);
						//attach XC in the list
						attach_xc(xc);
						TNRC_DBG("SIMULATOR EQPT: Created a new XC for virtual "
							 "RESERVE XC request");
					}
					reserve_xc_det->resp_cb(reserve_xc_det->handle, res,
								reserve_xc_det->resp_ctxt);
				} else if (xc->status != XC_BOOKED) {
					TNRC_ERR("SIMULATOR EQPT:  Requested a virtual RESERVE XC but the "
						 "xconn is already present and not in state XC_BOOKED");
					reserve_xc_det->resp_cb(reserve_xc_det->handle,
								TNRCSP_RESULT_PARAMERROR,
								reserve_xc_det->resp_ctxt);
				} else {
					TNRC_DBG("SIMULATOR EQPT: Retrieved requested virtual RESERVE XC");
					reserve_xc_det->resp_cb(reserve_xc_det->handle,
							     TNRCSP_RESULT_NOERROR,
							     reserve_xc_det->resp_ctxt);
				}
			} else {
				res = get_response();
				if (res == TNRCSP_RESULT_NOERROR) {
					xc = new_tnrcsp_sim_xc(new_xc_id(),
							       XC_BOOKED,
							       reserve_xc_det->eqpt_id_in,
							       reserve_xc_det->board_id_in,
							       reserve_xc_det->port_id_in,
							       reserve_xc_det->labelid_in,
							       reserve_xc_det->eqpt_id_out,
							       reserve_xc_det->board_id_out,
							       reserve_xc_det->port_id_out,
							       reserve_xc_det->labelid_out,
							       0,
							       0);
					//attach XC in the list
					attach_xc(xc);
					reserve_xc_det->resp_cb(reserve_xc_det->handle,
								res,
								reserve_xc_det->resp_ctxt);
				}
			}
			break;
		}
		case UNRESERVE_XC: {
			tnrcsp_sim_unreserve_xc_t * unreserve_xc_det;
			//get work item data details
			unreserve_xc_det = (tnrcsp_sim_unreserve_xc_t *)
				data->details;
			//This is an equipment simulator Plugin
			//so we return a pseudocasual answer
			//without "talking" with equipment
			xc = getXC(unreserve_xc_det->eqpt_id_in,
				   unreserve_xc_det->board_id_in,
				   unreserve_xc_det->port_id_in,
				   unreserve_xc_det->labelid_in,
				   unreserve_xc_det->eqpt_id_out,
				   unreserve_xc_det->board_id_out,
				   unreserve_xc_det->port_id_out,
				   unreserve_xc_det->labelid_out);
			if (xc != NULL && xc->status == XC_BOOKED) {
				res = get_response();
				if (res == TNRCSP_RESULT_NOERROR) {
					detach_xc(xc);
					delete xc;
						}
				unreserve_xc_det->resp_cb(unreserve_xc_det->handle,
							  res,
									  unreserve_xc_det->resp_ctxt);
			} else {
				TNRC_ERR ("SIMULATOR EQPT: Requested an UNRESERVE XC "
					  "for an xconn not existent or not reserved");
				unreserve_xc_det->resp_cb(unreserve_xc_det->handle,
									  TNRCSP_RESULT_PARAMERROR,
							  unreserve_xc_det->resp_ctxt);
			}
			break;
		}
	}
	return WQ_SUCCESS;
}

void
Psimulator::del_item_data(void *d)
{
	tnrcsp_action_t * data;

	//get work item data
	data = (tnrcsp_action_t *) d;
	//delete work item data
	switch (data->type) {
		case MAKE_XC:
			tnrcsp_sim_maxe_xc_t * make_xc_det;
			//get work item data details
			make_xc_det = (tnrcsp_sim_maxe_xc_t *) data->details;
			//delete details
			delete make_xc_det;
			break;
		case DESTROY_XC:
			tnrcsp_sim_destroy_xc_t * destroy_xc_det;
			//get work item data details
			destroy_xc_det = (tnrcsp_sim_destroy_xc_t *)
				data->details;
			//delete details
			delete destroy_xc_det;
			break;
		case RESERVE_XC:
			tnrcsp_sim_reserve_xc_t * reserve_xc_det;
			//get work item data details
			reserve_xc_det = (tnrcsp_sim_reserve_xc_t *)
				data->details;
			//delete details
			delete reserve_xc_det;
			break;
		case UNRESERVE_XC:
			tnrcsp_sim_unreserve_xc_t * unreserve_xc_det;
			//get work item data details
			unreserve_xc_det = (tnrcsp_sim_unreserve_xc_t *)
				data->details;
			//delete details
			delete unreserve_xc_det;
			break;
	}
	//delete data structure
	delete data;
}

tnrcapiErrorCode_t
Psimulator::probe(std::string location)
{
	TNRC::TNRC_AP *    t;
	std::string        err;
	tnrcapiErrorCode_t res;

	//check if a TNRC instance is running. If not, create e new one
	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		//create a new instance
		res = TNRC_CONF_API::tnrcStart(err);
		if (res != TNRC_API_ERROR_NONE) {
			return res;
		}
	}

	//read equipments details from speciifed file using vty library
	vty_read_config ((char *)location.c_str(), NULL);

	return TNRC_API_ERROR_NONE;
}

tnrcsp_result_t
Psimulator::get_response(void)
{
#ifndef _SIMULATOR_CASUAL
#define SUCCESS_PROBAB  0.8 //NO_ERROR response probability
	tnrcsp_result_t res;
	float           value;
	float           threshold;
	float           delta;

	//get pseudocasual value in the interval [0, 1)
	value  = (float)rand() / (float)RAND_MAX;

	//extract a pseudocasual equipment simulator response from value
	//success probability could be set with SUCCESS_PROB macro
	//decision threshold for NO_ERROR response
	threshold = (float)SUCCESS_PROBAB;
	//decision thresholds interval
	delta = (float)((1-threshold)/(N_RESP-1));
	res = EXTRACT_SIM_RESP(value, threshold, delta);

	return res;

#else
	return TNRCSP_RESULT_NOERROR;

#endif /* _SIMULATOR_CASUAL */
}

tnrcsp_result_t
Psimulator::tnrcsp_make_xc(tnrcsp_handle_t *        handlep,
			   tnrc_port_id_t           portid_in,
			   label_t                  labelid_in,
			   tnrc_port_id_t           portid_out,
			   label_t                  labelid_out,
			   xcdirection_t            direction,
			   tnrc_boolean_t           isvirtual,
			   tnrc_boolean_t           activate,
			   tnrcsp_response_cb_t     response_cb,
			   void *                   response_ctxt,
			   tnrcsp_notification_cb_t async_cb,
			   void *                   async_ctxt)
{
	tnrcsp_action_t      * data;
	tnrcsp_sim_maxe_xc_t * details;

	*handlep = new_handle();
	//create a work item data for this operation, and then add the item to the wqueue
	details = new_tnrcsp_sim_maxe_xc(*handlep,
					 portid_in,
					 labelid_in,
					 portid_out,
					 labelid_out,
					 direction,
					 isvirtual,
					 activate,
					 response_cb,
					 response_ctxt,
					 async_cb,
					 async_ctxt);

	data = new tnrcsp_action_t;

	data->type    = MAKE_XC;
	data->plugin  = this;
	data->details = details;

	work_queue_add(wqueue_, data);

	return TNRCSP_RESULT_NOERROR;
}

tnrcsp_result_t
Psimulator::tnrcsp_destroy_xc(tnrcsp_handle_t *    handlep,
			      tnrc_port_id_t       portid_in,
			      label_t              labelid_in,
			      tnrc_port_id_t       portid_out,
			      label_t              labelid_out,
			      xcdirection_t        direction,
			      tnrc_boolean_t       isvirtual,
			      tnrc_boolean_t       deactivate,
			      tnrcsp_response_cb_t response_cb,
			      void *               response_ctxt)
{
	tnrcsp_action_t         * data;
	tnrcsp_sim_destroy_xc_t * details;

	*handlep = new_handle();
	//create a work item data for this operation, and then add the item to the wqueue
	details = new_tnrcsp_sim_destroy_xc(*handlep,
					    portid_in,
					    labelid_in,
					    portid_out,
					    labelid_out,
					    direction,
					    isvirtual,
					    deactivate,
					    response_cb,
					    response_ctxt);

	data = new tnrcsp_action_t;

	data->type    = DESTROY_XC;
	data->plugin  = this;
	data->details = details;

	work_queue_add(wqueue_, data);

	return TNRCSP_RESULT_NOERROR;
}

tnrcsp_result_t
Psimulator::tnrcsp_reserve_xc(tnrcsp_handle_t *    handlep,
			      tnrc_port_id_t       portid_in,
			      label_t              labelid_in,
			      tnrc_port_id_t       portid_out,
			      label_t              labelid_out,
			      xcdirection_t        direction,
			      tnrc_boolean_t       isvirtual,
			      tnrcsp_response_cb_t response_cb,
			      void *               response_ctxt)
{
	tnrcsp_action_t         * data;
	tnrcsp_sim_reserve_xc_t * details;

	*handlep = new_handle();
	//create a work item data for this operation, and then add the item to the wqueue
	details = new_tnrcsp_sim_reserve_xc(*handlep,
					    portid_in,
					    labelid_in,
					    portid_out,
					    labelid_out,
					    direction,
					    isvirtual,
					    response_cb,
					    response_ctxt);

	data = new tnrcsp_action_t;

	data->type    = RESERVE_XC;
	data->plugin  = this;
	data->details = details;

	work_queue_add(wqueue_, data);

	return TNRCSP_RESULT_NOERROR;
}


tnrcsp_result_t
Psimulator::tnrcsp_unreserve_xc(tnrcsp_handle_t *    handlep,
				tnrc_port_id_t       portid_in,
				label_t              labelid_in,
				tnrc_port_id_t       portid_out,
				label_t              labelid_out,
				xcdirection_t        direction,
				tnrc_boolean_t       isvirtual,
				tnrcsp_response_cb_t response_cb,
				void *               response_ctxt)
{
	tnrcsp_action_t           * data;
	tnrcsp_sim_unreserve_xc_t * details;

	*handlep = new_handle();
	//create a work item data for this operation, and then add the item to the wqueue
	details = new_tnrcsp_sim_unreserve_xc(*handlep,
					      portid_in,
					      labelid_in,
					      portid_out,
					      labelid_out,
					      direction,
					      isvirtual,
					      response_cb,
					      response_ctxt);

	data = new tnrcsp_action_t;

	data->type    = UNRESERVE_XC;
	data->plugin  = this;
	data->details = details;

	work_queue_add(wqueue_, data);

	return TNRCSP_RESULT_NOERROR;
}

tnrcsp_result_t
Psimulator::tnrcsp_register_async_cb(tnrcsp_event_t *events)
{
	TNRC_DBG("Asynchronus notification received from equipment. "
		 "Updating data model..");

	TNRC_OPSPEC_API::update_dm_after_notify	(events->portid,
						 events->labelid,
						 events->events);

	TNRC_DBG("Data model updated");

	return TNRCSP_RESULT_NOERROR;
}
