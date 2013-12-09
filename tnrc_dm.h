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



#ifndef TNRC_DM_H
#define TNRC_DM_H

#include <list>
#include <map>
#include <iterator>
#include <string>
#include <assert.h>

#include "tnrc_common_types.h"

#define CHECK_DL_STATE(E, B, P)				\
  ((((E)->opstate()  == DOWN     ) ||			\
    ((E)->admstate() == DISABLED ) ||			\
    ((B)->opstate()  == DOWN     ) ||			\
    ((B)->admstate() == DISABLED ) ||			\
    ((P)->opstate()  == DOWN     ) ||			\
    ((P)->admstate() == DISABLED )) ? false : true)

#define CHECK_RESOURCE_STATE(R)				\
  ((((R)->opstate()  == DOWN     ) ||			\
    ((R)->admstate() == DISABLED )) ? false : true)

#define CHECK_RESOURCE_FREE(R)			\
  (((R)->state() == LABEL_FREE) ? true : false)

#define CHECK_RESOURCE_BOOKED(R)			\
  ((((R)->state() == LABEL_FREE) ||                     \
    ((R)->state() == LABEL_BOOKED)) ? true : false)

#define _DEFINE_DM_MAP_ITERATOR(TD,T1,T2,P,V)		\
	typedef std::map<T1, T2>::iterator TD;		\
	TD begin##P(void) { return V.begin(); };	\
	TD end##P(void)   { return V.end();   };

#define _DEFINE_DM_MAP_CONST_ITERATOR(TD,T1,T2,P,V)	\
	typedef std::map<T1, T2>::const_iterator TD;	\
	TD begin##P(void) const { return V.begin(); };	\
	TD end##P(void)   const { return V.end();   };

#define DEFINE_DM_MAP_ITERATOR(N,T)					      \
	_DEFINE_DM_MAP_ITERATOR(iterator_##N, T##Key_t, T *, _##N, N##_);     \
	_DEFINE_DM_MAP_CONST_ITERATOR(const_iterator_##N, T##Key_t,           \
				      T *, _##N, N##_);

#define _DEFINE_DM_MAP_COMP_ITERATOR(TD,T1,T2,C,P,V)	\
	typedef std::map<T1, T2, C>::iterator TD;	\
	TD begin##P(void) { return V.begin(); };	\
	TD end##P(void)   { return V.end();   };

#define DEFINE_DM_MAP_COMP_ITERATOR(N,T,C)			     \
	_DEFINE_DM_MAP_COMP_ITERATOR(iterator_##N, T##Key_t,         \
				     T *, C, _##N, N##_);

namespace TNRC {
	// Forward decls
	//
	class TNRC_AP;
	class Eqpt;
	class Board;
	class Port;
	class Resource;
	// Real decls

	typedef uint8_t  EqptKey_t;
	typedef uint16_t BoardKey_t;
	typedef uint16_t PortKey_t;
	typedef label_t  ResourceKey_t;

	class TNRC_AP {
	 public:
		TNRC_AP(void);
		~TNRC_AP(void);

		u_int      instance_id(void);

		bool       attach(EqptKey_t k, Eqpt * e);
		bool       detach(EqptKey_t k);

		Eqpt     * getEqpt(EqptKey_t    k);
		Board    * getBoard(EqptKey_t  e_id,
				    BoardKey_t b_id);
		Port     * getPort(EqptKey_t  e_id,
				   BoardKey_t b_id,
				   PortKey_t  p_id);
		Resource * getResource(EqptKey_t     e_id,
				       BoardKey_t    b_id,
				       PortKey_t     p_id,
				       ResourceKey_t l_id);

		int        n_eqpts(void);

		void       dwdm_lambdas_bitmaps_init(void);

		bool       eqpt_link_down(void);
		void       eqpt_link_down(bool val);

		// Defines eqpts_iterator
		DEFINE_DM_MAP_ITERATOR(eqpts, Eqpt);

	 private:

		// ID of this instance of tnrc module
		uint32_t tnrc_instance_id_;
		// retransmit interval
		uint32_t dflt_RetransmitInterval_;
		// when tnrc was initialized
		time_t   tnrc_start_time_;
		// Current time of day (in seconds)
		time_t   current_time_;
		// time when current stats were reset
		time_t   stats_reset_time_;
		// A "nice" shutdown delay
		time_t   shutdown_delay_;
		// Flag active if equipment link is down
		bool     eqpt_link_down_;
		// map of eqpts
		std::map<EqptKey_t, Eqpt *> eqpts_;
	};

	class Eqpt {
	 public:
		Eqpt(void);
		~Eqpt(void);
		Eqpt(TNRC_AP *     tnrc,
		     eqpt_id_t     id,
		     g2mpls_addr_t addr,
		     eqpt_type_t   t,
		     opstate_t     opst,
		     admstate_t    admst,
		     std::string   loc);

		bool            attach(BoardKey_t k, Board * b);
		bool            detach(BoardKey_t k);

		Board         * getBoard(BoardKey_t k);

		eqpt_id_t       eqpt_id(void);
		g2mpls_addr_t   address(void);
		eqpt_type_t     type(void);
		opstate_t       opstate(void);
		void            opstate(opstate_t st);
		admstate_t      admstate(void);
		void            admstate(admstate_t st);
		const char    * location(void);

		int             n_boards(void);

		// Defines boards_iterator
		DEFINE_DM_MAP_ITERATOR(boards, Board);

	 private:
		// tnrc parent instance
		TNRC_AP       * tnrc_ap_;
		//Eqpt Address
		g2mpls_addr_t   address_;
		//Eqpt Id
		eqpt_id_t       eqpt_id_;
		//Eqpt Type
		eqpt_type_t     type_;
		//Operational state
		opstate_t       opstate_;
		//Administrative state
		admstate_t      admstate_;
		// (String_type*)
		std::string     location_name_;
		// Map of Boards for this NE
		std::map<BoardKey_t, Board *> boards_;
	};

	class Board {
	 public:
		Board(void);
		~Board(void);
		Board(Eqpt *     e,
		      board_id_t id,
		      sw_cap_t   sw_cap,
		      enc_type_t enc_type,
		      opstate_t  opst,
		      admstate_t admst);

		Eqpt       * eqpt();

		bool         attach(PortKey_t k, Port * p);
		bool         detach(PortKey_t k);

		Port*        getPort (PortKey_t k);

		board_id_t   board_id(void);
		sw_cap_t     sw_cap(void);
		enc_type_t   enc_type(void);
		opstate_t    opstate(void);
		void         opstate(opstate_t st);
		admstate_t   admstate(void);
		void         admstate(admstate_t st);

		int          n_ports(void);

		// Defines ports_iterator
		DEFINE_DM_MAP_ITERATOR(ports, Port);

	 private:
		// eqpt parent instance
		Eqpt       * eqpt_;
		// ID of the board */
		board_id_t   board_id_;
		// Switching cap
		sw_cap_t     sw_cap_;
		// Encoding type
		enc_type_t   enc_type_;
		// Operational state
		opstate_t    opstate_;
		// Administrative state
		admstate_t   admstate_;
		// Map of Ports for this Board
		std::map<PortKey_t, Port *> ports_;
	};

	class myCompareResource {
	  public:
		bool operator() (const label_t& first, const label_t& second);
	};

	class Port {
	 public:
		Port(void);
		~Port(void);
		Port(Board *          b,
		     port_id_t        id,
		     int              flags,
		     g2mpls_addr_t    rem_eq_addr,
		     port_id_t        rem_port_id,
		     opstate_t        opst,
		     admstate_t       admst,
		     uint32_t         dwdm_lambda_base,
		     uint16_t         dwdm_lambda_count,
		     uint32_t         bandwidth,
		     gmpls_prottype_t protection);

		Board             * board();

		bool                attach(ResourceKey_t k, Resource * r);
		bool                detach(ResourceKey_t k);

		Resource          * getResource(ResourceKey_t k);

		port_id_t           port_id (void);
		int                 port_flags (void);
		g2mpls_addr_t       remote_eqpt_address (void);
		port_id_t           remote_port_id (void);
		opstate_t           opstate (void);
		void                opstate(opstate_t st);
		admstate_t          admstate(void);
		void                admstate(admstate_t st);
		uint32_t            max_bw(void);
		uint32_t            max_res_bw(void);
		bw_per_prio_t       unres_bw(void);
		uint32_t            min_lsp_bw(void);
		bw_per_prio_t       max_lsp_bw(void);
		void                upd_unres_bw(void);
		gmpls_prottype_t    prot_type(void);
		wdm_link_lambdas_bitmap_t dwdm_lambdas_bitmap();

		void                upd_parameters(label_t label_id);
		void                upd_dwdm_lambdas_bitmap(label_t label_id);

		void                dwdm_lambdas_bitmap_init(void);

		void                get_calendar_map(uint32_t size,
						     long     start_time,
						     long     time_quantum,
						     std::map<long,
						     bw_per_prio_t> &
						     calendar);

		int                 n_resources (void);

		// Defines resources_iterator
		DEFINE_DM_MAP_COMP_ITERATOR(resources,
					    Resource,
					    myCompareResource);
		//DEFINE_DM_MAP_ITERATOR(resources, Resource);

	 private:
		// build transitions map of advance reservations
		void                get_transition_map(std::map<long, int> &
						       transitions);
		// put an event in calendar
		void                insert_calendar_event(std::map<long,
							  bw_per_prio_t> &
							  calendar,
							  long time,
							  int  free_labels);
		// set/clear bit in the wavelength bitmask
		void                set_bit(uint32_t & word, uint32_t index);
		void                clear_bit(uint32_t & word, uint32_t index);
		// parent board instance
		Board             * board_;
		// port identifier
		port_id_t           port_id_;
		// bit mask describing the port behaviour
		int                 port_flags_;
		// IP address of the remote eqpt
		g2mpls_addr_t       remote_eqpt_address_;
		// id of the remote port
		port_id_t           remote_port_id_;
		// Operational state
		opstate_t           opstate_;
		// Administrative state
		admstate_t          admstate_;
		// DWDM lambdas bitmap
		wdm_link_lambdas_bitmap_t dwdm_lambdas_bitmap_;
		// Protection type
		gmpls_prottype_t    prot_type_;
		//OSNR
		//uint32_t          osnr_;
		// Total available bandwidth
		uint32_t            max_bw_;
		//Max reservable bandwidth
		uint32_t            max_res_bw_;
		//Unreserved  bandwidth per priority
		bw_per_prio_t       unres_bw_;
		// Minimum reservable bandiwdth
		uint32_t            min_lsp_bw_;
		// Maximum reservable bandiwdth per priority
		bw_per_prio_t       max_lsp_bw_;
		// Map of Resources for this Port
		std::map<ResourceKey_t,
			Resource *,
			myCompareResource> resources_;
	};

	class Resource {
	 public:
		Resource(void);
		~Resource(void);
		Resource(Port *        p,
			 int           tp_fl,
			 label_t       id,
			 opstate_t     opst,
			 admstate_t    admst,
			 label_state_t st);

		bool            attach(long start, long end);
		bool            detach(long start);
		bool            check_label_availability(long start, long end);
		void            upd_transition_map(std::map<long, int> &
						   transitions);

		Port          * port();
		int             tp_flags(void);
		label_t         label_id(void);
		opstate_t       opstate(void);
		void            opstate(opstate_t st);
		admstate_t      admstate(void);
		void            admstate(admstate_t st);
		label_state_t   state(void);
		void            state(label_state_t st);

		typedef std::map<long, long>::iterator iterator_reservations;
		iterator_reservations begin_reservations(void) {
			return reservations_.begin();
		};
		iterator_reservations end_reservations(void)   {
			return reservations_.end();
		};
		int             n_reservations(void);

	 private:
		// Parent port pointer
		Port          * port_;
		// Flag indicating the TP state
		int             tp_flags_;
		// Label identifier
		label_t         label_id_;
		// Operational state
		opstate_t       opstate_;
		// Administrative state
		admstate_t      admstate_;
		// Label state
		label_state_t   state_;
		// Advanced Reservation Calendar
		std::map<long, long> reservations_;
	};
};

#endif /* TNRC_DM_H */
