//
//  This file is part of phosphorus-g2mpls.
//
//  Copyright (C) 2006, 2007, 2008, 2009 PSNC
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
//  Adam Kaliszan         (PSNC)             <kaliszan_at_man.poznan.pl>
//  Damian Parniewicz     (PSNC)             <damianp_at_man.poznan.pl>
//



#include <zebra.h>

#include "tnrcd.h"
#include "tnrc_sp.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_plugin_tl1_adva.h"

namespace TNRC_SP_TL1
{
  /*******************************************************************************************************************
  *
  *            ADVA UTILS
  *
  *******************************************************************************************************************/
 /** iterate over AP Data Model and return board id for first maching port 
      - assuming that there are not present the same ports on different boards */
  static
  board_id_t find_board_id(tnrc_portid_t portid)
  {

        TNRC::TNRC_AP                 * t;
        TNRC::Eqpt                    * e;
        TNRC::Board                   * b;
        TNRC::Port                    * pp;
        tnrc_port_id_t                  tnrc_port_id;
        eqpt_id_t                       eqpt_id;
        board_id_t                      board_id;

        port_id_t                       port_id;
        TNRC::TNRC_AP::iterator_eqpts   iter_e;
        TNRC::Eqpt::iterator_boards     iter_b;
        TNRC::Board::iterator_ports     iter_p;

        //Get instance
        t = TNRC_MASTER.getTNRC();
        if (t == NULL) {
                return TNRC_API_ERROR_NO_INSTANCE;
        }
        //Check the presence of an EQPT object
        if (t->n_eqpts() == 0) {
                return TNRC_API_ERROR_GENERIC;
        }
        //Fetch all data links in the data model
        iter_e = t->begin_eqpts();
        eqpt_id = (*iter_e).first;
        e = (*iter_e).second;
        if (e == NULL) {
                return TNRC_API_ERROR_GENERIC;
        }
        //Iterate over Abstract Part Data Model: boards and ports
        for(iter_b = e->begin_boards(); iter_b != e->end_boards(); iter_b++) {
                board_id = (*iter_b).first;
                b = (*iter_b).second;
                for(iter_p = b->begin_ports();
                    iter_p != b->end_ports();
                    iter_p++) {
                        port_id = (*iter_p).first;
                        pp = (*iter_p).second;

                        //Check if port found
                        if (port_id == portid)
                            return board_id;
                }
        }
        return 0;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** create AP port represtation by adding eqpt and board ids */
  static
  tnrc_portid_t create_adva_api_port_id(tnrc_portid_t portid)
  {
    board_id_t board_id = find_board_id(portid);
    return TNRC_CONF_API::get_tnrc_port_id(1/*eqpt_id*/, board_id, portid);
  }

  //-------------------------------------------------------------------------------------------------------------------
  inline tnrc_portid_t internal_port_id(tnrc_portid_t portid)
  {
    return portid & 0x0000FFFF;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** create unique port_id from "bay-shelf-slot[-port]" */
  tnrc_portid_t adva_compose_id(std::string* aid)
  {
    int bay, shelve, slot, port;
    int ret_value = sscanf(aid->c_str(), "%2i-%2i-%2i-%2i", &bay, &shelve, &slot, &port);
    if(ret_value < 3)
      return ((tnrc_portid_t) -1);
    int internal_port = (bay & 0xF)<<12 | (shelve & 0xF)<<8 | slot & 0xFF;  //("1-8-12") is transformed to 0x180C
    return create_adva_api_port_id(internal_port); // finally transformed to 0x0101180C
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** calculates aid from port_id */
  std::string* adva_decompose_id(tnrc_portid_t port_id)
  {
    int slot  = (port_id & 0x00FF) >>  0;
    int shelf = (port_id & 0x0F00) >>  8;
    int bay   = (port_id & 0xF000) >> 12;

    if(0<bay && bay<3 && 0<shelf && shelf<34 && 0<slot && slot<28)
    {
      char buff[15];
      sprintf(buff, "%d-%d-%d", bay,shelf,slot);
      std::string* aid = new std::string(buff);
      return aid;
    }
    else
    {
      TNRC_WRN("port_id cannot be decomposed!");
      return NULL;
    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** decompose full aid for ingress port */
  std::string* adva_decompose_in(tnrc_portid_t port_id)
  {
    std::string* aid = adva_decompose_id(port_id);
    if(!aid)
      return NULL;
    *aid += "-1"; // RX port=1
    return aid;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** decompose full aid for egress port */
  std::string* adva_decompose_out(tnrc_portid_t port_id)
  {
    std::string* aid = adva_decompose_id(port_id);
    if(!aid)
      return NULL;
    if(adva_get_port_type(port_id) == TNRCSP_LSC_OLD)
      *aid += "-2"; // OLD has TX port=2
    else
      *aid += "-1"; // XPT has always RX/TX port=1
    return aid;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** decompose aid and channel if aid contains also channel,
  *   for example: 1-1-3-33-56 => 1-1-3-33, 55
  */
  bool adva_decompose_chaid(std::string* chaid, channel_t* channel)
  {
    std::string tokens[5];
      if(match(chaid, "----", 5, tokens) < 4)
        return false;

    std::string bay    = tokens[0];
    std::string shelve = tokens[1];
    std::string slot   = tokens[2];
    std::string port   = tokens[3];

    channel_t _channel = atoi(tokens[4].c_str());

    char buff[15];
    sprintf(buff, "%s-%s-%s-%s", bay.c_str(), shelve.c_str(), slot.c_str(), port.c_str());
    std::string str(buff);
    *chaid = str;
    *channel = _channel;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** calculates slot from port id */
  int adva_get_slot(tnrc_portid_t port_id)
  {
    int slot = port_id & 0x000000FF;
    return (0<slot && slot<28) ? slot : -1;  // knowing device structure
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** calculates plane from port id */
  int adva_get_plane(tnrc_portid_t port_id)
  {
    int slot = adva_get_slot(port_id);
    return (slot<13) ? 0 : 1;  // knowing device structure
  }

  //-------------------------------------------------------------------------------------------------------------------
  tnrcsp_lsc_eqtype_t adva_get_port_type(tnrc_portid_t port_id)
  {
    /* get device type of resource */
    if(adva_get_slot(port_id) == 8 || adva_get_slot(port_id) == 13) // knowing device structure
      return TNRCSP_LSC_OLD; // 'Optical Line driver' - interface towards network
    else
      return TNRCSP_LSC_XCVR; // XCVR (Transceiver) -> add/drop interface
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** get crossconnection type for ADVA
  *  check if given crossconnect is possible in hardware
  *  don't care with already allocated crossconnections */
  tnrcsp_lsp_xc_type adva_check_xc_type(tnrc_portid_t port_in, tnrc_portid_t port_out)
  {
    port_in = internal_port_id(port_in);
    port_out = internal_port_id(port_out);
    tnrcsp_lsc_eqtype_t port_in_type = adva_get_port_type(port_in);
    tnrcsp_lsc_eqtype_t port_out_type = adva_get_port_type(port_out);
    int port_in_plane = adva_get_plane(port_in);
    int port_out_plane = adva_get_plane(port_out);
    if(port_in_plane != port_out_plane && port_in_type == TNRCSP_LSC_OLD && port_out_type == TNRCSP_LSC_OLD)
        return TNRCSP_LSC_PASSTHRU;
    if(port_in_plane == port_out_plane && port_in_type == TNRCSP_LSC_OLD && port_out_type == TNRCSP_LSC_XCVR)
        return TNRCSP_LSC_DROP;
    if(port_in_plane == port_out_plane && port_in_type == TNRCSP_LSC_XCVR && port_out_type == TNRCSP_LSC_OLD)
        return TNRCSP_LSC_ADD;
    TNRC_WRN("xc_type out of range");
    return TNRCSP_LSC_UNKNOWN;
  }

  //-------------------------------------------------------------------------------------------------------------------
  bool adva_check_channel(channel_t channel)
  {
    if (19<channel && channel<60) // channels are DWDM
      return true;
    return false;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** check if given resources are correct for xc making */
  bool adva_check_resources(tnrc_portid_t port_in, channel_t channel_in, tnrc_portid_t port_out, channel_t channel_out)
  {
    bool success;
    std::string* ingress_aid = adva_decompose_in(port_in);
    std::string* egress_aid = adva_decompose_out(port_out);
    tnrcsp_lsp_xc_type conn_type = adva_check_xc_type(port_in, port_out);
    if(!ingress_aid || !egress_aid || conn_type==TNRCSP_LSC_UNKNOWN || !adva_check_channel(channel_in) || !adva_check_channel(channel_out))
        success = false;
    else
        success = true;
    if (channel_in != channel_out)
        success = false;
    delete ingress_aid, egress_aid;
    return success;
  }

  //-------------------------------------------------------------------------------------------------------------------
  label_t adva_channel_to_label(channel_t channel)
  {
    /// From draft-otani-ccamp-gmpls-lambda-labels-01.txt
    ///    0                   1                   2                   3
    ///   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    ///   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ///   |Grid | C.S   |S|    Reserved   |              n                |
    ///   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ///   Grid = 1, C.S = 4, Reserved = 0, n = abs(channel-31), s = (channel>=31) ? 0 : 1
    label_t result;
    memset(&result, 0, sizeof(result));
    result.type = LABEL_LSC;
    result.value.lsc.wavelen = (channel>=31) ? 0x28000000 + (channel-31) : 0x29000000 + (31-channel);
    return result;
  }

  //-------------------------------------------------------------------------------------------------------------------
  channel_t adva_label_to_channel(label_t label)
  {
    /// From draft-otani-ccamp-gmpls-lambda-labels-01.txt
    ///    0                   1                   2                   3
    ///    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    ///   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ///   |Grid | C.S   |S|    Reserved   |              n                |
    ///   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ///   Grid = 1, C.S = 4, Reserved = 0, channel = n+31 or 31-n
    int wavelen = label.value.lsc.wavelen;

    if((wavelen & 0xE0000000) >> 29 != 1)
    {
        TNRC_WRN("Unproper ITU-T frequenct grid value\n");
        return 0;
    }
    return ((wavelen & 0x01000000) == 0) ? 31 + (wavelen & 0x0000FFFF) : 31 - (wavelen & 0x0000FFFF);
  }

  //--------------------------------------------------------------------------------------------------------------------
  /** search description in the message, return True if description found */
  bool adva_error_analyze(std::string* message, std::string description)
  {
    if(!message)
      return false;
    return message->find(description) != -1;
  }

  //--------------------------------------------------------------------------------------------------------------------
  /** search description in the message and replace response if description found */
  tnrcsp_result_t adva_error_analyze2(std::string* message, std::string description, tnrcsp_result_t response, tnrcsp_result_t default_response)
  {
    if(!message)
      return TNRCSP_RESULT_INTERNALERROR;

    if(adva_error_analyze(message, description))
      return response;
    else
      return default_response;
  }

  /*******************************************************************************************************************
  *
  *            CLASS ADVA PLUGIN
  *
  *******************************************************************************************************************/

  /** add new fixed link parameters or modify existing fixed link parameters (usefull for AP Data model)*/ 
  void PluginAdvaLSC::tnrcsp_set_fixed_port(uint32_t key, int flags, g2mpls_addr_t rem_eq_addr, port_id_t rem_port_id, uint32_t no_of_wavelen, uint32_t max_bandwidth, gmpls_prottype_t protection)
  {
    fixed_port_map[key] = ((tnrcsp_adva_fixed_port_t){flags, rem_eq_addr, rem_port_id, no_of_wavelen, max_bandwidth, protection, false});
  }

  /** send login TL1 commnad
  * @param user   TL1 device login
  * @param passwd TL1 device password
  * @return 0 if login command send successful
  */
  int PluginAdvaLSC::tl1_log_in(std::string user, std::string passwd)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "ACT-USER::%s:%i::%s;", user.c_str(), ctag, passwd.c_str());
    std::string str(buff);
    asynch_send(&str, ctag, TNRCSP_ADVA_STATE_INIT);
    return 0;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** send equipment retrieve TL1 commnad */
  void PluginAdvaLSC::get_all_equipment()
  {
    /* send equipment retrieve TL1 commnad */
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "RTRV-EQPT-ALL:::%i;", ctag);
    std::string str(buff);
    asynch_send(&str, ctag, TNRCSP_ADVA_STATE_INIT);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** send channels retrieve TL1 commnad */
  void PluginAdvaLSC::get_all_xc()
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "RTRV-CHANNEL::ALL:%i;", ctag);
    std::string str(buff);
    asynch_send(&str, ctag, TNRCSP_ADVA_STATE_INIT);
  }

  //========================================================================================================================
  void PluginAdvaLSC::notify_event(std::string aid, channel_t channel, operational_state operstate, administrative_state adminstate)
  {
    /* notify if right event condition */
    bool success = false;
    label_t labelid;
    memset(&labelid, 0, sizeof(labelid));

    std::map<tnrc_sp_xc_key, xc_adva_params,xc_map_comp_lsc>::iterator iter;
    for(iter = xc_map.begin(); iter != xc_map.end(); iter++ )
    {
        xc_adva_params* params = &iter->second;
        if(params->channel != channel)
            continue;

        if( *(params->ingress_aid) == aid ||
            *(params->egress_aid)  == aid ||
            (params->direction == XCDIRECTION_BIDIR &&
                            (*(params->rev_ingress_aid) == aid ||
                            *(params->rev_egress_aid)  == aid )
            ))
        {
            // check if asynch notification exist and xc is in active state
            if(params->async_cb && params->activated)
            {
                TNRC_DBG("Adva notify via xconn callback");

                tnrcsp_lsc_resource_detail_t resource_det;
                resource_det.oper_state = operstate;
                resource_det.last_event = (operstate == UP) ? EVENT_RESOURCE_OPSTATE_UP : EVENT_RESOURCE_OPSTATE_DOWN;
                resource_det.admin_state = adminstate;


                labelid = adva_channel_to_label(channel);
                resource_det.labelid = labelid;

                tnrcsp_resource_id_t failed_resource;
                failed_resource.resource_list = list_new();

                failed_resource.portid = adva_compose_id(&aid);

                listnode_add(failed_resource.resource_list, &resource_det);

                // asynchronous callback function call
                tnrcsp_resource_id_t* ptr = &failed_resource;
                params->async_cb(params->handle, &ptr, params->async_cxt);

                success = true;
                break;         // only one xc match
            }
        }
    }
    if(success == false)
      register_event(adva_compose_id(&aid), channel, operstate, adminstate, 0);
  }

   //=======================================================================================================================
   void PluginAdvaLSC::register_event(tnrc_portid_t portid, 
                                      channel_t channel, 
				      operational_state operstate, 
                                      administrative_state adminstate,
				      tnrcsp_evmask_t e = 0)
  {
    TNRC_DBG("Adva notify via reqister callback: port=0x%x, channel=%d, operstate=%s, adminstate=%s", 
             portid, channel, SHOW_OPSTATE(operstate), SHOW_ADMSTATE(adminstate));
    // lack of xc but we must notify in another way
    tnrcsp_event_t event;
    event.event_list = list_new();
    event.portid = portid;

    if (find_board_id(event.portid&0xFFFF) == 0)
    {
      TNRC_DBG("Port not configured: port=0x%x, notification skipped", portid);
      return; //portid is not present in conf file
    }

    event.oper_state = operstate;
    event.admin_state = adminstate;
    if(channel != 0)
    {
      event.labelid = adva_channel_to_label(channel);
      if (e != 0)
        event.events = e;
      else
        event.events = (operstate == UP) ? EVENT_RESOURCE_OPSTATE_UP : EVENT_RESOURCE_OPSTATE_DOWN;
    }
    else
    {
      memset (&(event.labelid), 0, sizeof(label_t));
      if (e != 0)
        event.events = e;
      else
        event.events = (operstate == UP) ? EVENT_PORT_OPSTATE_UP : EVENT_PORT_OPSTATE_DOWN; // for DWDM port is equal 0
    }

    tnrcsp_register_async_cb(&event); 

    //save current equipment state
    std::map<tnrc_portid_t, tnrcsp_lsc_resource_detail_t>::iterator it2;
    it2 = resource_list.find(portid);
    if(it2 != resource_list.end())
    {
        it2->second.oper_state = operstate;
        it2->second.admin_state = adminstate;
        it2->second.last_event = event.events;
    }
    else
    {
      tnrcsp_lsc_resource_detail_t details = {adva_channel_to_label(channel), operstate, 
                                              adminstate, event.events, 
                                              (tnrcsp_lsc_eqtype_t)0, 0};
      resource_list[portid] = details;
    }
  }

  /*******************************************************************************************************************
  *
  *            ADVA CROSSCONNECT FINITE STATE MACHINES
  *
  *******************************************************************************************************************/

  void PluginAdvaLSC::trncsp_adva_xc_process(tnrcsp_adva_xc_state state, xc_adva_params* parameters, tnrcsp_result_t response, std::string* data)
  {
    tnrcsp_result_t result;
    bool call_callback = false;
    if(parameters->activity == TNRCSP_XC_MAKE)
    {
      if(state == TNRCSP_ADVA_STATE_INIT)
      {
        TNRC_DBG("Adding xconn to plugin database: port_in=0x%x, port_out=0x%x, channel=%d", 
	                      parameters->port_in, parameters->port_out, parameters->channel);
        parameters->activated = false;
	parameters->processed = true;
        tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out, parameters->channel};
        xc_map[key] = *parameters;
      }
      
      if(call_callback = trncsp_adva_make_xc(state, parameters, response, data, &result))
        if(result == TNRCSP_RESULT_NOERROR)
	{
	  parameters->processed = false;
	  parameters->activated = true;
	}
        else if(result != TNRCSP_RESULT_BUSYRESOURCES)
        {
          call_callback = false;
          parameters->final_result = true;
          parameters->result = result;
          TNRC_WRN("Make unsuccessful -> destroying");
          parameters->activity = TNRCSP_XC_DESTROY;
          trncsp_adva_destroy_xc(TNRCSP_ADVA_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
        }
    }
    else if(parameters->activity == TNRCSP_XC_DESTROY)
    {
      if(call_callback = trncsp_adva_destroy_xc(state, parameters, response, data, &result))
      {
        // updating activated param in both structures
        parameters->activated = false;
        tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out, parameters->channel};
        xc_map[key] = *parameters;

        if(!parameters->deactivate)
        {
          call_callback = false;
          parameters->activity = TNRCSP_XC_UNRESERVE;
          call_callback = trncsp_adva_unreserve_xc(TNRCSP_ADVA_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
        }
      }
    }
    else if(parameters->activity == TNRCSP_XC_RESERVE)
    {
      if(call_callback = trncsp_adva_reserve_xc(state, parameters, response, data, &result))
      {
        if(result == TNRCSP_RESULT_NOERROR)
        {
          parameters->activated = false;
          tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out, parameters->channel};
          xc_map[key] = *parameters;
        }
        else if(result != TNRCSP_RESULT_BUSYRESOURCES)
        {
          call_callback = false;
          parameters->final_result = true;
          parameters->result = result;
          TNRC_WRN("reservation unsuccessful -> unreserve resources\n");
          parameters->activity = TNRCSP_XC_UNRESERVE;
          call_callback = trncsp_adva_unreserve_xc(TNRCSP_ADVA_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
        }
      }
    }
    else if(parameters->activity == TNRCSP_XC_UNRESERVE)
    {
      if(call_callback = trncsp_adva_unreserve_xc(state, parameters, response, data, &result))
      {
        parameters->activated = false;
        tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out, parameters->channel};
        if (xc_map.count(key) > 0)
          xc_map.erase(key);
      }
    }
    if(call_callback && parameters->response_cb)
    {
      if(parameters->final_result == true) 
        result = parameters->result; // because of rollback action result is stored in parameters object
  
      TNRC_DBG("Adva-SP - xc notification to AP with %s", SHOW_TNRCSP_RESULT(result));
      parameters->response_cb(parameters->handle, result, parameters->response_ctx);
      delete parameters;
    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** make crossconnection */
  bool PluginAdvaLSC::trncsp_adva_make_xc(tnrcsp_adva_xc_state state, xc_adva_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("make state: %s", SHOW_TNRCSP_ADVA_XC_STATE(state));

    // TNRCSP_ADVA_STATE_INIT
    if(state == TNRCSP_ADVA_STATE_INIT)
    {
      if(!parameters->activate)
      {
        *result = assign_channel(parameters->ingress_aid, parameters->egress_aid, parameters->conn_type, TNRCSP_ADVA_STATE_ASSIGN_1, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
      else
      {
        *result = adva_restore_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_RESTORE_1, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
    }

    // TNRCSP_ADVA_STATE_ASSIGN_1
    else if(state == TNRCSP_ADVA_STATE_ASSIGN_1)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = adva_error_analyze2(data, "Channel has already been assigned", TNRCSP_RESULT_BUSYRESOURCES, response);
        
	// in case of virtual request the situation is expected
	// xconn is done thus we can finish make xc with success
	if (*result == TNRCSP_RESULT_BUSYRESOURCES && parameters->_virtual == true)
	{
	  TNRC_INF("MAKE_XC for resiliency: Xconn already installed");
	  *result = TNRCSP_RESULT_NOERROR;
	}
	
	return true;
      }
      if(parameters->direction == XCDIRECTION_BIDIR)
      {
        *result = assign_channel(parameters->rev_ingress_aid, parameters->rev_egress_aid, parameters->rev_conn_type, TNRCSP_ADVA_STATE_ASSIGN_2 , parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
      else
      {
        *result = adva_restore_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_RESTORE_1, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
    }

    // TNRCSP_ADVA_STATE_ASSIGN_2
    else if(state == TNRCSP_ADVA_STATE_ASSIGN_2)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = adva_error_analyze2(data, "Channel has already been assigned", TNRCSP_RESULT_BUSYRESOURCES, response);
        return true;
      }
      *result = adva_restore_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_RESTORE_1, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }

    // TNRCSP_ADVA_STATE_RESTORE_1
    else if(state == TNRCSP_ADVA_STATE_RESTORE_1)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        if(parameters->repeat_count < 5)
	{
          parameters->repeat_count += 1;
	  TNRC_DBG("Repeat xc Restore 1");
	  *result = adva_restore_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_RESTORE_1 , parameters);
          if(*result != TNRCSP_RESULT_NOERROR)
            return true;
	  return false;
	}
        *result = response;
        return true;
      }
      
      TNRC_DBG("Restore 1 successful");
      parameters->repeat_count = 0;
      if(parameters->direction == XCDIRECTION_BIDIR)
      {
        
        *result = adva_restore_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_RESTORE_2 , parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
      else
      {
        *result = adva_retrieve_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_CHECK_1, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
    }

    // TNRCSP_ADVA_STATE_RESTORE_2
    else if(state == TNRCSP_ADVA_STATE_RESTORE_2)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        if(parameters->repeat_count < 5)
	{
	  TNRC_DBG("Repeat xc Restore 2");
          parameters->repeat_count += 1;
	  *result = adva_restore_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_RESTORE_2 , parameters);
          if(*result != TNRCSP_RESULT_NOERROR)
            return true;
	  return false;
	}
	
        *result = response;
        return true;
      }
      
      TNRC_DBG("Restore 2 successful");
      parameters->repeat_count = 0;
      *result = adva_retrieve_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_CHECK_1, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    
    // TNRCSP_ADVA_STATE_EQUALIZE_1
    else if(state == TNRCSP_ADVA_STATE_EQUALIZE_1)
    {
	if(response != TNRCSP_RESULT_NOERROR)
	{
	    *result = response;
	    return true;
	}
	*result = adva_retrieve_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_CHECK_1, parameters);
	if(*result != TNRCSP_RESULT_NOERROR)
	    return true;
    }
    
    // TNRCSP_ADVA_STATE_EQUALIZE_2
    else if(state == TNRCSP_ADVA_STATE_EQUALIZE_2)
    {
	if(response != TNRCSP_RESULT_NOERROR)
	{
	    *result = response;
	    return true;
	}
	
	*result = adva_retrieve_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_CHECK_2, parameters);
	if(*result != TNRCSP_RESULT_NOERROR)
	    return true;
    }

    // TNRCSP_ADVA_STATE_CHECK_1
    else if(state == TNRCSP_ADVA_STATE_CHECK_1)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = response;
        return true;
      }

      // check if make-xc in successful condition
      std::string* tab;
      std::string tab1[] = {"Equalized-IS", ""};
      std::string tab2[] = {"EQ-Failure", "EQ-In-Progress", "EQ-Low", ""};
      std::string tab3[] = {"EQ-In-Progress", ""};
      tab = tab2;
      //tab = (ignore_equalization == true) ? tab2 : tab1;

      // check if timer event
      if(data == NULL)
      {
        TNRC_DBG("timer event");
        // check if equalize in progress or force additional equalization
        if(parameters->equalize_failed == false)
          *result = adva_retrieve_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_CHECK_1, parameters);
        else
	{
	  parameters->equalize_failed = false;
	  *result = adva_equalize_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_EQUALIZE_1, parameters);
	} 
	
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
	return false;
      }
      
      // response for request
      if(adva_retrieve_channel_resp(data, parameters->egress_aid, parameters, tab, "IS"))
      {
        // uncorrect condition of xc
	
        // check is made till 20 times
        if(parameters->repeat_count > 20)
        {
	  TNRC_DBG("Checking period reached the limit !!!");
          *result = TNRCSP_RESULT_GENERICERROR;
          return true;
        }

	// remember the equalization failure
	if(!adva_retrieve_channel_resp(data, parameters->egress_aid, parameters, tab3, "IS"))
	{
	  TNRC_DBG("equalization failed");
          parameters->equalize_failed = true;
	}

	// xc in not right condition; try again after 1 sec
        parameters->repeat_count += 1;
	TNRC_DBG("set timer event for CHECK_1");
	set_timer_event(TNRCSP_ADVA_STATE_CHECK_1, parameters, 1);
	return false;
      }
      else
      {
        TNRC_DBG("xc in right condition");
        // xc is in right condition; going to next operation
        if(parameters->direction == XCDIRECTION_BIDIR)
        {
	  parameters->repeat_count = 0;
	  //if(adva_retrieve_channel_resp(data, parameters->rev_egress_aid, parameters, tab3, "IS"))
          *result = adva_retrieve_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_CHECK_2, parameters);
          if(*result != TNRCSP_RESULT_NOERROR)
            return true;
        }
        else
        {
          *result = response;
          return true;
        }
      }
    }

    // TNRCSP_ADVA_STATE_CHECK_2
    else if(state == TNRCSP_ADVA_STATE_CHECK_2)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = response;
        return true;
      }

      // check if make-xc in successful condition
      std::string* tab;
      std::string tab1[] = {"Equalized-IS", ""};
      std::string tab2[] = {"EQ-Failure", "EQ-In-Progress", "EQ-Low", ""};
      tab = tab2;
      //tab = (ignore_equalization == true) ? tab2 : tab1;
      std::string tab3[] = {"EQ-In-Progress", ""};
      
      // check if timer event
      if(data == NULL)
      {
        TNRC_DBG("timer event");
        // check if equalize in progress or force additional equalization
        if(parameters->equalize_failed == false)
          *result = adva_retrieve_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_CHECK_2, parameters);
        else
	{ 
	  TNRC_DBG("event that equalization failed, repeat equalization");
	  parameters->equalize_failed = false;
	  *result = adva_equalize_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_EQUALIZE_2, parameters);
	}
        
	if(*result != TNRCSP_RESULT_NOERROR)
          return true;
	return false;
      }
      
      // response for request
      if(adva_retrieve_channel_resp(data, parameters->rev_egress_aid, parameters, tab, "IS"))
      {
        // check is made till 20 times
        if(parameters->repeat_count > 20)
        {
	  TNRC_DBG("Checking period reached the limit !!!");
          *result = TNRCSP_RESULT_GENERICERROR;
          return true;
        }


        // remember the equalization failure
	if(!adva_retrieve_channel_resp(data, parameters->rev_egress_aid, parameters, tab3, "IS"))
        {
	  TNRC_DBG("equalization failed");
	  parameters->equalize_failed = true;
        }	
	
 	// xc in not right condition; try again after 5 sec
        parameters->repeat_count += 1;
	TNRC_DBG("setting timer event");
	set_timer_event(TNRCSP_ADVA_STATE_CHECK_2, parameters, 5);
	return false;
      }
      else
      {
        TNRC_DBG("second xc direction in right state");
        *result = response;
        return true;
      }
    }
    return false;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** destroys crossconnection */
  bool PluginAdvaLSC::trncsp_adva_destroy_xc(tnrcsp_adva_xc_state state, xc_adva_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("destroy state: %s", SHOW_TNRCSP_ADVA_XC_STATE(state));

    // TNRCSP_ADVA_STATE_INIT
    if(state == TNRCSP_ADVA_STATE_INIT)
    {
      *result = adva_retrieve_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_CHECK_1, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true; // unsuccesful sending of retrieve request
    }

    // TNRCSP_ADVA_STATE_CHECK_1
    else if(state == TNRCSP_ADVA_STATE_CHECK_1)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        // unsuccesful response for retrieve request
        *result = response;
        return true;
      }
      if(adva_retrieve_channel_status(data, parameters->egress_aid, parameters))
      {
        // xc is in In-Service state
        *result = adva_remove_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_REMOVE_1, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true; // unsuccesful sending of remove request

        // after remove channel wait for oos autonomous message and change to TNRCSP_ADVA_STATE_OOS_1 state
        adva_autonomous_oos(parameters->egress_aid, TNRCSP_ADVA_STATE_OOS_1, parameters);
      }
      // xc is not in In-Service state so let's check if bidirectional
      else if(parameters->direction == XCDIRECTION_BIDIR)
      {
        *result = adva_retrieve_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_CHECK_2, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true; // unsuccesful sending of retrieve request
      }
      // xc is Out-of-Service or already deleted and unidirectional; anyway continue with unreserve
      else
      {
        *result = response;
        return true;
      }
    }

    // TNRCSP_ADVA_STATE_CHECK_2
    else if(state == TNRCSP_ADVA_STATE_CHECK_2)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        // unsuccesful response for retrieve request
        *result = response;
        return true;
      }

      if(adva_retrieve_channel_status(data, parameters->rev_egress_aid, parameters))
      {
        // xc is in In-Service state
        *result = adva_remove_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_REMOVE_2, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true; // unsuccesful sending of remove request

        // after remove channel wait for oos autonomous message and change to TNRCSP_ADVA_STATE_OOS_2 state
        adva_autonomous_oos(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_OOS_2, parameters);
      }
      else
      {
        // xc is Out-of-Service or already deleted; anyway continue with unreserve
        *result = response;
        return true;
      }
    }

    // TNRCSP_ADVA_STATE_REMOVE_1
    else if(state == TNRCSP_ADVA_STATE_REMOVE_1)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        // unsuccesful response for remove request
        *result = response;
        return true;
      }
      // NO ACTION HERE; waiting for oos autonomous message and change to TNRCSP_ADVA_STATE_OOS_1
    }

    // TNRCSP_ADVA_STATE_REMOVE_2
    else if(state == TNRCSP_ADVA_STATE_REMOVE_2)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        // unsuccesful response for remove request
        *result = response;
        return true;
      }
      // NO ACTION HERE; waiting for oos autonomous message and change to TNRCSP_ADVA_STATE_OOS_2
    }

    // TNRCSP_ADVA_STATE_OOS_1
    else if(state == TNRCSP_ADVA_STATE_OOS_1)
    {
      // first xc is OOS; check the second xc
      if(parameters->direction == XCDIRECTION_BIDIR)
      {
        *result = adva_retrieve_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_CHECK_2, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true; // unsuccesful sending of retrieve request
      }
      else
      {
        *result = TNRCSP_RESULT_NOERROR;
        return true;
      }
    }

    // TNRCSP_ADVA_STATE_OOS_2
    else if(state == TNRCSP_ADVA_STATE_OOS_2)
    {
      *result = TNRCSP_RESULT_NOERROR;
      return true;
    }

    return false;
  }
  //-------------------------------------------------------------------------------------------------------------------
  /** reserve crossconnection */
  bool PluginAdvaLSC::trncsp_adva_reserve_xc(tnrcsp_adva_xc_state state, xc_adva_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("reserve state: %s", SHOW_TNRCSP_ADVA_XC_STATE(state));
    
    if(state == TNRCSP_ADVA_STATE_INIT)
    {
      *result = assign_channel(parameters->ingress_aid, parameters->egress_aid, parameters->conn_type, TNRCSP_ADVA_STATE_ASSIGN_1, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_ADVA_STATE_ASSIGN_1)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = adva_error_analyze2(data, "Channel has already been assigned", TNRCSP_RESULT_BUSYRESOURCES, response);
	
	// in case of virtual request the situation is expected
	// xconn is done thus we can finish make xc with success
	if (*result == true && parameters->_virtual == true)
	{
	  TNRC_INF("MAKE_XC for resiliency: Xconn already installed");
	  *result = TNRCSP_RESULT_NOERROR;
	}
	  
        return true;
      }
      if(parameters->direction == XCDIRECTION_BIDIR)
      {
        *result = assign_channel(parameters->rev_ingress_aid, parameters->rev_egress_aid, parameters->rev_conn_type, TNRCSP_ADVA_STATE_ASSIGN_2 , parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
      else
      {
        *result = adva_retrieve_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_CHECK_1, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
    }
    else if(state == TNRCSP_ADVA_STATE_ASSIGN_2)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = adva_error_analyze2(data, "Channel has already been assigned", TNRCSP_RESULT_BUSYRESOURCES, response);
        return true;
      }
      *result = adva_retrieve_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_CHECK_1, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_ADVA_STATE_CHECK_1)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = response;
        return true;
      }
      std::string tab[] = {"Connected-OOS", "OOS-EqOOS", ""};
      if(!adva_retrieve_channel_resp(data, parameters->egress_aid, parameters, tab, "OOS"))
      {
        *result = TNRCSP_RESULT_GENERICERROR;
        return true;
      }
      if(parameters->direction == XCDIRECTION_BIDIR)
      {
        *result = adva_retrieve_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_CHECK_2, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
      else
      {
        *result = response;
        return true;
      }
    }
    else if(state == TNRCSP_ADVA_STATE_CHECK_2)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = response;
        return true;
      }
      std::string tab[] = {"Connected-OOS", "OOS-EqOOS", ""};
      if(!adva_retrieve_channel_resp(data, parameters->rev_egress_aid, parameters, tab, "OOS"))
      {
        *result = TNRCSP_RESULT_GENERICERROR;
        return true;
      }
      *result = response;
      return true;
    }
    return false;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** unreserve crossconnection */
  bool PluginAdvaLSC::trncsp_adva_unreserve_xc(tnrcsp_adva_xc_state state, xc_adva_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("unreserve state: %s", SHOW_TNRCSP_ADVA_XC_STATE(state));
    if(state == TNRCSP_ADVA_STATE_INIT)
    {
        *result = adva_delete_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_DELETE_1, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
            return true;
    }
    else if(state == TNRCSP_ADVA_STATE_DELETE_1)
    {
        if(parameters->direction == XCDIRECTION_BIDIR)
        {
            *result = adva_delete_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_DELETE_2, parameters);
            if(*result != TNRCSP_RESULT_NOERROR)
                return true;
        }
        else
        {
            *result = adva_retrieve_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_CHECK_1, parameters);
            if(*result != TNRCSP_RESULT_NOERROR)
                return true;
        }
    }
    else if(state == TNRCSP_ADVA_STATE_DELETE_2)
    {
        *result = adva_retrieve_channel(parameters->egress_aid, TNRCSP_ADVA_STATE_CHECK_1, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
    }
    else if(state == TNRCSP_ADVA_STATE_CHECK_1)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = response;
        return true;
      }
      if(parameters->direction == XCDIRECTION_BIDIR)
      {
        *result = adva_retrieve_channel(parameters->rev_egress_aid, TNRCSP_ADVA_STATE_CHECK_2, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
      else
      {
        std::string tab[] = {""};
        if(adva_retrieve_channel_resp(data, parameters->egress_aid, parameters, tab, ""))
        {
          *result = TNRCSP_RESULT_GENERICERROR;
          return true;
        }
        *result = response;
        return true;
      }
    }
    else if(state == TNRCSP_ADVA_STATE_CHECK_2)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = response;
        return true;
      }
      std::string tab[] = {""};
      if(adva_retrieve_channel_resp(data, parameters->rev_egress_aid, parameters, tab, ""))
      {
        *result = TNRCSP_RESULT_GENERICERROR;
        return true;
      }
      *result = response;
      return true;
    }
    return false;
  }

  /*******************************************************************************************************************
  *
  *            ADVA TL1 COMMANDS
  *
  *******************************************************************************************************************/

  //-------------------------------------------------------------------------------------------------------------------
  /** send channel creation command */
  tnrcsp_result_t PluginAdvaLSC::assign_channel(std::string* ingress_aid, std::string* egress_aid, tnrcsp_lsp_xc_type conn_type,
                                                tnrcsp_adva_xc_state state, xc_adva_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "ASG-CHANNEL:::%i::%s,%s,%i,%s;", ctag, ingress_aid->c_str(), egress_aid->c_str(), parameters->channel, SHOW_TNRCSP_LSP_XC_TYPE(conn_type));
    std::string command(buff);
    return asynch_send(&command, ctag, state, parameters);
  }

  //-------------------------------------------------------------------------------------------------------------------
  tnrcsp_result_t PluginAdvaLSC::adva_retrieve_channel(std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "RTRV-CHANNEL::%s:%i::%i;", egress_aid->c_str(), ctag, parameters->channel);
    std::string command(buff);
    return asynch_send(&command, ctag, state, parameters);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** send channel out-of-service command */
  tnrcsp_result_t PluginAdvaLSC::adva_remove_channel(std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "RMV-CHANNEL::%s:%i::%i;", egress_aid->c_str(), ctag, parameters->channel);
    std::string command(buff);
    return asynch_send(&command, ctag, state, parameters);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** send channel in-service command */
  tnrcsp_result_t PluginAdvaLSC::adva_restore_channel(std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "RST-CHANNEL::%s:%i::%i;", egress_aid->c_str(), ctag, parameters->channel);
    std::string command(buff);
    return asynch_send(&command, ctag, state, parameters);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** send channel delete command */
  tnrcsp_result_t PluginAdvaLSC::adva_delete_channel(std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "DLT-CHANNEL:::%i::,%s,%i;", ctag, egress_aid->c_str(), parameters->channel);
    std::string command(buff);
    return asynch_send(&command, ctag, state, parameters);
  }
  
  //-------------------------------------------------------------------------------------------------------------------
  /** send channel equalization command */
  tnrcsp_result_t PluginAdvaLSC::adva_equalize_channel(std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "INIT-EQ::%s:%i::%i;", egress_aid->c_str(), ctag, parameters->channel);
    std::string command(buff);
    return asynch_send(&command, ctag, state, parameters);
  }

  //-------------------------------------------------------------------------------------------------------------------
  void PluginAdvaLSC::adva_autonomous_oos(std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters)
  {
    int ctag = gen_identifier();
    autonomous_handle(ctag, egress_aid, "to Out-Of-Service", state, parameters); // need to be searched in autonomous messages
  }

  //-------------------------------------------------------------------------------------------------------------------
  bool PluginAdvaLSC::adva_retrieve_channel_resp(std::string* data, std::string* egress_aid, xc_adva_params* parameters, std::string esp_conn_statuses[], std::string esp_status)
  {
    std::string _data = *data;

    std::string conn_status, status;
    std::string tokens[9];

    if(match(&_data, ",:,,,,,,", 9, tokens) != 9)
      return false;

    if(esp_status.length() == 0)
      return true;            // if only xc listed return positive answer; xc statuses are not important

    conn_status = tokens[5];
    status = tokens[7];

    std::string esp_conn_status;
    for(int i=0; true; i++) //end condition -> look at break statement
    {
      esp_conn_status = esp_conn_statuses[i];
      if(esp_conn_status.length() == 0)
        break;              // end of esp_conn_statuses list
      if(esp_conn_status == conn_status && status.find(esp_status) != -1)
        return true;        // return positive answer because xc listed and statutes with espected values
    }
    return false;
  }

  //-------------------------------------------------------------------------------------------------------------------
  bool PluginAdvaLSC::adva_retrieve_channel_status(std::string* data, std::string* egress_aid, xc_adva_params* parameters)
  {
    std::string status;
    std::string tokens[8];

    if(match(data, ",:,,,,,", 8, tokens) != 8)
      return false;

    status = tokens[7];

    return status.find("IS") != -1; // return true if 'IS' else false
  }

  /*******************************************************************************************************************
  *
  *            ADVA UTILS
  *
  *******************************************************************************************************************/

  PluginAdvaLSC::PluginAdvaLSC (std::string name):   PluginTl1(name)
  {
    do_destroy_alien_xcs = false;
    listen_proc = new PluginAdvaLSC_listen(this);
    terminator = new char[3];
    terminator[0] = '>';
    terminator[1] = ';';
    terminator[2] = '<'; // probably ADVA's engineers bug - found in IP answer for ACT-USER
    no_of_term_chars = 3;
    received_buffer = "";
    xc_bidir_support_ = true; 
    ignore_alarms = true;
    ignore_equalization = true;
  }

  //-------------------------------------------------------------------------------------------------------------------
  PluginAdvaLSC::~PluginAdvaLSC (void)
  {
    close_connection();
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** Launch communication with device or its simulator
  * @param location <ipAddress>:<port>:<username>:<password>:<path_to_configuration_file> for communication with real device or <simulator> for device simulation
  */
  tnrcapiErrorCode_t
  PluginAdvaLSC::probe (std::string location)
  {
    TNRC::TNRC_AP *    t;
    std::string        err;
    tnrcapiErrorCode_t res;

    //check if a TNRC instance is running. If not, create e new one
    t = TNRC_MASTER.getTNRC();
    if (t == NULL)
    { //create a new instance
      res = TNRC_CONF_API::tnrcStart(err);
      if (res != TNRC_API_ERROR_NONE)
      {
        return res;
      }
    }

    in_addr remote_address;
    int remote_port;

    std::string strAddress="0.0.0.0";
    std::string strPort;
    std::string strPath;

    std::string separators(":");
    int stPos;
    int endPos;

    stPos = 0;
    endPos = location.find_first_of(separators);
    strAddress = location.substr(stPos, endPos-stPos);

    if (strAddress == "simulator")
    {
      TNRC_DBG("PluginAdvaLSC: connecting with adva simulator");
      sock = new TNRC_SP_TL1_ADVA_SIMULATOR::TL1_adva_simulator();
    }
    else
    {
      TNRC_DBG("PluginAdvaLSC address %s", strAddress.c_str());
      if (inet_aton(strAddress.c_str(), &remote_address) != 1)
      {
        TNRC_ERR("PluginAdvaLSC::probe: wrong IP address %s given string is: %s", inet_ntoa(remote_address), strAddress.c_str());
        return TNRC_API_ERROR_GENERIC;
      }

      if ((stPos = endPos+1) > location.length())
      {
        TNRC_ERR("PluginAdvaLSC::probe: wrong location descriotion: no port");
        return TNRC_API_ERROR_GENERIC;
      }
      endPos = location.find_first_of(separators, stPos);

      strPort = location.substr(stPos, endPos-stPos);

      TNRC_DBG("Plugin Port %s", strPort.c_str());
      if (sscanf(strPort.c_str(), "%d", &remote_port) != 1)
      {
        TNRC_ERR("PluginAdvaLSC::probe: wrong port (location = %s, readed port %d)", location.c_str(), remote_port);
        return TNRC_API_ERROR_GENERIC;
      }
      sock = new TL1_socket(remote_address, remote_port);
      TNRC_INF("PluginAdvaLSC::probe: sesion data: ip %s, port %d", inet_ntoa(remote_address), remote_port);
    }
    TL1_status = TNRC_SP_TL1_STATE_NO_SESSION;

    data_model_id        = 1;

    //login
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("PluginAdvaLSC::probe: wrong location descriotion: no path");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    login = location.substr(stPos, endPos-stPos);
    //password
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("PluginAdvaLSC::probe: wrong location descriotion: no path");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    password = location.substr(stPos, endPos-stPos);

    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("PluginAdvaLSC::probe: wrong location descriotion: no path");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    strPath = location.substr(stPos, endPos-stPos);

    retriveT (thread_add_timer (master, tl1_retrive_func, this, 0));

    TNRC_DBG("Plugin Adva configuration file %s", strPath.c_str());
    vty_read_config ((char *)strPath.c_str(), NULL);

    //Check the presence of an EQPT object
    if (t->n_eqpts() == 0) {
        return TNRC_API_ERROR_GENERIC;
    }

    return TNRC_API_ERROR_NONE;
  }

  //-------------------------------------------------------------------------------------------------------------------
  wq_item_status PluginAdvaLSC::wq_function (void *d)
  {
    TNRC_ERR("!!!!!!!!!!!! PluginAdvaLSC::wq_function function should be never executed");
    return WQ_SUCCESS;
  }

  //-------------------------------------------------------------------------------------------------------------------
  void
  PluginAdvaLSC::del_item_data(void *d)
  {
    TNRC_ERR("!!!!!!!!!!!! PluginAdvaLSC::del_item_data function should be never executed");

    tnrcsp_action_t		*data;

    //get work item data
    data = (tnrcsp_action_t *) d;
    //delete data structure
    delete data;
  }

  /*******************************************************************************************************************
  *
  *            ADVA SP API
  *
  *******************************************************************************************************************/

  tnrcsp_result_t PluginAdvaLSC::tnrcsp_make_xc(tnrcsp_handle_t * handlep,
                          tnrc_port_id_t portid_in,
                          label_t labelid_in,
                          tnrc_port_id_t portid_out,
                          label_t labelid_out,
                          xcdirection_t direction,
                          tnrc_boolean_t isvirtual,
                          tnrc_boolean_t activate,
                          tnrcsp_response_cb_t response_cb,
                          void * response_ctxt,
                          tnrcsp_notification_cb_t async_cb,
                          void * async_ctxt)
  {
    *handlep = gen_identifier();
    channel_t channel_in = adva_label_to_channel(labelid_in);
    channel_t channel_out = adva_label_to_channel(labelid_out);
    TNRC_DBG("Make XC: channel_in = 0x%x, channel_out = 0x%x, label_in = 0x%x, label_out = 0x%x", channel_in, channel_out, labelid_in.value.lsc.wavelen, labelid_out.value.lsc.wavelen);

    if (!adva_check_resources(portid_in, channel_in, portid_out, channel_out))
      return TNRCSP_RESULT_PARAMERROR;
    if(TL1_status != TNRC_SP_TL1_STATE_LOGGED_IN)
      return TNRCSP_RESULT_EQPTLINKDOWN;

    xc_adva_params* xc_params = new xc_adva_params(TNRCSP_XC_MAKE, *handlep,
                                portid_in, portid_out, channel_in,
                                direction, isvirtual, activate,
                                response_cb, response_ctxt, async_cb, async_ctxt);

    trncsp_adva_xc_process(TNRCSP_ADVA_STATE_INIT, xc_params);

    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  tnrcsp_result_t
  PluginAdvaLSC::tnrcsp_destroy_xc (tnrcsp_handle_t *handlep,
                                    tnrc_port_id_t portid_in,
                                    label_t labelid_in,
                                    tnrc_port_id_t portid_out,
                                    label_t labelid_out,
                                    xcdirection_t        direction,
                                    tnrc_boolean_t isvirtual,
                                    tnrc_boolean_t deactivate,
                                    tnrcsp_response_cb_t response_cb,
                                    void *response_ctxt)
  {
    *handlep = gen_identifier();
    channel_t channel_in = adva_label_to_channel(labelid_in);
    channel_t channel_out = adva_label_to_channel(labelid_out);
    TNRC_DBG("Destroy XC: channel_in = 0x%x, channel_out = 0x%x, label_in = 0x%x, label_out = 0x%x", channel_in, channel_out, labelid_in.value.lsc.wavelen, labelid_out.value.lsc.wavelen);

    if (!adva_check_resources(portid_in, channel_in, portid_out, channel_out))
        return TNRCSP_RESULT_PARAMERROR;
    if(TL1_status != TNRC_SP_TL1_STATE_LOGGED_IN)
        return TNRCSP_RESULT_EQPTLINKDOWN;


    xc_adva_params* xc_params = new xc_adva_params(TNRCSP_XC_DESTROY, *handlep,
                                portid_in, portid_out, channel_in,
                                direction,  isvirtual, deactivate,
                                response_cb, response_ctxt, NULL, NULL);
    trncsp_adva_xc_process(TNRCSP_ADVA_STATE_INIT, xc_params);
    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  tnrcsp_result_t
  PluginAdvaLSC::tnrcsp_reserve_xc (tnrcsp_handle_t *handlep,
                                            tnrc_port_id_t portid_in,
                                            label_t labelid_in,
                                            tnrc_port_id_t portid_out,
                                            label_t labelid_out,
                                            xcdirection_t        direction,
				            tnrc_boolean_t       isvirtual,
                                            tnrcsp_response_cb_t response_cb,
                                            void *response_ctxt)
  {
    *handlep = gen_identifier();
    channel_t channel_in  = adva_label_to_channel(labelid_out);
    channel_t channel_out = adva_label_to_channel(labelid_out);

    TNRC_DBG("Reserce XC: channel_in = 0x%x, channel_out = 0x%x, label_in = 0x%x, label_out = 0x%x", channel_in, channel_out, labelid_in.value.lsc.wavelen, labelid_out.value.lsc.wavelen);

    if (!adva_check_resources(portid_in, channel_in, portid_out, channel_out))
      return TNRCSP_RESULT_PARAMERROR;
    if(TL1_status != TNRC_SP_TL1_STATE_LOGGED_IN)
      return TNRCSP_RESULT_EQPTLINKDOWN;

    tnrc_boolean_t _virtual = 0;

    xc_adva_params* xc_params = new xc_adva_params (TNRCSP_XC_RESERVE, *handlep,
                                portid_in, portid_out, channel_in,
                                direction, _virtual, false,
                                response_cb, response_ctxt, NULL, NULL);
    trncsp_adva_xc_process(TNRCSP_ADVA_STATE_INIT, xc_params);
    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  tnrcsp_result_t
  PluginAdvaLSC::tnrcsp_unreserve_xc (tnrcsp_handle_t *handlep,
                                            tnrc_port_id_t portid_in,
                                            label_t labelid_in,
                                            tnrc_port_id_t portid_out,
                                            label_t labelid_out,
                                            xcdirection_t        direction,
				            tnrc_boolean_t       isvirtual,
                                            tnrcsp_response_cb_t response_cb,
                                            void *response_ctxt)
  {
    *handlep = gen_identifier();
    channel_t channel_in = adva_label_to_channel(labelid_in);
    channel_t channel_out = adva_label_to_channel(labelid_out);
    TNRC_DBG("Unreserve XC: channel_in = 0x%x, channel_out = 0x%x, label_in = 0x%x, label_out = 0x%x", channel_in, channel_out, labelid_in.value.lsc.wavelen, labelid_out.value.lsc.wavelen);

    if (!adva_check_resources(portid_in, channel_in, portid_out, channel_out))
        return TNRCSP_RESULT_PARAMERROR;
    if(TL1_status != TNRC_SP_TL1_STATE_LOGGED_IN)
        return TNRCSP_RESULT_EQPTLINKDOWN;

    tnrc_boolean_t _virtual = 0;

    xc_adva_params* xc_params = new xc_adva_params(TNRCSP_XC_UNRESERVE, *handlep,
                                    portid_in, portid_out, channel_in,
                                    direction, _virtual, false,
                                    response_cb, response_ctxt, NULL, NULL);
    trncsp_adva_xc_process(TNRCSP_ADVA_STATE_INIT, xc_params);
    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  tnrcsp_result_t PluginAdvaLSC::tnrcsp_register_async_cb (tnrcsp_event_t *events)
  {
    TNRC_DBG("Register callback: portid=0x%x, labelid=0x%x, operstate=%s, adminstate=%s, event=%s\n",
            events->portid, events->labelid.value.lsc.wavelen, SHOW_OPSTATE(events->oper_state), 
            SHOW_ADMSTATE(events->admin_state), SHOW_TNRCSP_EVENT(events->events));

    TNRC_OPSPEC_API::update_dm_after_notify	(events->portid, events->labelid, events->events);

    TNRC_DBG ("Data model updated");
    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** make resuorce list */
  tnrcsp_result_t PluginAdvaLSC::trncsp_adva_get_resource_list(tnrcsp_resource_id_t** resource_listp, int* num)
  {
    /* return all available resources; resource_id is composed using aid and channel */
    if(TL1_status != TNRC_SP_TL1_STATE_LOGGED_IN)
        return TNRCSP_RESULT_EQPTLINKDOWN;

    *num = resource_list.size();
    *resource_listp = new tnrcsp_resource_id_t[*num];

    int i =0;
    int j, no_of_res;
    std::map<tnrc_portid_t, tnrcsp_lsc_resource_detail_t>::iterator it;

    for(it = resource_list.begin(); it != resource_list.end(); it++)
    {
        (*resource_listp)[i].portid = it->first;
        (*resource_listp)[i].resource_list=list_new();
        (*resource_listp)[i].resource_list->del = delete_adva_resource_node;

        //writing resource list/ Please remember to delete the list elements
        no_of_res = 1;
        for (j=0; j<no_of_res; j++)
        {
          tnrcsp_resource_detail_t *res = new tnrcsp_resource_detail_t;
          res->labelid     =  it->second.labelid;
          res->oper_state  =  it->second.oper_state;
          res->admin_state =  it->second.admin_state;
          res->last_event  =  it->second.last_event;

          listnode_add((*resource_listp)[i].resource_list, res);
        }
        i++;
    }
    return TNRCSP_RESULT_NOERROR;
  }

  /*******************************************************************************************************************
  *
  *            CLASS ADVA LISTENER
  *
  *******************************************************************************************************************/

  PluginAdvaLSC_listen::PluginAdvaLSC_listen() : PluginTl1_listen()
  {
  }

  //-------------------------------------------------------------------------------------------------------------------
  PluginAdvaLSC_listen::PluginAdvaLSC_listen(PluginAdvaLSC *owner) : PluginTl1_listen()
  {
    plugin = owner;
    pluginAdvaLsc = owner;
  }

  //-------------------------------------------------------------------------------------------------------------------
  PluginAdvaLSC_listen::~PluginAdvaLSC_listen()
  {

  }

  //-------------------------------------------------------------------------------------------------------------------
  /** process successful responses */
  void PluginAdvaLSC_listen::process_successful(std::string* response_block, std::string* command, int ctag)
  {
    PluginTl1_listen::process_successful(response_block, command, ctag); //TODO
    if(response_block->find( ">") == -1) //message is parted
    {
      if(command->find("ACT-USER") != -1)
      {
        plugin->TL1_status = TNRC_SP_TL1_STATE_LOGGED_IN; // positive response for login command
        TNRC_INF("Logged in\n");
        thread_cancel(plugin->t_retrive);
        plugin->retriveT(thread_add_timer(master, tl1_retrive_func, plugin, 0));
      }
      else if(command->find("RTRV-EQPT-ALL") != -1)
        save_equipments(*response_block);
      else if(command->find("RTRV-CHANNEL::ALL") != -1)
      {
        save_xcs(*response_block);
        if(pluginAdvaLsc->do_destroy_alien_xcs == true)
            destroy_alien_xcs();
      }
    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** process denied responses */
  void PluginAdvaLSC_listen::process_denied(std::string* response_block, std::string* command, int ctag)
  {
    PluginTl1_listen::process_denied(response_block, command, ctag);  //TODO
    if(command->find("ACT-USER") != -1)
    {
        plugin->TL1_status = TNRC_SP_TL1_STATE_NOT_LOGGED_IN; // positive response for login command
        TNRC_WRN("Login unsuccessful");
    }
  };

  //-------------------------------------------------------------------------------------------------------------------
  /** process TL1 autonomous message; information searching */
  void PluginAdvaLSC_listen::process_autonomous(std::string* mes, std::string* mes_code)
  {
    char _vmm1[10], _vmm2[10], _vmm3[10];
    _vmm1[9] = '\x00';_vmm2[9] = '\x00';_vmm3[9] = '\x00';
    int ret_value = sscanf(mes->c_str(), "%*s  %*i %9s %9s %9s", _vmm1, _vmm2, _vmm3);
    std::string vmm1(_vmm1), vmm2(_vmm2), vmm3(_vmm3);
    remove_line(mes);
    if(vmm1 == "REPT" && vmm2 == "EVT")
        process_event(mes_code, mes);
    else if(vmm1 == "REPT" && vmm2 == "ALM")
        process_alarm(mes_code, mes);
    else if(vmm1 == "REPT" && vmm2 == "DBGH") {}
    else
      TNRC_WRN("Not catalogized autonomous message");
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** process autonomous message with no alarm event */
  void PluginAdvaLSC_listen::process_event(std::string* alarm_code, std::string* mes)
  {
//    TNRC_DBG("Entering process event");
    std::string aid, event_type, srveff, cond_derscr;
    std::string tokens[8];

    if(match(mes, ":,,,,,:", 8, tokens) != 8)
        return;

    aid               = tokens[0];
    aid               = aid.substr(4); //remove 3 spaces and '/'
    event_type        = tokens[1];
    srveff            = tokens[2];
    cond_derscr       = tokens[7];
    channel_t channel = 0;

    adva_decompose_chaid(&aid, &channel);

    tnrc_sp_handle_item handler;
    bool found = false;
    std::map<const int, tnrc_sp_handle_item>::iterator iter;
    for(iter = handlers.begin(); iter != handlers.end(); iter++)
    {
      if(iter->second.aid == "") // not all handlers are searching autonomous messages
        continue;
      //search aid in autonomous block
      if(aid.find(iter->second.aid) == -1)
        continue;
      //search one of the given strings
      if(cond_derscr.find(iter->second.text) != -1)
      {
        handler = iter->second;
        handlers.erase(iter->first);
        found = true;
        break;
      }
    }
    if(found && handler.parameters)
    {
      pluginAdvaLsc -> trncsp_adva_xc_process((tnrcsp_adva_xc_state)handler.state, (xc_adva_params*)handler.parameters);
    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** process autonomous message with alarm event */
  void PluginAdvaLSC_listen::process_alarm(std::string* alarm_code, std::string* mes)
  {
    if(pluginAdvaLsc->ignore_alarms == true)
    {
      TNRC_INF("alarm is ignored");
      return;
    }

    std::string aid, ntfcncde, conditiontype, srveff, cond_desc;
    std::string tokens[7];
    if(match(mes, ":,,,,:", 7, tokens) != 7)
      return;

    aid                = tokens[0];
    aid                = aid.substr(4); //remove 3 spaces and '/'
    ntfcncde           = tokens[1];
    conditiontype      = tokens[2];
    srveff             = tokens[3];
    cond_desc          = tokens[6];
    channel_t channel = 0;

    adva_decompose_chaid(&aid, &channel);

    if(ntfcncde == "CL") // CL - alarm cleared
        pluginAdvaLsc->notify_event(aid, channel, UP, ENABLED);
    else if(ntfcncde == "CR" || ntfcncde == "MJ" || ntfcncde == "MN" ||  ntfcncde == "DEFAULT") // CR - critical, MJ - major, MN - minor
        pluginAdvaLsc->notify_event(aid, channel, DOWN, ENABLED);
    else if(ntfcncde == "DEFERRED" || ntfcncde == "IN" ||
      ntfcncde == "NR" || ntfcncde == "PROMPT"   || ntfcncde == "UN" ||
      ntfcncde == "WN" || ntfcncde == "NA")
    {
      TNRC_INF("not processed notification code: %s", ntfcncde.c_str());
    }
    else
    {
      TNRC_WRN("Warning: Unknown noftification code: %s", ntfcncde.c_str());
    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** process incoming TL1 message from ADVA */
  void PluginAdvaLSC_listen::process_xc(int state, void* parameters, tnrcsp_result_t result, std::string* response_block)
  {
    plugin->retrive_interwall=plugin->default_retrive_interwall;
    pluginAdvaLsc->trncsp_adva_xc_process((tnrcsp_adva_xc_state)state, (xc_adva_params*)parameters, result, response_block);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** store information with equipments details */
  void PluginAdvaLSC_listen::save_equipments(std::string response_block)
  {
    std::string             line = "";
    tnrc_portid_t           port_id;
    tnrcsp_lsc_eqtype_t     equip_type;
    tnrcsp_lsc_eqplane_t    equip_plane;
    operational_state       oper_state;
    administrative_state    admin_state;

    std::string aid, type_id, primarystate, secondarystate;
    channel_t channel;

    std::string tokens[9];

    std::map<tnrc_portid_t, tnrcsp_lsc_resource_detail_t>::iterator it;
    std::list<tnrcsp_lsc_xc_detail_t>::iterator it2;

    while(response_block.length() > 0)
    {
      line = pop_line(&response_block);

      if(match(&line, ":,,,,,,,", 9, tokens) != 9)
        continue;
      aid            = tokens[0];
      aid            = aid.substr(4); //remove 3 spaces and '/'
      type_id        = tokens[1];
      primarystate   = tokens[3];
      secondarystate = tokens[4];
      channel        = atoi(tokens[7].c_str());

      if(type_id.find("OLD") == -1 &&  type_id.find("XPT") == -1)
        continue;

      port_id        = adva_compose_id(&aid);
      oper_state     = (primarystate.find("IS") != -1) ? UP : DOWN;
      admin_state    = ENABLED;
      equip_plane    = adva_get_plane(port_id);

      if(type_id.find("OLD") != -1)
      {
        equip_type   = TNRCSP_LSC_OLD; // Optical Line Driver
        channel_t _channel = 0;
        pluginAdvaLsc->label_map[port_id] = _channel;
      }
      else if(type_id.find("XPT") != -1)
      {
        equip_type = TNRCSP_LSC_XCVR; // Transceiver / Transponder
        channel_t _channel = channel;
        pluginAdvaLsc->label_map[port_id] = _channel;
      }

      /// update AP Data model
      int board;
      //tnrcsp_adva_map_fixed_port_t::iterator it;

      if (type_id.find("OLD") != -1 || type_id.find("XPT") != -1)
        board = 1;
      else
        board = 0;

      /// sent notification if state changed
      // sent notification if state changed
      if(pluginAdvaLsc->resource_list.count(port_id) == 0
        || oper_state != pluginAdvaLsc->resource_list[port_id].oper_state)
      {
        for(it2=pluginAdvaLsc->xc_list.begin(); it2!=pluginAdvaLsc->xc_list.end(); it2++)
          if(it2->portid_in == port_id || it2->portid_out == port_id)
          {
            if(it2->xc_state == TNRCSP_LSC_XCSTATE_BUSY)
              oper_state = DOWN; // port in busy state
            break;
          }

        pluginAdvaLsc->register_event(port_id, 0, oper_state, admin_state, 0);
      }

      //save current equipment state
      std::map<tnrc_portid_t, tnrcsp_lsc_resource_detail_t>::iterator it3;
      it3 = pluginAdvaLsc->resource_list.find(port_id);
      if(it3 != pluginAdvaLsc->resource_list.end())
      {
         it3->second.equip_type = equip_type;
         it3->second.equip_plane = equip_plane;
      }
      else
      {
        tnrcsp_lsc_resource_detail_t details = {adva_channel_to_label(channel), oper_state, 
                                               admin_state, EVENT_RESOURCE_OPSTATE_UP, 
                                               equip_type, equip_plane};
        pluginAdvaLsc->resource_list[port_id] = details;
      }
    }
    TNRC_INF("resource_list contains:");
    for(it = pluginAdvaLsc->resource_list.begin(); it != pluginAdvaLsc->resource_list.end(); it++)
    {
        tnrcsp_lsc_resource_detail_t d = it->second;
        TNRC_NOT("   ID=0x%x, OperState=%s, AdminState=%s, Event=%s, Type=%s, Plane=%i", 
                    it->first, SHOW_OPSTATE(d.oper_state),
                    SHOW_ADMSTATE(d.admin_state), SHOW_TNRCSP_EVENT(d.last_event),
                    SHOW_TNRCSP_LSP_EQPT_PLANE(d.equip_type), d.equip_plane);
    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** store information with xconn details */
  void PluginAdvaLSC_listen::save_xcs(std::string response_block)
  {
    std::string line = "";

    tnrc_portid_t port_in, port_out;
    tnrcsp_lsc_xc_state_t xc_state;

    std::string ingress_aid, egress_aid, conn_type, conn_status, status;
    channel_t channel;

    std::string tokens[11];

    std::list<tnrcsp_lsc_xc_detail_t> last_external; // a list with external xconns
    std::list<tnrcsp_lsc_xc_detail_t>::iterator ext_iter;

    /* copy last external xc list to new list
      futher, the currently existing external xc will be removed
      and list will contain external xc which deaseapered last period */
    std::list<tnrcsp_lsc_xc_detail_t>::iterator it;
    for(it=pluginAdvaLsc->xc_list.begin(); it!=pluginAdvaLsc->xc_list.end(); it++ )
      if(it->xc_state == TNRCSP_LSC_XCSTATE_BUSY)
        last_external.insert(last_external.end(), *it);

    pluginAdvaLsc->xc_list.clear();

    while(response_block.length() > 0)
    {
      line = pop_line(&response_block);
      
      if(match(&line, ",:,,,,,,", 9, tokens) != 9)
         continue;
	
      ingress_aid = tokens[0];
      ingress_aid = ingress_aid.substr(4); //remove 3 spaces and '/'
      egress_aid =  tokens[1];
      conn_type =   tokens[4];
      conn_status = tokens[5];
      status =      tokens[7];
      channel =     atoi(tokens[2].c_str());

      port_in =     adva_compose_id(&ingress_aid);
      port_out =    adva_compose_id(&egress_aid);
      
      //TNRC_DBG("XC status is %s, conn_status is %s", status.c_str(), conn_status.c_str());
      if(status == "OOS" && (conn_status == "Connected-OOS" || conn_status == "OOS-EqOOS"))
        xc_state = TNRCSP_LSC_XCSTATE_RESERVED;
      else if(status == "IS" && (conn_status == "Equalized-IS" || conn_status == "EQ-In-Progress"))
        xc_state = TNRCSP_LSC_XCSTATE_ACTIVE;
      else
        xc_state = TNRCSP_LSC_XCSTATE_FAILED;

      // check if port_in is present in config file
      if (find_board_id(port_in&0xFFFF) != 0 || find_board_id(port_out&0xFFFF) != 0)
      {
        TNRC_INF("Searching in db: In=0x%x, Out=0x%x, Channel=%d", port_in, port_out, channel);
             
        std::map<tnrc_sp_xc_key, xc_adva_params,xc_map_comp_lsc>::iterator it3, it4;
        tnrc_sp_xc_key xc_key1 = {port_in, port_out, channel};
        tnrc_sp_xc_key xc_key2 = {port_out, port_in, channel};
        it3 = pluginAdvaLsc->xc_map.find(xc_key1); // in xc_map contains only one of this two keys
        it4 = pluginAdvaLsc->xc_map.find(xc_key2);

        if(it3 == pluginAdvaLsc->xc_map.end() && it4 == pluginAdvaLsc->xc_map.end())
        {
          // externally created xconn - set busy flag
	      TNRC_INF("!!! External xconn found: In=0x%x, Out=0x%x, Channel=%d", port_in, port_out, channel);
    
          label_t labelid = adva_channel_to_label(channel);

          xc_state = TNRCSP_LSC_XCSTATE_BUSY;

          for(ext_iter=last_external.begin(); ext_iter!=last_external.end(); ext_iter++ )
          {
            if(ext_iter->portid_in == port_in && ext_iter->portid_out == port_out)
            {
	        TNRC_DBG("Erasing current external connection from previous external connection list !");
                last_external.erase(ext_iter);
                break;
            }
          }
          tnrcsp_evmask_t event = (channel == 0) ? EVENT_PORT_XCONNECTED : EVENT_RESOURCE_XCONNECTED;
          pluginAdvaLsc->register_event(port_in, channel, UP, ENABLED, event);
          pluginAdvaLsc->register_event(port_out, channel, UP, ENABLED, event);
        }
        else
          TNRC_DBG("Found in db");
      }
      else
        TNRC_DBG("Port_in not configured!");
      
      
      //TNRC_DBG("adding xc to list");
      tnrcsp_lsc_xc_detail_t details = {port_in, port_out, channel, xc_state};
      pluginAdvaLsc->xc_list.insert(pluginAdvaLsc->xc_list.end(), details);
    }
    
    for(ext_iter=last_external.begin(); ext_iter!=last_external.end(); ext_iter++ )
    {
      // list of ports for which external xconn has disappeared
      TNRC_INF("External xconn deletion detected: In=0x%x, Out=0x%x, Channel=%d", 
               ext_iter->portid_in, ext_iter->portid_out, ext_iter->channel);
      tnrcsp_evmask_t event = (channel == 0) ? EVENT_PORT_XDISCONNECTED : EVENT_RESOURCE_XDISCONNECTED;
      pluginAdvaLsc->register_event(ext_iter->portid_in, ext_iter->channel, UP, ENABLED, event);
      pluginAdvaLsc->register_event(ext_iter->portid_out, ext_iter->channel, UP, ENABLED, event);
    };

    TNRC_INF("xc_list contains:");
    for(it=pluginAdvaLsc->xc_list.begin(); it!=pluginAdvaLsc->xc_list.end(); it++ )
    {
      TNRC_INF("   In=0x%x, Out=0x%x, Channel=%d, State=%s", it->portid_in, it->portid_out,
                                                               it->channel,
                                                               SHOW_TNRCSP_XC_STATE(it->xc_state));
    };
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** delete unrecognized crossconnections listed on device */
  void PluginAdvaLSC_listen::destroy_alien_xcs()
  {
    std::list<tnrcsp_lsc_xc_detail_t>::iterator it;
    tnrcsp_handle_t handlep = 0;

    for(it=pluginAdvaLsc->xc_list.begin(); it!=pluginAdvaLsc->xc_list.end(); it++ )
    {
      tnrc_sp_xc_key key = {it->portid_in, it->portid_out, it->channel};
      if(pluginAdvaLsc->xc_map.count(key) == 0)
      {
          pluginAdvaLsc->tnrcsp_destroy_xc (&handlep,
                            it->portid_in,
                            adva_channel_to_label(it->channel),
                            it->portid_out,
                            adva_channel_to_label(it->channel),
                            XCDIRECTION_UNIDIR,
                            false, false, NULL, NULL);
      }
    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** if synch function call wait more then 120 sec release all TL1 request waiting for response */
  void PluginAdvaLSC_listen::check_func_waittime()
  {
    std::map<const int, tnrc_sp_handle_item>::iterator iter;
    for(iter = handlers.begin(); iter != handlers.end(); iter++ )
      if (time(NULL) - iter->second.start > 120 /* secs */)
      {
        TNRC_WRN("Timeout!");
        xc_adva_params* parameters = (xc_adva_params*)iter->second.parameters;
        if(parameters->response_cb)
          parameters->response_cb(parameters->handle, TNRCSP_RESULT_GENERICERROR, parameters->response_ctx);
      }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** release all TL1 request waiting for response */
  void PluginAdvaLSC_listen::delete_func_waittime()
  {
    std::map<const int, tnrc_sp_handle_item>::iterator iter;
    for(iter = handlers.begin(); iter != handlers.end(); iter++ )
    {
      delete (xc_adva_params*)iter->second.parameters;
      handlers.erase(iter->first);
    }
  }

  /*******************************************************************************************************************
  *
  *            CLASS XC ADVA PARAMETERS
  *
  *******************************************************************************************************************/

  xc_adva_params::xc_adva_params(trncsp_xc_activity activity, tnrcsp_handle_t handle,
                        tnrc_portid_t port_in, tnrc_portid_t port_out, channel_t channel,
                        xcdirection_t direction, tnrc_boolean_t _virtual,
                        tnrc_boolean_t activate, tnrcsp_response_cb_t response_cb,
                        void* response_ctx, tnrcsp_notification_cb_t async_cb, void* async_cxt)
  {
    this->activity        = activity;
    this->handle          = handle;
    this->port_in         = port_in;
    this->port_out        = port_out;
    this->channel         = channel;
    this->direction       = direction;
    this->_virtual        = _virtual;
    this->activate        = activate;
    this->response_cb     = response_cb;
    this->response_ctx    = response_ctx;
    this->async_cb        = async_cb;
    this->async_cxt       = async_cxt;
    this->deactivate      = activate;
    this->conn_type       = adva_check_xc_type(port_in, port_out);
    
    // first must be ADD xc type (order of xc creation is important - first must be configured ADD then DROP)
    if(this->conn_type == TNRCSP_LSC_ADD || this->conn_type == TNRCSP_LSC_PASSTHRU)
    {
	this->ingress_aid     = adva_decompose_in(port_in);
	this->egress_aid      = adva_decompose_out(port_out);
	
	this->rev_conn_type   = adva_check_xc_type(port_out, port_in);
        this->rev_ingress_aid = adva_decompose_in(port_out);
        this->rev_egress_aid  = adva_decompose_out(port_in);
    }
    else
    {
        // the second must be DROP xc type
        this->conn_type       = adva_check_xc_type(port_out, port_in);
        this->ingress_aid     = adva_decompose_in(port_out);
	this->egress_aid      = adva_decompose_out(port_in);
        this->rev_conn_type   = adva_check_xc_type(port_in, port_out);
        this->rev_ingress_aid = adva_decompose_in(port_in);
        this->rev_egress_aid  = adva_decompose_out(port_out);
    }
    
    // used for second part of the bidirectional xc
    
    this->activated       = false;
    this->processed       = false;
    this->repeat_count    = 0;
    this->equalize_failed = false;
    this->final_result    = false;
    this->result          = TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  xc_adva_params::xc_adva_params()
  {
    this->ingress_aid     = NULL;
    this->egress_aid      = NULL;
    this->rev_ingress_aid = NULL;
    this->rev_egress_aid  = NULL;
  }

  //-------------------------------------------------------------------------------------------------------------------
  xc_adva_params::~xc_adva_params()
  {
    if(ingress_aid)
        delete ingress_aid;
    //ingress_aid = NULL;

    if(egress_aid)
        delete egress_aid;
    egress_aid = NULL;

    if(rev_ingress_aid)
        delete rev_ingress_aid;
    rev_ingress_aid = NULL;

    if(rev_egress_aid)
        delete rev_egress_aid;
    rev_egress_aid = NULL;
  }

  //-------------------------------------------------------------------------------------------------------------------
  xc_adva_params& xc_adva_params::operator=(const xc_adva_params &from)
  {
    activity      = from.activity;
    handle        = from.handle;
    port_in       = from.port_in;
    channel       = from.channel;
    port_out      = from.port_out;
    direction     = from.direction;
    _virtual      = from._virtual;
    activate      = from.activate;
    response_cb   = from.response_cb;
    response_ctx  = from.response_ctx;
    async_cb      = from.async_cb;
    async_cxt     = from.async_cxt;
    deactivate    = from.activate;
    ingress_aid   = adva_decompose_in(from.port_in);
    egress_aid    = adva_decompose_out(from.port_out);
    conn_type     = adva_check_xc_type(from.port_in, from.port_out);
    // used for second part of the bidirectional xc
    rev_ingress_aid = adva_decompose_in(from.port_out);
    rev_egress_aid  = adva_decompose_out(from.port_in);
    rev_conn_type   = adva_check_xc_type(from.port_out, from.port_in);
    activated       = from.activated;
    processed       = from.processed;
    repeat_count    = from.repeat_count;
    equalize_failed = from.equalize_failed;
    final_result    = from.final_result;
    result          = from.result;
    return *this;
  }
}

//-------------------------------------------------------------------------------------------------------------------
void delete_adva_resource_node(void *node)
{
  tnrcsp_resource_detail_t *detail = (tnrcsp_resource_detail_t *) node;
  delete detail;
}
