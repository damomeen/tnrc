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



#ifndef TNRC_COMMON_TYPES_H
#define TNRC_COMMON_TYPES_H

#include <g2mpls_addr.h>
#include <g2mpls_types.h>
#if HAVE_OMNIORB
#include "idl/types.hh"
#include "idl/g2mplsTypes.hh"
#endif // HAVE_OMNIORB
#include <map>

typedef uint32_t tnrcap_cookie_t;

typedef uint32_t tnrc_port_id_t;

typedef enum {
	EQPT_UNDEFINED = 0,
	EQPT_SIMULATOR,
	EQPT_CALIENT_DW_PXC,
        EQPT_ADVA_LSC,
        EQPT_FOUNDRY_L2SC,
        EQPT_AT8000_L2SC,
        EQPT_AT9424_L2SC,
        EQPT_CLS_FSC
} eqpt_type_t;

#define SHOW_EQPT_TYPE(X)                                       \
  (((X) == EQPT_UNDEFINED       ) ? "EQPT_UNDEFINED"       :	\
  (((X) == EQPT_SIMULATOR       ) ? "EQPT_SIMULATOR"       :	\
  (((X) == EQPT_CALIENT_DW_PXC  ) ? "EQPT_CALIENT_DW_PXC"  :	\
  (((X) == EQPT_ADVA_LSC        ) ? "EQPT_ADVA_LSC"        :	\
  (((X) == EQPT_FOUNDRY_L2SC    ) ? "EQPT_FOUNDRY_L2SC"    :	\
  (((X) == EQPT_AT8000_L2SC     ) ? "EQPT_AT8000_L2SC"     :	\
  (((X) == EQPT_AT9424_L2SC     ) ? "EQPT_AT9424_L2SC"     :	\
  (((X) == EQPT_CLS_FSC		) ? "EQPT_CLS_FSC"	   :	\
		   "==UNKNOWN=="))))))))

typedef uint8_t eqpt_id_t;

typedef enum {
	BOARD_UNDEFINED = 0,
	BOARD_PSC,
	BOARD_L2SC,
	BOARD_FSC,
	BOARD_LSC,
	BOARD_TDM
} board_type_t;

#define EQPT_TYPE2NAME(X)					\
  (((X) == EQPT_SIMULATOR       ) ? SIMULATOR_STRING    :	\
  (((X) == EQPT_CALIENT_DW_PXC  ) ? CALIENT_STRING	:	\
  (((X) == EQPT_ADVA_LSC        ) ? ADVA_LSC_STRING	:	\
  (((X) == EQPT_FOUNDRY_L2SC    ) ? FOUNDRY_L2SC_STRING	:	\
  (((X) == EQPT_AT8000_L2SC     ) ? AT8000_L2SC_STRING	:	\
  (((X) == EQPT_AT9424_L2SC     ) ? AT9424_L2SC_STRING	:	\
  (((X)	== EQPT_CLS_FSC		) ? CLS_STRING :                \
					  "==UNKNOWN==")))))))


typedef uint16_t board_id_t;

typedef enum {
	LABEL_UNSPECIFIED = 0,
	LABEL_PSC,
	LABEL_L2SC,
	LABEL_TDM,
	LABEL_LSC,
	LABEL_FSC
} label_type_t;

#ifdef UBUNTU_OPERATOR_PATCH
#define ARE_LABEL_EQUAL(labelA, labelB)       ((labelA).value.lsc.wavelen == (labelB).value.lsc.wavelen)
#else
#define ARE_LABEL_EQUAL(labelA, labelB)       (labelA == labelB)
#endif
typedef struct label {

	label_type_t type;

	union {
		struct {
			uint32_t id;
		} psc;

		struct {
			char     dstMac[6];
			uint16_t vlanId;
		} l2sc;

		struct {
			uint16_t  s;
			uint32_t  u:4;
			uint32_t  k:4;
			uint32_t  l:4;
			uint32_t  m:4;
		} tdm;

		struct {
			uint32_t wavelen;
		} lsc;

		struct {
			uint32_t portId;
		} fsc;

		uint64_t rawId;
	} value;
	friend bool operator == (const struct label& first,
				 const struct label& second)
	{
		if (first.value.rawId == second.value.rawId) {
			return true;
		}

		if (first.type != second.type) {
			return false;
		}
		switch (first.type) {
			case LABEL_FSC: {
				return (first.value.fsc.portId ==
					second.value.fsc.portId);
				break;
			}
			case LABEL_PSC: {
				return (first.value.psc.id ==
					second.value.psc.id);
				break;
			}
			case LABEL_L2SC: {
				return ((first.value.l2sc.vlanId ==
					 second.value.l2sc.vlanId) &&
					(first.value.l2sc.dstMac ==
					 second.value.l2sc.dstMac));
				break;
			}
			case LABEL_LSC: {
				return (first.value.lsc.wavelen ==
					second.value.lsc.wavelen);
				break;
			}
			case LABEL_TDM: {
				return ((first.value.tdm.s ==
					 second.value.tdm.s) &&
					(first.value.tdm.u ==
					 second.value.tdm.u) &&
					(first.value.tdm.k ==
					 second.value.tdm.k) &&
					(first.value.tdm.l ==
					 second.value.tdm.l) &&
					(first.value.tdm.m ==
					 second.value.tdm.m));
				break;
			}
			default: {
				return false;
			}
		}
	}
} label_t;

using namespace std;

typedef uint16_t port_id_t;

typedef struct {
	// IP address of the eqpt
	g2mpls_addr_t                 eqpt_addr;
	// IP address of the remote eqpt
	g2mpls_addr_t                 rem_eqpt_addr;
	// id of the remote port
	port_id_t                     rem_port_id;
	// Operational state
	opstate_t                     opstate;
	// Switching capability
	sw_cap_t                      sw_cap;
	// Switching capability
	enc_type_t                    enc_type;
	// total available bandwidth
	uint32_t                      tot_bw;
	// max reservable bandwidth
	uint32_t                      max_res_bw;
	// Available bandwidth per priority
	bw_per_prio_t                 unres_bw;
	// Minimum reservable bandiwdth
	uint32_t                      min_lsp_bw;
	// Maximum reservable bandiwdth per priority
	bw_per_prio_t                 max_lsp_bw;
	// DWDM wavelength bitmask
	wdm_link_lambdas_bitmap_t     bitmap;
	// Protection type available
	gmpls_prottype_t              prot_type;
	// Advance reservation calendar
	std::map<long, bw_per_prio_t> calendar;
} tnrc_api_dl_details_t;

typedef struct {
	label_t       label_id;
	opstate_t     opstate;
	label_state_t state;
} tnrc_api_label_t;

#if HAVE_OMNIORB
label_t &
operator<< (label_t &                        dst,
	    const g2mplsTypes::labelId_var & src);

g2mplsTypes::labelId_var &
operator<< (g2mplsTypes::labelId_var & dst,
	    const label_t &            src);
#endif // HAVE_OMNIORB

#endif // TNRC_COMMON_TYPES_H
