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



#ifndef __TNRC_G2RSVPTE_API_H__
#define __TNRC_G2RSVPTE_API_H__

#include "tnrcd.h"
#if HAVE_OMNIORB
#include "g2mpls_types.h"
#include "idl/types.hh"
#include "idl/g2mplsTypes.hh"
#include <omniORB4/CORBA.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#endif // HAVE_OMNIORB

void action_response(tnrcap_cookie_t    cookie,
		     tnrc_action_type_t action_type,
		     tnrcap_result_t    result,
		     bool               executed,
		     long               ctxt);

void async_notification(tnrcap_cookie_t cookie,
			void **         notify_list,
			long            ctxt);

#if HAVE_OMNIORB

using namespace std;

#endif // HAVE_OMNIORB

#endif // __TNRC_G2RSVPTE_API_H__
