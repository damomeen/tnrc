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

#include "thread.h"
#include "vty.h"
#include "command.h"
#include "linklist.h"

#include "tnrcd.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_g2rsvpte_api.h"
#include "tnrc_lrm_api.h"

//
// Statics initialization
//
TNRC_Master *               TNRC_Master::instance_   = 0;
struct thread_master *      TNRC_Master::master_     = 0; // thread master
time_t                      TNRC_Master::start_time_ = 0; // start time
tnrcap_cookie_t             TNRC_Master::cookie_     = 0; // cookie
uint32_t                    TNRC_Master::xc_id_      = 0; // xc_id_

std::list<TNRC::TNRC_AP *> TNRC_Master::tnrcs_;


int
process_api_queue (struct thread *t)
{
	Action * a;

	//process the first element in the api queue
	TNRC_MASTER.api_queue_process();

	return 0;
}

//////////////////////////////////////
////////CLASS XC/////////////////////
////////////////////////////////////

XC::XC(u_int                    id,
       tnrcap_cookie_t          ck,
       tnrcap_xc_state_t        st,
       tnrc_port_id_t           portid_in,
       label_t                  labelid_in,
       tnrc_port_id_t           portid_out,
       label_t                  labelid_out,
       xcdirection_t            direction,
       long                     ctxt)
{
	id_           = id;
	cookie_       = ck;
	state_        = st;
	portid_in_    = portid_in;
	labelid_in_   = labelid_in;
	portid_out_   = portid_out;
	labelid_out_  = labelid_out;
	direction_    = direction;
	async_ctxt_   = ctxt;
}

u_int
XC::id(void)
{
	return id_;
}

tnrcap_cookie_t
XC::cookie(void)
{
	return cookie_;
}

void
XC::cookie(tnrcap_cookie_t ck)
{
	cookie_ = ck;
}

tnrcap_xc_state_t
XC::state(void)
{
	return state_;
}

void
XC::state(tnrcap_xc_state_t st)
{
	state_ = st;
}

tnrc_port_id_t
XC::portid_in(void)
{
	return portid_in_;
}

label_t
XC::labelid_in(void)
{
	return labelid_in_;
}

tnrc_port_id_t
XC::portid_out(void)
{
	return portid_out_;
}

label_t
XC::labelid_out(void)
{
	return labelid_out_;
}

xcdirection_t
XC::direction(void)
{
	return direction_;
}

long
XC::async_ctxt (void)
{
	return async_ctxt_;
}

void
XC::async_ctxt (long ctxt)
{
	async_ctxt_ = ctxt;
}

//////////////////////////////////////
////////CLASS ApiQueue///////////////
////////////////////////////////////

ApiQueue::ApiQueue (void)
{
	tot_req_ = 0;
}

bool
ApiQueue::insert (api_queue_element_t *e)
{
	queue_.push (e);
	TNRC_DBG ("Queued a new action request");

	tot_req_++;

	return true;
}

api_queue_element_t *
ApiQueue::extract (void)
{
	api_queue_element_t * e;

	if (queue_.empty()) {
		TNRC_WRN ("There are no actions queued to execute");
		return NULL;
	}
	e = queue_.front();
	queue_.pop();

	return e;
}

int
ApiQueue::size(void)
{
	return queue_.size();
}

u_int
ApiQueue::tot_request(void)
{
	return tot_req_;
}


//////////////////////////////////////
////////CLASS TNRC_Master////////////
////////////////////////////////////

TNRC_Master::TNRC_Master(void)
{
	METIN();

	TNRC_WRN("Called TNRC_Master constructor");

	METOUT();
}

TNRC_Master::~TNRC_Master(void)
{
	METIN();

	TNRC_WRN("Called TNRC_Master destructor");

	METOUT();
}

bool
TNRC_Master::init(void)
{
	METIN();

	TNRC_DBG("Initializing TNRC_Master");

	instance_ = new TNRC_Master;
	if (!instance_) {
		TNRC_ERR("Cannot allocate enough memory ...");
		METOUT();
		assert(0);
	}

	master_ = thread_master_create();
	if (!master_) {
		TNRC_ERR("Cannot create ZEBRA thread master ");
		assert(0);
	}

	cookie_ = 1;
	xc_id_  = 1;

	tnrcs_.clear();

	start_time_ = quagga_time(NULL);

	METOUT();

	return true;
}

bool
TNRC_Master::destroy(void)
{
	METIN();
	std::list<TNRC::TNRC_AP *>::iterator iter;

	TNRC_DBG("Finalizing TNRC_Master");

	for (iter = tnrcs_.begin(); iter != tnrcs_.end(); iter++) {
		delete (*iter);
	}
	tnrcs_.clear();

	delete instance_;
	instance_ = 0;

	thread_master_free(master_);

	start_time_ = 0;

	METOUT();

	return true;
}

TNRC_Master &
TNRC_Master::instance(void)
{
	METIN();

	if (instance_ == 0) {
		TNRC_DBG("TNRC_Master singleton instance is empty\n");

		instance_ = new TNRC_Master;
		if (!instance_) {
			TNRC_ERR("Cannot allocate enough memory ...");
			METOUT();
			assert(0);
		}
	}

	METOUT();

	return * instance_;
}

struct thread_master *
TNRC_Master::getMaster(void)
{
	METIN();

	METOUT();
	return master_;
}

bool
TNRC_Master::test_mode(void)
{
	return test_mode_;
}

void
TNRC_Master::test_mode(bool val)
{
	test_mode_ = val;
}

char*
TNRC_Master::test_file(void)
{
	return (char *) test_file_.c_str();
}

void
TNRC_Master::test_file(std::string loc)
{
	test_file_ = loc;
}

TNRC::TNRC_AP *
TNRC_Master::getTNRC(void)
{
	if (tnrcs_.empty()) {
		return NULL;
	}
	return tnrcs_.front();
}

void
TNRC_Master::pc(Pcontainer * PC)
{
	PC_ = PC;
}

Pcontainer *
TNRC_Master::getPC (void)
{
	return PC_;
}

u_int
TNRC_Master::n_instances(void)
{
	return (u_int)tnrcs_.size();
}

tnrcap_cookie_t
TNRC_Master::new_cookie()
{
	return cookie_++;
}

uint32_t
TNRC_Master::new_xc_id()
{
	return xc_id_++;
}

eqpt_type_t
TNRC_Master::getEqpt_type(eqpt_id_t id)
{
	eqpt_type_t   type;
	TNRC::Eqpt  * e;

	e = getTNRC()->getEqpt(id);
	if (e == NULL) {
		return EQPT_UNDEFINED;
	}
	type = e->type();

	return type;
}

bool
TNRC_Master::attach_instance(TNRC::TNRC_AP * t)
{
	std::list<TNRC::TNRC_AP *>::iterator iter;

	for (iter = tnrcs_.begin(); iter != tnrcs_.end(); iter++) {
		if ((*iter) == t) {
			TNRC_ERR("TNRC instance already "
				 "attached to TNRC_Master");
			METOUT();
			return false;
		}
	}

	tnrcs_.push_back(t);

	TNRC_DBG ("TNRC instance correctly added in the TNRC_Master list. "\
		  "There are %d instances running", tnrcs_.size());
	return true;
}

bool
TNRC_Master::detach_instance(TNRC::TNRC_AP * t)
{
	assert(t);

	std::list<TNRC::TNRC_AP *>::iterator iter;

	for (iter = tnrcs_.begin(); iter != tnrcs_.end(); iter++) {
		if ((*iter) == t) {
			tnrcs_.erase(iter);
			TNRC_DBG("TNRC instance detached "
				 "from TNRC_Master."
				 "There are %d instances running",
				 tnrcs_.size());
			METOUT();
			return true;
		}
	}

	TNRC_DBG("TNRC instance not found in PCERA_Master");

	METOUT();

	return false;

}

Plugin*
TNRC_Master::getPlugin(void)
{
	return plugin_;
}

void
TNRC_Master::installPlugin(Plugin* p)
{
	plugin_ = p;
}

std::string
TNRC_Master::plugin_location(void)
{
	return plugin_location_;
}

void
TNRC_Master::plugin_location(std::string loc)
{
	plugin_location_ = loc;
}

bool
TNRC_Master::api_queue_insert(api_queue_element_t * e)
{
	if (api_aq_.size() >= TNRC_MAX_API_QUEUE_SIZE) {
		TNRC_DBG ("Cannot insert element in API Queue. Reached "
			  "TNRC_MAX_API_QUEUE_SIZE");
		return false;
	}

	return api_aq_.insert(e);
}

api_queue_element_t*
TNRC_Master::api_queue_extract(void)
{
	return api_aq_.extract();
}

int
TNRC_Master::api_queue_size(void)
{
	return api_aq_.size();
}

void
TNRC_Master::api_queue_process(void)
{
	api_queue_element_t	* el;
	Action			* a;
	tnrc_rsrv_xc_flag_t	  rsrv_flag;

	//Extract first element from the queue
	el = api_queue_extract();
	if (el == NULL) {
		TNRC_WRN ("api_queue_process: Api queue is empty. "
			  "No actions to execute");
		return;
	}
	//Get type of action to execute
	switch (el->type) {
		case MAKE_XC:
			process_make_xc(el);
			break;
		case DESTROY_XC:
			process_destroy_xc(el);
			break;
		case RESERVE_XC:
			process_reserve_xc(el);
			break;
		case UNRESERVE_XC:
			process_unreserve_xc(el);
			break;
		default:
			TNRC_DBG("Action type not supported. Discard action");
			break;
	}
}

void
TNRC_Master::process_make_xc(api_queue_element_t * el)
{
	Action *             a;
	MakeDestroyxc *      make_act;
	make_xc_param_t *    make_xc;
	bool                 advance_rsrv;
	bool                 executed;
	tnrc_rsrv_xc_flag_t  rsrv_flag;
	long                 start;
	long                 end;

	TNRC_DBG ("MAKE XC action initializing...");

	//get specific action parameters
	make_xc = (make_xc_param_t *) el->action_param;
	rsrv_flag = make_xc->rsrv_xc_flag;

	advance_rsrv = false;
	executed     = true;
	start        = 0;
	end          = 0;
	//create a new Action object
	make_act = new MakeDestroyxc(make_xc->cookie,
				     make_xc->resp_ctxt,
				     make_xc->async_ctxt,
				     el->type,
				     make_xc->dl_id_in,
				     make_xc->labelid_in,
				     make_xc->dl_id_out,
				     make_xc->labelid_out,
				     make_xc->dir,
				     make_xc->is_virtual ? true : false,
				     advance_rsrv,
				     start,
				     end,
				     make_xc->plugin);

	//Set reservation XC flag
	make_act->rsrv_xc_flag(rsrv_flag);
	//Check the presence of a RESERVE XC action executed
	if (rsrv_flag == ACTIVATE) {
		if (!make_act->check_rsrv(make_xc->rsrv_cookie)) {
			//respond negatively to client
			action_response(make_xc->cookie,
					MAKE_XC,
					TNRCSP_RESULT_PARAMERROR,
					executed,
					make_xc->resp_ctxt);
			//we can delete api_queue_element_t
			delete el;
			delete make_xc;
			delete make_act;
			//check if there are any operation requests queued
			if (api_queue_size() != 0) {
				//there are other requests in the api queue
				//schedule a new event thread
				thread_add_event(getMaster(),
						 process_api_queue,
						 NULL, 0);
			}
			return;
		} else {
			Action             * rsrv;
			ReserveUnreservexc * rsrv_act;

			rsrv = getAction(make_xc->rsrv_cookie);
			rsrv_act = dynamic_cast<ReserveUnreservexc *>(rsrv);
			//Set XC id
			make_act->xc_id(rsrv_act->xc_id());
		}
	} else {
		//check if resources are free
		if (!make_act->check_resources()) {
			//respond negatively to client
			action_response(make_xc->cookie,
					MAKE_XC,
					TNRCSP_RESULT_PARAMERROR,
					executed,
					make_xc->resp_ctxt);
			//we can delete api_queue_element_t
			delete el;
			delete make_xc;
			delete make_act;
			//check if there are any operation requests queued
			if (api_queue_size() != 0) {
				//there are other requests in the api queue
				//schedule a new event thread
				thread_add_event(getMaster(),
						 process_api_queue,
						 NULL, 0);
			}
			return;
		}
		//Set XC id
		make_act->xc_id(new_xc_id());
	}
	a = make_act;
	// Push first atomic action
	make_act->push_todo (a);
	//Set reservation cookie
	make_act->rsrv_cookie(make_xc->rsrv_cookie);
	// Check if MAKE_XC is bidirectional
	if (make_xc->dir == XCDIRECTION_BIDIR       &&
	    (!make_act->plugin()->xc_bidir_support())) {
		// Add another atomic action
		MakeDestroyxc   * at_act;
		at_act = new MakeDestroyxc(make_xc->cookie,
					   make_xc->resp_ctxt,
					   make_xc->async_ctxt,
					   el->type,
					   make_xc->dl_id_out,
					   make_xc->labelid_out,
					   make_xc->dl_id_in,
					   make_xc->labelid_in,
					   make_xc->dir,
					   make_xc->is_virtual ? true : false,
					   advance_rsrv,
					   start,
					   end,
					   make_xc->plugin);
		a = at_act;
		//Set reservation XC flag
		at_act->rsrv_xc_flag(rsrv_flag);
		//Set reservation cookie
		at_act->rsrv_cookie(make_xc->rsrv_cookie);
		//Set XC id
		if (rsrv_flag == ACTIVATE) {
			at_act->xc_id(make_act->xc_id() + 1);
		} else {
			at_act->xc_id(new_xc_id());
		}
		// Push atmoic action
		make_act->push_todo(a);
		// Set correctly action pointer
		a = make_act;
		TNRC_DBG ("Requesting MAKE XC bidirectional: "
			  "there will be two atomic actions");
	}
	//Put the action object created in the map of actions in execution
	attach_action (a->ap_cookie(), a);
	//Action object is ready, we can delete api_queue_element_t
	delete el;
	delete make_xc;
	//Now we can start the FSM for the action, posting an ActionCreate event
	TNRC_DBG ("Post ActionCreate event for MAKE XC action");
	a->fsm_start();
	a->fsm_post(fsm::TNRC::virtFsm::TNRC_ActionCreate, a);

}

void
TNRC_Master::process_destroy_xc(api_queue_element_t * el)
{
	Action *             a;
	tnrc_rsrv_xc_flag_t  rsrv_flag;
	destroy_xc_param_t * destroy_xc;
	MakeDestroyxc *      destroy_act;
	bool                 executed;

	TNRC_DBG ("DESTROY XC action initializing...");

	executed = true;

	//get specific action parameters
	destroy_xc = (destroy_xc_param_t *) el->action_param;
	rsrv_flag = destroy_xc->rsrv_xc_flag;

	//fetch the correct MAKE XC action to destroy
	a = getAction (destroy_xc->cookie);
	if ((a == NULL) || (a->action_type() != MAKE_XC)) {
		TNRC_ERR("Cannot DESTROY XC with cookie %d: "
			 "action neither in execution nor "
			 "executed by TNRC.",
			 destroy_xc->cookie);
		//respond negatively to client
		action_response(destroy_xc->cookie,
				DESTROY_XC,
				TNRCSP_RESULT_PARAMERROR,
				executed,
				destroy_xc->resp_ctxt);
		//we can delete api_queue_element_t
		delete el;
		delete destroy_xc;
		//check if there are any operation requests queued
		if (api_queue_size() != 0) {
			//there are other requests in the api queue
			//schedule a new event thread
			thread_add_event(getMaster(),
					 process_api_queue,
					 NULL, 0);
		}
		return;
	}
	//if there's the MAKE XC action, update the response parameter
	a->resp_ctxt(destroy_xc->resp_ctxt);
	//update parameters for atomic actions set correctly reservation xc flag
	destroy_act = dynamic_cast<MakeDestroyxc *>(a);
	destroy_act->rsrv_xc_flag(rsrv_flag);
	//update parameters for atomic actions
	if (a->have_atomic()) {
		Action::iterator_atomic_done iter;
		MakeDestroyxc *              atomic;
		for(iter  = a->begin_atomic_done();
		    iter != a->end_atomic_done();
		    iter++) {
			atomic = dynamic_cast<MakeDestroyxc *>
				(*iter);
			atomic->resp_ctxt(destroy_xc->resp_ctxt);
			atomic->rsrv_xc_flag(rsrv_flag);
		}
	}
	//Action object is ready, we can delete api_queue_element_t
	delete el;
	delete destroy_xc;
	//then post an ActionDestroy event for the relative FSM
	TNRC_DBG("Post ActionDestroy event for MAKE XC action");
	a->fsm_post(fsm::TNRC::virtFsm::TNRC_ActionDestroy, a);

}

void
TNRC_Master::process_reserve_xc(api_queue_element_t * el)
{

	Action *             a;
	ReserveUnreservexc * reserve_act;
	reserve_xc_param_t * reserve_xc;
	bool                 deactivation;
	bool                 advance_rsrv;
	bool                 executed;
	tnrc_rsrv_xc_flag_t  rsrv_flag;

	TNRC_DBG ("RESERVE XC action initializing...");

	deactivation = false;
	executed     = true;

	//get specific action parameters
	reserve_xc = (reserve_xc_param_t *) el->action_param;

	//create a new Action object
	reserve_act = new ReserveUnreservexc(reserve_xc->cookie,
					     reserve_xc->resp_ctxt,
					     el->type,
					     deactivation,
					     reserve_xc->dl_id_in,
					     reserve_xc->labelid_in,
					     reserve_xc->dl_id_out,
					     reserve_xc->labelid_out,
					     reserve_xc->dir,
					     reserve_xc->is_virtual ? true : false,
					     reserve_xc->advance_rsrv,
					     reserve_xc->start,
					     reserve_xc->end,
					     reserve_xc->plugin);

	//check if resources are free
	if (!reserve_act->check_resources()) {
		//respond negatively to client
		action_response(reserve_xc->cookie,
				RESERVE_XC,
				TNRCSP_RESULT_PARAMERROR,
				executed,
				reserve_xc->resp_ctxt);
		//we can delete api_queue_element_t
		delete el;
		delete reserve_xc;
		delete reserve_act;
		//check if there are any operation requests queued
		if (api_queue_size() != 0) {
			//there are other requests in the api queue
			//schedule a new event thread
			thread_add_event(getMaster(),
					 process_api_queue,
					 NULL, 0);
		}
		return;
	}
	//Set XC id
	reserve_act->xc_id(new_xc_id());
	a = reserve_act;
	//Push first atomic action
	reserve_act->push_todo (a);
	// Check if RESERVE_XC is bidirectional
	if ((reserve_xc->dir == XCDIRECTION_BIDIR)       &&
	    (!reserve_act->plugin()->xc_bidir_support())) {
		// Add another atomic action
		ReserveUnreservexc   * at_act;
		at_act = new ReserveUnreservexc(reserve_xc->cookie,
						reserve_xc->resp_ctxt,
						el->type,
						deactivation,
						reserve_xc->dl_id_out,
						reserve_xc->labelid_out,
						reserve_xc->dl_id_in,
						reserve_xc->labelid_in,
						reserve_xc->dir,
						reserve_xc->is_virtual ? true : false,
						reserve_xc->advance_rsrv,
						reserve_xc->start,
						reserve_xc->end,
						reserve_xc->plugin);
		a = at_act;
		//Set XC id
		at_act->xc_id(new_xc_id());
		// Push atmoic action
		reserve_act->push_todo(a);
		// Set correctly action pointer
		a = reserve_act;
		TNRC_DBG ("Requesting RESERVE XC bidirectional:"
			  " there will be two atomic actions");
	}
	//Put the action object created in the map of actions in execution
	attach_action (a->ap_cookie(), a);
	//Action object is ready, we can delete api_queue_element_t
	delete el;
	delete reserve_xc;
	//Check if action is ADVANCE RESERVATION
	if (reserve_act->advance_rsrv()) {
		struct timeval  t;
		long            timer;
		long            start_time;
		tm *            curTime;
		const char *    timeStringFormat = "%a, %d %b %Y %H:%M:%S %zGMT";
		const int       timeStringLength = 40;
		char            timeString[timeStringLength];
		struct thread * thr;
		executed   = false;
		start_time = reserve_act->start();
		//respond positively to client (fake answer)
		action_response(reserve_act->ap_cookie(),
				RESERVE_XC,
				TNRCSP_RESULT_NOERROR,
				executed,
				reserve_act->resp_ctxt());
		//Get current time
		gettimeofday(&t, NULL);
		//Get timer
		timer = start_time - t.tv_sec;
		//Update DLink details (calendar) in LRM
		TNRC::TNRC_AP * tnrc;
		TNRC::Port *    p;
		eqpt_id_t       e_id;
		board_id_t      b_id;
		port_id_t       p_id;
		// IN DLink
		TNRC_CONF_API::convert_tnrc_port_id(&e_id,
						    &b_id,
						    &p_id,
						    reserve_act->portid_in());
		//get instance
		tnrc = TNRC_MASTER.getTNRC();
		//check if port id exist in the data model
		p = tnrc->getPort(e_id, b_id, p_id);
		if (p == NULL) {
			TNRC_ERR ("In Data-link id uncorrect: no such port");
			return;
		}
		update_datalink_details(p);
		// OUT DLink
		TNRC_CONF_API::convert_tnrc_port_id(&e_id,
						    &b_id,
						    &p_id,
						    reserve_act->portid_out());
		//check if port id exist in the data model
		p = tnrc->getPort(e_id, b_id, p_id);
		if (p == NULL) {
			TNRC_ERR ("Out Data-link id uncorrect: no such port");
			return;
		}
		update_datalink_details(p);

		curTime = localtime((time_t *) &start_time);
		strftime(timeString, timeStringLength, timeStringFormat, curTime);
		if (timer <= 0) {
			TNRC_DBG ("An ADVANCE RESERVATION "
				  "had to start %s",
				  timeString);
			TNRC_DBG ("Schedule an event to get it start");
			thr = thread_add_event(TNRC_MASTER.
					       getMaster(),
					       start_advance_rsrv,
					       reserve_act,
					       0);
			//Set the thread
			reserve_act->thread_start(thr);
			return;
		}
		TNRC_DBG ("Schedule ActionCreate timer for "
			  "ADVANCE RESERVATION, starting %s",
			  timeString);
		thr = thread_add_timer(TNRC_MASTER.getMaster(),
				       start_advance_rsrv,
				       reserve_act,
				       timer);
		//Set the thread
		reserve_act->thread_start(thr);
		//Set waiting to start flag
		reserve_act->waiting_to_start(true);
	} else {
		//Now we can start the FSM for the action,
		//posting an ActionCreate event
		TNRC_DBG ("Post ActionCreate event for RESERVE XC action");
		a->fsm_start();
		a->fsm_post(fsm::TNRC::virtFsm::TNRC_ActionCreate, a);
	}

}

void
TNRC_Master::process_unreserve_xc(api_queue_element_t * el)
{
	Action *               a;
	ReserveUnreservexc *   reserve;
	unreserve_xc_param_t * unreserve_xc;
	bool                   executed;

	TNRC_DBG ("UNRESERVE XC action initializing...");

	executed = true;

	//get specific action parameters
	unreserve_xc = (unreserve_xc_param_t *) el->action_param;

	//fetch the correct RESERVE XC action to destroy
	a = getAction (unreserve_xc->cookie);
	if ((a != NULL) && (a->action_type() == MAKE_XC)) {
		MakeDestroyxc * make;
		make = dynamic_cast<MakeDestroyxc *>(a);
		if (make->advance_rsrv()) {
			destroy_xc_param_t * destr_xc;
			tnrc_boolean_t       deactivate = 0;
			TNRC_DBG("Action is ADVANCE RESERVATION: "
				 "going to destroy it");
			destr_xc = new_destroy_xc_element(unreserve_xc->
							  cookie,
							  deactivate,
							  unreserve_xc->
							  resp_ctxt);
			el->type = DESTROY_XC;
			el->action_param = destr_xc;
			delete unreserve_xc;
			//call method to destroy xc
			process_destroy_xc(el);
			return;
		}
	}
	if ((a == NULL) || ((a->action_type() != RESERVE_XC))) {
		TNRC_ERR("Cannot UNRESERVE XC with cookie %d: "
			 "action neither in execution nor "
			 "executed by TNRC.",
			 unreserve_xc->cookie);
		//respond negatively to client
		action_response(unreserve_xc->cookie,
				UNRESERVE_XC,
				TNRCSP_RESULT_PARAMERROR,
				executed,
				unreserve_xc->resp_ctxt);
		//we can delete api_queue_element_t
		delete el;
		delete unreserve_xc;
		//check if there are any operation
		//requests queued
		if (api_queue_size() != 0) {
			//there are other requests in the api queue
			//schedule a new event thread
			thread_add_event(getMaster(),
					 process_api_queue,
					 NULL, 0);
		}
		return;
	}
	//if there's the RESERVE XC action, update the response callback
	//parameters
	a->resp_ctxt(unreserve_xc->resp_ctxt);
	if (a->have_atomic()) {
		Action::iterator_atomic_done iter;
		Action *                     atomic;
		for(iter  = a->begin_atomic_done();
		    iter != a->end_atomic_done();
		    iter++) {
			atomic = (*iter);
			atomic->resp_ctxt(unreserve_xc->resp_ctxt);
		}
	}
	//Action object is ready, we can delete api_queue_element_t
	delete el;
	delete unreserve_xc;
	//Check if action is ADVANCE RESERVATION not started
	reserve = dynamic_cast<ReserveUnreservexc *>(a);
	if (reserve->waiting_to_start()) {
		TNRC_DBG("ADVANCE RESERVATION not yet "
			 "started: delete action scheduled");
		//respond positively to client
		action_response(reserve->ap_cookie(),
				UNRESERVE_XC,
				TNRCSP_RESULT_NOERROR,
				executed,
				reserve->resp_ctxt());
		//detach action
		detach_action (a->ap_cookie());
		//detach reservations in data model
		reserve->cancel_advance_rsrv();
		//delete action
		delete reserve;
		return;
	}
	//then post an ActionDestroy event for the relative FSM
	TNRC_DBG ("Post ActionDestroy event for "
		  "RESERVE XC action");
	a->fsm_post (fsm::TNRC::virtFsm::TNRC_ActionDestroy, a);

}

u_int
TNRC_Master::api_queue_tot_request(void)
{
	return api_aq_.tot_request();
}

bool
TNRC_Master::attach_action(tnrcap_cookie_t ck, Action *a)
{
	iterator_actions                  iter;
	std::pair<iterator_actions, bool> ret;

	//try to insert the Action object
	ret = actions_.insert(std::pair<tnrcap_cookie_t, Action *> (ck, a));
	if (ret.second == false) {
		TNRC_WRN("Action with key %d is already registered", ck);
		return false;
	}

	return true;
}

bool
TNRC_Master::detach_action(tnrcap_cookie_t ck)
{
	iterator_actions iter;

	iter = actions_.find(ck);
	if (iter == actions_.end()) {
		TNRC_WRN ("Action with key %d is not registered", ck);
		return false;
	}
	actions_.erase (iter);

	return true;
}

Action *
TNRC_Master::getAction(tnrcap_cookie_t ck)
{
	iterator_actions iter;

	iter = actions_.find(ck);
	if (iter == actions_.end()) {
		TNRC_WRN("Action with key %d is not registered", ck);
		return NULL;
	}

	return (*iter).second;
}

bool
TNRC_Master::attach_xc(u_int id, XC *xc)
{
	iterator_xcs iter;
	std::pair<iterator_xcs, bool> ret;

	//try to insert the Action object
	ret = xcs_.insert(std::pair<u_int, XC *> (id, xc));
	if (ret.second == false) {
		TNRC_WRN ("XC with key %d is already registered", id);
		return false;
	}

	return true;
}

bool
TNRC_Master::detach_xc(u_int id)
{
	iterator_xcs iter;

	iter = xcs_.find(id);
	if (iter == xcs_.end()) {
		TNRC_WRN ("XC with key %d is not registered", id);
		return false;
	}
	xcs_.erase (iter);

	return true;
}

XC *
TNRC_Master::getXC(u_int id)
{
	iterator_xcs iter;

	iter = xcs_.find(id);
	if (iter == xcs_.end()) {
		TNRC_WRN("XC with key %d is not registered", id);
		return NULL;
	}

	return (*iter).second;
}

int
TNRC_Master::n_xcs (void)
{
	return (int) xcs_.size();
}

//////////////////////////////////////
/////////////////////////////////////
////VERY TMP - TESTING FUNCTIONS////
///////////////////////////////////
//////////////////////////////////

//VERY TMP !!
int
read_test_file(struct thread *t)
{
	//read test file
	vty_read_config(TNRC_MASTER.test_file(), NULL);

	return 0;
}

