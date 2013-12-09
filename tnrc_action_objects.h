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



#ifndef TNRC_ACTION_OBJECTS_H
#define TNRC_ACTION_OBJECTS_H

class Action;

typedef enum {
	MAKE_XC = 0,
	DESTROY_XC,
	GET_EQPT_DETAILS,
	GET_DL_LIST,
	GET_DL_DETAILS,
	GET_DL_ALARMS,
	ENABLE_ALARMS,
	DISABLE_ALARMS,
	FLUSH_RESOURCES,
	PROTECT_XC,
	UNPROTECT_XC,
	RESERVE_XC,
	UNRESERVE_XC,
	RESERVE_RESOURCE,
	UNRESERVE_RESOURCE,
	UNDEFINED
} tnrc_action_type_t;

#include <deque>
#include "thread.h"

#include "tnrc_action.h"
#include "tnrc_sp.h"
#include "tnrc_common_types.h"

#define _DEFINE_QUEUE_ITERATOR(TD,T1,P,V)		\
	typedef std::deque<T1>::iterator TD;		\
	TD begin##P(void) { return V.begin(); };	\
	TD end##P(void)   { return V.end();   };

#define DEFINE_QUEUE_ITERATOR(N,T)					\
	_DEFINE_QUEUE_ITERATOR(iterator_##N, T *, _##N, N##_);

#define SHOW_ACTION_TYPE(X)                                     \
  (((X) == MAKE_XC            ) ? "MAKE XC"             :	\
  (((X) == DESTROY_XC         ) ? "DESTROY XC"          :	\
  (((X) == GET_EQPT_DETAILS   ) ? "GET EQPT DETAILS"    :	\
  (((X) == GET_DL_LIST        ) ? "GET DL LIST"         :	\
  (((X) == GET_DL_DETAILS     ) ? "GET DL DETAILS"      :	\
  (((X) == GET_DL_ALARMS      ) ? "GET DL ALARMS"       :	\
  (((X) == ENABLE_ALARMS      ) ? "ENABLE ALARMS"       :	\
  (((X) == DISABLE_ALARMS     ) ? "DISABLE ALARMS"      :	\
  (((X) == FLUSH_RESOURCES    ) ? "FLUSH RESOURCES"     :	\
  (((X) == PROTECT_XC         ) ? "PROTECT XC"          :	\
  (((X) == UNPROTECT_XC       ) ? "UNPROTECT XC"        :	\
  (((X) == RESERVE_XC         ) ? "RESERVE XC"          :	\
  (((X) == UNRESERVE_XC       ) ? "UNRESERVE XC"        :	\
  (((X) == RESERVE_RESOURCE   ) ? "RESERVE RESOURCE"    :	\
  (((X) == UNRESERVE_RESOURCE ) ? "UNRESERVE RESOURCE"  :	\
				  "==UNKNOWN==")))))))))))))))

//Forward declaration
class Plugin;

typedef enum {
	NO_RSRV = 0,
	ACTIVATE,
	DEACTIVATE
} tnrc_rsrv_xc_flag_t;

typedef struct {
	tnrc_action_type_t   type;
	void		   * action_param;

} api_queue_element_t;

typedef struct {
	tnrcap_cookie_t            cookie;

	Plugin                   * plugin;

	tnrc_port_id_t             dl_id_in;
	label_t                    labelid_in;
	tnrc_port_id_t             dl_id_out;
	label_t                    labelid_out;
	xcdirection_t              dir;
	tnrc_boolean_t             is_virtual;

	tnrc_rsrv_xc_flag_t        rsrv_xc_flag;
	tnrcap_cookie_t            rsrv_cookie;

	long                       resp_ctxt;
	long                       async_ctxt;

} make_xc_param_t;

typedef struct {
	tnrcap_cookie_t        cookie;

	tnrc_rsrv_xc_flag_t    rsrv_xc_flag;

	long                   resp_ctxt;

} destroy_xc_param_t;

typedef struct {
	tnrcap_cookie_t        cookie;

	Plugin               * plugin;

	tnrc_port_id_t         dl_id_in;
	label_t                labelid_in;
	tnrc_port_id_t         dl_id_out;
	label_t                labelid_out;
	xcdirection_t          dir;
	tnrc_boolean_t         is_virtual;

	tnrc_boolean_t         advance_rsrv;
	long                   start;
	long                   end;

	long                   resp_ctxt;

} reserve_xc_param_t;

typedef destroy_xc_param_t unreserve_xc_param_t;

class Action {
 protected:
	tnrcap_cookie_t            ap_cookie_;

	tnrcsp_handle_t            sp_handle_;

	long                       resp_ctxt_;

	long                       async_ctxt_;

	tnrc_action_type_t         action_type_;

	tnrcsp_result_t            prel_check_;

	tnrcsp_result_t            eqpt_resp_;

	bool                       have_atomic_;

	bool                       is_virtual_;

	bool                       have_atomic_todo_;

	bool                       wait_answer_;

	int                        n_retry_;

	Plugin                   * plugin_;

	Action                   * atomic_; //atomic action in execution

	std::deque<Action *>       atomic_actions_;

	std::deque<Action *>       atomic_todo_;

	std::deque<Action *>       atomic_done_;

 public:
	Action (void) {};
	~Action (void){};

	Plugin * plugin();

	Action * atomic();
	void     atomic(Action * at);

	tnrcap_cookie_t ap_cookie(void);

	void            sp_handle(tnrcsp_handle_t h);
	tnrcsp_handle_t sp_handle(void);

	long   resp_ctxt(void);
	void   resp_ctxt(long ctxt);

	long   async_ctxt(void);

	tnrc_action_type_t action_type(void);
	void               action_type(tnrc_action_type_t type);

	void              prel_check(tnrcsp_result_t pc);
	tnrcsp_result_t   prel_check(void);

	void            eqpt_resp(tnrcsp_result_t res);
	tnrcsp_result_t eqpt_resp(void);

	void have_atomic(bool atomic);
	bool have_atomic(void);

	void is_virtual(bool is_virt);
	bool is_virtual(void);

	bool have_atomic_todo(void);
	bool have_atomic_todestroy(void);

	bool wait_answer(void);
	void wait_answer(bool val);

	//atomic actions to do management
	void     pop_todo();
	Action * front_todo(void);
	void     push_todo(Action * at);
	int      todo_size(void);

	//atomic actions done management
	void     pop_done();
	Action * front_done(void);
	void     push_done(Action * at);
	int      done_size(void);
	void     swap_action_type(void);

	int  n_retry(void);
	void n_retry_inc();

	virtual void fsm_start(void) = 0;
	virtual void fsm_post(fsm::TNRC::virtFsm::root_events_t ev,
			      void *                            ctxt,
			      bool                              queue = false)
		= 0;

	//define iterator_atomic_actions
	DEFINE_QUEUE_ITERATOR(atomic_actions, Action);
	//define iterator_atomic_done
	DEFINE_QUEUE_ITERATOR(atomic_done, Action);
};

class MakeDestroyxc : public Action {
 private:

	u_int               xc_id_;

	tnrc_port_id_t      portid_in_;

	label_t             labelid_in_;

	tnrc_port_id_t      portid_out_;

	label_t             labelid_out_;

	xcdirection_t       direction_;

	bool                advance_rsrv_;

	long                start_;

	long                end_;

	tnrc_rsrv_xc_flag_t rsrv_xc_flag_;

	tnrcap_cookie_t     rsrv_cookie_;

	struct thread *     thread_end_;

	// Action FSM instance
	fsm::TNRC::virtFsm  *FSM_;

 public:

	MakeDestroyxc(void) {};
	~MakeDestroyxc(void);
	MakeDestroyxc(tnrcap_cookie_t          ck,
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
		      Plugin *                 p);

	u_int xc_id (void);
	void  xc_id (u_int id);

	tnrc_port_id_t portid_in(void);
	tnrc_port_id_t portid_out(void);

	label_t labelid_in(void);
	label_t labelid_out(void);

	xcdirection_t direction(void);

        bool           advance_rsrv(void);
	long           start(void);
        long           end(void);
	void           cancel_advance_rsrv(void);

	tnrc_rsrv_xc_flag_t rsrv_xc_flag(void);
	void                rsrv_xc_flag(tnrc_rsrv_xc_flag_t flag);

	tnrcap_cookie_t rsrv_cookie(void);
	void            rsrv_cookie(tnrcap_cookie_t cookie);

	bool check_rsrv(tnrcap_cookie_t ck);
	bool check_resources(void);

	void thread_end(struct thread *thr);

	void fsm_start(void);
	void fsm_post (fsm::TNRC::virtFsm::root_events_t ev,
		       void *                            ctxt,
		       bool                              queue = false);

};

class ReserveUnreservexc : public Action {
 private:

	u_int              xc_id_;

	tnrc_port_id_t     portid_in_;

	label_t            labelid_in_;

	tnrc_port_id_t     portid_out_;

	label_t            labelid_out_;

	xcdirection_t      direction_;

	bool               advance_rsrv_;

	long               start_;

	long               end_;

	bool               waiting_to_start_;

	bool               deactivation_;

	struct thread *    thread_start_;

	// Action FSM instance
	fsm::TNRC::virtFsm *FSM_;

 public:

	ReserveUnreservexc(void) {};
	~ReserveUnreservexc(void);
	ReserveUnreservexc(tnrcap_cookie_t      ck,
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
			   Plugin *             p);

	u_int xc_id(void);
	void  xc_id(u_int id);

	tnrc_port_id_t portid_in(void);
	tnrc_port_id_t portid_out(void);

	label_t labelid_in(void);
	label_t labelid_out(void);

	xcdirection_t direction(void);

        bool           advance_rsrv(void);
	long           start(void);
	long           end(void);
	bool           waiting_to_start(void);
	void           waiting_to_start(bool wait);
	void           cancel_advance_rsrv(void);

	bool deactivation(void);

	bool check_resources(void);

	void thread_start(struct thread *thr);

	void fsm_start(void);
	void fsm_post(fsm::TNRC::virtFsm::root_events_t ev,
		      void *                            ctxt,
		      bool                              queue = false);

};

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
		    long                     async_ctxt);
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
		       long                 response_ctxt);
destroy_xc_param_t *
new_destroy_xc_element(tnrcap_cookie_t      cookie,
		       tnrc_boolean_t       deactivate,
		       long                 response_ctxt);
unreserve_xc_param_t *
new_unreserve_xc_element(tnrcap_cookie_t      cookie,
			 long                 response_ctxt);

extern "C" {

int delete_action         (struct thread *t);
int deactivate_action     (struct thread *t);
int timer_expired         (struct thread *t);
int start_advance_rsrv    (struct thread *t);
int stop_advance_rsrv     (struct thread *t);
};

#endif /* _TNRC_ACTION_OBJECTS_H */
