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



#ifndef TNRC_TRACE_H
#define TNRC_TRACE_H

#include <assert.h>
#include <string>
#include <iostream>

#include "log.h"

//#define VERBOSE_DEBUG

#ifdef VERBOSE_DEBUG
#define TNRC_ERR(FMT, ...) {		\
	zlog_err("%s:%d [%s] " FMT,		\
	         __FILE__,			\
		 __LINE__,			\
	         __PRETTY_FUNCTION__,		\
                 ##__VA_ARGS__);		\
}

#define TNRC_WRN(FMT, ...) {		\
	zlog_warn("%s:%d [%s] " FMT,		\
	          __FILE__,			\
		  __LINE__,			\
	          __PRETTY_FUNCTION__,		\
                  ##__VA_ARGS__);		\
}

#define TNRC_INF(FMT, ...) {		\
	zlog_info("%s:%d [%s] " FMT,		\
	          __FILE__,			\
		  __LINE__,			\
	          __PRETTY_FUNCTION__,		\
                  ##__VA_ARGS__);		\
}

#define TNRC_NOT(FMT, ...) {		\
	zlog_notice("%s:%d [%s] " FMT,		\
	            __FILE__,			\
		    __LINE__,			\
	            __PRETTY_FUNCTION__,	\
                    ##__VA_ARGS__);		\
}

#define TNRC_DBG(FMT, ...) {		\
	zlog_debug("%s:%d [%s] " FMT,		\
	           __FILE__,			\
		   __LINE__,			\
	           __PRETTY_FUNCTION__,		\
                   ##__VA_ARGS__);		\
}

#else // VERBOSE_DEBUG

#define TNRC_ERR(FMT, ...) {		        \
	zlog_err("[ERR] " FMT, ##__VA_ARGS__);	        \
}

#define TNRC_WRN(FMT, ...) {		        \
	zlog_warn("[WRN] " FMT, ##__VA_ARGS__);	        \
}

#define TNRC_INF(FMT, ...) {		        \
	zlog_info("[INF] " FMT, ##__VA_ARGS__);	        \
}

#define TNRC_NOT(FMT, ...) {			\
	zlog_notice("[NOT] " FMT, ##__VA_ARGS__);	\
}

#define TNRC_DBG(FMT, ...) {			\
	zlog_debug("[DBG] " FMT, ##__VA_ARGS__);	\
}

#endif // VERBOSE_DEBUG

#endif // TNRC_TRACE_H
