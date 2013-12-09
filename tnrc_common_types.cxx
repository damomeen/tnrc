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



#include "tnrc_common_types.h"

#if HAVE_OMNIORB

#include "g2mpls_corba_utils.h"
#include <stdexcept>

label_t &
operator<< (label_t &                            dst,
	    const g2mplsTypes::labelId_var &     src)
{
	memset(&dst, 0, sizeof(dst));

	dst.type = LABEL_UNSPECIFIED;

	switch (src->_d()) {
		case g2mplsTypes:: LABELTYPE_L32: {
			dst.value.rawId = (uint64_t) src->label32();
			break;
		}
		case g2mplsTypes:: LABELTYPE_L60: {
			//FIXME
			dst.value.rawId = src->label60();
			break;
		}
		default:
			throw out_of_range("LABEL ID type out-of-range");
			break;
	}

	return dst;
}

g2mplsTypes::labelId_var &
operator<< (g2mplsTypes::labelId_var &           dst,
	    const label_t &                      src)
{
	dst = new g2mplsTypes::labelId;

	switch (src.type) {
		case LABEL_PSC:
		case LABEL_FSC:
		case LABEL_LSC:
		case LABEL_TDM: {
			dst->label32((u_int32_t) src.value.rawId);
			break;
		}
		case LABEL_L2SC: {
			uint64_t l60v;
			int i;

			l60v = 0x0ull;
			for (i = 0; i < 6; i++) {
				int disp = ((5 - i) * 8 + 12);
				l60v |= src.value.l2sc.dstMac[i] << disp;
			}
			l60v |= src.value.l2sc.vlanId;

			dst->label60(l60v);
			break;
		}
		default:
			throw out_of_range("G2.LABEL type out-of-range");
			break;
	}

	return dst;
}

#endif // HAVE_OMNIORB
