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



#include <zebra.h>
#include <stdio.h>
#include <stdlib.h>

#include "tnrc_plugin.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"

wq_item_status
workfunction(struct work_queue *wq, void *d)
{
	wq_item_status    res;
	tnrcsp_action_t * data;
	Plugin          * p;

	data = (tnrcsp_action_t *) d;
	//get plugin for this action plugin
	p = data->plugin;
	//call plugin work item execution function
	res = p->wq_function(d);

	return res;
}

void
delete_item_data(struct work_queue *wq, void *d)
{
	tnrcsp_action_t * data;
	Plugin          * p;

	data = (tnrcsp_action_t *) d;
	//get plugin for this action plugin
	p = data->plugin;
	//call plugin work item execution function
	p->del_item_data(d);

}

///////////////////////////////////
///////Class Plugin///////////////
/////////////////////////////////

Plugin::Plugin(std::string name)
{
	name_ = name;
}

std::string
Plugin::name(void)
{
	return name_;
}

tnrcsp_handle_t
Plugin::new_handle(void)
{
	tnrcsp_handle_t h;

	h = handle_++;

	return h;
}

bool
Plugin::xc_bidir_support(void)
{
	return xc_bidir_support_;
}

///////////////////////////////////
///////Class Pcontainer///////////
/////////////////////////////////

bool
Pcontainer::attach(std::string name, Plugin * p)
{
	iterator_plugins                  iter;
	std::pair<iterator_plugins, bool> ret;

	//try to insert the Plugin object
	ret = plugins_.insert(std::pair<std::string, Plugin *> (name, p));
	if (ret.second == false) {
		TNRC_WRN("Plug-in with name %s is already present in "
			 "the container", name.c_str());
		return false;
	}

	return true;
}

bool
Pcontainer::detach(std::string name)
{
	iterator_plugins iter;

	iter = plugins_.find(name);
	if (iter == plugins_.end()) {
		TNRC_WRN ("Plugin %s is not registered", name.c_str());
		return false;
	}
	plugins_.erase(iter);

	return true;
}

Plugin*
Pcontainer::getPlugin(std::string name)
{
	iterator_plugins iter;

	iter = plugins_.find(name);
	if (iter == plugins_.end()) {
		TNRC_WRN ("Plugin %s is not registered", name.c_str());
		return NULL;
	}

	return (*iter).second;
}
