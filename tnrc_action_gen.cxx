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



#include <iostream>
#include <stdio.h>
#include <string>
#include <map>
#include <list>
#include <stdlib.h>
#include "tnrc_action_gen.h"

// GG FIX 2008-11-14
#include <algorithm>

#include "tnrc_action.h"



/*****************************************************************************/
/*                             Finite State Machine                          */
/*****************************************************************************/
namespace fsm {


	/*************************************/
	/*    Finite State Machine - Core    */
	/*************************************/
	namespace base_TNRC {

		//
		// State/s
	        //

		//
		// BaseFSM
		//

		//
		// GenericFSM
		//
		bool GenericFSM::startModify(void)
		{
			if (changeInProgress_) {
				err("Changes already in progress ... call "
				    "endModify before!!!");
				return false;
			}
			dbg("### Start FSM changes");
			changeInProgress_ = true;
			return true;
		}

		bool GenericFSM::endModify(void)
		{
			if (!check()) {
				err("End FSM changes ... FAILED ###");
				return false;
			}
			dbg("End FSM changes ###");
			changeInProgress_ = false;
			return true;
		}

		bool GenericFSM::addState(std::string state)
		{
			if (!changeInProgress_) {
				err("Call startModify before any changes!!!");
				return false;
			}

			if (state.empty()) {
				err("Invalid state: string is empty");
				return false;
			}

			std::map<std::string, state_data_t>::iterator it;
			it = states_.find(state);
			if (it != states_.end()) {
				err("State " + state + " already added");
				return false;
			}

			state_data_t data;
			data.pre       = 0;
			data.post      = 0;
			data.in        = 0;
			states_[state] = data;

			dbg("Added state: " + state);
			return true;
		}

		bool GenericFSM::remState(std::string state)
		{
			if (!changeInProgress_) {
				err("Call startModify before any changes!!!");
				return false;
			}

			if (state.empty()) {
				err("Invalid state: string is empty");
				return false;
			}

			if (!states_.erase(state)) {
				err("State " + state + " not present");
				return false;
			}

			// Remove all the rows == state
			transitions_.removeRow(state);

			// Remove all the data (to-state) == state
			transitions_.remove(state);

			dbg("Removed state: " + state);

			if (startState_ == state) {
				wrn("State '" + state + "' was the start state"
				    " ... change it!!!");
				startState_ = std::string();
			}

			return true;
		}

		bool GenericFSM::addEvent(std::string event)
		{
			if (!changeInProgress_) {
				err("Call startModify before any changes!!!");
				return false;
			}

			if (event.empty()) {
				err("Invalid event: string is empty");
				return false;
			}

			std::list<std::string>::iterator it;
			it = find(events_.begin(),events_.end(),event);
			if (it != events_.end()) {
				err("Event " + event + " already added");
				return false;
			}
			events_.push_back(event);
			dbg("Added event: " + event);
			return true;
		}

		bool GenericFSM::remEvent(std::string event)
		{
			if (!changeInProgress_) {
				err("Call startModify before any changes!!!");
				return false;
			}

			if (event.empty()) {
				err("Invalid event: string is empty");
				return false;
			}

			events_.remove(event);

			// Remove all the columns == event
			transitions_.removeCol(event);

			dbg("Removed event: " + event);
			return true;
		}

		bool GenericFSM::addTransition(std::string from,
					       std::string to,
					       std::string event)
		{
			if (!changeInProgress_) {
				err("Call startModify before any changes!!!");
				return false;
			}

			if (from.empty() || to.empty() ||
			    event.empty()) {
				err("Invalid transition: some data are empty");
				return false;
			}

			Matrix<std::string,
				std::string,
				std::string>::dataIter it;
			it = transitions_.find(from, event);
			if (it != 0) {
				err("Transition from state '" + from + "' with"
				    " event '" + event + "' already added");
				return false;
			}

			transitions_[from][event] = to;
			dbg("Added transition: from state '" + from +
			    "' to state '" + to + "' with event '" +
			     event + "'");
			return true;
		}

		bool GenericFSM::remTransition(std::string from,
					       std::string to,
					       std::string event)
		{
			if (!changeInProgress_) {
				err("Call startModify before any changes!!!");
				return false;
			}

			if (from.empty() || to.empty() || event.empty()) {
				err("Invalid transition: some data are empty");
				return false;
			}

			Matrix<std::string,
				std::string,
				std::string>::dataIter it;
			it = transitions_.find(from, event);
			if (it != 0 && *it != to) {
				err("To-state for the transition from state '"
				    + from + "' event '" + event + "' is not '"
				    + to + "' but '" + *it + "'");
				return false;
			}

			if (!transitions_.remove(it)) {
				err("Transition from state '" + from +
				    "' to state '"+ to + "' with event '" +
				    event + "' is not present");
				return false;
			}

			dbg("Removed transition: from state '" + from +
			    "' to state '"+ to + "' with event '" + event +
			    "'");
			return true;
		}

		bool GenericFSM::setStartState(std::string state)
		{
			if (state.empty()) {
				err("Impossible to set start state to "
				    + state);
				return false;
			}

			dbg(std::string("Setting start state to ") + state);
			startState_ = state;
			return true;
		}

		bool GenericFSM::check(void)
		{
			dbg("Checking FSM integrity ...");

			if (transitions_.empty() && states_.empty()) {
				dbg("The FSM is completely empty!!!");
				return true;
			}

			if (states_.find(startState_) ==
			    states_.end()) {
				err("The start state is not a valid state");
				return false;
			}

			/*
			 *  Check if any transition state/event belong
			 *  to FSM states and events
			 */
			Matrix<std::string,
				std::string,
				std::string>::rowIter rowIt;
			for (rowIt = transitions_.begin();
			     rowIt != transitions_.end();
			     rowIt = transitions_.next(rowIt)) {

				// Check from-state
				if (states_.find(*rowIt) == states_.end()) {
					err("From-state " + *rowIt +
					    " is not a valid state");
					return false;
				}
				dbg("From-state '" + *rowIt +
				    "' is a valid state");

				Matrix<std::string,
					std::string,
					std::string>::colIter colIt;
				for (colIt = transitions_.begin(rowIt);
				     colIt != transitions_.end(rowIt);
				     colIt = transitions_.next(rowIt, colIt)){

					// Check event
					if (find(events_.begin(),
						 events_.end(),
						 *colIt) == events_.end()) {
						err("Event " + *colIt +
						    " is not a valid event");
						return false;
					}
					dbg("Event '" + *colIt +
					    "' is a valid event");

					// Check to-state
					std::string tmp;
					tmp = transitions_[*rowIt][*colIt];
					if (states_.find(tmp) ==
					    states_.end()) {
						err("To-state " + tmp +
						    " is not a valid state");
						return false;
					}
					dbg("To-state '" + tmp +
					    "' is a valid state");
				}
			}

			//TODO Graph check (partitioned/discontinued..)

			return true;
		}


		//
		// Fsm
		//
		Fsm::Fsm(traceLevel_t level)
			throw(std::string) :
			GenericFSM(level)
		{
			if (!startModify()) {
				throw(std::string("Impossible to init FSM"));
			}

			if (!setStartState("Down")) {
				throw(std::string("Impossible to set start "
						  "state for th FSM"));
			}
			currentState_ = TNRC_Down;
			prevState_    = currentState_;


			if (!addTransition("Closing",
					   "virt_Closing",
					   "AtomicActionOk")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Closing][TNRC_AtomicActionOk] = TNRC_virt_Closing;
			if (!addTransition("virt_Closing",
					   "Down",
					   "evActionEndDown")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Closing][TNRC_evActionEndDown] = TNRC_Down;
			if (!addTransition("Closing",
					   "virt_Closing",
					   "AtomicActionDownTimeout")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Closing][TNRC_AtomicActionDownTimeout] = TNRC_virt_Closing;
			if (!addTransition("virt_Closing",
					   "Closing",
					   "evAtomicActionDownTimeout")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Closing][TNRC_evAtomicActionDownTimeout] = TNRC_Closing;
			if (!addTransition("Closing",
					   "virt_Closing",
					   "AtomicActionKo")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Closing][TNRC_AtomicActionKo] = TNRC_virt_Closing;
			if (!addTransition("virt_Closing",
					   "Closing",
					   "evAtomicActionRetry")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Closing][TNRC_evAtomicActionRetry] = TNRC_Closing;
			if (!addTransition("virt_Closing",
					   "Down",
					   "evAtomicActionAbort")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Closing][TNRC_evAtomicActionAbort] = TNRC_Down;
			if (!addTransition("Closing",
					   "virt_Closing",
					   "AtomicActionRetryTimer")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Closing][TNRC_AtomicActionRetryTimer] = TNRC_virt_Closing;
			if (!addTransition("virt_Closing",
					   "Closing",
					   "evAtomicActionRetryTimer")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Closing][TNRC_evAtomicActionRetryTimer] = TNRC_Closing;
			if (!addTransition("virt_Closing",
					   "Closing",
					   "evAtomicActionNext")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Closing][TNRC_evAtomicActionNext] = TNRC_Closing;
			if (!addTransition("Incomplete",
					   "virt_Incomplete",
					   "ActionRollback")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Incomplete][TNRC_ActionRollback] = TNRC_virt_Incomplete;
			if (!addTransition("virt_Incomplete",
					   "Closing",
					   "evActionRollback")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Incomplete][TNRC_evActionRollback] = TNRC_Closing;
			if (!addTransition("Incomplete",
					   "virt_Incomplete",
					   "EqptDown")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Incomplete][TNRC_EqptDown] = TNRC_virt_Incomplete;
			if (!addTransition("virt_Incomplete",
					   "Down",
					   "evEqptDown")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Incomplete][TNRC_evEqptDown] = TNRC_Down;
			if (!addTransition("Creating",
					   "virt_Creating",
					   "AtomicActionKo")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Creating][TNRC_AtomicActionKo] = TNRC_virt_Creating;
			if (!addTransition("virt_Creating",
					   "Down",
					   "evAtomicActionKo")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Creating][TNRC_evAtomicActionKo] = TNRC_Down;
			if (!addTransition("Creating",
					   "virt_Creating",
					   "ActionDestroy")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Creating][TNRC_ActionDestroy] = TNRC_virt_Creating;
			if (!addTransition("virt_Creating",
					   "Dismissed",
					   "evActionPending")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Creating][TNRC_evActionPending] = TNRC_Dismissed;
			if (!addTransition("Creating",
					   "virt_Creating",
					   "EqptDown")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Creating][TNRC_EqptDown] = TNRC_virt_Creating;
			if (!addTransition("virt_Creating",
					   "Down",
					   "evEqptDown")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Creating][TNRC_evEqptDown] = TNRC_Down;
			if (!addTransition("Creating",
					   "virt_Creating",
					   "AtomicActionOk")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Creating][TNRC_AtomicActionOk] = TNRC_virt_Creating;
			if (!addTransition("virt_Creating",
					   "Up",
					   "evActionEndUp")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Creating][TNRC_evActionEndUp] = TNRC_Up;
			if (!addTransition("virt_Creating",
					   "Creating",
					   "evAtomicActionNext")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Creating][TNRC_evAtomicActionNext] = TNRC_Creating;
			if (!addTransition("virt_Creating",
					   "Down",
					   "evActionDestroy")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Creating][TNRC_evActionDestroy] = TNRC_Down;
			if (!addTransition("virt_Creating",
					   "Incomplete",
					   "evAtomicActionIncomplete")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Creating][TNRC_evAtomicActionIncomplete] = TNRC_Incomplete;
			if (!addTransition("Up",
					   "virt_Up",
					   "ActionNotification")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Up][TNRC_ActionNotification] = TNRC_virt_Up;
			if (!addTransition("virt_Up",
					   "Up",
					   "evActionNotification")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Up][TNRC_evActionNotification] = TNRC_Up;
			if (!addTransition("Up",
					   "virt_Up",
					   "ActionDestroy")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Up][TNRC_ActionDestroy] = TNRC_virt_Up;
			if (!addTransition("virt_Up",
					   "Closing",
					   "evActionDestroy")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Up][TNRC_evActionDestroy] = TNRC_Closing;
			if (!addTransition("Down",
					   "virt_Down",
					   "ActionCreate")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Down][TNRC_ActionCreate] = TNRC_virt_Down;
			if (!addTransition("virt_Down",
					   "Creating",
					   "evActionCreate")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Down][TNRC_evActionCreate] = TNRC_Creating;
			if (!addTransition("Dismissed",
					   "virt_Dismissed",
					   "AtomicActionTimeout")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Dismissed][TNRC_AtomicActionTimeout] = TNRC_virt_Dismissed;
			if (!addTransition("virt_Dismissed",
					   "Down",
					   "evAtomicActionTimeout")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Dismissed][TNRC_evAtomicActionTimeout] = TNRC_Down;
			if (!addTransition("Dismissed",
					   "virt_Dismissed",
					   "AtomicActionKo")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Dismissed][TNRC_AtomicActionKo] = TNRC_virt_Dismissed;
			if (!addTransition("virt_Dismissed",
					   "Down",
					   "evAtomicActionKo")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Dismissed][TNRC_evAtomicActionKo] = TNRC_Down;
			if (!addTransition("Dismissed",
					   "virt_Dismissed",
					   "AtomicActionOk")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Dismissed][TNRC_AtomicActionOk] = TNRC_virt_Dismissed;
			if (!addTransition("virt_Dismissed",
					   "Incomplete",
					   "evAtomicActionOk")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Dismissed][TNRC_evAtomicActionOk] = TNRC_Incomplete;
			if (!addTransition("Dismissed",
					   "virt_Dismissed",
					   "EqptDown")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_Dismissed][TNRC_EqptDown] = TNRC_virt_Dismissed;
			if (!addTransition("virt_Dismissed",
					   "Down",
					   "evEqptDown")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Dismissed][TNRC_evEqptDown] = TNRC_Down;
			if (!addTransition("virt_Dismissed",
					   "Incomplete",
					   "evAtomicActionIncomplete")) {
				throw(std::string("Impossible to add "
						  "transition .... "));
			}

			nextState_[TNRC_virt_Dismissed][TNRC_evAtomicActionIncomplete] = TNRC_Incomplete;


			if (!addState("Closing")) {
				throw(std::string("Impossible to add state "
						  "Closing"));
			}
			states_[TNRC_Closing] = new ::Closing_i;
			if (!addState("Incomplete")) {
				throw(std::string("Impossible to add state "
						  "Incomplete"));
			}
			states_[TNRC_Incomplete] = new ::Incomplete_i;
			if (!addState("Creating")) {
				throw(std::string("Impossible to add state "
						  "Creating"));
			}
			states_[TNRC_Creating] = new ::Creating_i;
			if (!addState("Up")) {
				throw(std::string("Impossible to add state "
						  "Up"));
			}
			states_[TNRC_Up] = new ::Up_i;
			if (!addState("Down")) {
				throw(std::string("Impossible to add state "
						  "Down"));
			}
			states_[TNRC_Down] = new ::Down_i;
			if (!addState("Dismissed")) {
				throw(std::string("Impossible to add state "
						  "Dismissed"));
			}
			states_[TNRC_Dismissed] = new ::Dismissed_i;

			if (!addState("virt_Closing")) {
				throw(std::string("Impossible to add state "
						  "virt_Closing"));
			}
			states_[TNRC_virt_Closing] = new ::virt_Closing_i;
			if (!addState("virt_Incomplete")) {
				throw(std::string("Impossible to add state "
						  "virt_Incomplete"));
			}
			states_[TNRC_virt_Incomplete] = new ::virt_Incomplete_i;
			if (!addState("virt_Creating")) {
				throw(std::string("Impossible to add state "
						  "virt_Creating"));
			}
			states_[TNRC_virt_Creating] = new ::virt_Creating_i;
			if (!addState("virt_Up")) {
				throw(std::string("Impossible to add state "
						  "virt_Up"));
			}
			states_[TNRC_virt_Up] = new ::virt_Up_i;
			if (!addState("virt_Down")) {
				throw(std::string("Impossible to add state "
						  "virt_Down"));
			}
			states_[TNRC_virt_Down] = new ::virt_Down_i;
			if (!addState("virt_Dismissed")) {
				throw(std::string("Impossible to add state "
						  "virt_Dismissed"));
			}
			states_[TNRC_virt_Dismissed] = new ::virt_Dismissed_i;

			//Only to check FSM integrity

			if (!addEvent("evAtomicActionRetryTimer")) {
				throw(std::string("Impossible to add event "
						  "evAtomicActionRetryTimer"));
			}
			if (!addEvent("evActionDestroy")) {
				throw(std::string("Impossible to add event "
						  "evActionDestroy"));
			}
			if (!addEvent("evActionPending")) {
				throw(std::string("Impossible to add event "
						  "evActionPending"));
			}
			if (!addEvent("evAtomicActionDownTimeout")) {
				throw(std::string("Impossible to add event "
						  "evAtomicActionDownTimeout"));
			}
			if (!addEvent("evActionNotification")) {
				throw(std::string("Impossible to add event "
						  "evActionNotification"));
			}
			if (!addEvent("evAtomicActionOk")) {
				throw(std::string("Impossible to add event "
						  "evAtomicActionOk"));
			}
			if (!addEvent("evAtomicActionNext")) {
				throw(std::string("Impossible to add event "
						  "evAtomicActionNext"));
			}
			if (!addEvent("evActionEndUp")) {
				throw(std::string("Impossible to add event "
						  "evActionEndUp"));
			}
			if (!addEvent("evActionEndDown")) {
				throw(std::string("Impossible to add event "
						  "evActionEndDown"));
			}
			if (!addEvent("evAtomicActionTimeout")) {
				throw(std::string("Impossible to add event "
						  "evAtomicActionTimeout"));
			}
			if (!addEvent("evEqptDown")) {
				throw(std::string("Impossible to add event "
						  "evEqptDown"));
			}
			if (!addEvent("evActionRollback")) {
				throw(std::string("Impossible to add event "
						  "evActionRollback"));
			}
			if (!addEvent("evAtomicActionKo")) {
				throw(std::string("Impossible to add event "
						  "evAtomicActionKo"));
			}
			if (!addEvent("evAtomicActionRetry")) {
				throw(std::string("Impossible to add event "
						  "evAtomicActionRetry"));
			}
			if (!addEvent("evAtomicActionIncomplete")) {
				throw(std::string("Impossible to add event "
						  "evAtomicActionIncomplete"));
			}
			if (!addEvent("evAtomicActionAbort")) {
				throw(std::string("Impossible to add event "
						  "evAtomicActionAbort"));
			}
			if (!addEvent("evActionCreate")) {
				throw(std::string("Impossible to add event "
						  "evActionCreate"));
			}

			if (!addEvent("AtomicActionRetryTimer")) {
				throw(std::string("Impossible to add event "
						  "AtomicActionRetryTimer"));
			}
			if (!addEvent("ActionDestroy")) {
				throw(std::string("Impossible to add event "
						  "ActionDestroy"));
			}
			if (!addEvent("AtomicActionDownTimeout")) {
				throw(std::string("Impossible to add event "
						  "AtomicActionDownTimeout"));
			}
			if (!addEvent("ActionNotification")) {
				throw(std::string("Impossible to add event "
						  "ActionNotification"));
			}
			if (!addEvent("AtomicActionOk")) {
				throw(std::string("Impossible to add event "
						  "AtomicActionOk"));
			}
			if (!addEvent("AtomicActionTimeout")) {
				throw(std::string("Impossible to add event "
						  "AtomicActionTimeout"));
			}
			if (!addEvent("EqptDown")) {
				throw(std::string("Impossible to add event "
						  "EqptDown"));
			}
			if (!addEvent("ActionRollback")) {
				throw(std::string("Impossible to add event "
						  "ActionRollback"));
			}
			if (!addEvent("AtomicActionKo")) {
				throw(std::string("Impossible to add event "
						  "AtomicActionKo"));
			}
			if (!addEvent("ActionCreate")) {
				throw(std::string("Impossible to add event "
						  "ActionCreate"));
			}

			if (!endModify()) {
				throw(std::string("Impossible to fini FSM"));
			}
		}

		Fsm::~Fsm(void)
		{
			startModify(); //useless ... just for check
			std::map<states_t, State *>::iterator it;
			for (it = states_.begin(); it != states_.end(); it++) {
				State * tmp;
				tmp = (*it).second;
				if (tmp) {
					// check and print error if needed
					remState(tmp->name());
					dbg(std::string("Deleting state: ") +
					    tmp->name());
					delete tmp;
				}
			}
			endModify(); //useless ... just for check
		}

		std::ostream& operator<<(std::ostream& s, const Fsm& f)
		{
			s << std::endl << "###### FSM [" << f.name_
			  << "] DUMP ### START ######" << std::endl;
			s << f.nextState_;
			s << "###### FSM [" << f.name_
			  << "] DUMP ###  END  ######" << std::endl
			  << std::endl;
			return s;
		}


		bool Fsm::evAtomicActionRetryTimer(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evAtomicActionRetryTimer");

			if (!tmp->evAtomicActionRetryTimer(context)) {
				err("Error in event 'evAtomicActionRetryTimer' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evAtomicActionRetryTimer];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Closing) : {
					tmp->after_evAtomicActionRetryTimer_from_virt_Closing(context);
					break;
				}
				default: {
					dbg(std::string("No after_evAtomicActionRetryTimer_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evActionDestroy(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evActionDestroy");

			if (!tmp->evActionDestroy(context)) {
				err("Error in event 'evActionDestroy' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evActionDestroy];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Creating) : {
					tmp->after_evActionDestroy_from_virt_Creating(context);
					break;
				}
				case (TNRC_virt_Up) : {
					tmp->after_evActionDestroy_from_virt_Up(context);
					break;
				}
				default: {
					dbg(std::string("No after_evActionDestroy_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evActionPending(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evActionPending");

			if (!tmp->evActionPending(context)) {
				err("Error in event 'evActionPending' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evActionPending];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Creating) : {
					tmp->after_evActionPending_from_virt_Creating(context);
					break;
				}
				default: {
					dbg(std::string("No after_evActionPending_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evAtomicActionDownTimeout(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evAtomicActionDownTimeout");

			if (!tmp->evAtomicActionDownTimeout(context)) {
				err("Error in event 'evAtomicActionDownTimeout' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evAtomicActionDownTimeout];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Closing) : {
					tmp->after_evAtomicActionDownTimeout_from_virt_Closing(context);
					break;
				}
				default: {
					dbg(std::string("No after_evAtomicActionDownTimeout_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evActionNotification(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evActionNotification");

			if (!tmp->evActionNotification(context)) {
				err("Error in event 'evActionNotification' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evActionNotification];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Up) : {
					tmp->after_evActionNotification_from_virt_Up(context);
					break;
				}
				default: {
					dbg(std::string("No after_evActionNotification_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evAtomicActionOk(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evAtomicActionOk");

			if (!tmp->evAtomicActionOk(context)) {
				err("Error in event 'evAtomicActionOk' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evAtomicActionOk];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Dismissed) : {
					tmp->after_evAtomicActionOk_from_virt_Dismissed(context);
					break;
				}
				default: {
					dbg(std::string("No after_evAtomicActionOk_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evAtomicActionNext(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evAtomicActionNext");

			if (!tmp->evAtomicActionNext(context)) {
				err("Error in event 'evAtomicActionNext' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evAtomicActionNext];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Closing) : {
					tmp->after_evAtomicActionNext_from_virt_Closing(context);
					break;
				}
				case (TNRC_virt_Creating) : {
					tmp->after_evAtomicActionNext_from_virt_Creating(context);
					break;
				}
				default: {
					dbg(std::string("No after_evAtomicActionNext_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evActionEndUp(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evActionEndUp");

			if (!tmp->evActionEndUp(context)) {
				err("Error in event 'evActionEndUp' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evActionEndUp];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Creating) : {
					tmp->after_evActionEndUp_from_virt_Creating(context);
					break;
				}
				default: {
					dbg(std::string("No after_evActionEndUp_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evActionEndDown(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evActionEndDown");

			if (!tmp->evActionEndDown(context)) {
				err("Error in event 'evActionEndDown' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evActionEndDown];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Closing) : {
					tmp->after_evActionEndDown_from_virt_Closing(context);
					break;
				}
				default: {
					dbg(std::string("No after_evActionEndDown_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evAtomicActionTimeout(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evAtomicActionTimeout");

			if (!tmp->evAtomicActionTimeout(context)) {
				err("Error in event 'evAtomicActionTimeout' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evAtomicActionTimeout];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Dismissed) : {
					tmp->after_evAtomicActionTimeout_from_virt_Dismissed(context);
					break;
				}
				default: {
					dbg(std::string("No after_evAtomicActionTimeout_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evEqptDown(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evEqptDown");

			if (!tmp->evEqptDown(context)) {
				err("Error in event 'evEqptDown' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evEqptDown];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Incomplete) : {
					tmp->after_evEqptDown_from_virt_Incomplete(context);
					break;
				}
				case (TNRC_virt_Creating) : {
					tmp->after_evEqptDown_from_virt_Creating(context);
					break;
				}
				case (TNRC_virt_Dismissed) : {
					tmp->after_evEqptDown_from_virt_Dismissed(context);
					break;
				}
				default: {
					dbg(std::string("No after_evEqptDown_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evActionRollback(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evActionRollback");

			if (!tmp->evActionRollback(context)) {
				err("Error in event 'evActionRollback' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evActionRollback];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Incomplete) : {
					tmp->after_evActionRollback_from_virt_Incomplete(context);
					break;
				}
				default: {
					dbg(std::string("No after_evActionRollback_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evAtomicActionKo(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evAtomicActionKo");

			if (!tmp->evAtomicActionKo(context)) {
				err("Error in event 'evAtomicActionKo' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evAtomicActionKo];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Creating) : {
					tmp->after_evAtomicActionKo_from_virt_Creating(context);
					break;
				}
				case (TNRC_virt_Dismissed) : {
					tmp->after_evAtomicActionKo_from_virt_Dismissed(context);
					break;
				}
				default: {
					dbg(std::string("No after_evAtomicActionKo_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evAtomicActionRetry(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evAtomicActionRetry");

			if (!tmp->evAtomicActionRetry(context)) {
				err("Error in event 'evAtomicActionRetry' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evAtomicActionRetry];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Closing) : {
					tmp->after_evAtomicActionRetry_from_virt_Closing(context);
					break;
				}
				default: {
					dbg(std::string("No after_evAtomicActionRetry_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evAtomicActionIncomplete(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evAtomicActionIncomplete");

			if (!tmp->evAtomicActionIncomplete(context)) {
				err("Error in event 'evAtomicActionIncomplete' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evAtomicActionIncomplete];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Creating) : {
					tmp->after_evAtomicActionIncomplete_from_virt_Creating(context);
					break;
				}
				case (TNRC_virt_Dismissed) : {
					tmp->after_evAtomicActionIncomplete_from_virt_Dismissed(context);
					break;
				}
				default: {
					dbg(std::string("No after_evAtomicActionIncomplete_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evAtomicActionAbort(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evAtomicActionAbort");

			if (!tmp->evAtomicActionAbort(context)) {
				err("Error in event 'evAtomicActionAbort' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evAtomicActionAbort];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Closing) : {
					tmp->after_evAtomicActionAbort_from_virt_Closing(context);
					break;
				}
				default: {
					dbg(std::string("No after_evAtomicActionAbort_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}

		bool Fsm::evActionCreate(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: evActionCreate");

			if (!tmp->evActionCreate(context)) {
				err("Error in event 'evActionCreate' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return false;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_evActionCreate];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_virt_Down) : {
					tmp->after_evActionCreate_from_virt_Down(context);
					break;
				}
				default: {
					dbg(std::string("No after_evActionCreate_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return true;
		}



		nextEvFor_AtomicActionRetryTimer_t Fsm::AtomicActionRetryTimer(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: AtomicActionRetryTimer");

                        nextEvFor_AtomicActionRetryTimer_t next_ev;
                        next_ev = tmp->AtomicActionRetryTimer(context);

			if (!next_ev) {
				err("Error in event 'AtomicActionRetryTimer' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return next_ev;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_AtomicActionRetryTimer];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_Closing) : {
					tmp->after_AtomicActionRetryTimer_from_Closing(context);
					break;
				}
				default: {
					dbg(std::string("No after_AtomicActionRetryTimer_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return next_ev;
		}

		nextEvFor_ActionDestroy_t Fsm::ActionDestroy(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: ActionDestroy");

                        nextEvFor_ActionDestroy_t next_ev;
                        next_ev = tmp->ActionDestroy(context);

			if (!next_ev) {
				err("Error in event 'ActionDestroy' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return next_ev;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_ActionDestroy];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_Creating) : {
					tmp->after_ActionDestroy_from_Creating(context);
					break;
				}
				case (TNRC_Up) : {
					tmp->after_ActionDestroy_from_Up(context);
					break;
				}
				default: {
					dbg(std::string("No after_ActionDestroy_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return next_ev;
		}

		nextEvFor_AtomicActionDownTimeout_t Fsm::AtomicActionDownTimeout(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: AtomicActionDownTimeout");

                        nextEvFor_AtomicActionDownTimeout_t next_ev;
                        next_ev = tmp->AtomicActionDownTimeout(context);

			if (!next_ev) {
				err("Error in event 'AtomicActionDownTimeout' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return next_ev;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_AtomicActionDownTimeout];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_Closing) : {
					tmp->after_AtomicActionDownTimeout_from_Closing(context);
					break;
				}
				default: {
					dbg(std::string("No after_AtomicActionDownTimeout_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return next_ev;
		}

		nextEvFor_ActionNotification_t Fsm::ActionNotification(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: ActionNotification");

                        nextEvFor_ActionNotification_t next_ev;
                        next_ev = tmp->ActionNotification(context);

			if (!next_ev) {
				err("Error in event 'ActionNotification' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return next_ev;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_ActionNotification];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_Up) : {
					tmp->after_ActionNotification_from_Up(context);
					break;
				}
				default: {
					dbg(std::string("No after_ActionNotification_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return next_ev;
		}

		nextEvFor_AtomicActionOk_t Fsm::AtomicActionOk(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: AtomicActionOk");

                        nextEvFor_AtomicActionOk_t next_ev;
                        next_ev = tmp->AtomicActionOk(context);

			if (!next_ev) {
				err("Error in event 'AtomicActionOk' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return next_ev;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_AtomicActionOk];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_Closing) : {
					tmp->after_AtomicActionOk_from_Closing(context);
					break;
				}
				case (TNRC_Creating) : {
					tmp->after_AtomicActionOk_from_Creating(context);
					break;
				}
				case (TNRC_Dismissed) : {
					tmp->after_AtomicActionOk_from_Dismissed(context);
					break;
				}
				default: {
					dbg(std::string("No after_AtomicActionOk_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return next_ev;
		}

		nextEvFor_AtomicActionTimeout_t Fsm::AtomicActionTimeout(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: AtomicActionTimeout");

                        nextEvFor_AtomicActionTimeout_t next_ev;
                        next_ev = tmp->AtomicActionTimeout(context);

			if (!next_ev) {
				err("Error in event 'AtomicActionTimeout' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return next_ev;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_AtomicActionTimeout];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_Dismissed) : {
					tmp->after_AtomicActionTimeout_from_Dismissed(context);
					break;
				}
				default: {
					dbg(std::string("No after_AtomicActionTimeout_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return next_ev;
		}

		nextEvFor_EqptDown_t Fsm::EqptDown(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: EqptDown");

                        nextEvFor_EqptDown_t next_ev;
                        next_ev = tmp->EqptDown(context);

			if (!next_ev) {
				err("Error in event 'EqptDown' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return next_ev;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_EqptDown];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_Incomplete) : {
					tmp->after_EqptDown_from_Incomplete(context);
					break;
				}
				case (TNRC_Creating) : {
					tmp->after_EqptDown_from_Creating(context);
					break;
				}
				case (TNRC_Dismissed) : {
					tmp->after_EqptDown_from_Dismissed(context);
					break;
				}
				default: {
					dbg(std::string("No after_EqptDown_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return next_ev;
		}

		nextEvFor_ActionRollback_t Fsm::ActionRollback(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: ActionRollback");

                        nextEvFor_ActionRollback_t next_ev;
                        next_ev = tmp->ActionRollback(context);

			if (!next_ev) {
				err("Error in event 'ActionRollback' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return next_ev;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_ActionRollback];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_Incomplete) : {
					tmp->after_ActionRollback_from_Incomplete(context);
					break;
				}
				default: {
					dbg(std::string("No after_ActionRollback_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return next_ev;
		}

		nextEvFor_AtomicActionKo_t Fsm::AtomicActionKo(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: AtomicActionKo");

                        nextEvFor_AtomicActionKo_t next_ev;
                        next_ev = tmp->AtomicActionKo(context);

			if (!next_ev) {
				err("Error in event 'AtomicActionKo' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return next_ev;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_AtomicActionKo];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_Closing) : {
					tmp->after_AtomicActionKo_from_Closing(context);
					break;
				}
				case (TNRC_Creating) : {
					tmp->after_AtomicActionKo_from_Creating(context);
					break;
				}
				case (TNRC_Dismissed) : {
					tmp->after_AtomicActionKo_from_Dismissed(context);
					break;
				}
				default: {
					dbg(std::string("No after_AtomicActionKo_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return next_ev;
		}

		nextEvFor_ActionCreate_t Fsm::ActionCreate(void * context)
		{
			State * tmp;
			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid current state!"));
				//return false;
			}
			dbg("Event: ActionCreate");

                        nextEvFor_ActionCreate_t next_ev;
                        next_ev = tmp->ActionCreate(context);

			if (!next_ev) {
				err("Error in event 'ActionCreate' ... "
				    "maintaining the old state '"
				    + tmp->name() + "'");
				return next_ev;
			}

			prevState_    = currentState_;
			currentState_ = nextState_[currentState_][TNRC_ActionCreate];
			dbg(std::string("New state: ") +
			    STATE_NAME(currentState_));

			tmp = currentState();
			if (!tmp) {
				throw(std::string("Invalid next state!"));
			}

			switch (prevState_) {
				case (TNRC_Down) : {
					tmp->after_ActionCreate_from_Down(context);
					break;
				}
				default: {
					dbg(std::string("No after_ActionCreate_from_") +
					    STATE_NAME(prevState_));
					break;
				}
			}

			return next_ev;
		}


		State * Fsm::currentState(void)
		{
			std::map<states_t, State *>::iterator it;
			it = states_.find(currentState_);
			if (it == states_.end()) {
				err(std::string("Invalid pointer for "
						"current state!"));
				return 0;
			}
			std::string name = (*it).second ?
				(*it).second->name() : "Invalid pointer";
			dbg("Current state: " + name);
			return (*it).second;
		}

		bool Fsm::go2prevState(void)
		{
			if (prevState_ == currentState_) {
				err("Previous and current state are equal!"
				    " ... only 1 step back is allowed");
				return false;
			}

			currentState_ = prevState_;

			std::string name = STATE_NAME(currentState_);
			dbg("Setting FSM state to previous one: " + name);

			return true;
		}

		std::ostream& operator<<(std::ostream& s,
					 const fsm::base_TNRC::
					 Fsm::states_t& st)
		{
			s << STATE_NAME(st);
			return s;
		}

		std::ostream& operator<<(std::ostream& s,
					 const fsm::base_TNRC::
					 Fsm::events_t& ev)
		{
			s << EVENT_NAME(ev);
			return s;
		}
	}



	/*************************************/
	/*   Finite State Machine - Wrapper  */
	/*************************************/


	namespace TNRC {

		//
		// virtFsm
		//
		virtFsm::virtFsm(base_TNRC::BaseFSM::traceLevel_t level)
			throw(std::string)
		{
			try {
				fsm_ = new base_TNRC::Fsm(level);
				assert(fsm_);
			} catch (std::string e) {
				throw(e);
			} catch (...) {
				throw(std::string("Impossible to start virtFSM"));
			}
		}

		virtFsm::~virtFsm(void)
		{
                        if (fsm_) delete fsm_;
		}

		std::ostream& operator<<(std::ostream& s, const virtFsm& f)
		{
			s << std::endl << "###### virtFSM [" //<< f.name_
			  << "] DUMP ### START ######" << std::endl;
			//s << f.nextState_;
			s << "###### FSM [" //<< f.name_
			  << "] DUMP ###  END  ######" << std::endl
			  << std::endl;
			return s;
		}

		void virtFsm::post(root_events_t ev, void * context, bool enqueue)
		{
			data_event_t * data;
			data = new data_event_t;
			if (!data) {
				std::cout << "ERR: Cannot allocate memory for "
					"a new data event struct" << std::endl;
				return;
			}
			data->ev      = ev;
			data->context = context;

			events_.push_back(data);

			if (!enqueue) {
				runPendingWork();
			}
		}

		void virtFsm::runPendingWork(void)
		{
			if (events_.empty()) {
				std::cout << "[DBG]: Empty event list ... "
					"nothing else to do" << std::endl;
				return;
			}

			data_event_t * data;
			data = events_.front();
			events_.pop_front();

			if (!data) {
				std::cout << "ERR: Invalid data event from "
					"event list" << std::endl;
				return;
			}

			switch (data->ev) {

				case TNRC_AtomicActionRetryTimer:
                                        AtomicActionRetryTimer(data->context);
					break;

				case TNRC_ActionDestroy:
                                        ActionDestroy(data->context);
					break;

				case TNRC_AtomicActionDownTimeout:
                                        AtomicActionDownTimeout(data->context);
					break;

				case TNRC_ActionNotification:
                                        ActionNotification(data->context);
					break;

				case TNRC_AtomicActionOk:
                                        AtomicActionOk(data->context);
					break;

				case TNRC_AtomicActionTimeout:
                                        AtomicActionTimeout(data->context);
					break;

				case TNRC_EqptDown:
                                        EqptDown(data->context);
					break;

				case TNRC_ActionRollback:
                                        ActionRollback(data->context);
					break;

				case TNRC_AtomicActionKo:
                                        AtomicActionKo(data->context);
					break;

				case TNRC_ActionCreate:
                                        ActionCreate(data->context);
					break;

                                //case TNRC_invalid_event:
				default:
					std::cout << "ERR: Invalid event type"
						  << std::endl;
			}

			delete data;

			runPendingWork();
		}


		void virtFsm::AtomicActionRetryTimer(void * context)
                {
			// case sul tipo di next_event e se errore
			//  torno stato precedente

                        fsm::base_TNRC::nextEvFor_AtomicActionRetryTimer_t next_ev;
			try {
				next_ev = fsm_->AtomicActionRetryTimer(context);
			} catch (std::string e) {
				std::cout << "EXCEPTION: " << e << std::endl;
				return;
			} catch (...) {
				std::cout << "UNKNOWN EXCEPTION" << std::endl;
				return;
			}

                        bool res;
                        switch (next_ev) {
			        case fsm::base_TNRC::TNRC_from_AtomicActionRetryTimer_to_evAtomicActionRetryTimer:
				        try {
						res = fsm_->evAtomicActionRetryTimer(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

				case fsm::base_TNRC::TNRC_from_AtomicActionRetryTimer_to_InvalidEvent:
				default:
					std::cout << "ERR: Invalid event type"
						  << std::endl;
                                        fsm_->go2prevState();
				        return;
                        }
                        if (!res) {
				std::cout << "ERR: First step for "
					  << __FUNCTION__ <<" failed"
					  << std::endl;
                                fsm_->go2prevState();
                        }
                }

		void virtFsm::ActionDestroy(void * context)
                {
			// case sul tipo di next_event e se errore
			//  torno stato precedente

                        fsm::base_TNRC::nextEvFor_ActionDestroy_t next_ev;
			try {
				next_ev = fsm_->ActionDestroy(context);
			} catch (std::string e) {
				std::cout << "EXCEPTION: " << e << std::endl;
				return;
			} catch (...) {
				std::cout << "UNKNOWN EXCEPTION" << std::endl;
				return;
			}

                        bool res;
                        switch (next_ev) {
			        case fsm::base_TNRC::TNRC_from_ActionDestroy_to_evActionDestroy:
				        try {
						res = fsm_->evActionDestroy(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

			        case fsm::base_TNRC::TNRC_from_ActionDestroy_to_evActionPending:
				        try {
						res = fsm_->evActionPending(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

				case fsm::base_TNRC::TNRC_from_ActionDestroy_to_InvalidEvent:
				default:
					std::cout << "ERR: Invalid event type"
						  << std::endl;
                                        fsm_->go2prevState();
				        return;
                        }
                        if (!res) {
				std::cout << "ERR: First step for "
					  << __FUNCTION__ <<" failed"
					  << std::endl;
                                fsm_->go2prevState();
                        }
                }

		void virtFsm::AtomicActionDownTimeout(void * context)
                {
			// case sul tipo di next_event e se errore
			//  torno stato precedente

                        fsm::base_TNRC::nextEvFor_AtomicActionDownTimeout_t next_ev;
			try {
				next_ev = fsm_->AtomicActionDownTimeout(context);
			} catch (std::string e) {
				std::cout << "EXCEPTION: " << e << std::endl;
				return;
			} catch (...) {
				std::cout << "UNKNOWN EXCEPTION" << std::endl;
				return;
			}

                        bool res;
                        switch (next_ev) {
			        case fsm::base_TNRC::TNRC_from_AtomicActionDownTimeout_to_evAtomicActionDownTimeout:
				        try {
						res = fsm_->evAtomicActionDownTimeout(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

				case fsm::base_TNRC::TNRC_from_AtomicActionDownTimeout_to_InvalidEvent:
				default:
					std::cout << "ERR: Invalid event type"
						  << std::endl;
                                        fsm_->go2prevState();
				        return;
                        }
                        if (!res) {
				std::cout << "ERR: First step for "
					  << __FUNCTION__ <<" failed"
					  << std::endl;
                                fsm_->go2prevState();
                        }
                }

		void virtFsm::ActionNotification(void * context)
                {
			// case sul tipo di next_event e se errore
			//  torno stato precedente

                        fsm::base_TNRC::nextEvFor_ActionNotification_t next_ev;
			try {
				next_ev = fsm_->ActionNotification(context);
			} catch (std::string e) {
				std::cout << "EXCEPTION: " << e << std::endl;
				return;
			} catch (...) {
				std::cout << "UNKNOWN EXCEPTION" << std::endl;
				return;
			}

                        bool res;
                        switch (next_ev) {
			        case fsm::base_TNRC::TNRC_from_ActionNotification_to_evActionNotification:
				        try {
						res = fsm_->evActionNotification(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

				case fsm::base_TNRC::TNRC_from_ActionNotification_to_InvalidEvent:
				default:
					std::cout << "ERR: Invalid event type"
						  << std::endl;
                                        fsm_->go2prevState();
				        return;
                        }
                        if (!res) {
				std::cout << "ERR: First step for "
					  << __FUNCTION__ <<" failed"
					  << std::endl;
                                fsm_->go2prevState();
                        }
                }

		void virtFsm::AtomicActionOk(void * context)
                {
			// case sul tipo di next_event e se errore
			//  torno stato precedente

                        fsm::base_TNRC::nextEvFor_AtomicActionOk_t next_ev;
			try {
				next_ev = fsm_->AtomicActionOk(context);
			} catch (std::string e) {
				std::cout << "EXCEPTION: " << e << std::endl;
				return;
			} catch (...) {
				std::cout << "UNKNOWN EXCEPTION" << std::endl;
				return;
			}

                        bool res;
                        switch (next_ev) {
			        case fsm::base_TNRC::TNRC_from_AtomicActionOk_to_evAtomicActionOk:
				        try {
						res = fsm_->evAtomicActionOk(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

			        case fsm::base_TNRC::TNRC_from_AtomicActionOk_to_evAtomicActionNext:
				        try {
						res = fsm_->evAtomicActionNext(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

			        case fsm::base_TNRC::TNRC_from_AtomicActionOk_to_evActionEndUp:
				        try {
						res = fsm_->evActionEndUp(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

			        case fsm::base_TNRC::TNRC_from_AtomicActionOk_to_evActionEndDown:
				        try {
						res = fsm_->evActionEndDown(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

				case fsm::base_TNRC::TNRC_from_AtomicActionOk_to_InvalidEvent:
				default:
					std::cout << "ERR: Invalid event type"
						  << std::endl;
                                        fsm_->go2prevState();
				        return;
                        }
                        if (!res) {
				std::cout << "ERR: First step for "
					  << __FUNCTION__ <<" failed"
					  << std::endl;
                                fsm_->go2prevState();
                        }
                }

		void virtFsm::AtomicActionTimeout(void * context)
                {
			// case sul tipo di next_event e se errore
			//  torno stato precedente

                        fsm::base_TNRC::nextEvFor_AtomicActionTimeout_t next_ev;
			try {
				next_ev = fsm_->AtomicActionTimeout(context);
			} catch (std::string e) {
				std::cout << "EXCEPTION: " << e << std::endl;
				return;
			} catch (...) {
				std::cout << "UNKNOWN EXCEPTION" << std::endl;
				return;
			}

                        bool res;
                        switch (next_ev) {
			        case fsm::base_TNRC::TNRC_from_AtomicActionTimeout_to_evAtomicActionTimeout:
				        try {
						res = fsm_->evAtomicActionTimeout(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

				case fsm::base_TNRC::TNRC_from_AtomicActionTimeout_to_InvalidEvent:
				default:
					std::cout << "ERR: Invalid event type"
						  << std::endl;
                                        fsm_->go2prevState();
				        return;
                        }
                        if (!res) {
				std::cout << "ERR: First step for "
					  << __FUNCTION__ <<" failed"
					  << std::endl;
                                fsm_->go2prevState();
                        }
                }

		void virtFsm::EqptDown(void * context)
                {
			// case sul tipo di next_event e se errore
			//  torno stato precedente

                        fsm::base_TNRC::nextEvFor_EqptDown_t next_ev;
			try {
				next_ev = fsm_->EqptDown(context);
			} catch (std::string e) {
				std::cout << "EXCEPTION: " << e << std::endl;
				return;
			} catch (...) {
				std::cout << "UNKNOWN EXCEPTION" << std::endl;
				return;
			}

                        bool res;
                        switch (next_ev) {
			        case fsm::base_TNRC::TNRC_from_EqptDown_to_evEqptDown:
				        try {
						res = fsm_->evEqptDown(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

				case fsm::base_TNRC::TNRC_from_EqptDown_to_InvalidEvent:
				default:
					std::cout << "ERR: Invalid event type"
						  << std::endl;
                                        fsm_->go2prevState();
				        return;
                        }
                        if (!res) {
				std::cout << "ERR: First step for "
					  << __FUNCTION__ <<" failed"
					  << std::endl;
                                fsm_->go2prevState();
                        }
                }

		void virtFsm::ActionRollback(void * context)
                {
			// case sul tipo di next_event e se errore
			//  torno stato precedente

                        fsm::base_TNRC::nextEvFor_ActionRollback_t next_ev;
			try {
				next_ev = fsm_->ActionRollback(context);
			} catch (std::string e) {
				std::cout << "EXCEPTION: " << e << std::endl;
				return;
			} catch (...) {
				std::cout << "UNKNOWN EXCEPTION" << std::endl;
				return;
			}

                        bool res;
                        switch (next_ev) {
			        case fsm::base_TNRC::TNRC_from_ActionRollback_to_evActionRollback:
				        try {
						res = fsm_->evActionRollback(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

				case fsm::base_TNRC::TNRC_from_ActionRollback_to_InvalidEvent:
				default:
					std::cout << "ERR: Invalid event type"
						  << std::endl;
                                        fsm_->go2prevState();
				        return;
                        }
                        if (!res) {
				std::cout << "ERR: First step for "
					  << __FUNCTION__ <<" failed"
					  << std::endl;
                                fsm_->go2prevState();
                        }
                }

		void virtFsm::AtomicActionKo(void * context)
                {
			// case sul tipo di next_event e se errore
			//  torno stato precedente

                        fsm::base_TNRC::nextEvFor_AtomicActionKo_t next_ev;
			try {
				next_ev = fsm_->AtomicActionKo(context);
			} catch (std::string e) {
				std::cout << "EXCEPTION: " << e << std::endl;
				return;
			} catch (...) {
				std::cout << "UNKNOWN EXCEPTION" << std::endl;
				return;
			}

                        bool res;
                        switch (next_ev) {
			        case fsm::base_TNRC::TNRC_from_AtomicActionKo_to_evAtomicActionKo:
				        try {
						res = fsm_->evAtomicActionKo(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

			        case fsm::base_TNRC::TNRC_from_AtomicActionKo_to_evAtomicActionRetry:
				        try {
						res = fsm_->evAtomicActionRetry(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

			        case fsm::base_TNRC::TNRC_from_AtomicActionKo_to_evAtomicActionIncomplete:
				        try {
						res = fsm_->evAtomicActionIncomplete(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

			        case fsm::base_TNRC::TNRC_from_AtomicActionKo_to_evAtomicActionAbort:
				        try {
						res = fsm_->evAtomicActionAbort(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

				case fsm::base_TNRC::TNRC_from_AtomicActionKo_to_InvalidEvent:
				default:
					std::cout << "ERR: Invalid event type"
						  << std::endl;
                                        fsm_->go2prevState();
				        return;
                        }
                        if (!res) {
				std::cout << "ERR: First step for "
					  << __FUNCTION__ <<" failed"
					  << std::endl;
                                fsm_->go2prevState();
                        }
                }

		void virtFsm::ActionCreate(void * context)
                {
			// case sul tipo di next_event e se errore
			//  torno stato precedente

                        fsm::base_TNRC::nextEvFor_ActionCreate_t next_ev;
			try {
				next_ev = fsm_->ActionCreate(context);
			} catch (std::string e) {
				std::cout << "EXCEPTION: " << e << std::endl;
				return;
			} catch (...) {
				std::cout << "UNKNOWN EXCEPTION" << std::endl;
				return;
			}

                        bool res;
                        switch (next_ev) {
			        case fsm::base_TNRC::TNRC_from_ActionCreate_to_evActionCreate:
				        try {
						res = fsm_->evActionCreate(context);
					} catch (std::string e) {
						std::cout << "EXCEPTION: " << e << std::endl;
						fsm_->go2prevState();
						return;
					} catch (...) {
						std::cout << "UNKNOWN EXCEPTION" << std::endl;
						fsm_->go2prevState();
						return;
					}
                                        break;

				case fsm::base_TNRC::TNRC_from_ActionCreate_to_InvalidEvent:
				default:
					std::cout << "ERR: Invalid event type"
						  << std::endl;
                                        fsm_->go2prevState();
				        return;
                        }
                        if (!res) {
				std::cout << "ERR: First step for "
					  << __FUNCTION__ <<" failed"
					  << std::endl;
                                fsm_->go2prevState();
                        }
                }


		std::string virtFsm::currentState(void) {
			return fsm_->currentState() ?
				fsm_->currentState()->name(): "Unknown";
		}

		//std::ostream& operator<<(std::ostream& s,
		//			 const fsm::virtual_RSVP::
		//			 virtFsm::states_t& st)
		//{
		//	s << STATE_NAME(st);
		//	return s;
		//}
		//
		//std::ostream& operator<<(std::ostream& s,
		//			 const fsm::virtual_RSVP::
		//			 virtFsm::events_t& ev)
		//{
		//	s << EVENT_NAME(ev);
		//	return s;
		//}
	}

}
