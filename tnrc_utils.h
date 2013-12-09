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



#ifndef TNRC_UTILS_H
#define TNRC_UTILS_H

#include "zebra.h"
#include "zassert.h"

#include "tnrc_config.h"

#ifdef VERBOSE_METHODS
#define METIN() {					\
	zlog_debug("Entering %s", __PRETTY_FUNCTION__);	\
}


#define METOUT() {					\
	zlog_debug("Exiting %s",  __PRETTY_FUNCTION__);	\
}
#else
#define METIN()
#define METOUT()
#endif /* VERBOSE_METHODS */

#define HBEAT()					\
	zlog_debug("[HBEAT]> %s:%d (%s)\n",	\
	           __FILE__,			\
	           __LINE__,			\
	           __PRETTY_FUNCTION__)

#define BUG()					\
	zlog_err("Bug hit in %s:%d (%s)\n",	\
	         __FILE__,			\
	         __LINE__,			\
	         __PRETTY_FUNCTION__);

#endif /* TNRC_UTILS_H */
