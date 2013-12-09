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



#ifndef TNRC_APIS_H
#define TNRC_APIS_H

#include <string>
#include <list>
#include "thread.h"

#include "tnrc_action_objects.h"
#include "tnrc_sp.h"
#include "tnrc_common_types.h"

#define TNRCAPIERR2STRING(X)					\
  (((X) == TNRC_API_ERROR_NONE                     ) ?		\
   "NO ERROR"                                        :		\
  (((X) == TNRC_API_ERROR_INVALID_PLUGIN	   ) ?		\
   "ERROR INVALID PLUGIN"	                     :		\
  (((X) == TNRC_API_ERROR_NO_PLUGIN		   ) ?		\
   "ERROR NO PLUGIN"	                             :		\
  (((X) == TNRC_API_ERROR_INVALID_PARAMETER	   ) ?		\
   "ERROR INVALID PARAMETER"                         :		\
  (((X) == TNRC_API_ERROR_INSTANCE_ALREADY_EXISTS  ) ?	        \
   "ERROR INSTANCE ALREADY EXIST"                    :		\
  (((X) == TNRC_API_ERROR_NO_INSTANCE		   ) ?		\
   "ERROR NO INSTANCE"                               :		\
  (((X) == TNRC_API_ERROR_INVALID_EQPT_ID	   ) ?		\
   "ERROR INVALID EQPT ID"                           :		\
  (((X) == TNRC_API_ERROR_INVALID_BOARD_ID	   ) ?		\
   "ERROR INVALID BOARD ID"                          :		\
  (((X) == TNRC_API_ERROR_INVALID_PORT_ID	   ) ?		\
   "ERROR INVALID PORT ID"                           :		\
  (((X) == TNRC_API_ERROR_INVALID_RESOURCE_ID	   ) ?		\
   "ERROR INVALID RESOURCE ID"                       :		\
  (((X) == TNRC_API_ERROR_GENERIC                  ) ?		\
   "ERROR GENERIC"                                   :		\
  (((X) == TNRC_API_ERROR_INTERNAL_ERROR           ) ?		\
   "INTERNAL ERROR"                                  :		\
   "<unrecognized>"))))))))))))

#define BOARDTYPE2SWCAP(X)                      \
  (((X) == BOARD_PSC       ) ? SWCAP_PSC_1 :	\
  (((X) == BOARD_L2SC      ) ? SWCAP_L2SC  :	\
  (((X) == BOARD_FSC       ) ? SWCAP_FSC   :	\
  (((X) == BOARD_LSC       ) ? SWCAP_LSC   :	\
  (((X) == BOARD_TDM       ) ? SWCAP_TDM   :	\
			       SWCAP_UNKNOWN)))))

//typedef short tnrc_boolean_t;

//typedef tnrcsp_handle_t tnrcap_cookie_t;

typedef tnrcsp_result_t tnrcap_result_t;

typedef enum {
	TNRC_API_ERROR_NONE = 0,
	TNRC_API_ERROR_INVALID_PLUGIN,
	TNRC_API_ERROR_NO_PLUGIN,
	TNRC_API_ERROR_INVALID_PARAMETER,
	TNRC_API_ERROR_INSTANCE_ALREADY_EXISTS,
	TNRC_API_ERROR_NO_INSTANCE,
	TNRC_API_ERROR_INVALID_EQPT_ID,
	TNRC_API_ERROR_INVALID_BOARD_ID,
	TNRC_API_ERROR_INVALID_PORT_ID,
	TNRC_API_ERROR_INVALID_RESOURCE_ID,
	TNRC_API_ERROR_INTERNAL_ERROR,
	TNRC_API_ERROR_GENERIC
} tnrcapiErrorCode_t;

namespace TNRC_OPEXT_API {

	tnrcapiErrorCode_t tnrcap_make_xc(tnrcap_cookie_t *        cookiep,
					  g2mpls_addr_t            dl_id_in,
					  label_t                  labelid_in,
					  g2mpls_addr_t            dl_id_out,
					  label_t                  labelid_out,
					  xcdirection_t            direction,
					  tnrc_boolean_t           is_virtual,
					  tnrc_boolean_t           activate,
					  tnrcap_cookie_t          rsrv_cookie,
					  long                     response_ctxt,
					  long                     async_ctxt);

	tnrcapiErrorCode_t tnrcap_destroy_xc(tnrcap_cookie_t      cookie,
					     tnrc_boolean_t       deactivate,
					     long                 response_ctxt);

	tnrcapiErrorCode_t tnrcap_reserve_xc(tnrcap_cookie_t *    cookiep,
					     g2mpls_addr_t        dl_id_in,
					     label_t              labelid_in,
					     g2mpls_addr_t        dl_id_out,
					     label_t              labelid_out,
					     xcdirection_t        direction,
					     tnrc_boolean_t       is_virtual,
					     tnrc_boolean_t       advance_rsrv,
					     long                 start,
					     long                 end,
					     long                 response_ctxt);

	tnrcapiErrorCode_t tnrcap_unreserve_xc(tnrcap_cookie_t      cookie,
					       long                 response_ctxt);

	tnrcapiErrorCode_t tnrcap_get_eqpt_details(g2mpls_addr_t & eqpt_addr,
						   eqpt_type_t &   type);

	tnrcapiErrorCode_t tnrcap_get_dl_list(std::list<g2mpls_addr_t> & dl_list);

	tnrcapiErrorCode_t tnrcap_get_dl_details(g2mpls_addr_t dl_id,
						 tnrc_api_dl_details_t
						 & details);

	tnrcapiErrorCode_t tnrcap_get_dl_calendar(g2mpls_addr_t   dl_id,
						  uint32_t        start_time,
						  uint32_t        num_events,
						  uint32_t        quantum,
						  std::map<long,
						  bw_per_prio_t> &
						  calendar);

	tnrcapiErrorCode_t tnrcap_get_label_list(g2mpls_addr_t dl_id,
						 std::list<tnrc_api_label_t>
						 & label_list);

	tnrcapiErrorCode_t tnrcap_get_label_details(g2mpls_addr_t   dl_id,
						    label_t         label_id,
						    opstate_t     & opstate,
						    label_state_t & label_state);

	tnrcapiErrorCode_t tnrcap_get_dl_alarms(g2mpls_addr_t dl_id);

	tnrcapiErrorCode_t tnrcap_enable_alarms(void);

	tnrcapiErrorCode_t tnrcap_disable_alarms(void);

	tnrcapiErrorCode_t tnrcap_flush_resources(void);

};

namespace TNRC_OPSPEC_API {

	void make_xc_resp_cb(tnrcsp_handle_t handle,
			     tnrcsp_result_t result,
			     void *          ctxt);

	void destroy_xc_resp_cb(tnrcsp_handle_t handle,
				tnrcsp_result_t result,
				void *          ctxt);

	void notification_xc_cb(tnrcsp_handle_t         handle,
				tnrcsp_resource_id_t ** failed_resource_listp,
				void *                  cxt);

	void reserve_xc_resp_cb(tnrcsp_handle_t handle,
				tnrcsp_result_t result,
				void *          ctxt);

	void unreserve_xc_resp_cb(tnrcsp_handle_t handle,
				  tnrcsp_result_t result,
				  void *          ctxt);

	void update_dm_after_action(Action * action);

	void update_dm_after_notify(tnrc_port_id_t  portid,
				    label_t         labelid,
				    tnrcsp_evmask_t event);

	void update_eqpt(tnrc_port_id_t portid, tnrcsp_evmask_t event);

	void update_board(tnrc_port_id_t portid, tnrcsp_evmask_t event);

	void update_port(tnrc_port_id_t portid, tnrcsp_evmask_t event);

	void update_resource(tnrc_port_id_t     portid,
			     label_t            labelid,
			     label_state_t      label_state,
			     tnrcsp_evmask_t    event,
			     bool               upd_unres_bw);
};

namespace TNRC_CONF_API {

	tnrcapiErrorCode_t tnrcStart(std::string & resp);

	tnrcapiErrorCode_t tnrcStop(std::string & resp);

	tnrcapiErrorCode_t init_plugin(std::string name, std::string loc);
extern "C" {
	int                plugin_probe(struct thread *t);
}
	tnrc_port_id_t     get_tnrc_port_id(eqpt_id_t  eqpt_id,
				            board_id_t board_id,
				            port_id_t  port_id);

	g2mpls_addr_t      get_dl_id(tnrc_port_id_t tnrc_port_id);

	void               convert_tnrc_port_id(eqpt_id_t *    eqpt_id,
						board_id_t *   board_id,
						port_id_t *    port_id,
						tnrc_port_id_t tnrc_port_id);

	void               convert_dl_id(tnrc_port_id_t * tnrc_port_id,
					 g2mpls_addr_t    dl_id);

	tnrcapiErrorCode_t add_Eqpt(eqpt_id_t     id,
				    g2mpls_addr_t address,
				    eqpt_type_t   type,
				    opstate_t     opst,
				    admstate_t    admst,
				    std::string   location);

	tnrcapiErrorCode_t add_Board(eqpt_id_t  eqpt_id,
				     board_id_t id,
				     sw_cap_t   sw_cap,
				     enc_type_t enc_type,
				     opstate_t  opst,
				     admstate_t admst);

	tnrcapiErrorCode_t add_Port(eqpt_id_t        eqpt_id,
				    board_id_t       board_id,
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

	tnrcapiErrorCode_t add_Resource(eqpt_id_t     eqpt_id,
					board_id_t    board_id,
					port_id_t     port_id,
					int           tp_fl,
					label_t       id,
					opstate_t     opst,
					admstate_t    admst,
					label_state_t st);

};

#endif /* __TNRC_APIS_H__ */
