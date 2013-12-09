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



#ifndef _TNRC_CORBA_H_
#define _TNRC_CORBA_H_

#if HAVE_OMNIORB

#include "lib/corba.h"
#include "idl/g2rsvpte.hh"
#include "idl/lrm.hh"

extern g2rsvpte::TnrControl_var g2rsvpte_proxy;
extern LRM::Info_var            lrm_proxy;

int corba_server_setup(void);
int corba_server_shutdown(void);
int corba_client_setup(g2rsvpte::TnrControl_var & g);
int corba_client_setup(LRM::Info_var & l);
int corba_client_shutdown(g2rsvpte::TnrControl_var & g);
int corba_client_shutdown(LRM::Info_var & l);

#endif // HAVE_OMNIORB

#endif // _TNRC_CORBA_H_
