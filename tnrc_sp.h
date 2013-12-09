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



#ifndef _TNRC_SP_H_
#define _TNRC_SP_H_

typedef uint32_t tnrcsp_handle_t;

#include <zebra.h>
#include "linklist.h"

#include "tnrc_common_types.h"

#define EVENT_EQPT_SYNC_LOST              0
#define EVENT_EQPT_OPSTATE_UP             1
#define EVENT_EQPT_OPSTATE_DOWN           2
#define EVENT_EQPT_ADMSTATE_ENABLED       3
#define EVENT_EQPT_ADMSTATE_DISABLED      4
#define EVENT_BOARD_OPSTATE_UP            5
#define EVENT_BOARD_OPSTATE_DOWN          6
#define EVENT_BOARD_ADMSTATE_ENABLED      7
#define EVENT_BOARD_ADMSTATE_DISABLED     8
#define EVENT_PORT_OPSTATE_UP             9
#define EVENT_PORT_OPSTATE_DOWN           10
#define EVENT_PORT_ADMSTATE_ENABLED       11
#define EVENT_PORT_ADMSTATE_DISABLED      12
#define EVENT_PORT_XCONNECTED             13
#define EVENT_PORT_XDISCONNECTED          14
#define EVENT_RESOURCE_OPSTATE_UP         15
#define EVENT_RESOURCE_OPSTATE_DOWN       16
#define EVENT_RESOURCE_ADMSTATE_ENABLED   17
#define EVENT_RESOURCE_ADMSTATE_DISABLED  18
#define EVENT_RESOURCE_XCONNECTED         19
#define EVENT_RESOURCE_XDISCONNECTED      20
#define EVENT_NULL                        21

#define SHOW_TNRCSP_EVENT(X)                                                     \
  (((X) == EVENT_EQPT_SYNC_LOST            ) ? "EQPT_SYNC_LOST"                : \
  (((X) == EVENT_EQPT_OPSTATE_UP           ) ? "EQPT_OPSTATE_UP"               : \
  (((X) == EVENT_EQPT_OPSTATE_DOWN         ) ? "EQPT_OPSTATE_DOWN"             : \
  (((X) == EVENT_EQPT_ADMSTATE_ENABLED     ) ? "EQPT_ADMSTATE_ENABLED"         : \
  (((X) == EVENT_EQPT_ADMSTATE_DISABLED    ) ? "EQPT_ADMSTATE_DISABLED"        : \
  (((X) == EVENT_BOARD_OPSTATE_DOWN        ) ? "BOARD_OPSTATE_DOWN"            : \
  (((X) == EVENT_BOARD_OPSTATE_DOWN        ) ? "BOARD_OPSTATE_DOWN"            : \
  (((X) == EVENT_BOARD_ADMSTATE_ENABLED    ) ? "BOARD_ADMSTATE_ENABLEDR"       : \
  (((X) == EVENT_BOARD_ADMSTATE_DISABLED   ) ? "BOARD_ADMSTATE_DISABLED"       : \
  (((X) == EVENT_PORT_OPSTATE_UP           ) ? "PORT_OPSTATE_UP"               : \
  (((X) == EVENT_PORT_OPSTATE_DOWN         ) ? "PORT_OPSTATE_DOWN"             : \
  (((X) == EVENT_PORT_ADMSTATE_ENABLED     ) ? "PORT_ADMSTATE_ENABLED"         : \
  (((X) == EVENT_PORT_ADMSTATE_DISABLED    ) ? "PORT_ADMSTATE_DISABLED"        : \
  (((X) == EVENT_PORT_XCONNECTED           ) ? "PORT_XCONNECTED"               : \
  (((X) == EVENT_PORT_XDISCONNECTED        ) ? "PORT_XDISCONNECTED"            : \
  (((X) == EVENT_RESOURCE_OPSTATE_UP       ) ? "RESOURCE_OPSTATE_UP"           : \
  (((X) == EVENT_RESOURCE_OPSTATE_DOWN     ) ? "RESOURCE_OPSTATE_DOWN"         : \
  (((X) == EVENT_RESOURCE_ADMSTATE_ENABLED ) ? "RESOURCE_ADMSTATE_ENABLED"     : \
  (((X) == EVENT_RESOURCE_ADMSTATE_ENABLED ) ? "RESOURCE_ADMSTATE_ENABLED"     : \
  (((X) == EVENT_RESOURCE_XCONNECTED       ) ? "RESOURCE_XCONNECTED"           : \
  (((X) == EVENT_RESOURCE_XDISCONNECTED    ) ? "RESOURCE_XDISCONNECTED"        : \
  (((X) == EVENT_NULL                      ) ? "EVENT_NULL"                    : \
              "==UNKNOWN=="))))))))))))))))))))))

#define SHOW_TNRCSP_RESULT(X)                                     \
  (((X) == TNRCSP_RESULT_NOERROR       ) ? "NO ERROR"           : \
  (((X) == TNRCSP_RESULT_EQPTLINKDOWN  ) ? "EQPT LINK DOWN"     : \
  (((X) == TNRCSP_RESULT_PARAMERROR    ) ? "PARAM ERROR"        : \
  (((X) == TNRCSP_RESULT_NOTCAPABLE    ) ? "NOT CAPABLE"        : \
  (((X) == TNRCSP_RESULT_BUSYRESOURCES ) ? "BUSY RESOURCES"     : \
  (((X) == TNRCSP_RESULT_INTERNALERROR ) ? "INTERNAL ERROR"     : \
  (((X) == TNRCSP_RESULT_GENERICERROR  ) ? "GENERIC ERROR"      : \
					   "==UNKNOWN==")))))))

typedef short tnrc_boolean_t;

typedef enum {
	TNRCSP_RESULT_NOERROR = 0,
	TNRCSP_RESULT_EQPTLINKDOWN,
	TNRCSP_RESULT_PARAMERROR,
	TNRCSP_RESULT_NOTCAPABLE,
	TNRCSP_RESULT_BUSYRESOURCES,
	TNRCSP_RESULT_INTERNALERROR,
	TNRCSP_RESULT_GENERICERROR
} tnrcsp_result_t;

typedef struct {
	struct zlist    * resource_list;
	tnrc_port_id_t   portid;
} tnrcsp_resource_id_t;


typedef void (*tnrcsp_response_cb_t)(tnrcsp_handle_t handle,
				     tnrcsp_result_t result,
				     void *          ctxt);

typedef void (*tnrcsp_notification_cb_t)(tnrcsp_handle_t         handle,
					 tnrcsp_resource_id_t ** failed_resource_listp,
					 void *                  ctxt);

typedef unsigned int tnrcsp_evmask_t;

typedef struct {
	struct zlist     * event_list;

	tnrc_port_id_t    portid;
	label_t           labelid;
	opstate_t         oper_state;
	admstate_t        admin_state;
	tnrcsp_evmask_t   events;
} tnrcsp_event_t;

typedef struct {
	label_t            labelid;
	opstate_t          oper_state;
	admstate_t         admin_state;
	tnrcsp_evmask_t    last_event;
} tnrcsp_resource_detail_t;

typedef enum {
	TNRCSP_LISTTYPE_UNSPECIFIED,
	TNRCSP_LISTTYPE_RESOURCES
} tnrcsp_list_type_t;

/* typedef struct { */
/* 	tnrc_action_type_t   type; */
/* 	Plugin             * plugin; */
/* 	void               * details; */
/* } tnrcsp_action_t; */

#endif /* _TNRC_SP_H_ */
