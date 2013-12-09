/*
 *  This file is part of phosphorus-g2mpls.
 *
 *  Copyright (C) 2006, 2007, 2008, 2009 Nextworks s.r.l.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Authors:
 *
 *  Giacomo Bernini       (Nextworks s.r.l.) <g.bernini_at_nextworks.it>
 *  Gino Carrozzo         (Nextworks s.r.l.) <g.carrozzo_at_nextworks.it>
 *  Nicola Ciulli         (Nextworks s.r.l.) <n.ciulli_at_nextworks.it>
 *  Giodi Giorgi          (Nextworks s.r.l.) <g.giorgi_at_nextworks.it>
 *  Francesco Salvestrini (Nextworks s.r.l.) <f.salvestrini_at_nextworks.it>
 */



#ifndef TNRC_ACTION_H
#define TNRC_ACTION_H

#include <iostream>
#include <stdio.h>
#include "tnrc_action_gen.h"





class Closing_i : public fsm::base_TNRC::Closing {

 public:

	fsm::base_TNRC::nextEvFor_AtomicActionOk_t AtomicActionOk(void* context);

	fsm::base_TNRC::nextEvFor_AtomicActionDownTimeout_t AtomicActionDownTimeout(void* context);

	void after_evAtomicActionDownTimeout_from_virt_Closing(void * context);

	fsm::base_TNRC::nextEvFor_AtomicActionKo_t AtomicActionKo(void* context);

	void after_evAtomicActionRetry_from_virt_Closing(void * context);

	fsm::base_TNRC::nextEvFor_AtomicActionRetryTimer_t AtomicActionRetryTimer(void* context);

	void after_evAtomicActionRetryTimer_from_virt_Closing(void * context);

	void after_evAtomicActionNext_from_virt_Closing(void * context);

	void after_evActionRollback_from_virt_Incomplete(void * context);

	void after_evActionDestroy_from_virt_Up(void * context);

};

class Incomplete_i : public fsm::base_TNRC::Incomplete {

 public:

	fsm::base_TNRC::nextEvFor_ActionRollback_t ActionRollback(void* context);

	fsm::base_TNRC::nextEvFor_EqptDown_t EqptDown(void* context);

	void after_evAtomicActionIncomplete_from_virt_Creating(void * context);

	void after_evAtomicActionOk_from_virt_Dismissed(void * context);

	void after_evAtomicActionIncomplete_from_virt_Dismissed(void * context);

};

class Creating_i : public fsm::base_TNRC::Creating {

 public:

	fsm::base_TNRC::nextEvFor_AtomicActionKo_t AtomicActionKo(void* context);

	fsm::base_TNRC::nextEvFor_ActionDestroy_t ActionDestroy(void* context);

	fsm::base_TNRC::nextEvFor_EqptDown_t EqptDown(void* context);

	fsm::base_TNRC::nextEvFor_AtomicActionOk_t AtomicActionOk(void* context);

	void after_evAtomicActionNext_from_virt_Creating(void * context);

	void after_evActionCreate_from_virt_Down(void * context);

};

class Up_i : public fsm::base_TNRC::Up {

 public:

	void after_evActionEndUp_from_virt_Creating(void * context);

	fsm::base_TNRC::nextEvFor_ActionNotification_t ActionNotification(void* context);

	void after_evActionNotification_from_virt_Up(void * context);

	fsm::base_TNRC::nextEvFor_ActionDestroy_t ActionDestroy(void* context);

};

class Down_i : public fsm::base_TNRC::Down {

 public:

	void after_evActionEndDown_from_virt_Closing(void * context);

	void after_evAtomicActionAbort_from_virt_Closing(void * context);

	void after_evEqptDown_from_virt_Incomplete(void * context);

	void after_evAtomicActionKo_from_virt_Creating(void * context);

	void after_evEqptDown_from_virt_Creating(void * context);

	void after_evActionDestroy_from_virt_Creating(void * context);

	fsm::base_TNRC::nextEvFor_ActionCreate_t ActionCreate(void* context);

	void after_evAtomicActionTimeout_from_virt_Dismissed(void * context);

	void after_evAtomicActionKo_from_virt_Dismissed(void * context);

	void after_evEqptDown_from_virt_Dismissed(void * context);

};

class Dismissed_i : public fsm::base_TNRC::Dismissed {

 public:

	void after_evActionPending_from_virt_Creating(void * context);

	fsm::base_TNRC::nextEvFor_AtomicActionTimeout_t AtomicActionTimeout(void* context);

	fsm::base_TNRC::nextEvFor_AtomicActionKo_t AtomicActionKo(void* context);

	fsm::base_TNRC::nextEvFor_AtomicActionOk_t AtomicActionOk(void* context);

	fsm::base_TNRC::nextEvFor_EqptDown_t EqptDown(void* context);

};



class virt_Closing_i : public fsm::base_TNRC::virt_Closing {

 public:

	void after_AtomicActionOk_from_Closing(void * context);

	bool evActionEndDown(void* context);

	void after_AtomicActionDownTimeout_from_Closing(void * context);

	bool evAtomicActionDownTimeout(void* context);

	void after_AtomicActionKo_from_Closing(void * context);

	bool evAtomicActionRetry(void* context);

	bool evAtomicActionAbort(void* context);

	void after_AtomicActionRetryTimer_from_Closing(void * context);

	bool evAtomicActionRetryTimer(void* context);

	bool evAtomicActionNext(void* context);

};

class virt_Incomplete_i : public fsm::base_TNRC::virt_Incomplete {

 public:

	void after_ActionRollback_from_Incomplete(void * context);

	bool evActionRollback(void* context);

	void after_EqptDown_from_Incomplete(void * context);

	bool evEqptDown(void* context);

};

class virt_Creating_i : public fsm::base_TNRC::virt_Creating {

 public:

	void after_AtomicActionKo_from_Creating(void * context);

	bool evAtomicActionKo(void* context);

	void after_ActionDestroy_from_Creating(void * context);

	bool evActionPending(void* context);

	void after_EqptDown_from_Creating(void * context);

	bool evEqptDown(void* context);

	void after_AtomicActionOk_from_Creating(void * context);

	bool evActionEndUp(void* context);

	bool evAtomicActionNext(void* context);

	bool evActionDestroy(void* context);

	bool evAtomicActionIncomplete(void* context);

};

class virt_Up_i : public fsm::base_TNRC::virt_Up {

 public:

	void after_ActionNotification_from_Up(void * context);

	bool evActionNotification(void* context);

	void after_ActionDestroy_from_Up(void * context);

	bool evActionDestroy(void* context);

};

class virt_Down_i : public fsm::base_TNRC::virt_Down {

 public:

	void after_ActionCreate_from_Down(void * context);

	bool evActionCreate(void* context);

};

class virt_Dismissed_i : public fsm::base_TNRC::virt_Dismissed {

 public:

	void after_AtomicActionTimeout_from_Dismissed(void * context);

	bool evAtomicActionTimeout(void* context);

	void after_AtomicActionKo_from_Dismissed(void * context);

	bool evAtomicActionKo(void* context);

	void after_AtomicActionOk_from_Dismissed(void * context);

	bool evAtomicActionOk(void* context);

	void after_EqptDown_from_Dismissed(void * context);

	bool evEqptDown(void* context);

	bool evAtomicActionIncomplete(void* context);

};


#endif // TNRC_ACTION_H
