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



#include <assert.h>

#include "tnrc_action.h"
#include "tnrcd.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_g2rsvpte_api.h"

//
// This file MUST BE generated automatically only once, ADD your code here !
//

#define DUMP_STATE(STATE,EVENT)						   \
	std::cout << "STATE: '" << STATE << "' -> " << EVENT << std::endl;

#define DUMP_STATE2(STATE,EVENT,FROM_STATE)				   \
	std::cout << "* STATE: '" << STATE << "' -> after " << EVENT       \
                  << " -> from " << FROM_STATE << std::endl;

/*****************************************************************************
 *                    Class Closing_i
 *****************************************************************************/

fsm::base_TNRC::nextEvFor_AtomicActionOk_t
Closing_i::AtomicActionOk(void* context)
{
	Action             * ctxt;
	Action             * at_ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE(name(), "AtomicActionOk");

	//Get context
	ctxt = static_cast<Action*> (context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	//Pop first (already executed) atomic action destroyed
	ctxt->pop_done();

	//Last atomic action destroying succeeds, control if there any other to destroy
	if (ctxt->have_atomic_todestroy()) {
		TNRC_DBG("Atomic action %s destroyed for action %s",
			SHOW_ACTION_TYPE(act_type),
			SHOW_ACTION_TYPE(ctxt->action_type()));
		return fsm::base_TNRC::TNRC_from_AtomicActionOk_to_evAtomicActionNext;
	}
	else {
		TNRC_DBG("All atomic actions destroyed for action %s",
			SHOW_ACTION_TYPE(ctxt->action_type()));
		return fsm::base_TNRC::TNRC_from_AtomicActionOk_to_evActionEndDown;
	}

};

fsm::base_TNRC::nextEvFor_AtomicActionDownTimeout_t
Closing_i::AtomicActionDownTimeout(void* context)
{
	DUMP_STATE(name(), "AtomicActionDownTimeout");

	return fsm::base_TNRC::TNRC_from_AtomicActionDownTimeout_to_evAtomicActionDownTimeout;
};

void
Closing_i::after_evAtomicActionDownTimeout_from_virt_Closing(void * context)
{
	DUMP_STATE2(name(), "evAtomicActionDownTimeout", "virt_Closing");
};

fsm::base_TNRC::nextEvFor_AtomicActionKo_t
Closing_i::AtomicActionKo(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE(name(), "AtomicActionKo");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get atomic action type
	act_type = ctxt->atomic()->action_type();

	//Check number of retrys for this atomic action
	if (ctxt->atomic()->n_retry() < MAX_AT_ACT_RETRY) {
		TNRC_DBG("Retry to destroy atomic action %s for action %s",
			SHOW_ACTION_TYPE(act_type),
			SHOW_ACTION_TYPE(ctxt->action_type()));
		return fsm::base_TNRC::TNRC_from_AtomicActionKo_to_evAtomicActionRetry;
	}
	else {
		return fsm::base_TNRC::TNRC_from_AtomicActionKo_to_evAtomicActionAbort;
	}

};

void
Closing_i::after_evAtomicActionRetry_from_virt_Closing(void * context)
{
	DUMP_STATE2(name(), "evAtomicActionRetry", "virt_Closing");
};

fsm::base_TNRC::nextEvFor_AtomicActionRetryTimer_t
Closing_i::AtomicActionRetryTimer(void* context)
{
	DUMP_STATE(name(), "AtomicActionRetryTimer");

	return fsm::base_TNRC::TNRC_from_AtomicActionRetryTimer_to_evAtomicActionRetryTimer;
};

void
Closing_i::after_evAtomicActionRetryTimer_from_virt_Closing(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE2(name(), "evAtomicActionRetryTimer", "virt_Closing");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	//Check if preliminary check for destroy action request failed
	if (ctxt->atomic()->prel_check() != TNRCSP_RESULT_NOERROR) {
		TNRC_WRN ("Preliminary check for action %s failed",
			  SHOW_ACTION_TYPE(act_type));
		TNRC_DBG ("Retry the same operation");

		ctxt->atomic()->wait_answer(false);
		//Set atomic action equipment response to preliminary check
		ctxt->atomic()->eqpt_resp(ctxt->atomic()->prel_check());
		//post an AtomicActionKo event
		ctxt->fsm_post(fsm::TNRC::virtFsm::TNRC_AtomicActionKo, ctxt, true);

		return;
	}

	TNRC_DBG ("Preliminary check for atomic action %s OK",
		  SHOW_ACTION_TYPE(act_type));

};

void
Closing_i::after_evAtomicActionNext_from_virt_Closing(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE2(name(), "evAtomicActionNext", "virt_Closing");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	//Check if preliminary check for action requested failed
	if (ctxt->atomic()->prel_check() != TNRCSP_RESULT_NOERROR) {
		TNRC_WRN ("Preliminary check for action %s failed",
			  SHOW_ACTION_TYPE(act_type));
		TNRC_DBG ("Retry the same operation");

		ctxt->atomic()->wait_answer(false);
		//Set atomic action equipment response to preliminary check
		ctxt->atomic()->eqpt_resp(ctxt->atomic()->prel_check());
		//post an AtomicActionKo event
		ctxt->fsm_post(fsm::TNRC::virtFsm::TNRC_AtomicActionKo, ctxt, true);

		return;
	}
	TNRC_DBG ("Preliminary check for atomic action %s OK",
		  SHOW_ACTION_TYPE(act_type));

};

void
Closing_i::after_evActionRollback_from_virt_Incomplete(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE2(name(), "evActionRollback", "virt_Incomplete");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	//Check if preliminary check for destroying action request failed
	if (ctxt->atomic()->prel_check() != TNRCSP_RESULT_NOERROR) {
		TNRC_WRN ("Preliminary check for action %s failed",
			  SHOW_ACTION_TYPE(act_type));
		TNRC_DBG ("Retry the same operation", act_type);

		ctxt->atomic()->wait_answer(false);
		//Set atomic action equipment response to preliminary check
		ctxt->atomic()->eqpt_resp(ctxt->atomic()->prel_check());
		//post an AtomicActionKo event
		ctxt->fsm_post(fsm::TNRC::virtFsm::TNRC_AtomicActionKo, ctxt, true);

		return;
	}
	TNRC_DBG ("Preliminary check for destroy action %s OK",
		  SHOW_ACTION_TYPE(act_type));

};

void
Closing_i::after_evActionDestroy_from_virt_Up(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE2(name(), "evActionDestroy", "virt_Up");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	//Check if preliminary check for destroying action request failed
	if (ctxt->atomic()->prel_check() != TNRCSP_RESULT_NOERROR) {
		TNRC_WRN ("Preliminary check for action %s failed",
			  SHOW_ACTION_TYPE(act_type));
		TNRC_DBG ("Retry the same operation");

		ctxt->atomic()->wait_answer(false);
		//Set atomic action equipment response to preliminary check
		ctxt->atomic()->eqpt_resp(ctxt->atomic()->prel_check());
		//post an AtomicActionKo event
		ctxt->fsm_post(fsm::TNRC::virtFsm::TNRC_AtomicActionKo,
			       ctxt,
			       true);

		return;
	}
	TNRC_DBG ("Preliminary check for destroy action %s OK",
		  SHOW_ACTION_TYPE(act_type));

};


/*****************************************************************************
 *                    Class Incomplete_i
 *****************************************************************************/

fsm::base_TNRC::nextEvFor_ActionRollback_t
Incomplete_i::ActionRollback(void* context)
{
	DUMP_STATE(name(), "ActionRollback");
	return fsm::base_TNRC::TNRC_from_ActionRollback_to_evActionRollback;
};

fsm::base_TNRC::nextEvFor_EqptDown_t
Incomplete_i::EqptDown(void* context)
{
	DUMP_STATE(name(), "EqptDown");
	return fsm::base_TNRC::TNRC_from_EqptDown_to_evEqptDown;
};

void
Incomplete_i::after_evAtomicActionIncomplete_from_virt_Creating(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE2(name(), "evAtomicActionIncomplete", "virt_Creating");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	//check the eqpt response and verify if Eqpt link is down
	if (ctxt->atomic()->eqpt_resp() == TNRCSP_RESULT_EQPTLINKDOWN) {
		//post an EqptDown event
		ctxt->fsm_post(fsm::TNRC::virtFsm::TNRC_EqptDown,
			       ctxt,
			       true);
	}
	else {
		TNRC_DBG("Post an ActionRollback event to destroy action %s",
			SHOW_ACTION_TYPE(act_type));
		//Swap correctly all atomic actions type, to perform correct
		//destructions
		ctxt->swap_action_type();
		//post an ActionRollback event
		ctxt->fsm_post(fsm::TNRC::virtFsm::TNRC_ActionRollback,
			       ctxt,
			       true);
	}

};

void
Incomplete_i::after_evAtomicActionOk_from_virt_Dismissed(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE2(name(), "evAtomicActionOk", "virt_Dismissed");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->action_type();

	TNRC_DBG("Post an ActionRollback event to destroy action %s",
		SHOW_ACTION_TYPE(act_type));

	//Swap correctly all atomic actions type,to perform correct destructions
	ctxt->swap_action_type();

	//post an ActionRollback event
	ctxt->fsm_post(fsm::TNRC::virtFsm::TNRC_ActionRollback,
		       ctxt,
		       true);

};

void
Incomplete_i::after_evAtomicActionIncomplete_from_virt_Dismissed(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE2(name(), "evAtomicActionIncomplete", "virt_Dismissed");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	//check the eqpt response and verify if Eqpt link is down
	if (ctxt->atomic()->eqpt_resp() == TNRCSP_RESULT_EQPTLINKDOWN) {
		//post an EqptDown event
		ctxt->fsm_post(fsm::TNRC::virtFsm::TNRC_EqptDown,
			       ctxt,
			       true);
	}
	else {
		//Swap correctly all atomic actions type,to perform correct destructions
		ctxt->swap_action_type();
		TNRC_DBG("Post an ActionRollback event to destroy action %s",
			SHOW_ACTION_TYPE(act_type));
		//post an ActionRollback event
		ctxt->fsm_post(fsm::TNRC::virtFsm::TNRC_ActionRollback,
			       ctxt,
			       true);
	}

};


/*****************************************************************************
 *                    Class Creating_i
 *****************************************************************************/

fsm::base_TNRC::nextEvFor_AtomicActionKo_t
Creating_i::AtomicActionKo(void* context)
{
	Action		   * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE(name(), "AtomicActionKo");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();
	//Set waiting resp to false
	ctxt->atomic()->wait_answer(false);

	//check if the action has atomic actions already executed
	if (ctxt->have_atomic_todestroy()) {
		return fsm::base_TNRC::TNRC_from_AtomicActionKo_to_evAtomicActionIncomplete;
	}
	else {
		return fsm::base_TNRC::TNRC_from_AtomicActionKo_to_evAtomicActionKo;
	}

};

fsm::base_TNRC::nextEvFor_ActionDestroy_t
Creating_i::ActionDestroy(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE(name(), "ActionDestroy");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get atomic action type
	act_type = ctxt->atomic()->action_type();
	//check if the current atomic action is waiting for a response
	if (!ctxt->atomic()->wait_answer()  &&
	    !ctxt->have_atomic_todestroy()) {
		TNRC_DBG("Action destroy received for action %s",
			 SHOW_ACTION_TYPE(act_type));
		return fsm::base_TNRC::TNRC_from_ActionDestroy_to_evActionDestroy;
	}
	else {
		TNRC_DBG("Action destroy received for action %s "
			 "while waiting response for an atomic action",
			 SHOW_ACTION_TYPE(act_type));
		return fsm::base_TNRC::TNRC_from_ActionDestroy_to_evActionPending;
	}

};

fsm::base_TNRC::nextEvFor_EqptDown_t
Creating_i::EqptDown(void* context)
{
	DUMP_STATE(name(), "EqptDown");

	return fsm::base_TNRC::TNRC_from_EqptDown_to_evEqptDown;

};

fsm::base_TNRC::nextEvFor_AtomicActionOk_t
Creating_i::AtomicActionOk(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE(name(), "AtomicActionOk");

	//Get context
	ctxt = static_cast<Action*> (context);

	//Get action type
	act_type = ctxt->atomic()->action_type();
	//Set waiting resp to false
	ctxt->atomic()->wait_answer(false);

	//push atomic action executed in done queue
	ctxt->push_done(ctxt->atomic());

	if (!ctxt->have_atomic()) {
		TNRC_DBG ("Action %s executed correctly",
			  SHOW_ACTION_TYPE(act_type));
		return fsm::base_TNRC::TNRC_from_AtomicActionOk_to_evActionEndUp;
	}
	else {
		//Pop first (already executed) atomic action from todo queue
		ctxt->pop_todo();

		//Last atomic action succeeds, control if there any other atomic actions
		if (!ctxt->have_atomic_todo()) {
			TNRC_DBG ("All atomic actions executed for action %s",
				  SHOW_ACTION_TYPE(ctxt->action_type()));
			return fsm::base_TNRC::TNRC_from_AtomicActionOk_to_evActionEndUp;
		}
		else {
			TNRC_DBG ("Atomic action %s executed for action %s",
				  SHOW_ACTION_TYPE(act_type),
				  SHOW_ACTION_TYPE(ctxt->action_type()));
			return fsm::base_TNRC::TNRC_from_AtomicActionOk_to_evAtomicActionNext;
		}
	}

};

void
Creating_i::after_evAtomicActionNext_from_virt_Creating(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE2(name(), "evAtomicActionNext", "virt_Creating");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	//Check if preliminary check for action requested failed
	if (ctxt->atomic()->prel_check() != TNRCSP_RESULT_NOERROR) {
		TNRC_WRN ("Preliminary check for action %s failed",
			  SHOW_ACTION_TYPE(act_type));
		//Set waiting resp to false
		ctxt->atomic()->wait_answer(false);
		//Set atomic action equipment response to preliminary check
		ctxt->atomic()->eqpt_resp(ctxt->atomic()->prel_check());
		switch (ctxt->atomic()->prel_check()) {
			case TNRCSP_RESULT_EQPTLINKDOWN:
				//Post an EqptDown event
				ctxt->fsm_post(fsm::TNRC::virtFsm::
					       TNRC_EqptDown,
					       ctxt,
					       true);
				break;
			default:
				//Post an AtomicActionKo event
				ctxt->fsm_post(fsm::TNRC::virtFsm::
					       TNRC_AtomicActionKo,
					       ctxt,
					       true);
				break;
		}
		return;
	}
	TNRC_DBG ("Preliminary check for atomic action %s OK",
		  SHOW_ACTION_TYPE(act_type));

};

void
Creating_i::after_evActionCreate_from_virt_Down(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE2(name(), "evActionCreate", "virt_Down");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	//Check if preliminary check for action requested failed
	if (ctxt->atomic()->prel_check() != TNRCSP_RESULT_NOERROR) {
		TNRC_WRN ("Preliminary check for action %s failed",
			  SHOW_ACTION_TYPE(act_type));
		//Set waiting resp to false
		ctxt->atomic()->wait_answer(false);
		//Set atomic action equipment response to preliminary check
		ctxt->atomic()->eqpt_resp(ctxt->atomic()->prel_check());
		switch (ctxt->atomic()->prel_check()) {
			case TNRCSP_RESULT_EQPTLINKDOWN:
				//Post an EqptDown event
				ctxt->fsm_post(fsm::TNRC::virtFsm::
					       TNRC_EqptDown,
					       ctxt,
					       true);
				break;
			default:
				//Post an AtomicActionKo event
				ctxt->fsm_post(fsm::TNRC::virtFsm::
					       TNRC_AtomicActionKo,
					       ctxt,
					       true);
				break;
		}
		return;
	}
	TNRC_DBG ("Preliminary check for action %s OK",
		  SHOW_ACTION_TYPE(act_type));

};


/*****************************************************************************
 *                    Class Up_i
 *****************************************************************************/

void
Up_i::after_evActionEndUp_from_virt_Creating(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcap_result_t      result;
	bool                 process_queue;
	bool                 executed;

	process_queue = false;
	executed      = true;

	DUMP_STATE2(name(), "evActionEndUp", "virt_Creating");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->action_type();

	result = ctxt->eqpt_resp();

	//Positive response to client
	if (act_type == RESERVE_XC) {
		ReserveUnreservexc * rsrv;
		rsrv = dynamic_cast<ReserveUnreservexc*>(ctxt);
		if (!rsrv->deactivation()) {
			action_response(ctxt->ap_cookie(),
					act_type,
					result,
					executed,
					ctxt->resp_ctxt());
			process_queue = true;
		}
	}
	else {
		action_response(ctxt->ap_cookie(),
				act_type,
				result,
				executed,
				ctxt->resp_ctxt());
		process_queue = true;
	}
	//Schedule a Destroy action in case of ADVANCE RESERVATION
	if (act_type == MAKE_XC) {
		MakeDestroyxc * make;
		make = dynamic_cast<MakeDestroyxc *>(ctxt);
		if (make->advance_rsrv()) {
			struct timeval  t;
			long            timer;
			long            end_time;
			tm *            curTime;
			const char *    timeStringFormat = "%a, %d %b %Y %H:%M:%S %zGMT";
			const int       timeStringLength = 40;
			char            timeString[timeStringLength];
			struct thread * thr;
			end_time = make->end();
			//Get current time
			gettimeofday(&t, NULL);
			//Get timer
			timer = end_time - t.tv_sec;
			curTime = localtime((time_t *) &end_time);
			strftime(timeString, timeStringLength, timeStringFormat, curTime);
			if (timer <= 0) {
				TNRC_DBG ("An ADVANCE RESERVATION "
					  "had to end %s",
					  timeString);
				TNRC_DBG ("Schedule an event to get it end");
				thr = thread_add_event(TNRC_MASTER.getMaster(),
						       stop_advance_rsrv,
						       make,
						       0);
			} else {
				TNRC_DBG ("Schedule ActionDestroy event for "
					  "ADVANCE RESERVATION, ending %s",
					  timeString);
				thr = thread_add_timer(TNRC_MASTER.getMaster(),
						       stop_advance_rsrv,
						       make,
						       timer);
			}
			//Set the thread
			make->thread_end(thr);
		}
	}
	//check if there are any operation requests queued
	if ((TNRC_MASTER.api_queue_size() != 0) && process_queue) {
		//there are other requests in the api queue
		//schedule a new event thread
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

};

fsm::base_TNRC::nextEvFor_ActionNotification_t
Up_i::ActionNotification(void* context)
{
	DUMP_STATE(name(), "ActionNotification");

	return fsm::base_TNRC::TNRC_from_ActionNotification_to_evActionNotification;

};

void
Up_i::after_evActionNotification_from_virt_Up(void * context)
{
	DUMP_STATE2(name(), "evActionNotification", "virt_Up");
};

fsm::base_TNRC::nextEvFor_ActionDestroy_t
Up_i::ActionDestroy(void* context)
{
	DUMP_STATE(name(), "ActionDestroy");

	return fsm::base_TNRC::TNRC_from_ActionDestroy_to_evActionDestroy;

};


/*****************************************************************************
 *                    Class Down_i
 *****************************************************************************/

void
Down_i::after_evActionEndDown_from_virt_Closing(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcap_result_t      result;
	bool                 detach;
	bool                 executed;

	detach   = true;
	executed = true;

	DUMP_STATE2(name(), "evActionEndDown", "virt_Closing");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->action_type();

	result = ctxt->eqpt_resp();

	//Positive response to client for destroying action
	action_response(ctxt->ap_cookie(),
			act_type,
			result,
			executed,
			ctxt->resp_ctxt());
	//schedule an event to delete action object
	if (ctxt->action_type() == DESTROY_XC) {
		MakeDestroyxc * dest_ctxt;
		dest_ctxt = dynamic_cast<MakeDestroyxc*>(ctxt);
		if (dest_ctxt->rsrv_cookie() == 0) {
			if (dest_ctxt->rsrv_xc_flag() == DEACTIVATE) {
				thread_add_event (TNRC_MASTER.getMaster(),
						  deactivate_action, ctxt, 0);
			}
			else {
				thread_add_event (TNRC_MASTER.getMaster(),
						  delete_action, ctxt, detach);
			}
		}
		else {
			if (dest_ctxt->rsrv_xc_flag() == NO_RSRV) {
				Action			*rsrv;
				rsrv = TNRC_MASTER.getAction (dest_ctxt->
							      rsrv_cookie());
				if (rsrv != NULL) {
					thread_add_event (TNRC_MASTER.
							  getMaster(),
							  delete_action,
							  rsrv, detach);
				}
			}
			thread_add_event (TNRC_MASTER.getMaster(),
					  delete_action, ctxt, detach);
		}
	}
	else {
		thread_add_event (TNRC_MASTER.getMaster(),
				  delete_action, ctxt, detach);
	}
	//check if there are any operation requests queued
	if (TNRC_MASTER.api_queue_size() != 0) {
		//there are other requests in the api queue
		//schedule a new even thread
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

};

void
Down_i::after_evAtomicActionAbort_from_virt_Closing(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcap_result_t      result;
	bool                 detach;
	bool                 executed;

	detach   = true;
	executed = true;

	DUMP_STATE2(name(), "evAtomicActionAbort", "virt_Closing");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->action_type();

	result = ctxt->eqpt_resp();

	//Negative response to client for destroying action
	action_response(ctxt->ap_cookie(),
			act_type,
			result,
			executed,
			ctxt->resp_ctxt());
	//schedule an event to delete action object
	thread_add_event (TNRC_MASTER.getMaster(), delete_action, ctxt, detach);

	//check if there are any operation requests queued
	if (TNRC_MASTER.api_queue_size() != 0) {
		//there are other requests in the api queue
		//schedule a new even thread
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

};

void
Down_i::after_evEqptDown_from_virt_Incomplete(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcap_result_t      result;
	bool                 detach;
	bool                 executed;

	detach   = true;
	executed = true;

	DUMP_STATE2(name(), "evEqptDown", "virt_Incomplete");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->action_type();

	result = ctxt->eqpt_resp();

	//Negative response to client for creating action
	action_response(ctxt->ap_cookie(),
			act_type,
			result,
			executed,
			ctxt->resp_ctxt());
	//schedule an event to delete action object
	thread_add_event (TNRC_MASTER.getMaster(), delete_action, ctxt, detach);

	//check if there are any operation requests queued
	if (TNRC_MASTER.api_queue_size() != 0) {
		//there are other requests in the api queue
		//schedule a new even thread
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

};

void
Down_i::after_evAtomicActionKo_from_virt_Creating(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcap_result_t      result;
	bool                 detach;
	bool                 executed;

	detach   = true;
	executed = true;

	DUMP_STATE2(name(), "evAtomicActionKo", "virt_Creating");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->action_type();

	result = ctxt->eqpt_resp();

	//Negative response to client for creating action
	action_response(ctxt->ap_cookie(),
			act_type,
			result,
			executed,
			ctxt->resp_ctxt());
	//schedule an event to delete action object
	thread_add_event (TNRC_MASTER.getMaster(), delete_action, ctxt, detach);

	//check if there are any operation requests queued
	if (TNRC_MASTER.api_queue_size() != 0) {
		//there are other requests in the api queue
		//schedule a new even thread
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

};

void
Down_i::after_evEqptDown_from_virt_Creating(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcap_result_t      result;
	bool                 detach;
	bool                 executed;

	detach   = true;
	executed = true;

	DUMP_STATE2(name(), "evEqptDown", "virt_Creating");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->action_type();

	result = ctxt->eqpt_resp();

	//Negative response to client for creating action
	action_response(ctxt->ap_cookie(),
			act_type,
			result,
			executed,
			ctxt->resp_ctxt());
	//schedule an event to delete action object
	thread_add_event (TNRC_MASTER.getMaster(), delete_action, ctxt, detach);

	//check if there are any operation requests queued
	if (TNRC_MASTER.api_queue_size() != 0) {
		//there are other requests in the api queue
		//schedule a new even thread
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

};

void
Down_i::after_evActionDestroy_from_virt_Creating(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcap_result_t      result;
	bool                 detach;
	bool                 executed;

	detach   = true;
	executed = true;

	DUMP_STATE2(name(), "evActionDestroy", "virt_Creating");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->action_type();

	result = TNRCSP_RESULT_NOERROR;

	//Positive response to client for destroying action
	action_response(ctxt->ap_cookie(),
			act_type,
			result,
			executed,
			ctxt->resp_ctxt());
	//schedule an event to delete action object
	thread_add_event (TNRC_MASTER.getMaster(), delete_action, ctxt, detach);

	//check if there are any operation requests queued
	if (TNRC_MASTER.api_queue_size() != 0) {
		//there are other requests in the api queue
		//schedule a new even thread
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

};

fsm::base_TNRC::nextEvFor_ActionCreate_t
Down_i::ActionCreate(void* context)
{

	DUMP_STATE(name(), "ActionCreate");

	return fsm::base_TNRC::TNRC_from_ActionCreate_to_evActionCreate;

};

void
Down_i::after_evAtomicActionTimeout_from_virt_Dismissed(void * context)
{
	DUMP_STATE2(name(), "evAtomicActionTimeout", "virt_Dismissed");
};

void
Down_i::after_evAtomicActionKo_from_virt_Dismissed(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcap_result_t      result;
	bool                 detach;
	bool                 executed;

	detach   = true;
	executed = true;

	DUMP_STATE2(name(), "evAtomicActionKo", "virt_Dismissed");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->action_type();

	result = ctxt->eqpt_resp();

	//Negative response to client for creating action
	action_response(ctxt->ap_cookie(),
			act_type,
			result,
			executed,
			ctxt->resp_ctxt());
	//schedule an event to delete action object
	thread_add_event (TNRC_MASTER.getMaster(), delete_action, ctxt, detach);

	//check if there are any operation requests queued
	if (TNRC_MASTER.api_queue_size() != 0) {
		//there are other requests in the api queue
		//schedule a new even thread
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

};

void
Down_i::after_evEqptDown_from_virt_Dismissed(void * context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcap_result_t      result;
	bool                 detach;
	bool                 executed;

	detach   = true;
	executed = true;

	DUMP_STATE2(name(), "evEqptDown", "virt_Dismissed");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->action_type();

	result = ctxt->eqpt_resp();

	//Negative response to client for creating action
	action_response(ctxt->ap_cookie(),
			act_type,
			result,
			executed,
			ctxt->resp_ctxt());
	//schedule an event to delete action object
	thread_add_event (TNRC_MASTER.getMaster(), delete_action, ctxt, detach);

	//check if there are any operation requests queued
	if (TNRC_MASTER.api_queue_size() != 0) {
		//there are other requests in the api queue
		//schedule a new even thread
		thread_add_event (TNRC_MASTER.getMaster(),
				  process_api_queue, NULL, 0);
	}

};


/*****************************************************************************
 *                    Class Dismissed_i
 *****************************************************************************/

void
Dismissed_i::after_evActionPending_from_virt_Creating(void * context)
{
	DUMP_STATE2(name(), "evActionPending", "virt_Creating");
};

fsm::base_TNRC::nextEvFor_AtomicActionTimeout_t
Dismissed_i::AtomicActionTimeout(void* context)
{
	DUMP_STATE(name(), "AtomicActionTimeout");

	return fsm::base_TNRC::TNRC_from_AtomicActionTimeout_to_evAtomicActionTimeout;
};

fsm::base_TNRC::nextEvFor_AtomicActionKo_t
Dismissed_i::AtomicActionKo(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE(name(), "AtomicActionKo");

	//Get context
	ctxt = static_cast<Action*>(context);
	//Set waitintg resp to false
	ctxt->atomic()->wait_answer(false);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	//check if the atomic action that is waiting for a response is the first one
	if (ctxt->done_size() == 0) {
		TNRC_DBG("Received a negative response for pending first "
			 "atomic action  of action %s",
			 SHOW_ACTION_TYPE(ctxt->action_type()));
		return fsm::base_TNRC::TNRC_from_AtomicActionKo_to_evAtomicActionKo;
	}
	else {
		TNRC_DBG("Received a negative response for pending atomic "
			 "action %s of action %s", SHOW_ACTION_TYPE(act_type),
			 SHOW_ACTION_TYPE(ctxt->action_type()));
		return fsm::base_TNRC::TNRC_from_AtomicActionKo_to_evAtomicActionIncomplete;
	}

};

fsm::base_TNRC::nextEvFor_AtomicActionOk_t
Dismissed_i::AtomicActionOk(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE(name(), "AtomicActionOk");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();
	//Set waiting resp to false
	ctxt->atomic()->wait_answer(false);

	TNRC_DBG("Received a positive response for pending atomic action "
		 "of action %s", SHOW_ACTION_TYPE(ctxt->action_type()));

	return fsm::base_TNRC::TNRC_from_AtomicActionOk_to_evAtomicActionOk;

};

fsm::base_TNRC::nextEvFor_EqptDown_t
Dismissed_i::EqptDown(void* context)
{
	DUMP_STATE(name(), "EqptDown");

	return fsm::base_TNRC::TNRC_from_EqptDown_to_evEqptDown;

};


/*****************************************************************************
 *                    Class virt_Closing_i
 *****************************************************************************/

void
virt_Closing_i::after_AtomicActionOk_from_Closing(void * context)
{
	Action * ctxt;

	DUMP_STATE2(name(), "AtomicActionOk", "Closing");

	//Get context
	ctxt = static_cast<Action*>(context);
	//atomic action destruction succeeds, so we have to update data model
	TNRC_OPSPEC_API::update_dm_after_action (ctxt);

};

bool
virt_Closing_i::evActionEndDown(void* context)
{
	Action * ctxt;

	DUMP_STATE(name(), "evActionEndDown");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Set the final equipment response to positive: all atomic actions
	//have been destroyed
	ctxt->eqpt_resp(TNRCSP_RESULT_NOERROR);

	return true;
};

void
virt_Closing_i::after_AtomicActionDownTimeout_from_Closing(void * context)
{
	DUMP_STATE2(name(), "AtomicActionDownTimeout", "Closing");
};

bool
virt_Closing_i::evAtomicActionDownTimeout(void* context)
{
	DUMP_STATE(name(), "evAtomicActionDownTimeout");

	return true;
};

void
virt_Closing_i::after_AtomicActionKo_from_Closing(void * context)
{
	DUMP_STATE2(name(), "AtomicActionKo", "Closing");
};

bool
virt_Closing_i::evAtomicActionRetry(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE(name(), "evAtomicActionRetry");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	ctxt->atomic()->n_retry_inc();

	thread_add_timer (TNRC_MASTER.getMaster(), timer_expired,
			  context, RETRY_TIMER);

	return true;
};

bool
virt_Closing_i::evAtomicActionAbort(void* context)
{
	Action          * ctxt;
	tnrcap_result_t   res;

	DUMP_STATE(name(), "evAtomicActionAbort");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get last atomic action equipment response
	res = ctxt->atomic()->eqpt_resp();

	//Set the final equipment response to negative
	ctxt->eqpt_resp(res);

	return true;
};

void
virt_Closing_i::after_AtomicActionRetryTimer_from_Closing(void * context)
{
	DUMP_STATE2(name(), "AtomicActionRetryTimer", "Closing");
};

bool
virt_Closing_i::evAtomicActionRetryTimer(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcsp_handle_t      handlep;
	tnrcsp_result_t      res;
	xcdirection_t        dir;
	tnrc_boolean_t       isvirtual;

	DUMP_STATE(name(), "evAtomicActionRetryTimer");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	TNRC_DBG("Retrying destroy atomic action %s for action %s",
		 SHOW_ACTION_TYPE(act_type),
		 SHOW_ACTION_TYPE(ctxt->action_type()));

	switch (act_type) {
		case DESTROY_XC:
			//Get action context
			MakeDestroyxc  * dest_ctxt;
			tnrc_boolean_t   deactivate;

			dest_ctxt = dynamic_cast<MakeDestroyxc*>
				(ctxt->atomic());
			isvirtual  = 0;
			deactivate = (dest_ctxt->rsrv_xc_flag() == DEACTIVATE) ? 1 : 0;

			dir = dest_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    dest_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			res = dest_ctxt->plugin()->
				tnrcsp_destroy_xc(&handlep,
						  dest_ctxt->portid_in(),
						  dest_ctxt->labelid_in(),
						  dest_ctxt->portid_out(),
						  dest_ctxt->labelid_out(),
						  dir,
						  isvirtual,
						  deactivate,
						  TNRC_OPSPEC_API::
						  destroy_xc_resp_cb,
						  ctxt);
			break;
		case UNRESERVE_XC:
			//Get action context
			ReserveUnreservexc * unreserve_ctxt;

			isvirtual  = 0;
			unreserve_ctxt = dynamic_cast<ReserveUnreservexc*>
				(ctxt->atomic());
			dir = unreserve_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    unreserve_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			res = unreserve_ctxt->plugin()->
				tnrcsp_unreserve_xc(&handlep,
						    unreserve_ctxt->
						    portid_in(),
						    unreserve_ctxt->
						    labelid_in(),
						    unreserve_ctxt->
						    portid_out(),
						    unreserve_ctxt->
						    labelid_out(),
						    dir,
						    isvirtual,
						    TNRC_OPSPEC_API::
						    unreserve_xc_resp_cb,
						    ctxt);
			break;
	}
	ctxt->atomic()->prel_check(res);
	ctxt->atomic()->sp_handle (handlep);
	ctxt->atomic()->wait_answer (true);

	return true;
};

bool
virt_Closing_i::evAtomicActionNext(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcsp_handle_t      handlep;
	tnrcsp_result_t      res;
	xcdirection_t        dir;
	tnrc_boolean_t       isvirtual;

	DUMP_STATE(name(), "evAtomicActionNext");

	//Get context
	ctxt = static_cast<Action*>(context);
	//Get next atomic action to destroy
	ctxt->atomic(ctxt->front_done());

	//Get action type
	act_type = ctxt->atomic()->action_type();

	TNRC_DBG ("Destroying next atomic action for action %s",
		 SHOW_ACTION_TYPE(ctxt->action_type()));

	switch (act_type) {
		case DESTROY_XC:
			//Get action context
			MakeDestroyxc  * dest_ctxt;
			tnrc_boolean_t   deactivate;

			dest_ctxt = dynamic_cast<MakeDestroyxc*>
				(ctxt->atomic());
			isvirtual  = 0;
			deactivate = (dest_ctxt->rsrv_xc_flag() == DEACTIVATE) ? 1 : 0;

			dir = dest_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    dest_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			res = dest_ctxt->plugin()->
				tnrcsp_destroy_xc (&handlep,
						   dest_ctxt->portid_in(),
						   dest_ctxt->labelid_in(),
						   dest_ctxt->portid_out(),
						   dest_ctxt->labelid_out(),
						   dir,
						   isvirtual,
						   deactivate,
						   TNRC_OPSPEC_API::
						   destroy_xc_resp_cb,
						   ctxt);
			break;
		case UNRESERVE_XC:
			//Get action context
			ReserveUnreservexc * unreserve_ctxt;

			isvirtual  = 0;
			unreserve_ctxt = dynamic_cast<ReserveUnreservexc*>
				(ctxt->atomic());
			dir = unreserve_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    unreserve_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			res = unreserve_ctxt->plugin()->
				tnrcsp_unreserve_xc(&handlep,
						    unreserve_ctxt->
						    portid_in(),
						    unreserve_ctxt->
						    labelid_in(),
						    unreserve_ctxt->
						    portid_out(),
						    unreserve_ctxt->
						    labelid_out(),
						    dir,
						    isvirtual,
						    TNRC_OPSPEC_API::
						    unreserve_xc_resp_cb,
						    ctxt);
			break;
	}
	ctxt->atomic()->prel_check(res);
	ctxt->atomic()->sp_handle (handlep);
	ctxt->atomic()->wait_answer (true);

	return true;
};


/*****************************************************************************
 *                    Class virt_Incomplete_i
 *****************************************************************************/

void
virt_Incomplete_i::after_ActionRollback_from_Incomplete(void * context)
{
	DUMP_STATE2(name(), "ActionRollback", "Incomplete");
};

bool
virt_Incomplete_i::evActionRollback(void* context)
{
	Action             * ctxt;
	Action             * at_ctxt;
	tnrc_action_type_t   act_type;
	tnrcsp_handle_t      handlep;
	tnrcsp_result_t      res;
	xcdirection_t        dir;
	tnrc_boolean_t       isvirtual;

	DUMP_STATE(name(), "evActionRollback");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->action_type();

	TNRC_DBG ("Destroying first atomic action for action %s",
		  SHOW_ACTION_TYPE(act_type));

	//Get first atomic action to destroy
	ctxt->atomic(ctxt->front_done());
	switch (ctxt->atomic()->action_type()) {
		case DESTROY_XC: {
			//Get action context
			MakeDestroyxc  * dest_ctxt;
			tnrc_boolean_t   deactivate;

			dest_ctxt = dynamic_cast<MakeDestroyxc*>
				(ctxt->atomic());
			isvirtual  = 0;
			deactivate = (dest_ctxt->rsrv_xc_flag() == DEACTIVATE) ? 1 : 0;

			dir = dest_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    dest_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			res = dest_ctxt->plugin()->
				tnrcsp_destroy_xc(&handlep,
						  dest_ctxt->portid_in(),
						  dest_ctxt->labelid_in(),
						  dest_ctxt->portid_out(),
						  dest_ctxt->labelid_out(),
						  dir,
						  isvirtual,
						  deactivate,
						  TNRC_OPSPEC_API::
						  destroy_xc_resp_cb,
						  ctxt);
			break;
		}
		case UNRESERVE_XC: {
			//Get action context
			ReserveUnreservexc * unreserve_ctxt;

			isvirtual  = 0;
			unreserve_ctxt = dynamic_cast<ReserveUnreservexc*>
				(ctxt->atomic());
			dir = unreserve_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    unreserve_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			res = unreserve_ctxt->plugin()->
				tnrcsp_unreserve_xc(&handlep,
						    unreserve_ctxt->
						    portid_in(),
						    unreserve_ctxt->
						    labelid_in(),
						    unreserve_ctxt->
						    portid_out(),
						    unreserve_ctxt->
						    labelid_out(),
						    dir,
						    isvirtual,
						    TNRC_OPSPEC_API::
						    unreserve_xc_resp_cb,
						    ctxt);
			break;
		}
	}
	ctxt->atomic()->prel_check(res);
	ctxt->atomic()->sp_handle (handlep);
	ctxt->atomic()->wait_answer (true);

	return true;
};

void
virt_Incomplete_i::after_EqptDown_from_Incomplete(void * context)
{
	DUMP_STATE2(name(), "EqptDown", "Incomplete");
};

bool
virt_Incomplete_i::evEqptDown(void* context)
{
	Action          * ctxt;
	tnrcap_result_t   res;

	DUMP_STATE(name(), "evEqptDown");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get last atomic action equipment response
	res = ctxt->atomic()->eqpt_resp();

	//Set the final equipment response to negative
	ctxt->eqpt_resp(res);

	return true;
};


/*****************************************************************************
 *                    Class virt_Creating_i
 *****************************************************************************/

void
virt_Creating_i::after_AtomicActionKo_from_Creating(void * context)
{
	DUMP_STATE2(name(), "AtomicActionKo", "Creating");
};

bool
virt_Creating_i::evAtomicActionKo(void* context)
{
	Action          * ctxt;
	tnrcap_result_t   res;

	DUMP_STATE(name(), "evAtomicActionKo");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get last atomic action equipment response
	res = ctxt->atomic()->eqpt_resp();

	//Set the final equipment response to negative
	ctxt->eqpt_resp(res);

	return true;
};

void
virt_Creating_i::after_ActionDestroy_from_Creating(void * context)
{
	DUMP_STATE2(name(), "ActionDestroy", "Creating");
};

bool
virt_Creating_i::evActionPending(void* context)
{
	DUMP_STATE(name(), "evActionPending");

	return true;
};

void
virt_Creating_i::after_EqptDown_from_Creating(void * context)
{
	DUMP_STATE2(name(), "EqptDown", "Creating");
};

bool
virt_Creating_i::evEqptDown(void* context)
{
	Action           * ctxt;
	tnrcap_result_t    res;

	DUMP_STATE(name(), "evEqptDown");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get last atomic action equipment response
	res = ctxt->atomic()->eqpt_resp();

	//Set the final equipment response to negative
	ctxt->eqpt_resp(res);

	return true;
};

void
virt_Creating_i::after_AtomicActionOk_from_Creating(void * context)
{
	Action * ctxt;

	DUMP_STATE2(name(), "AtomicActionOk", "Creating");

	//Get context
	ctxt = static_cast<Action*>(context);
	//atomic action succeeds, so we have to update data model
	TNRC_OPSPEC_API::update_dm_after_action (ctxt);

};

bool
virt_Creating_i::evActionEndUp(void* context)
{
	Action * ctxt;

	DUMP_STATE(name(), "evActionEndUp");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Set the final equipment response
	ctxt->eqpt_resp(TNRCSP_RESULT_NOERROR);

	return true;
};

bool
virt_Creating_i::evAtomicActionNext(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcsp_handle_t      handlep;
	tnrcsp_result_t      res;
	xcdirection_t        dir;
	tnrc_boolean_t       isvirtual;

	DUMP_STATE(name(), "evAtomicActionNext");

	//Get context
	ctxt = static_cast<Action*>(context);
	//Get next atomic context
	ctxt->atomic(ctxt->front_todo());

	//Get action type
	act_type = ctxt->atomic()->action_type();

	TNRC_DBG("Creating next atomic action for action %s",
		 SHOW_ACTION_TYPE(ctxt->action_type()));

	switch (act_type) {
		case MAKE_XC:
			//Get action context
			MakeDestroyxc * make_ctxt;
			tnrc_boolean_t  activate;

			make_ctxt = dynamic_cast<MakeDestroyxc*>
				(ctxt->atomic());
			isvirtual = make_ctxt->is_virtual() ? 1 : 0;
			activate  = (make_ctxt->rsrv_xc_flag() == ACTIVATE) ? 1 : 0;

			dir = make_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    make_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			res = make_ctxt->plugin()->
				tnrcsp_make_xc (&handlep,
						make_ctxt->portid_in(),
						make_ctxt->labelid_in(),
						make_ctxt->portid_out(),
						make_ctxt->labelid_out(),
						dir,
						isvirtual,
						activate,
						TNRC_OPSPEC_API::
						make_xc_resp_cb,
						ctxt,
						TNRC_OPSPEC_API::
						notification_xc_cb,
						ctxt);
			break;
		case RESERVE_XC:
			//Get action context
			ReserveUnreservexc * reserve_ctxt;

			reserve_ctxt = dynamic_cast<ReserveUnreservexc*>
				(ctxt->atomic());
			isvirtual = reserve_ctxt->is_virtual() ? 1 : 0;
			dir = reserve_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    reserve_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			if (!reserve_ctxt->deactivation()) {
				res = reserve_ctxt->plugin()->
					tnrcsp_reserve_xc (&handlep,
							   reserve_ctxt->
							   portid_in(),
							   reserve_ctxt->
							   labelid_in(),
							   reserve_ctxt->
							   portid_out(),
							   reserve_ctxt->
							   labelid_out(),
							   dir,
							   isvirtual,
							   TNRC_OPSPEC_API::
							   reserve_xc_resp_cb,
							   ctxt);
			}
			else {
				res = TNRCSP_RESULT_NOERROR;
			}
			break;
	}
	ctxt->atomic()->prel_check(res);
	ctxt->atomic()->sp_handle (handlep);
	ctxt->atomic()->wait_answer (true);

	return true;
};

bool
virt_Creating_i::evActionDestroy(void* context)
{
	DUMP_STATE(name(), "evActionDestroy");

	return true;
};

bool
virt_Creating_i::evAtomicActionIncomplete(void* context)
{
	DUMP_STATE(name(), "evAtomicActionIncomplete");

	return true;
};


/*****************************************************************************
 *                    Class virt_Up_i
 *****************************************************************************/

void
virt_Up_i::after_ActionNotification_from_Up(void * context)
{
	DUMP_STATE2(name(), "ActionNotification", "Up");
};

bool
virt_Up_i::evActionNotification(void* context)
{
	tnrcsp_resource_detail_t * res_det;
	tnrcsp_resource_id_t     * res_id;
	struct zlistnode  * res_det_node;
	struct zlistnode  * res_det_nnode;
	void             * data;

	DUMP_STATE(name(), "evActionNotification");

	//get structure containing list of events
	res_id = (tnrcsp_resource_id_t *) context;

	//process all events notified
	for (ALL_LIST_ELEMENTS (res_id->resource_list, res_det_node,
				res_det_nnode, data)) {
		res_det = (tnrcsp_resource_detail_t *) data;
		//update data model to register notification from equipment
		TNRC_OPSPEC_API::update_dm_after_notify (res_id->portid,
							 res_det->labelid,
							 res_det->last_event);
	}
	TNRC_DBG ("Data model updated");

	return true;
};

void
virt_Up_i::after_ActionDestroy_from_Up(void * context)
{
	DUMP_STATE2(name(), "ActionDestroy", "Up");
};

bool
virt_Up_i::evActionDestroy(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcsp_handle_t      handle;
	tnrcsp_result_t      res;
	xcdirection_t        dir;
	tnrc_boolean_t       isvirtual;

	DUMP_STATE(name(), "evActionDestroy");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Swap correctly all atomic actions type, to perform correct destructions
	ctxt->swap_action_type();

	//Get first atomic action to destroy
	ctxt->atomic(ctxt->front_done());

	//Get action type
	act_type = ctxt->atomic()->action_type();

	TNRC_DBG ("Destroying first atomic action for action %s",
		  SHOW_ACTION_TYPE(act_type));

	switch (act_type) {
		case DESTROY_XC:
			//Get action context
			MakeDestroyxc * dest_ctxt;
			tnrc_boolean_t  deactivate;

			dest_ctxt = dynamic_cast<MakeDestroyxc*>
				(ctxt->atomic());
			isvirtual  = 0;
			deactivate = (dest_ctxt->rsrv_xc_flag() == DEACTIVATE) ? 1 : 0;

			dir = dest_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    dest_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			res = dest_ctxt->plugin()->
				tnrcsp_destroy_xc(&handle,
						  dest_ctxt->portid_in(),
						  dest_ctxt->labelid_in(),
						  dest_ctxt->portid_out(),
						  dest_ctxt->labelid_out(),
						  dir,
						  isvirtual,
						  deactivate,
						  TNRC_OPSPEC_API::
						  destroy_xc_resp_cb,
						  ctxt);
			break;
		case UNRESERVE_XC:
			//Get action context
			ReserveUnreservexc * unreserve_ctxt;

			isvirtual  = 0;
			unreserve_ctxt = dynamic_cast<ReserveUnreservexc*>
				(ctxt->atomic());
			dir = unreserve_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    unreserve_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			res = unreserve_ctxt->plugin()->
				tnrcsp_unreserve_xc(&handle,
						    unreserve_ctxt->
						    portid_in(),
						    unreserve_ctxt->
						    labelid_in(),
						    unreserve_ctxt->
						    portid_out(),
						    unreserve_ctxt->
						    labelid_out(),
						    dir,
						    isvirtual,
						    TNRC_OPSPEC_API::
						    unreserve_xc_resp_cb,
						    ctxt);
			break;
	}
	ctxt->atomic()->prel_check(res);
	ctxt->atomic()->sp_handle (handle);
	ctxt->atomic()->wait_answer (true);

	return true;
};


/*****************************************************************************
 *                    Class virt_Down_i
 *****************************************************************************/

void
virt_Down_i::after_ActionCreate_from_Down(void * context)
{
	DUMP_STATE2(name(), "ActionCreate", "Down");
};

bool
virt_Down_i::evActionCreate(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;
	tnrcsp_handle_t      handlep;
	tnrcsp_result_t      res;
	xcdirection_t        dir;
	tnrc_boolean_t       isvirtual;

	DUMP_STATE(name(), "evActionCreate");

	//Get action context
	ctxt = static_cast<Action*>(context);
	//Get atomic action context
	ctxt->atomic(ctxt->front_todo());

	//Get atomic action type
	act_type = ctxt->atomic()->action_type();

	TNRC_DBG ("Creating a %s action", SHOW_ACTION_TYPE(act_type));

	switch (act_type) {
		case MAKE_XC:
			//Get action context
			MakeDestroyxc  * make_ctxt;
			tnrc_boolean_t  activate;

			make_ctxt = dynamic_cast<MakeDestroyxc*>
				(ctxt->atomic());
			isvirtual = make_ctxt->is_virtual() ? 1 : 0;
			activate = (make_ctxt->rsrv_xc_flag() == ACTIVATE) ? 1 : 0;

			dir = make_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    make_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			res = make_ctxt->plugin()->
				tnrcsp_make_xc (&handlep,
						make_ctxt->portid_in(),
						make_ctxt->labelid_in(),
						make_ctxt->portid_out(),
						make_ctxt->labelid_out(),
						dir,
						isvirtual,
						activate,
						TNRC_OPSPEC_API::
						make_xc_resp_cb,
						ctxt,
						TNRC_OPSPEC_API::
						notification_xc_cb,
						ctxt);
			break;
		case RESERVE_XC:
			//Get action context
			ReserveUnreservexc * reserve_ctxt;

			reserve_ctxt = dynamic_cast<ReserveUnreservexc*>
				(ctxt->atomic());
			isvirtual = reserve_ctxt->is_virtual() ? 1 : 0;
			dir = reserve_ctxt->direction();
			if (!ctxt->plugin()->xc_bidir_support() &&
			    reserve_ctxt->direction() == XCDIRECTION_BIDIR) {
				dir = XCDIRECTION_UNIDIR;
			}
			//call tnrc sp function
			if (!reserve_ctxt->deactivation()) {
				res = reserve_ctxt->plugin()->
					tnrcsp_reserve_xc (&handlep,
							   reserve_ctxt->
							   portid_in(),
							   reserve_ctxt->
							   labelid_in(),
							   reserve_ctxt->
							   portid_out(),
							   reserve_ctxt->
							   labelid_out(),
							   dir,
							   isvirtual,
							   TNRC_OPSPEC_API::
							   reserve_xc_resp_cb,
							   ctxt);
			}
			else {
				res = TNRCSP_RESULT_NOERROR;
			}
			break;
		}
	ctxt->atomic()->prel_check(res);
	ctxt->atomic()->sp_handle (handlep);
	ctxt->atomic()->wait_answer (true);

	return true;
};


/*****************************************************************************
 *                    Class virt_Dismissed_i
 *****************************************************************************/

void
virt_Dismissed_i::after_AtomicActionTimeout_from_Dismissed(void * context)
{
	DUMP_STATE2(name(), "AtomicActionTimeout", "Dismissed");
};

bool
virt_Dismissed_i::evAtomicActionTimeout(void* context)
{
	DUMP_STATE(name(), "evAtomicActionTimeout");

	return true;
};

void
virt_Dismissed_i::after_AtomicActionKo_from_Dismissed(void * context)
{
	DUMP_STATE2(name(), "AtomicActionKo", "Dismissed");
};

bool
virt_Dismissed_i::evAtomicActionKo(void* context)
{
	Action          * ctxt;
	tnrcap_result_t   res;

	DUMP_STATE(name(), "evAtomicActionKo");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get last atomic action equipment response
	res = ctxt->atomic()->eqpt_resp();

	//Set the final equipment response to negative
	ctxt->eqpt_resp(res);

	return true;
};

void
virt_Dismissed_i::after_AtomicActionOk_from_Dismissed(void * context)
{
	Action * ctxt;

	DUMP_STATE2(name(), "AtomicActionOk", "Dismissed");

	//Get context
	ctxt = static_cast<Action*>(context);
	//atomic action succeeds, so we have to update data model
	TNRC_OPSPEC_API::update_dm_after_action (ctxt);

};

bool
virt_Dismissed_i::evAtomicActionOk(void* context)
{
	Action             * ctxt;
	tnrc_action_type_t   act_type;

	DUMP_STATE(name(), "evAtomicActionOk");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get action type
	act_type = ctxt->atomic()->action_type();

	//Pop last executed atomic action from todo queue. This action has to
	//be destroyed with a rollback action
	ctxt->push_done(ctxt->atomic());
	ctxt->pop_todo();

	return true;
};

void
virt_Dismissed_i::after_EqptDown_from_Dismissed(void * context)
{
	DUMP_STATE2(name(), "EqptDown", "Dismissed");
};

bool
virt_Dismissed_i::evEqptDown(void* context)
{
	Action          * ctxt;
	tnrcap_result_t   res;

	DUMP_STATE(name(), "evEqptDown");

	//Get context
	ctxt = static_cast<Action*>(context);

	//Get last atomic action equipment response
	res = ctxt->atomic()->eqpt_resp();

	//Set the final equipment response to negative
	ctxt->eqpt_resp(res);

	return true;
};

bool
virt_Dismissed_i::evAtomicActionIncomplete(void* context)
{
	DUMP_STATE(name(), "evAtomicActionIncomplete");

	return true;
};
