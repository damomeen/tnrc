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




#ifndef TNRCD_H
#define TNRCD_H

#include <list>
#include <queue>
#include <assert.h>
#include "thread.h"

#include "tnrc_dm.h"
#include "tnrc_plugin.h"
#include "tnrcd/tnrc_action_objects.h"
#include "tnrcd/tnrc_apis.h"

//Maximum number of elements in API Queue
#define TNRC_MAX_API_QUEUE_SIZE         50

#define SHOW_XC_STATE(X)                \
  (((X) == XCONN   ) ? "XCONNECTED"  :	\
  (((X) == RESERVED) ? "RESERVED"    :	\
                       "==UNKNOWN=="))

#define _DEFINE_MASTER_MAP_ITERATOR(TD,T1,T2,P,V)	\
	typedef std::map<T1, T2>::iterator TD;		\
	TD begin##P(void) { return V.begin(); };	\
	TD end##P(void)   { return V.end();   };

#define DEFINE_MASTER_MAP_ITERATOR(N,T1,T)                             \
	_DEFINE_MASTER_MAP_ITERATOR(iterator_##N, T1, T *, _##N, N##_);

typedef enum {
	XCONN = 0,
	RESERVED
} tnrcap_xc_state_t;

class XC {
 public:
	XC(void) {};
	~XC(void){};
	XC(u_int                    id,
	   tnrcap_cookie_t          ck,
	   tnrcap_xc_state_t        st,
	   tnrc_port_id_t           portid_in,
	   label_t                  labelid_in,
	   tnrc_port_id_t           portid_out,
	   label_t                  labelid_out,
	   xcdirection_t            direction,
	   long                     ctxt);

	u_int             id(void);

	tnrcap_cookie_t   cookie(void);
	void              cookie(tnrcap_cookie_t ck);

	tnrcap_xc_state_t state(void);
	void              state(tnrcap_xc_state_t st);

	tnrc_port_id_t    portid_in(void);
	label_t           labelid_in(void);

	tnrc_port_id_t    portid_out(void);
	label_t           labelid_out(void);

	xcdirection_t     direction(void);

	long              async_ctxt (void);
	void              async_ctxt (long ctxt);

 private:
	u_int             id_;

	tnrcap_cookie_t   cookie_;

	tnrcap_xc_state_t state_;

	tnrc_port_id_t    portid_in_;
	label_t           labelid_in_;

	tnrc_port_id_t    portid_out_;
	label_t           labelid_out_;

	xcdirection_t     direction_;

	long              async_ctxt_;
};

class ApiQueue {
 public:
	ApiQueue(void);
	~ApiQueue(void) {};
	bool                  insert(api_queue_element_t * e);
	api_queue_element_t * extract(void);
	int                   size(void);
	u_int                 tot_request (void);
 private:
	// total (progressive) number of action requests
	u_int                 tot_req_;
	// queue of actions to execute
	std::queue<api_queue_element_t *> queue_;
};

// TNRC  master for system wide configuration
class TNRC_Master {
 public:
	static bool            init(void);

	static bool            destroy(void);

	static TNRC_Master   & instance(void);

	void                   init_vty(void);

	void                   pc(Pcontainer * PC);

	Pcontainer           * getPC();

	struct thread_master * getMaster();

	bool                   test_mode(void);

	void                   test_mode(bool val);

	char*                  test_file(void);

	void                   test_file(std::string loc);

	static TNRC::TNRC_AP * getTNRC();

	u_int                  n_instances();

	tnrcap_cookie_t        new_cookie();

	uint32_t               new_xc_id();

	eqpt_type_t            getEqpt_type(eqpt_id_t id);

	static bool            attach_instance(TNRC::TNRC_AP * t);

	static bool            detach_instance(TNRC::TNRC_AP * t);

	Plugin*                getPlugin(void);

	void                   installPlugin(Plugin * p);

	std::string            plugin_location(void);

	void                   plugin_location(std::string loc);

	bool                   api_queue_insert(api_queue_element_t * e);

	api_queue_element_t  * api_queue_extract(void);

	int                    api_queue_size(void);

	void                   api_queue_process(void);

	void                   process_make_xc(api_queue_element_t * el);

	void                   process_destroy_xc(api_queue_element_t * el);

	void                   process_reserve_xc(api_queue_element_t * el);

	void                   process_unreserve_xc(api_queue_element_t * el);

	u_int                  api_queue_tot_request(void);

	bool                   attach_action(tnrcap_cookie_t ck, Action * a);

	bool                   detach_action(tnrcap_cookie_t ck);

	Action               * getAction (tnrcap_cookie_t ck);

	bool                   attach_xc(u_int id, XC * xc);

	bool                   detach_xc(u_int id);

	XC                   * getXC (u_int id);

	int                    n_xcs (void);

	// define iterator_actions
	DEFINE_MASTER_MAP_ITERATOR (actions, tnrcap_cookie_t, Action);
	// define iterator_xcs
	DEFINE_MASTER_MAP_ITERATOR (xcs, u_int, XC);

 protected:
	TNRC_Master& operator=(const TNRC_Master& j);
	TNRC_Master(const TNRC_Master & j);
	TNRC_Master(void);
	~TNRC_Master(void);

 private:
	static TNRC_Master                  * instance_;
	//value for the next cookie
	static tnrcap_cookie_t                cookie_;
	//value for the next Xc id
	static uint32_t                       xc_id_;
	// thread master
	static struct thread_master         * master_;
	//test mode
	bool                                  test_mode_;
	//test file path
	std::string                           test_file_;
	// instances list
	static std::list<TNRC::TNRC_AP *>     tnrcs_;
	// pointer to Plugins container
	Pcontainer                          * PC_;
	// pointer to installed Plugin
	Plugin                              * plugin_;
	// location string of installed Plugin
	std::string                           plugin_location_;
	// queue of operation to execute
	ApiQueue                              api_aq_;
	// actions in execution or executed
	std::map<tnrcap_cookie_t, Action *>   actions_;
	// XCs active or reserved
	std::map<u_int, XC *>                 xcs_;
	// start time
	static time_t                         start_time_;
};

#define TNRC_MASTER    (TNRC_Master::instance())

extern "C" {
int process_api_queue(struct thread *t);
}

//VERY TEMP !!!
extern "C" {
int read_test_file(struct thread *t);
}

#endif /* TNRCD_H */
