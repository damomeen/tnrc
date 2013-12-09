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



#ifndef TNRC_PLUGIN_H_
#define TNRC_PLUGIN_H_

class Plugin;

#include <string>
#include <map>
#include <iterator>
#include "workqueue.h"

#include "tnrc_action_objects.h"
#include "tnrc_sp.h"
#include "tnrc_apis.h"

#define _DEFINE_PIN_MAP_ITERATOR(TD,T1,T2,P,V)		\
	typedef std::map<T1, T2>::iterator TD;		\
	TD begin##P(void) { return V.begin(); };	\
	TD end##P(void)   { return V.end();   };

#define DEFINE_PIN_MAP_ITERATOR(N,T1,T2)				   \
	_DEFINE_PIN_MAP_ITERATOR(iterator_##N, T1, T2 *, _##N, N##_);	   \

typedef struct {
	tnrc_action_type_t   type;
	Plugin             * plugin;
	void               * details;
} tnrcsp_action_t;

class Plugin {
 protected:
	std::string         name_;

	tnrcsp_handle_t     handle_;

	bool                xc_bidir_support_;

	struct work_queue * wqueue_;

 public:
	Plugin(void) {};
	~Plugin(void){};
	Plugin(std::string name);

	std::string                name(void);
	tnrcsp_handle_t            new_handle(void);

	bool                       xc_bidir_support(void);

	virtual wq_item_status     wq_function(void *d) = 0;
	virtual void               del_item_data(void *d) = 0;
	virtual tnrcapiErrorCode_t probe(std::string location) = 0;

	virtual tnrcsp_result_t
		tnrcsp_make_xc(tnrcsp_handle_t *        handlep,
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
			       void *                   async_ctxt) = 0;

	virtual tnrcsp_result_t
		tnrcsp_destroy_xc(tnrcsp_handle_t *    handlep,
				  tnrc_port_id_t       portid_in,
				  label_t              labelid_in,
				  tnrc_port_id_t       portid_out,
				  label_t              labelid_out,
				  xcdirection_t        direction,
				  tnrc_boolean_t       isvirtual,
				  tnrc_boolean_t       deactivate,
				  tnrcsp_response_cb_t response_cb,
				  void *               response_ctxt) = 0;

	virtual tnrcsp_result_t
		tnrcsp_reserve_xc(tnrcsp_handle_t *    handlep,
				  tnrc_port_id_t       portid_in,
				  label_t              labelid_in,
				  tnrc_port_id_t       portid_out,
				  label_t              labelid_out,
				  xcdirection_t        direction,
				  tnrc_boolean_t       isvirtual,
				  tnrcsp_response_cb_t response_cb,
				  void *               response_ctxt) = 0;

	virtual tnrcsp_result_t
		tnrcsp_unreserve_xc(tnrcsp_handle_t *    handlep,
				    tnrc_port_id_t       portid_in,
				    label_t              labelid_in,
				    tnrc_port_id_t       portid_out,
				    label_t              labelid_out,
				    xcdirection_t        direction,
				    tnrc_boolean_t       isvirtual,
				    tnrcsp_response_cb_t response_cb,
				    void *               response_ctxt) = 0;

	virtual tnrcsp_result_t
		tnrcsp_register_async_cb(tnrcsp_event_t *events) = 0;
};

class Pcontainer {
 public:
	Pcontainer(void) {};
	~Pcontainer(void){};

	bool     attach(std::string name, Plugin *p);
	bool     detach(std::string name);

	Plugin * getPlugin(std::string name);

	// Defines iterator_plugins
	DEFINE_PIN_MAP_ITERATOR(plugins, std::string, Plugin);

 private:
	std::map<std::string, Plugin *> plugins_;

};

extern "C" {

wq_item_status  workfunction    (struct work_queue *wq, void *d);
void            delete_item_data(struct work_queue *wq, void *d);

}


#endif /* TNRC_PLUGIN_H */
