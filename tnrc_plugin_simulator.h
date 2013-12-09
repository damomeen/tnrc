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



#ifndef TNRC_PLUGIN_SIMULATOR_H_
#define TNRC_PLUGIN_SIMULATOR_H_

#include "tnrc_plugin.h"

#define _DEFINE_SIM_LIST_ITERATOR(TD,T,P,V)		\
	typedef std::list<T>::iterator TD;		\
	TD begin##P(void) { return V.begin(); };	\
	TD end##P(void)   { return V.end();   };

#define DEFINE_SIM_LIST_ITERATOR(N,T)				\
	_DEFINE_SIM_LIST_ITERATOR(iterator_##N, T*, _##N, N##_);

#define XC_BIDIR_SUPPORT  0

#define _SIMULATOR_CASUAL

#ifndef _SIMULATOR_CASUAL
//Only for equipment simulator Plugin
#define N_RESP 7 //number of possible response from equipment

#define EXTRACT_SIM_RESP(X, THR, d)					       \
  (( (X) <   THR                         ) ? TNRCSP_RESULT_NOERROR      :      \
  ((((X) >=  THR     )&&((X) < (THR+  d))) ? TNRCSP_RESULT_EQPTLINKDOWN :      \
  ((((X) >= (THR+  d))&&((X) < (THR+2*d))) ? TNRCSP_RESULT_PARAMERROR   :      \
  ((((X) >= (THR+2*d))&&((X) < (THR+3*d))) ? TNRCSP_RESULT_NOTCAPABLE   :      \
  ((((X) >= (THR+3*d))&&((X) < (THR+4*d))) ? TNRCSP_RESULT_BUSYRESOURCES:      \
  ((((X) >= (THR+4*d))&&((X) < (THR+5*d))) ? TNRCSP_RESULT_INTERNALERROR:      \
                                             TNRCSP_RESULT_GENERICERROR ))))))
#endif /* _SIMULATOR_CASUAL */

typedef enum {
	XC_FREE = 0,
	XC_ACTIVE,
	XC_BOOKED,
	XC_BUSY
} xc_status_t;

typedef struct {
	int id;

	xc_status_t status;

	eqpt_id_t  eqpt_id_in;
	board_id_t board_id_in;
	port_id_t  port_id_in;
	label_t    labelid_in;

	eqpt_id_t  eqpt_id_out;
	board_id_t board_id_out;
	port_id_t  port_id_out;
	label_t    labelid_out;

	tnrcsp_notification_cb_t   async_cb;
	void                     * async_ctxt;
} tnrcsp_sim_xc_t;

typedef struct {
	tnrcsp_handle_t handle;

	eqpt_id_t  eqpt_id_in;
	board_id_t board_id_in;
	port_id_t  port_id_in;
	label_t    labelid_in;

	eqpt_id_t  eqpt_id_out;
	board_id_t board_id_out;
	port_id_t  port_id_out;
	label_t    labelid_out;

	xcdirection_t      dir;
	tnrc_boolean_t     isvirtual;
	tnrc_boolean_t     activate;

	tnrcsp_response_cb_t       resp_cb;
	void                     * resp_ctxt;
	tnrcsp_notification_cb_t   async_cb;
	void                     * async_ctxt;
} tnrcsp_sim_maxe_xc_t;

typedef struct {
	tnrcsp_handle_t handle;

	eqpt_id_t  eqpt_id_in;
	board_id_t board_id_in;
	port_id_t  port_id_in;
	label_t    labelid_in;

	eqpt_id_t  eqpt_id_out;
	board_id_t board_id_out;
	port_id_t  port_id_out;
	label_t    labelid_out;

	xcdirection_t  dir;
	tnrc_boolean_t isvirtual;
	tnrc_boolean_t deactivate;

	tnrcsp_response_cb_t   resp_cb;
	void                 * resp_ctxt;
} tnrcsp_sim_destroy_xc_t;

typedef struct {
	tnrcsp_handle_t handle;

	eqpt_id_t  eqpt_id_in;
	board_id_t board_id_in;
	port_id_t  port_id_in;
	label_t    labelid_in;

	eqpt_id_t  eqpt_id_out;
	board_id_t board_id_out;
	port_id_t  port_id_out;
	label_t    labelid_out;

	xcdirection_t  dir;
	tnrc_boolean_t isvirtual;

	tnrcsp_response_cb_t   resp_cb;
	void                 * resp_ctxt;
} tnrcsp_sim_reserve_xc_t;

typedef tnrcsp_sim_reserve_xc_t tnrcsp_sim_unreserve_xc_t;


class Psimulator : public Plugin {
 private:

	std::list<tnrcsp_sim_xc_t*> xconns_;

	int xc_id_;

 public:
	Psimulator(void) {};
	~Psimulator(void);
	Psimulator(std::string name);

	bool              detach_xc(tnrcsp_sim_xc_t *xc);
	bool              attach_xc(tnrcsp_sim_xc_t *xc);

	tnrcsp_sim_xc_t * getXC(eqpt_id_t  eqpt_id_in,
				board_id_t board_id_in,
				port_id_t  port_id_in,
				label_t    labelid_in,
				eqpt_id_t  eqpt_id_out,
				board_id_t board_id_out,
				port_id_t  port_id_out,
				label_t    labelid_out);

	tnrcsp_sim_xc_t * getXC(int id);

	bool              matchXC(tnrcsp_sim_xc_t * xc,
				  eqpt_id_t         eqpt_id_in,
				  board_id_t        board_id_in,
				  port_id_t         port_id_in,
				  label_t           labelid_in,
				  eqpt_id_t         eqpt_id_out,
				  board_id_t        board_id_out,
				  port_id_t         port_id_out,
				  label_t           labelid_out);

	int              new_xc_id(void);
	int              n_xconns(void);

	// Defines iterator_xconns
	DEFINE_SIM_LIST_ITERATOR(xconns, tnrcsp_sim_xc_t);

	wq_item_status	   wq_function(void *d);
	void		   del_item_data(void *d);
	tnrcapiErrorCode_t probe(std::string location);

	tnrcsp_result_t get_response(void);

	tnrcsp_result_t tnrcsp_make_xc(tnrcsp_handle_t *        handlep,
				       tnrc_port_id_t           portid_in,
				       label_t                  labelid_in,
				       tnrc_port_id_t           portid_out,
				       label_t                  labelid_out,
				       xcdirection_t            direction,
				       tnrc_boolean_t           isvirtual,
				       tnrc_boolean_t           activate,
				       tnrcsp_response_cb_t     response_cb,
				       void *                   response_ctxt,
				       tnrcsp_notification_cb_t async_cb,
				       void *                   async_ctxt);

	tnrcsp_result_t tnrcsp_destroy_xc(tnrcsp_handle_t *    handlep,
					  tnrc_port_id_t       portid_in,
					  label_t              labelid_in,
					  tnrc_port_id_t       portid_out,
					  label_t              labelid_out,
					  xcdirection_t        direction,
					  tnrc_boolean_t       isvirtual,
					  tnrc_boolean_t       deactivate,
					  tnrcsp_response_cb_t response_cb,
					  void *               response_ctxt);

	tnrcsp_result_t tnrcsp_reserve_xc(tnrcsp_handle_t *    handlep,
					  tnrc_port_id_t       portid_in,
					  label_t              labelid_in,
					  tnrc_port_id_t       portid_out,
					  label_t              labelid_out,
					  xcdirection_t        direction,
					  tnrc_boolean_t       isvirtual,
					  tnrcsp_response_cb_t response_cb,
					  void *               response_ctxt);

	tnrcsp_result_t tnrcsp_unreserve_xc(tnrcsp_handle_t *    handlep,
					    tnrc_port_id_t       portid_in,
					    label_t              labelid_in,
					    tnrc_port_id_t       portid_out,
					    label_t              labelid_out,
					    xcdirection_t        direction,
					    tnrc_boolean_t       isvirtual,
					    tnrcsp_response_cb_t response_cb,
					    void *               response_ctxt);

	tnrcsp_result_t tnrcsp_register_async_cb(tnrcsp_event_t * events);
};


//Function to export

tnrcsp_sim_xc_t *
new_tnrcsp_sim_xc(int                      xc_id,
		  xc_status_t              status,
		  eqpt_id_t                eqpt_id_in,
		  board_id_t               board_id_in,
		  port_id_t                port_id_in,
		  label_t                  labelid_in,
		  eqpt_id_t                eqpt_id_out,
		  board_id_t               board_id_out,
		  port_id_t                port_id_out,
		  label_t                  labelid_out,
		  tnrcsp_notification_cb_t async_cb,
		  void *                   async_ctxt);

tnrcsp_sim_maxe_xc_t *
new_tnrcsp_sim_maxe_xc(tnrcsp_handle_t          handle,
		       tnrc_port_id_t           portid_in,
		       label_t                  labelid_in,
		       tnrc_port_id_t           portid_out,
		       label_t                  labelid_out,
		       xcdirection_t            direction,
		       tnrc_boolean_t           isvirtual,
		       tnrc_boolean_t           activate,
		       tnrcsp_response_cb_t     response_cb,
		       void *                   response_ctxt,
		       tnrcsp_notification_cb_t async_cb,
		       void *                   async_ctxt);

tnrcsp_sim_destroy_xc_t *
new_tnrcsp_sim_destroy_xc(tnrcsp_handle_t      handle,
			  tnrc_port_id_t       portid_in,
			  label_t              labelid_in,
			  tnrc_port_id_t       portid_out,
			  label_t              labelid_out,
			  xcdirection_t        direction,
			  tnrc_boolean_t       isvirtual,
			  tnrc_boolean_t       deactivate,
			  tnrcsp_response_cb_t response_cb,
			  void *               response_ctxt);

tnrcsp_sim_reserve_xc_t *
new_tnrcsp_sim_reserve_xc(tnrcsp_handle_t      handle,
			  tnrc_port_id_t       portid_in,
			  label_t              labelid_in,
			  tnrc_port_id_t       portid_out,
			  label_t              labelid_out,
			  xcdirection_t        direction,
			  tnrc_boolean_t       isvirtual,
			  tnrcsp_response_cb_t response_cb,
			  void *               response_ctxt);

tnrcsp_sim_unreserve_xc_t *
new_tnrcsp_sim_unreserve_xc(tnrcsp_handle_t      handle,
			    tnrc_port_id_t       portid_in,
			    label_t              labelid_in,
			    tnrc_port_id_t       portid_out,
			    label_t              labelid_out,
			    xcdirection_t        direction,
			    tnrc_boolean_t       isvirtual,
			    tnrcsp_response_cb_t response_cb,
			    void *               response_ctxt);


#endif /* TNRC_PLUGIN_SIMULATOR_H */
