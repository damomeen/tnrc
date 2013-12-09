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
#include "tnrc_plugin.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_g2rsvpte_api.h"


//////////////////////////////////
////////Class Action/////////
/////////////////////////////////

Plugin *
Action::plugin(void)
{
	return plugin_;
}

Action *
Action::atomic(void)
{
	return atomic_;
}

void
Action::atomic(Action * at)
{
	atomic_ = at;
}

tnrcap_cookie_t
Action::ap_cookie(void)
{
	return ap_cookie_;
}

void
Action::sp_handle(tnrcsp_handle_t h)
{
	sp_handle_ = h;
}

tnrcsp_handle_t
Action::sp_handle(void)
{
	return sp_handle_;
}

long
Action::resp_ctxt(void)
{
	return resp_ctxt_;
}

void
Action::resp_ctxt(long ctxt)
{
	resp_ctxt_ = ctxt;
}

long
Action::async_ctxt(void)
{
	return async_ctxt_;
}

tnrc_action_type_t
Action::action_type(void)
{
	return action_type_;
}

void
Action::action_type(tnrc_action_type_t type)
{
	action_type_ = type;
}

void
Action::prel_check(tnrcsp_result_t pc)
{
	prel_check_ = pc;
}

tnrcsp_result_t
Action::prel_check  (void)
{
	return prel_check_;
}

void
Action::eqpt_resp(tnrcsp_result_t res)
{
	eqpt_resp_ = res;
}

tnrcsp_result_t
Action::eqpt_resp(void)
{
	return eqpt_resp_;
}

void
Action::have_atomic(bool atomic)
{
	have_atomic_ = atomic;
}

bool
Action::have_atomic(void)
{
	return have_atomic_;
}

void
Action::is_virtual(bool is_virt)
{
	is_virtual_ = is_virt;
}

bool
Action::is_virtual(void)
{
	return is_virtual_;
}

bool
Action::have_atomic_todo(void)
{
	if (atomic_todo_.empty()) {
		return false;
	}
	return true;
}

bool
Action::have_atomic_todestroy(void)
{
	if (atomic_done_.empty()) {
		return false;
	}
	return true;
}

bool
Action::wait_answer(void)
{
	return wait_answer_;
}

void
Action::wait_answer(bool val)
{
	wait_answer_ = val;
}

void
Action::pop_todo()
{
	atomic_todo_.pop_front();
}

Action *
Action::front_todo(void)
{
	return atomic_todo_.front();
}

void
Action::push_todo(Action * at)
{
	atomic_todo_.push_back(at);

	//Push this action in atomic_actions_ deque too
	atomic_actions_.push_back(at);
}

int
Action::todo_size(void)
{
	return (int) atomic_todo_.size();
}

void
Action::pop_done()
{
	atomic_done_.pop_front();
}

Action*
Action::front_done(void)
{
	return atomic_done_.front();
}

void
Action::push_done (Action * at)
{
	atomic_done_.push_front(at);
}

int
Action::done_size(void)
{
	return (int) atomic_done_.size();
}

void
Action::swap_action_type(void)
{
	iterator_atomic_done   iter;
	Action		     * atomic;

	for (iter = begin_atomic_done(); iter != end_atomic_done(); iter++) {
		atomic = *iter;
		//swap correctly action type for each atomic action to destroy
		switch (atomic->action_type()) {
			case MAKE_XC:
				atomic->action_type(DESTROY_XC);
				break;
			case PROTECT_XC:
				atomic->action_type(UNPROTECT_XC);
				break;
			case RESERVE_XC:
				atomic->action_type(UNRESERVE_XC);
				break;
		}
	}
}

int
Action::n_retry(void)
{
	return n_retry_;
}

void
Action::n_retry_inc()
{
	n_retry_++;
}


////////////////////////////////////////
////////Class MakeDestroyxc////////////
///////////////////////////////////////

MakeDestroyxc::MakeDestroyxc(tnrcap_cookie_t          ck,
			     long                     resp_ctxt,
			     long                     notify_ctxt,
			     tnrc_action_type_t       type,
			     tnrc_port_id_t           p_in,
			     label_t                  l_in,
			     tnrc_port_id_t           p_out,
			     label_t                  l_out,
			     xcdirection_t            dir,
			     bool                     is_virtual,
			     bool                     advance_rsrv,
			     long                     start,
			     long                     end,
			     Plugin *                 p)
{
	ap_cookie_     = ck;
	resp_ctxt_     = resp_ctxt;
	async_ctxt_    = notify_ctxt;
	action_type_   = type;
	n_retry_       = 0;
	portid_in_     = p_in;
	labelid_in_    = l_in;
	portid_out_    = p_out;
	labelid_out_   = l_out;
	direction_     = dir;
	is_virtual_    = is_virtual;
	advance_rsrv_  = advance_rsrv;
	start_         = start;
	end_           = end;
	thread_end_    = 0;
	plugin_        = p;
	FSM_           =  0;
	if((direction_ == XCDIRECTION_BIDIR) &&
	   (!plugin_->xc_bidir_support()   )) {
		have_atomic_ = true;
	} else {
		have_atomic_ = false;
	}
}

MakeDestroyxc::~MakeDestroyxc(void)
{
	iterator_atomic_actions iter;
	Action *                atomic;

	//Cancel advance reservation thread
	if (thread_end_) {
		thread_cancel(thread_end_);
	}

	//Pop first atomic (this action)
	if (atomic_actions_.size()) {
		atomic_actions_.pop_front();
	}
	//Delete all atomics
	for(iter  = begin_atomic_actions();
	    iter != end_atomic_actions();
	    iter++) {
		atomic = dynamic_cast<Action *> (*iter);
		//delete atomic
		switch (atomic->action_type()) {
			case MAKE_XC:
			case DESTROY_XC: {
				MakeDestroyxc * action;
				action = static_cast<MakeDestroyxc *>
					(atomic);
				delete action;
				break;
			}
			case RESERVE_XC:
			case UNRESERVE_XC: {
				ReserveUnreservexc * action;
				action = static_cast<ReserveUnreservexc *>
					(atomic);
				delete action;
				break;
			}
			default:
				delete atomic;
				break;
		}
	}
}

u_int
MakeDestroyxc::xc_id(void)
{
	return xc_id_;
}
void
MakeDestroyxc::xc_id(u_int id)
{
	xc_id_ = id;
}

tnrc_port_id_t
MakeDestroyxc::portid_in(void)
{
	return portid_in_;
}

label_t
MakeDestroyxc::labelid_in(void)
{
	return labelid_in_;
}

tnrc_port_id_t
MakeDestroyxc::portid_out(void)
{
	return portid_out_;
}

label_t
MakeDestroyxc::labelid_out(void)
{
	return labelid_out_;
}

xcdirection_t
MakeDestroyxc::direction(void)
{
	return direction_;
}

bool
MakeDestroyxc::advance_rsrv(void)
{
	return advance_rsrv_;
}

long
MakeDestroyxc::start(void)
{
	return start_;
}

long
MakeDestroyxc::end(void)
{
	return end_;
}

void
MakeDestroyxc::cancel_advance_rsrv(void)
{
	TNRC::TNRC_AP *  tnrc;
	TNRC::Resource * r_in;
	TNRC::Resource * r_out;
	eqpt_id_t        e_id_in;
	eqpt_id_t        e_id_out;
	board_id_t       b_id_in;
	board_id_t       b_id_out;
	port_id_t        p_id_in;
	port_id_t        p_id_out;

	//get instance
	tnrc = TNRC_MASTER.getTNRC();
	if (tnrc == NULL) {
		return;
	}
	TNRC_CONF_API::convert_tnrc_port_id(&e_id_in,
					    &b_id_in,
					    &p_id_in,
					    portid_in_);
	TNRC_CONF_API::convert_tnrc_port_id(&e_id_out,
					    &b_id_out,
					    &p_id_out,
					    portid_out_);

	r_in  = tnrc->getResource(e_id_in, b_id_in, p_id_in, labelid_in_);
	r_out = tnrc->getResource(e_id_out, b_id_out, p_id_out, labelid_out_);
	if ((r_in == NULL) || (r_out == NULL)) {
		return;
	}
	r_in->detach(start_);
	r_out->detach(start_);
}

tnrc_rsrv_xc_flag_t
MakeDestroyxc::rsrv_xc_flag(void)
{
	return rsrv_xc_flag_;
}
void
MakeDestroyxc::rsrv_xc_flag(tnrc_rsrv_xc_flag_t flag)
{
	rsrv_xc_flag_ = flag;
}

tnrcap_cookie_t
MakeDestroyxc::rsrv_cookie(void)
{
	return rsrv_cookie_;
}

void
MakeDestroyxc::rsrv_cookie(tnrcap_cookie_t cookie)
{
	rsrv_cookie_ = cookie;
}

bool
MakeDestroyxc::check_rsrv(tnrcap_cookie_t ck)
{
	Action	           * a;
	ReserveUnreservexc * rsrv;

	a = TNRC_MASTER.getAction(ck);
	if (a == NULL) {
		TNRC_ERR ("No action with cookie %d is present", ck);
		return false;
	}
	if (a->action_type() != RESERVE_XC) {
		TNRC_ERR ("No RESERVE XC action with cookie %d is present", ck);
		return false;
	}
	rsrv = dynamic_cast<ReserveUnreservexc *>(a);
	if ((portid_in()     != rsrv->portid_in()   ) ||
	    (portid_out()    != rsrv->portid_out()  ) ||
	    (!ARE_LABEL_EQUAL(labelid_in(), rsrv->labelid_in()) ) ||
	    (!ARE_LABEL_EQUAL(labelid_out(), rsrv->labelid_out())) ||
	    (direction()     != rsrv->direction()   )) {
		TNRC_ERR ("Wrong rsrv-cookie parameter", ck);
		return false;
	}
	return true;
}

bool
MakeDestroyxc::check_resources(void)
{
	eqpt_id_t        e_id_in;
	board_id_t       b_id_in;
	port_id_t        p_id_in;
	eqpt_id_t        e_id_out;
	board_id_t       b_id_out;
	port_id_t        p_id_out;
	TNRC::TNRC_AP  * t;
	TNRC::Resource * r;

	TNRC_CONF_API::convert_tnrc_port_id(&e_id_in, &b_id_in,
					    &p_id_in, portid_in_);
	TNRC_CONF_API::convert_tnrc_port_id(&e_id_out, &b_id_out,
					    &p_id_out, portid_out_);

	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		TNRC_ERR ("check_resources: no instance running");
		return false;
	}
	r = t->getResource (e_id_in, b_id_in, p_id_in, labelid_in_);
	if (r == NULL) {
		return false;
	}
	//check IN resource
	if (!is_virtual_) {
		if (!CHECK_RESOURCE_FREE(r)) {
			TNRC_ERR ("Resource IN is not free");
			return false;
		}
	}
	r = t->getResource (e_id_out, b_id_out, p_id_out, labelid_out_);
	if (r == NULL) {
		return false;
	}
	//check OUT resource
	if (!is_virtual_) {
		if (!CHECK_RESOURCE_FREE(r)) {
			TNRC_ERR ("Resource OUT is not free");
			return false;
		}
	}

	return true;
}

void
MakeDestroyxc::thread_end(struct thread *thr)
{
	thread_end_ = thr;
}

void
MakeDestroyxc::fsm_start(void)
{
	//Start FSM
	FSM_ = new fsm::TNRC::virtFsm(fsm::base_TNRC::BaseFSM::TRACE_LOG);
}

void
MakeDestroyxc::fsm_post (fsm::TNRC::virtFsm::root_events_t ev,
			 void *                            ctxt,
			 bool                              queue)
{
	FSM_->post(ev, ctxt, queue);
}

////////////////////////////////////////
////////Class ReserveUnreservexc////////
///////////////////////////////////////

ReserveUnreservexc::ReserveUnreservexc(tnrcap_cookie_t      ck,
				       long                 ctxt,
				       tnrc_action_type_t   type,
				       bool                 deactivation,
				       tnrc_port_id_t       p_in,
				       label_t              l_in,
				       tnrc_port_id_t       p_out,
				       label_t              l_out,
				       xcdirection_t        dir,
				       bool                 is_virtual,
				       bool                 advance_rsrv,
				       long                 start,
				       long                 end,
				       Plugin *             p)
{
	ap_cookie_        = ck;
	resp_ctxt_        = ctxt;
	action_type_      = type;
	deactivation_     = deactivation;
	n_retry_          = 0;
	portid_in_        = p_in;
	labelid_in_       = l_in;
	portid_out_       = p_out;
	labelid_out_      = l_out;
	direction_        = dir;
	is_virtual_       = is_virtual;
	advance_rsrv_     = advance_rsrv;
	start_            = start;
	end_              = end;
	waiting_to_start_ = false;
	plugin_           = p;
	thread_start_     = 0;
	FSM_              = 0;
	if((direction_ == XCDIRECTION_BIDIR) &&
	   (!plugin_->xc_bidir_support()   )) {
		have_atomic_ = true;
	} else {
		have_atomic_ = false;
	}
}

ReserveUnreservexc::~ReserveUnreservexc(void)
{
	iterator_atomic_actions iter;
	Action *                atomic;

	//Cancel advance reservation thread
	if (thread_start_) {
		thread_cancel(thread_start_);
	}

	//Pop first atomic (this action)
	atomic_actions_.pop_front();
	//Delete all atomics
	for(iter  = begin_atomic_actions();
	    iter != end_atomic_actions();
	    iter++) {
		atomic = dynamic_cast<Action *> (*iter);
		//delete atomic
		switch (atomic->action_type()) {
			case MAKE_XC:
			case DESTROY_XC: {
				MakeDestroyxc * action;
				action = static_cast<MakeDestroyxc *>
					(atomic);
				delete atomic;
				break;
			}
			case RESERVE_XC:
			case UNRESERVE_XC: {
				ReserveUnreservexc * action;
				action = static_cast<ReserveUnreservexc *>
					(atomic);
				delete atomic;
				break;
			}
			default:
				delete atomic;
				break;
		}
	}
}

u_int
ReserveUnreservexc::xc_id(void)
{
	return xc_id_;
}
void
ReserveUnreservexc::xc_id(u_int id)
{
	xc_id_ = id;
}

tnrc_port_id_t
ReserveUnreservexc::portid_in(void)
{
	return portid_in_;
}

label_t
ReserveUnreservexc::labelid_in(void)
{
	return labelid_in_;
}

tnrc_port_id_t
ReserveUnreservexc::portid_out(void)
{
	return portid_out_;
}

label_t
ReserveUnreservexc::labelid_out(void)
{
	return labelid_out_;
}

xcdirection_t
ReserveUnreservexc::direction(void)
{
	return direction_;
}

bool
ReserveUnreservexc::advance_rsrv(void)
{
	return advance_rsrv_;
}

long
ReserveUnreservexc::start(void)
{
	return start_;
}

long
ReserveUnreservexc::end(void)
{
	return end_;
}

bool
ReserveUnreservexc::waiting_to_start(void)
{
	return waiting_to_start_;
}

void
ReserveUnreservexc::waiting_to_start(bool wait)
{
	waiting_to_start_ = wait;
}

void
ReserveUnreservexc::cancel_advance_rsrv(void)
{
	TNRC::TNRC_AP *  tnrc;
	TNRC::Resource * r_in;
	TNRC::Resource * r_out;
	eqpt_id_t        e_id_in;
	eqpt_id_t        e_id_out;
	board_id_t       b_id_in;
	board_id_t       b_id_out;
	port_id_t        p_id_in;
	port_id_t        p_id_out;

	//get instance
	tnrc = TNRC_MASTER.getTNRC();
	if (tnrc == NULL) {
		return;
	}
	TNRC_CONF_API::convert_tnrc_port_id(&e_id_in,
					    &b_id_in,
					    &p_id_in,
					    portid_in_);
	TNRC_CONF_API::convert_tnrc_port_id(&e_id_out,
					    &b_id_out,
					    &p_id_out,
					    portid_out_);

	r_in  = tnrc->getResource(e_id_in, b_id_in, p_id_in, labelid_in_);
	r_out = tnrc->getResource(e_id_out, b_id_out, p_id_out, labelid_out_);
	if ((r_in == NULL) || (r_out == NULL)) {
		return;
	}
	r_in->detach(start_);
	r_out->detach(start_);
}

bool
ReserveUnreservexc::deactivation(void)
{
	return deactivation_;
}

bool
ReserveUnreservexc::check_resources(void)
{
	eqpt_id_t        e_id_in;
	board_id_t       b_id_in;
	port_id_t        p_id_in;
	eqpt_id_t        e_id_out;
	board_id_t       b_id_out;
	port_id_t        p_id_out;
	TNRC::TNRC_AP  * t;
	TNRC::Resource * r;

	TNRC_CONF_API::convert_tnrc_port_id(&e_id_in, &b_id_in,
					    &p_id_in, portid_in_);
	TNRC_CONF_API::convert_tnrc_port_id(&e_id_out, &b_id_out,
					    &p_id_out, portid_out_);

	t = TNRC_MASTER.getTNRC();
	if (t == NULL) {
		TNRC_ERR ("check_resources: no instance running");
		return false;
	}
	r = t->getResource (e_id_in, b_id_in, p_id_in, labelid_in_);
	if (r == NULL) {
		return false;
	}
	//check IN resource
	if (!is_virtual_) {
		if (!CHECK_RESOURCE_FREE(r)) {
			TNRC_ERR ("Resource IN is not free");
			return false;
		}
	}
	r = t->getResource (e_id_out, b_id_out, p_id_out, labelid_out_);
	if (r == NULL) {
		return false;
	}
	//check OUT resource
	if (!is_virtual_) {
		if (!CHECK_RESOURCE_FREE(r)) {
			TNRC_ERR ("Resource OUT is not free");
			return false;
		}
	}

	return true;
}

void
ReserveUnreservexc::thread_start(struct thread *thr)
{
	thread_start_ = thr;
}

void
ReserveUnreservexc::fsm_start(void)
{
	//Start FSM
	FSM_ = new fsm::TNRC::virtFsm(fsm::base_TNRC::BaseFSM::TRACE_LOG);
}

void
ReserveUnreservexc::fsm_post(fsm::TNRC::virtFsm::root_events_t ev,
			     void *                            ctxt,
			     bool                              queue)
{
	FSM_->post(ev, ctxt, queue);
}

/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////

make_xc_param_t *
new_make_xc_element(tnrcap_cookie_t *        cookiep,
		    tnrc_port_id_t           dl_id_in,
		    label_t                  labelid_in,
		    tnrc_port_id_t           dl_id_out,
		    label_t                  labelid_out,
		    xcdirection_t            dir,
		    tnrc_boolean_t           is_virtual,
		    tnrc_boolean_t           activate,
		    tnrcap_cookie_t          rsrv_cookie,
		    long                     response_ctxt,
		    long                     async_ctxt)
{
	make_xc_param_t * param;

	param = new make_xc_param_t;

	*cookiep = TNRC_MASTER.new_cookie();

	param->cookie      = *cookiep;
	param->dl_id_in    = dl_id_in;
	param->labelid_in  = labelid_in;
	param->dl_id_out   = dl_id_out;
	param->labelid_out = labelid_out;
	param->dir         = dir;
	param->is_virtual  = is_virtual;
	if (activate) {
		param->rsrv_xc_flag = ACTIVATE;
		param->rsrv_cookie  = rsrv_cookie;
	}
	else {
		param->rsrv_xc_flag = NO_RSRV;
		param->rsrv_cookie  = 0;
	}
	param->resp_ctxt  = response_ctxt;
	param->async_ctxt = async_ctxt;

	return param;
}

reserve_xc_param_t *
new_reserve_xc_element(tnrcap_cookie_t *    cookiep,
		       tnrc_port_id_t       dl_id_in,
		       label_t              labelid_in,
		       tnrc_port_id_t       dl_id_out,
		       label_t              labelid_out,
		       xcdirection_t        dir,
		       tnrc_boolean_t       is_virtual,
		       tnrc_boolean_t       advance_rsrv,
		       long                 start,
		       long                 end,
		       long                 response_ctxt)
{
	reserve_xc_param_t * param;

	param = new reserve_xc_param_t;

	*cookiep = TNRC_MASTER.new_cookie();

	param->cookie        = *cookiep;
	param->dl_id_in      = dl_id_in;
	param->labelid_in    = labelid_in;
	param->dl_id_out     = dl_id_out;
	param->labelid_out   = labelid_out;
	param->dir           = dir;
	param->is_virtual    = is_virtual;
	param->advance_rsrv  = advance_rsrv;
	param->start         = start;
	param->end           = end;
	param->resp_ctxt     = response_ctxt;

	return param;
}

destroy_xc_param_t *
new_destroy_xc_element(tnrcap_cookie_t      cookie,
		       tnrc_boolean_t       deactivate,
		       long                 response_ctxt)
{
	destroy_xc_param_t * param;

	param = new destroy_xc_param_t;

	param->cookie = cookie;
	if (deactivate) {
		param->rsrv_xc_flag = DEACTIVATE;
	}
	else {
		param->rsrv_xc_flag = NO_RSRV;
	}
	param->resp_ctxt = response_ctxt;

	return param;
}

unreserve_xc_param_t *
new_unreserve_xc_element(tnrcap_cookie_t      cookie,
			 long                 response_ctxt)
{
	unreserve_xc_param_t * param;

	param = new unreserve_xc_param_t;

	param->cookie    = cookie;
	param->resp_ctxt = response_ctxt;

	return param;
}

int
delete_action (struct thread *t)
{
	Action             * a;
	tnrc_action_type_t   act_type;
	bool                 detach;

	detach = THREAD_VAL (t);

	//get Action
	a = static_cast<Action *> (THREAD_ARG (t));
	//get action type
	act_type = a->action_type();

	//detach Action from map of actions in execution or executed
	if (detach) {
		TNRC_MASTER.detach_action (a->ap_cookie());
	}
	//delete Action object representing an Action in Down state
	switch (act_type) {
		case MAKE_XC:
		case DESTROY_XC: {
			MakeDestroyxc * action;
			action = static_cast<MakeDestroyxc *> (a);
			if (action->advance_rsrv()) {
				action->cancel_advance_rsrv();
			}
			delete action;
			break;
		}
		case RESERVE_XC:
		case UNRESERVE_XC: {
			ReserveUnreservexc * action;
			action = static_cast<ReserveUnreservexc *> (a);
			if (action->advance_rsrv()) {
				action->cancel_advance_rsrv();
			}
			delete action;
			break;
		}
		default:
			delete a;
			break;
	}

	TNRC_DBG ("Action %s deleted", SHOW_ACTION_TYPE(act_type));

	return 0;
}

int
deactivate_action (struct thread *t)
{
	Action             * a;
	MakeDestroyxc      * d;
	ReserveUnreservexc * reserve_io;
	tnrc_action_type_t   act_type;
	bool                 res;
	bool                 detach;
	bool                 advance_rsrv;
	long                 start;
	long                 end;

	detach = false;
	advance_rsrv = false;
	start = 0;
	end = 0;
	//get Action
	a = static_cast<Action *> (THREAD_ARG (t));
	//get action type
	act_type = a->action_type();

	//detach Action from map of actions in execution or executed
	TNRC_MASTER.detach_action (a->ap_cookie());

	d = dynamic_cast<MakeDestroyxc *> (a);
	//Create a Reserve action
	reserve_io = new ReserveUnreservexc(d->ap_cookie(),
					    0,
					    RESERVE_XC,
					    true,
					    d->portid_in(),
					    d->labelid_in(),
					    d->portid_out(),
					    d->labelid_out(),
					    d->direction(),
					    false,
					    advance_rsrv,
					    start,
					    end,
					    d->plugin());
	//Push atomic action
	reserve_io->push_todo(reserve_io);
	//Set XC id
	reserve_io->xc_id(d->xc_id());
	//post an event to allow action to reach Up state
	reserve_io->fsm_start();
	reserve_io->fsm_post(fsm::TNRC::virtFsm::TNRC_ActionCreate,
			     reserve_io);
	//Check if XC is bidirectional
	if ((d->direction() == XCDIRECTION_BIDIR) &&
	    (!d->plugin()->xc_bidir_support()   )) {
		//Create another Reserve Action
		ReserveUnreservexc * reserve_oi;
		reserve_oi = new ReserveUnreservexc(d->ap_cookie(),
						    0,
						    RESERVE_XC,
						    true,
						    d->portid_out(),
						    d->labelid_out(),
						    d->portid_in(),
						    d->labelid_in(),
						    d->direction(),
						    true,
						    advance_rsrv,
						    start,
						    end,
						    d->plugin());
		//Push atomic action
		reserve_io->push_todo(reserve_oi);
		reserve_io->have_atomic(true);
		//Set XC id
		reserve_oi->xc_id(d->xc_id() + 1);
		//post an event to allow action to reach Up state
		reserve_io->fsm_post(fsm::TNRC::virtFsm::TNRC_AtomicActionOk,
				     reserve_io);
	}
	//delete Action object representing an Action in Down state
	thread_add_event (TNRC_MASTER.getMaster(), delete_action, a, detach);
	//attach Action to map of actions in execution or executed
	TNRC_MASTER.attach_action (reserve_io->ap_cookie(), reserve_io);
	//post last event to allow action to reach Up state
	reserve_io->fsm_post (fsm::TNRC::virtFsm::TNRC_AtomicActionOk,
			      reserve_io);

	return 0;
}

int
timer_expired(struct thread *t)
{
	Action * a;

	//get Action
	a = static_cast<Action*>(THREAD_ARG (t));

	//post an AtomicActionRetryTimer event
	TNRC_DBG ("Retry Timer expired for action %s, post an "
		  "AtomicActionRetryTimer event.",
		  SHOW_ACTION_TYPE(a->atomic()->action_type()));
	a->fsm_post(fsm::TNRC::virtFsm::TNRC_AtomicActionRetryTimer, a);

	return 0;
}

int
start_advance_rsrv(struct thread *t)
{
	ReserveUnreservexc * reserve_act;
	MakeDestroyxc *      make_act;
	Action *             a;
	bool                 executed;

	executed = true;

	//get Action
	reserve_act = static_cast<ReserveUnreservexc*>(THREAD_ARG (t));

	//Create a new MAKE_XC action
	make_act = new MakeDestroyxc(reserve_act->ap_cookie(),
				     reserve_act->resp_ctxt(),
				     0,
				     MAKE_XC,
				     reserve_act->portid_in(),
				     reserve_act->labelid_in(),
				     reserve_act->portid_out(),
				     reserve_act->labelid_out(),
				     reserve_act->direction(),
				     false,
				     reserve_act->advance_rsrv(),
				     reserve_act->start(),
				     reserve_act->end(),
				     reserve_act->plugin());

	//Set reservation XC flag
	make_act->rsrv_xc_flag(NO_RSRV);
	//Check if resources are free
	if (!make_act->check_resources()) {
		//respond negatively to client
		action_response(make_act->ap_cookie(),
				MAKE_XC,
				TNRCSP_RESULT_PARAMERROR,
				executed,
				make_act->resp_ctxt());
		//we can delete actions
		delete reserve_act;
		delete make_act;
		//check if there are any operation
		//requests queued
		if (TNRC_MASTER.api_queue_size() != 0) {
			//there are other requests in
			//the api queue
			//schedule a new event thread
			thread_add_event(TNRC_MASTER.getMaster(),
					 process_api_queue,
					 NULL, 0);
		}
		return 0;
	}
	//Set XC id
	make_act->xc_id(reserve_act->xc_id());
	// Set correctly action pointer
	a = make_act;
	// Push first atomic action
	make_act->push_todo(a);
	//Set reservation cookie
	make_act->rsrv_cookie(0);
	// Check if MAKE_XC is bidirectional
	if (make_act->direction() == XCDIRECTION_BIDIR) {
		// Add another atomic action
		MakeDestroyxc   * at_act;
		at_act = new MakeDestroyxc(reserve_act->ap_cookie(),
					   reserve_act->resp_ctxt(),
					   0,
					   MAKE_XC,
					   reserve_act->portid_out(),
					   reserve_act->labelid_out(),
					   reserve_act->portid_in(),
					   reserve_act->labelid_in(),
					   reserve_act->direction(),
					   false,
					   reserve_act->advance_rsrv(),
					   reserve_act->start(),
					   reserve_act->end(),
					   reserve_act->plugin());
		a = at_act;
		//Set reservation XC flag
		at_act->rsrv_xc_flag(NO_RSRV);
		//Set reservation cookie
		at_act->rsrv_cookie(0);
		//Set XC id
		at_act->xc_id(reserve_act->xc_id() + 1);
		// Push atmoic action
		make_act->push_todo(a);
		// Set correctly action pointer
		a = make_act;
		TNRC_DBG ("Requesting MAKE XC bidirectional: "
			  "there will be two atomic actions");
	}
	//Detach reserve action
	TNRC_MASTER.detach_action (reserve_act->ap_cookie());
	//Delete reserve action
	delete reserve_act;
	//Put the action object created in the map of actions
	//in execution
	TNRC_MASTER.attach_action (a->ap_cookie(), a);
	//post an ActionCreate event for MAKE_XC action
	TNRC_DBG ("Post an AcionCreate event for the "
		  "ADVANCE RESERVATION action");
	make_act->fsm_start();
	make_act->fsm_post(fsm::TNRC::virtFsm::TNRC_ActionCreate, make_act);

	return 0;
}

int
stop_advance_rsrv(struct thread *t)
{
	MakeDestroyxc *      make_act;
	Action *             a;

	//get Action
	make_act = static_cast<MakeDestroyxc*>(THREAD_ARG (t));

	make_act->rsrv_xc_flag(NO_RSRV);
	//update parameters for atomic actions
	if (make_act->have_atomic()) {
		Action::iterator_atomic_done iter;
		MakeDestroyxc *              atomic;
		for(iter  = make_act->begin_atomic_done();
		    iter != make_act->end_atomic_done();
		    iter++) {
			atomic = dynamic_cast<MakeDestroyxc *>(*iter);
			atomic->rsrv_xc_flag(NO_RSRV);
		}
	}
	//Cancel advance reservation in data model
	make_act->cancel_advance_rsrv();
	//post an ActionDestroy event for the relative FSM
	TNRC_DBG("Post an ActionDestroy event for the "
		 "ADVANCE RESERVATION action");
	make_act->fsm_post(fsm::TNRC::virtFsm::TNRC_ActionDestroy, make_act);

	return 0;
}
