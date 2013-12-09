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

#include "tnrcd.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"

//
// Data model implementation
//

namespace TNRC {
	////////////////////////////
	// Class TNRC
	///////////////////////////
	TNRC_AP::TNRC_AP(void)
	{
		METIN();
		tnrc_instance_id_ = TNRC_MASTER.n_instances();
		tnrc_start_time_  = quagga_time(NULL);
		current_time_     = tnrc_start_time_;
		stats_reset_time_ = tnrc_start_time_;
		eqpt_link_down_   = false;
		METOUT();
	}

	TNRC_AP::~TNRC_AP(void)
	{
		iterator_eqpts iter;
		TNRC_DBG("Called TNRC destructor");
		METIN();
		//delete all "Eqpt" objects for the instance
		for(iter = begin_eqpts(); iter != end_eqpts(); iter++) {
			Eqpt * e;

			e = (*iter).second;
			delete e;
		}
		METOUT();
	}

	uint32_t
	TNRC_AP::instance_id(void)
	{
		return tnrc_instance_id_;
	}

	bool
	TNRC_AP::attach(EqptKey_t k, Eqpt * e)
	{
		iterator_eqpts                  iter;
		std::pair<iterator_eqpts, bool> ret;

		//try to insert the Eqpt object
		ret = eqpts_.insert(std::pair<EqptKey_t, Eqpt *> (k, e));
		if (ret.second == false) {
			TNRC_WRN ("Equipment with key %d is already "
				  "registered", k);
			return false;
		}

		return true;
	}

	bool
	TNRC_AP::detach(EqptKey_t k)
	{
		iterator_eqpts iter;

		iter = eqpts_.find(k);
		if (iter == end_eqpts()) {
			TNRC_WRN ("Equipment with key %d is not registered", k);
			return false;
		}
		eqpts_.erase (iter);

		return true;
	}

	Eqpt *
	TNRC_AP::getEqpt(EqptKey_t k)
	{
		iterator_eqpts iter;

		iter = eqpts_.find(k);
		if (iter == end_eqpts()) {
			TNRC_WRN ("Equipment with key %d is not registered", k);
			return NULL;
		}

		return (*iter).second;
	}

	Board *
	TNRC_AP::getBoard(EqptKey_t e_id, BoardKey_t b_id)
	{
		Eqpt  * e;
		Board * b;

		e = getEqpt(e_id);
		if (e == NULL) {
			return NULL;
		}
		b = e->getBoard(b_id);

		return b;
	}

	Port *
	TNRC_AP::getPort(EqptKey_t e_id, BoardKey_t b_id, PortKey_t p_id)
	{
		Board * b;
		Port  * p;

		b = getBoard(e_id, b_id);
		if (b == NULL) {
			return NULL;
		}
		p = b->getPort(p_id);

		return p;
	}

	Resource *
	TNRC_AP::getResource(EqptKey_t     e_id,
			     BoardKey_t    b_id,
			     PortKey_t     p_id,
			     ResourceKey_t l_id)
	{
		Port     * p;
		Resource * r;

		p = getPort(e_id, b_id, p_id);
		if (p == NULL) {
			return NULL;
		}
		r = p->getResource(l_id);

		return r;
	}

	int
	TNRC_AP::n_eqpts(void)
	{
		return (int) eqpts_.size();
	}

	void
	TNRC_AP::dwdm_lambdas_bitmaps_init()
	{
		iterator_eqpts        iter_e;
		Eqpt::iterator_boards iter_b;
		Board::iterator_ports iter_p;
		Eqpt *                e;
		Board *               b;
		Port *                p;
		for (iter_e  = begin_eqpts();
		     iter_e != end_eqpts();
		     iter_e++) {
			e = (*iter_e).second;
			for (iter_b  = e->begin_boards();
			     iter_b != e->end_boards();
			     iter_b++) {
				b = (*iter_b).second;
				if (b->sw_cap() == SWCAP_LSC) {
					for (iter_p  = b->begin_ports();
					     iter_p != b->end_ports();
					     iter_p++) {
						p = (*iter_p).second;
						p->dwdm_lambdas_bitmap_init();
					}
				}
			}
		}
	}

	bool
	TNRC_AP::eqpt_link_down(void)
	{
		return eqpt_link_down_;
	}

	void
	TNRC_AP::eqpt_link_down(bool val)
	{
		eqpt_link_down_ = val;
	}

	////////////////////////////
	// Class Eqpt
	///////////////////////////

	Eqpt::Eqpt(void)
	{
		METIN();

		METOUT();
	}

	Eqpt::~Eqpt(void)
	{
		iterator_boards iter;
		TNRC_DBG ("Called Eqpt destructor");
		METIN();
		//delete all "Board" objects for this Eqpt
		for(iter = begin_boards(); iter != end_boards(); iter++) {
			Board * b;

			b = (*iter).second;
			delete b;
		}
		METOUT();
	}

	Eqpt::Eqpt(TNRC_AP *     tnrc,
		   eqpt_id_t     id,
		   g2mpls_addr_t addr,
		   eqpt_type_t   t,
		   opstate_t     opst,
		   admstate_t    admst,
		   std::string   loc)
	{
		tnrc_ap_       = tnrc;
		eqpt_id_       = id;
		address_       = addr;
		type_          = t;
		opstate_       = opst;
		admstate_      = admst;
		location_name_ = loc;
	}

	bool
	Eqpt::attach(BoardKey_t k, Board * b)
	{
		iterator_boards                  iter;
		std::pair<iterator_boards, bool> ret;

		//try to insert the Board object
		ret = boards_.insert(std::pair<BoardKey_t, Board *> (k, b));
		if (ret.second == false) {
			TNRC_WRN("Board with key %d is already registered "
				 "on the equipment", k);
			return false;
		}

		return true;
	}

	bool
	Eqpt::detach(BoardKey_t k)
	{
		iterator_boards iter;

		iter = boards_.find(k);
		if (iter == end_boards()) {
			TNRC_WRN("Board with key %d is not registered "
				 "on the equipment", k);
			return false;
		}
		boards_.erase(iter);

		return true;
	}

	Board*
	Eqpt::getBoard(BoardKey_t k)
	{
		iterator_boards iter;

		iter = boards_.find(k);
		if (iter == end_boards()) {
			TNRC_WRN ("Board with key %d is not registered "
				  "on the equipment", k);
			return NULL;
		}

		return (*iter).second;
	}

	eqpt_id_t
	Eqpt::eqpt_id(void)
	{
		return eqpt_id_;
	}

	g2mpls_addr_t
	Eqpt::address(void)
	{
		return address_;
	}

	eqpt_type_t
	Eqpt::type(void)
	{
		return type_;
	}

	opstate_t
	Eqpt::opstate(void)
	{
		return opstate_;
	}

	void
	Eqpt::opstate(opstate_t opst)
	{
		opstate_ = opst;
	}

	admstate_t
	Eqpt::admstate(void)
	{
		return admstate_;
	}

	void
	Eqpt::admstate(admstate_t admst)
	{
		admstate_ = admst;
	}

	const char*
	Eqpt::location(void)
	{
		return location_name_.c_str();
	}

	int
	Eqpt::n_boards(void)
	{
		return (int) boards_.size();
	}

	////////////////////////////
	// Class Board
	///////////////////////////

	Board::Board(void)
	{
		METIN();

		METOUT();
	}

	Board::~Board(void)
	{
		iterator_ports iter;

		TNRC_DBG("Called Board destructor");
		METIN();
		//delete all "Port" objects for this Port
		for(iter = begin_ports(); iter != end_ports(); iter++) {
			Port *p;

			p = (*iter).second;
			delete p;
		}
		METOUT();
	}

	Board::Board(Eqpt *     e,
		     board_id_t id,
		     sw_cap_t   sw_cap,
		     enc_type_t enc_type,
		     opstate_t  opst,
		     admstate_t admst)
	{
		eqpt_     = e;
		board_id_ = id;
		sw_cap_   = sw_cap;
		enc_type_ = enc_type;
		opstate_  = opst;
		admstate_ = admst;
	}

	Eqpt *
	Board::eqpt()
	{
		return eqpt_;
	}

	bool
	Board::attach(PortKey_t k, Port * p)
	{
		iterator_ports iter;
		std::pair<iterator_ports, bool> ret;

		//try to insert the Port object
		ret = ports_.insert (std::pair<PortKey_t, Port *> (k, p));
		if (ret.second == false) {
			TNRC_WRN ("Port with key %d is already registered "
				  "on the board %d", k, board_id_);
			return false;
		}

		return true;
	}

	bool
	Board::detach(PortKey_t k)
	{
		iterator_ports iter;

		iter = ports_.find(k);
		if (iter == end_ports()) {
			TNRC_WRN ("Port with key %d is not registered on "
				  "the board %d", k, board_id_);
			return false;
		}
		ports_.erase(iter);

		return true;
	}

	Port*
	Board::getPort(PortKey_t k)
	{
		iterator_ports iter;

		iter = ports_.find(k);
		if (iter == end_ports()) {
			TNRC_WRN ("Port with key %d is not registered on "
				  "the board %d", k, board_id_);
			return NULL;
		}

		return (*iter).second;
	}

	board_id_t
	Board::board_id(void)
	{
		return board_id_;
	}

	sw_cap_t
	Board::sw_cap(void)
	{
		return sw_cap_;
	}

	enc_type_t
	Board::enc_type(void)
	{
		return enc_type_;
	}

	opstate_t
	Board::opstate(void)
	{
		return opstate_;
	}

	void
	Board::opstate(opstate_t st)
	{
		opstate_ = st;
	}

	admstate_t
	Board::admstate(void)
	{
		return admstate_;
	}

	void
	Board::admstate(admstate_t st)
	{
		admstate_ = st;
	}

	int
	Board::n_ports(void)
	{
		return (int) ports_.size();
	}

	////////////////////////////
	// Class myCompareResource
	///////////////////////////

	bool
	myCompareResource::operator() (const label_t& first,
				       const label_t& second)
	{
		switch (first.type) {
			case LABEL_FSC:
				return (first.value.fsc.portId <
					second.value.fsc.portId);
				break;
			case LABEL_PSC:
				return (first.value.psc.id <
					second.value.psc.id);
				break;
			case LABEL_L2SC:
				return (first.value.l2sc.vlanId <
					second.value.l2sc.vlanId);
				break;
			case LABEL_LSC:
				return (first.value.lsc.wavelen <
					second.value.lsc.wavelen);
				break;
			case LABEL_TDM:
				return (first.value.tdm.s <
					second.value.tdm.s);
				break;
			case LABEL_UNSPECIFIED:
#ifdef UBUNTU_OPERATOR_PATCH
				return (first.value.lsc.wavelen <
					second.value.lsc.wavelen);
#else
				return (first.value.rawId <
					second.value.rawId);
#endif
				break;
		}
	}

	////////////////////////////
	// Class Port
	///////////////////////////

	Port::Port(void)
	{
		METIN();

		METOUT();
	}

	Port::~Port(void)
	{
		iterator_resources iter;

		TNRC_DBG ("Called Port destructor");
		METIN();
		//delete all "Resource" objects for this Port
		for(iter = begin_resources(); iter != end_resources(); iter++){
			Resource * r;

			r = (*iter).second;
			delete r;
		}
		METOUT();
	}

	Port::Port(Board *          b,
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
		float    bwFl;
		uint32_t bw;
		uint32_t codedBw;

		board_               = b;
		port_id_             = id;
		port_flags_          = flags;
		remote_eqpt_address_ = rem_eq_addr;
		remote_port_id_      = rem_port_id;
		opstate_             = opst;
		admstate_            = admst;

		memset(&dwdm_lambdas_bitmap_, 0, sizeof(dwdm_lambdas_bitmap_));

		bw = bandwidth;
		switch (b->sw_cap()) {
			case SWCAP_FSC:
				min_lsp_bw_ = bw;
				break;
			case SWCAP_LSC:
				codedBw = LSC_BASIC_BW_UNIT;
				bwFl = (BW_HEX2BPS(codedBw) * (float) dwdm_lambda_count);
				if (bwFl != BW_HEX2BPS(bandwidth)) {
					bw = BW_BPS2HEX(bwFl);
				}
				min_lsp_bw_ = LSC_BASIC_BW_UNIT;
				dwdm_lambdas_bitmap_.base_lambda_label =
					dwdm_lambda_base;
				dwdm_lambdas_bitmap_.num_wavelengths =
					dwdm_lambda_count;
				dwdm_lambdas_bitmap_init();
				break;
			default:
				break;
		}

		max_bw_              = bw;
		max_res_bw_          = max_bw_;
		unres_bw_.bw[0]      = 0;
		for(int i = 1; i < GMPLS_BW_PRIORITIES; i++) {
			unres_bw_.bw[i] = max_res_bw_;
		}
		max_lsp_bw_          = unres_bw_;

		prot_type_           = protection;

	}

	Board *
	Port::board()
	{
		return board_;
	}

	bool
	Port::attach(ResourceKey_t k, Resource * r)
	{
		iterator_resources iter;
		std::pair<iterator_resources, bool> ret;

		//try to insert the Resource object
		ret = resources_.insert(std::pair<ResourceKey_t, Resource *>
					(k, r));
		if (ret.second == false) {
			TNRC_WRN("Resource with specified key is already "
				 "associated to port %d", port_id_);
			return false;
		}

		return true;
	}

	bool
	Port::detach(ResourceKey_t k)
	{
		iterator_resources iter;

		iter = resources_.find(k);
		if (iter == end_resources()) {
			TNRC_WRN ("Resource with specified key is not "
				  "associated to port %d", port_id_);
			return false;
		}
		resources_.erase(iter);

		return true;
	}

	Resource *
	Port::getResource(ResourceKey_t k)
	{
		iterator_resources iter;

		iter = resources_.find(k);
		if (iter == end_resources()) {
			TNRC_WRN ("Resource with specified key is not "
				  "associated to port %d", port_id_);
			return NULL;
		}
		return (*iter).second;

	}

	port_id_t
	Port::port_id(void)
	{
		return port_id_;
	}

	int
	Port::port_flags(void)
	{
		return port_flags_;
	}

	g2mpls_addr_t
	Port::remote_eqpt_address(void)
	{
		return remote_eqpt_address_;
	}

	port_id_t
	Port::remote_port_id(void)
	{
		return remote_port_id_;
	}

	opstate_t
	Port::opstate(void)
	{
		return opstate_;
	}

	void
	Port::opstate(opstate_t st)
	{
		opstate_ = st;
	}

	admstate_t
	Port::admstate(void)
	{
		return admstate_;
	}

	void
	Port::admstate(admstate_t st)
	{
		admstate_ = st;
	}

	uint32_t
	Port::max_bw(void)
	{
		return max_bw_;
	}

	uint32_t
	Port::max_res_bw(void)
	{
		return max_res_bw_;
	}

	bw_per_prio_t
	Port::unres_bw(void)
	{
		return unres_bw_;
	}

	uint32_t
	Port::min_lsp_bw(void)
	{
		return min_lsp_bw_;
	}

	bw_per_prio_t
	Port::max_lsp_bw(void)
	{
		return max_lsp_bw_;
	}

	void
	Port::upd_parameters(label_t label_id)
	{
		upd_unres_bw();

		switch (board()->sw_cap()) {
			case SWCAP_LSC:
				upd_dwdm_lambdas_bitmap(label_id);
				break;
			default:
				break;
		}
	}

	void
	Port::set_bit(uint32_t & word, uint32_t index)
	{
		uint32_t bit_mask = 1;

		bit_mask <<= index ;
		word |= bit_mask;
	}

	void
	Port::clear_bit(uint32_t & word, uint32_t index)
	{
		uint32_t bit_mask = 1;

		bit_mask <<= index;
		bit_mask = ~bit_mask;
		word &= bit_mask;
	}

	void
	Port::upd_dwdm_lambdas_bitmap(label_t label_id)
	{
		iterator_resources iter;
		uint32_t *         bitmap;
		int                index;
		uint32_t           nword;
		uint32_t           nbit;
		uint8_t            base_cs;
		uint8_t            cs;
		bool               base_s;
		bool               s;
		uint16_t           base_n;
		uint16_t           n;

		if (!dwdm_lambdas_bitmap_.bitmap_size &&
		    !dwdm_lambdas_bitmap_.bitmap_word) {
			TNRC_WRN("LSC port 0x%x with a wavelength "
				 "bitmap not configured...nothing to do!!",
				 port_id_);
			return;
		}
		if ((dwdm_lambdas_bitmap_.bitmap_size &&
		     !dwdm_lambdas_bitmap_.bitmap_word) ||
		    (!dwdm_lambdas_bitmap_.bitmap_size &&
		     dwdm_lambdas_bitmap_.bitmap_word)) {
			TNRC_ERR("Wavelength bitmap corrupted into "
				 "LSC port 0x%x...nothing to do!!",
				  port_id_);
			return;
		}

		bitmap = dwdm_lambdas_bitmap_.bitmap_word;
		assert(bitmap);

		iter = resources_.find(label_id);
		if (iter == end_resources()) {
			TNRC_WRN("Cannot update lambdas bitmap: resource "
				 "not found");
			return;
		}


		GMPLS_LABEL_TO_DWDM_CS_S_N(dwdm_lambdas_bitmap_.base_lambda_label,
					   base_cs,
					   base_s,
					   base_n);

		assert(label_id.type == LABEL_LSC);

		GMPLS_LABEL_TO_DWDM_CS_S_N(label_id.value.lsc.wavelen,
					   cs,
					   s,
					   n);

		assert(base_cs);
		assert(cs);
		assert(base_cs == cs);

		index = ((s ? -1 : 1) * n) - ((base_s ? -1 : 1) * base_n);
		assert(index >= 0);

		if (index >= dwdm_lambdas_bitmap_.num_wavelengths) {
			TNRC_ERR("Trying to update bitmap out of its scope"
				 "(%d > %d)",
				  index, dwdm_lambdas_bitmap_.num_wavelengths);
			return;
		}

		nbit  = ((unsigned int) index) % 32;
		nword = ((unsigned int) index) / 32;

		nbit = (32 - 1) - nbit; // reverse bitmap

		switch (((*iter).second)->state()) {
			case LABEL_FREE:
				set_bit(*(bitmap + nword), nbit);
				break;
			default:
				clear_bit(*(bitmap + nword), nbit);
				break;
		}
	}

	void
	Port::upd_unres_bw(void)
	{
		uint32_t             count;
		iterator_resources   iter;
		Resource	   * r;
		float                bw;

		assert(board());
		count = 0;

		for(iter = begin_resources(); iter != end_resources(); iter++){
			r = (*iter).second;
			if(r->state() == LABEL_FREE) {
				count++;
			}
		}
		switch (board()->sw_cap())  {
			case SWCAP_FSC: {
				assert(count <= 1);
				bw = count ?  BW_HEX2BPS(max_res_bw_) : 0;
			}
				break;
			case SWCAP_LSC: {
				uint32_t codedBw = LSC_BASIC_BW_UNIT;
				bw = (BW_HEX2BPS(codedBw) * (float) count);
			}
				break;
			default: {
				uint32_t codedBw = DEFAULT_BASIC_BW_UNIT;
				bw = (BW_HEX2BPS(codedBw) * (float) count);
			}
				break;
		}

		unres_bw_.bw[0] = BW_BPS2HEX(bw);

		max_lsp_bw_     = unres_bw_;
	}

	gmpls_prottype_t
	Port::prot_type(void)
	{
		return prot_type_;
	}

	wdm_link_lambdas_bitmap_t
	Port::dwdm_lambdas_bitmap()
	{
		return dwdm_lambdas_bitmap_;
	}

	void
	Port::dwdm_lambdas_bitmap_init(void)
	{
		uint16_t n_extbits;
		uint16_t n_words;

		n_extbits = dwdm_lambdas_bitmap_.num_wavelengths % 32 ;
		n_words   = ((dwdm_lambdas_bitmap_.num_wavelengths / 32) +
			     (n_extbits ? 1 : 0));

		dwdm_lambdas_bitmap_.bitmap_size = n_words;

		dwdm_lambdas_bitmap_.bitmap_word =
			(uint32_t *) malloc(n_words * 4);

		memset(dwdm_lambdas_bitmap_.bitmap_word, 0, (n_words * 4));
	}

	void
	Port::get_transition_map(std::map<long, int> & transitions)
	{
		iterator_resources iter;
		Resource *         r;

		//get trasition_map
		for(iter  = begin_resources();
		    iter != end_resources();
		    iter++) {
			//get Resource
			r = (*iter).second;
			r->upd_transition_map(transitions);
		}
	}

	void
	Port::insert_calendar_event(std::map<long,
				    bw_per_prio_t> & calendar,
				    long                   time,
				    int                    free_labels)
	{
		bw_per_prio_t avail_bw;
		float         bw;

		for(int i = 1; i < GMPLS_BW_PRIORITIES; i++) {
			avail_bw.bw[i] = unres_bw_.bw[i];
		}

		switch (board()->sw_cap())  {
			case SWCAP_FSC: {
				assert(free_labels <= 1);
				bw = free_labels ?  BW_HEX2BPS(max_res_bw_) : 0;
			}
				break;
			case SWCAP_LSC: {
				uint32_t codedBw = LSC_BASIC_BW_UNIT;
				bw = (BW_HEX2BPS(codedBw) * (float) free_labels);
			}
				break;
			default: {
				uint32_t codedBw = DEFAULT_BASIC_BW_UNIT;
				bw = (BW_HEX2BPS(codedBw) * (float) free_labels);
			}
				break;
		}
		avail_bw.bw[0] = BW_BPS2HEX(bw);

		calendar[time] = avail_bw;
	}

	void
	Port::get_calendar_map(uint32_t               size,
			       long                   start_time,
			       long                   time_quantum,
			       std::map<long,
			       bw_per_prio_t> & calendar)
	{
		map<long, int>           transitions;
		map<long, int>::iterator low_iter;
		map<long, int>::iterator high_iter;
		map<long, int>::iterator start_iter;
		map<long, int>::iterator iter;
		int                      val;      //current value
		int                      min;      //min value
		int                      pre;      //previous min
		long                     low_thr;  //low threshold
		long                     high_thr; //high threshold
		long                     n_quant;

		//get trasition_map
		get_transition_map(transitions);
		//build calendar map
		if (transitions.size() == 0) {
			//no transitions
			TNRC_DBG("No transition in the calendar");
			return;
		}
		high_iter = transitions.upper_bound(start_time);
		val = n_resources();
		for (iter = transitions.begin(); iter != high_iter; iter++) {
			val += (*iter).second;
		}
		//initialization
		low_iter   = transitions.lower_bound(start_time);
		start_iter = low_iter++;
		low_thr    = start_time;
		high_thr   = start_time + time_quantum;
		min        = val;
		pre        = n_resources() + 1; //dummy value
		for (iter = start_iter; iter != transitions.end(); iter++) {
			if ((*iter).first >= high_thr) {
				n_quant = ((*iter).first - start_time)/
					time_quantum;
				if (min != pre) {
					//calendar[low_thr] = min;
					insert_calendar_event(calendar,
							      low_thr,
							      min);
					pre = min;
				}
				if ((((*iter).first - high_thr) >= time_quantum)
				    && (val != pre))  {
					low_thr = high_thr;
					//calendar[low_thr] = val;
					insert_calendar_event(calendar,
							      low_thr,
							      val);
					pre = val;
				}
				min = val;
				low_thr = start_time + n_quant*time_quantum;
				high_thr = start_time +
					(n_quant + 1)*time_quantum;
			}
			val += (*iter).second;
			if (val < min) {
				min = val;
			}
			if (calendar.size() >= size) {
				break;
			}
		}
		//insert last event
		if (calendar.size() < size) {
			insert_calendar_event(calendar, low_thr, val);
		}
	}

	int
	Port::n_resources(void)
	{
		return (int) resources_.size();
	}

	////////////////////////////
	// Class Resource
	///////////////////////////


	Resource::Resource(void)
	{
		METIN();

		METOUT();
	}

	Resource::~Resource(void)
	{
		METIN();
		TNRC_DBG("Called Resource destructor");
		METOUT();
	}

	Resource::Resource(Port *        p,
			   int           tp_fl,
			   label_t       id,
			   opstate_t     opst,
			   admstate_t    admst,
			   label_state_t st)
	{
		port_     = p;
		tp_flags_ = tp_fl;
		label_id_ = id;
		opstate_  = opst;
		admstate_ = admst;
		state_    = st;
	}

	bool
	Resource::attach(long start,
			 long end)
	{
		iterator_reservations                  iter;
		std::pair<iterator_reservations, bool> ret;
		tm *                                   curTime;
		const char *                           timeStringFormat = "%a, %d %b %Y %H:%M:%S %zGMT";
		const int                              timeStringLength = 40;
		char                                   timeString[timeStringLength];

		//try to insert the Resource object
		ret = reservations_.insert(std::pair<long, long> (start, end));
		if (ret.second == false) {
			curTime = localtime((time_t *) &start);
			strftime(timeString, timeStringLength, timeStringFormat, curTime);
			TNRC_WRN ("An ADVANCED RESERVATION is already "
				  "registered with start time %s",
				  timeString);
			return false;
		}

		return true;
	}

	bool
	Resource::detach(long start)
	{
		iterator_reservations iter;
		tm *                  curTime;
		const char *          timeStringFormat = "%a, %d %b %Y %H:%M:%S %zGMT";
		const int             timeStringLength = 40;
		char                  timeString[timeStringLength];

		iter = reservations_.find(start);
		if (iter == end_reservations()) {
			curTime = localtime((time_t *) &start);
			strftime(timeString, timeStringLength, timeStringFormat, curTime);
			TNRC_WRN ("No ADVANCED RESERVATION registered with "
				  "start time %s", timeString);
			return false;
		}
		reservations_.erase(iter);

		return true;
	}

	bool
	Resource::check_label_availability(long start, long end)
	{
		iterator_reservations iter;

		//If there aren't reservations, return true
		if (reservations_.empty()) {
			return true;
		}
		//Check if [start, end] slot overlaps other slots
		for (iter  = begin_reservations();
		     iter != end_reservations();
		     iter++) {
			if ((start == (*iter).first )  ||
			    (start == (*iter).second)  ||
			    (end   == (*iter).first )  ||
			    (end   == (*iter).second)) {
				return false;
			}
			if (start < (*iter).first) {
				if ((*iter).first < end) {
					return false;
				}
				continue;
			} else if ((*iter).first < start) {
				if (start < (*iter).second) {
					return false;
				}
				continue;
			}
		}
		return true;
	}

	void
	Resource::upd_transition_map(std::map<long, int> & transitions)
	{
	        iterator_reservations         iter_rsrv;
		std::map<long, int>::iterator iter_trans;
		long                          start;
		long                          end;

		for(iter_rsrv  = begin_reservations();
		    iter_rsrv != end_reservations();
		    iter_rsrv++) {
			start = (*iter_rsrv).first;
			end   = (*iter_rsrv).second;
			//insert start transition
			iter_trans = transitions.find(start);
			if (iter_trans == transitions.end()) {
				//new transition
				transitions[start] = -1;
			} else {
				//transition already present
				(*iter_trans).second--;
				if ((*iter_trans).second == 0) {
					transitions.erase(iter_trans);
				}
			}
			//insert end transition
			iter_trans = transitions.find(end);
			if (iter_trans == transitions.end()) {
				//new transition
				transitions[end] = 1;
			} else {
				//transition already present
				(*iter_trans).second++;
				if ((*iter_trans).second == 0) {
					transitions.erase(iter_trans);
				}
			}
		}

	}

	Port *
	Resource::port()
	{
		return port_;
	}

	int
	Resource::tp_flags(void)
	{
		return tp_flags_;
	}

	label_t
	Resource::label_id(void)
	{
		return label_id_;
	}

	opstate_t
	Resource::opstate(void)
	{
		return opstate_;
	}

	void
	Resource::opstate(opstate_t st)
	{
		opstate_ = st;
	}

	admstate_t
	Resource::admstate(void)
	{
		return admstate_;
	}

	void
	Resource::admstate(admstate_t st)
	{
		admstate_ = st;
	}

	label_state_t
	Resource::state(void)
	{
		return state_;
	}

	void
	Resource::state(label_state_t st)
	{
		state_ = st;
	}

	int
	Resource::n_reservations(void)
	{
		return reservations_.size();
	}

};
