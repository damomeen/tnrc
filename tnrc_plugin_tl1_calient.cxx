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
#include "vty.h"

#include "tnrcd.h"
#include "tnrc_sp.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_plugin_tl1_calient.h"
#include "vty.h"
//#include "command.h" //in need to be comment. Conflict between zebra and std names

namespace TNRC_SP_TL1
{
  /*******************************************************************************************************************
  *
  *            CALIENT UTILS
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
  tnrc_portid_t create_calient_api_port_id(tnrc_portid_t portid)
  {
    board_id_t board_id = find_board_id(portid);
    return TNRC_CONF_API::get_tnrc_port_id(1/*eqpt_id*/, board_id, portid);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** change AP port representation (containing eqpt,board,port) to Calient SP port (containg segment,fibre,port) */
  inline tnrc_portid_t internal_port_id(tnrc_portid_t portid)
  {
    return portid & 0x0000FFFF;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** create unique port_id from "segment.fibre.port" 
  *  @param aid string in for of segment.fibre.port
  *  @return unique port_id (bits: 15-12 segment, 11-8 fibre, 7-0 port)
  */
  tnrc_portid_t calient_compose_id(std::string* aid)
  {
    int segment, fibre, port;
    int ret_value = sscanf(aid->c_str(), "%2i.%2i.%2i", &segment, &fibre, &port);
    if(ret_value < 3)
      return ((tnrc_portid_t) -1);
    //("1.1.2") is transformed to 0x1102
    int internal_port = (segment & 0xF)<<12 | (fibre & 0xF)<<8 | port & 0xFF;
    return create_calient_api_port_id(internal_port);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** creates string aid from port_id 
  *  @param  port_id (bits 15-12 segment, 11-8 fibre, 7-0 port)
  *  @return segment.fibre.port
  */
  std::string* calient_decompose_id(tnrc_portid_t port_id)
  {
    int port    = (port_id & 0x00FF) >>  0;
    int fibre   = (port_id & 0x0F00) >>  8;
    int segment = (port_id & 0xF000) >> 12;
    if(0<segment && segment<7 && 0<fibre && fibre<9 && 0<port && port<9)
    {
        char buff[16];
        sprintf(buff, "%d.%d.%d", segment,fibre,port);
        std::string* aid = new std::string(buff);
        return aid;
    }
    else
    {
        TNRC_WRN("Warning: port_id cannot be decomposed!\n");
        return NULL;
    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** decompose full aid for ingress port */
  std::string* calient_decompose_in(tnrc_portid_t port_id)
  {
    std::string* aid = calient_decompose_id(port_id);

    if(!aid)
      return NULL;
    return aid;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** decompose full aid for egress port */
  std::string* calient_decompose_out(tnrc_portid_t port_id)
  {
    std::string* aid = calient_decompose_id(port_id);
 
    if(!aid)
        return NULL;
    return aid;
  }

  //-------------------------------------------------------------------------------------------------------------------
  //TODO finish it
  /** check if given resources are correct for xc making */
  bool calient_check_resources(tnrc_portid_t port_in, tnrc_portid_t port_out)
  {
    //TODO finish it
    bool success;
    std::string* ingress_aid = calient_decompose_in(port_in);
    std::string* egress_aid = calient_decompose_out(port_out);

    if(!ingress_aid || !egress_aid)
        success = false;
    else
        success = true;
    delete ingress_aid, egress_aid;
    return success;
  }

  /*******************************************************************************************************************
  *
  *            CLASS CALIENT PLUGIN 
  *
  *******************************************************************************************************************/

  /** send login TL1 commnad */
  int PluginCalientFSC::tl1_log_in(std::string user, std::string passwd)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "ACT-USER::%s:%i::%s;", user.c_str(), ctag, passwd.c_str());
    std::string str(buff);
    asynch_send(&str, ctag, TNRCSP_CALIENT_STATE_INIT);
    return 0;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** send equipment retrieve TL1 commnad 
   *  each time all 8 fiebers from one segment are checked
   *  in order to check all equipment the method have to be 5 times invoked */
  void PluginCalientFSC::get_all_equipment()
  {
    char buff[100];
    static int seg = 1;
    static int fib = 1;
    static bool get_equipment_first_time = true;
    int ctag;

    if (get_equipment_first_time)
    {
      get_equipment_first_time = false;
      for (seg = 1; seg <= 5; seg++)
      {
        if (fixed_segments[seg] == true)
        {
          for (fib=1;fib<9;fib++)
          {
             if (fixed_fibers[seg][fib])
             {
               ctag = gen_identifier();
               sprintf(buff, "RTRV-PORT::%i.%i:%i::,;", seg,fib,ctag);;
               std::string str(buff);
               asynch_send(&str, ctag, TNRCSP_CALIENT_STATE_INIT);
             }
          }
        }
      }
      seg = 1;
      fib = 1;
    }
    else
    {
      TNRC_DBG("get_all_equipment: Seg = %d, Fib = %d", seg, fib);
      if (fixed_segments[seg] == true)
      {
        if (fixed_fibers[seg][fib])
        {
          ctag = gen_identifier();
          sprintf(buff, "RTRV-PORT::%i.%i:%i::,;", seg,fib,ctag);
          std::string str(buff);
          asynch_send(&str, ctag, TNRCSP_CALIENT_STATE_INIT);
        }
      }
      if (++fib == 9)
      {
        fib = 1;
        if (++seg == 6)
          seg = 1;
      }
    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** send channels retrieve TL1 commnad */
  void PluginCalientFSC::get_all_xc()
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "RTRV-CRS::,:%i;", ctag);
    std::string str(buff);
    asynch_send(&str, ctag, TNRCSP_CALIENT_STATE_INIT);
  }

  //========================================================================================================================
  void PluginCalientFSC::notify_event(std::string aid, operational_state operstate, administrative_state adminstate)
  {
    /* notify if right event condition */
    bool success = false;

    std::map<tnrc_sp_xc_key, xc_calient_params,xc_map_comp_lsc>::iterator iter;
    for(iter = xc_map.begin(); iter != xc_map.end(); iter++ )
    {
        xc_calient_params* params = &iter->second;
        if(*(params->ingress_aid) == aid)
        {
            if(params->async_cb && params->activated)
            {
                TNRC_DBG("Calient notify via xconn callback");

                tnrcsp_lsc_resource_detail_t resource_det;
                resource_det.oper_state = operstate;
                resource_det.last_event = (operstate == UP) ? EVENT_PORT_OPSTATE_UP : EVENT_PORT_OPSTATE_DOWN;
                resource_det.admin_state = adminstate;

                tnrcsp_resource_id_t failed_resource;
                failed_resource.resource_list = list_new();

                failed_resource.portid = calient_compose_id(&aid);
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
      register_event(calient_compose_id(&aid), operstate, adminstate, 0);
  }

   //=======================================================================================================================
   void PluginCalientFSC::register_event(tnrc_portid_t portid, 
                                         operational_state operstate, 
					 administrative_state adminstate,
					 tnrcsp_evmask_t e)
  {
    TNRC_DBG("Calient notify via reqister callback: port=0x%x, operstate=%s, adminstate=%s", 
             portid, SHOW_OPSTATE(operstate), SHOW_ADMSTATE(adminstate));
    // lack of xc but we must notify in another way
    tnrcsp_event_t event;
    event.event_list = list_new();
    event.portid = portid;

    label_t labelid;
    memset(&labelid, 0, sizeof(label_t));
    labelid.value.fsc.portId = 1;
    event.labelid = labelid;

    if (find_board_id(event.portid&0xFFFF) == 0)
    {
      TNRC_DBG("Port not configured: port=0x%x, notification skipped", portid);
      return; //portid is not present in conf file
    }

    event.oper_state = operstate;
    event.admin_state = adminstate;
    if (e == 0)
      event.events = (operstate == UP) ? EVENT_PORT_OPSTATE_UP : EVENT_PORT_OPSTATE_DOWN;
    else
      event.events = e;
    tnrcsp_register_async_cb(&event); 

    tnrcsp_lsc_resource_detail_t details = {labelid, operstate, adminstate, event.events, (tnrcsp_lsc_eqtype_t)0, 0};
    resource_list[portid] = details;
  }

  /*******************************************************************************************************************
  *
  *            CALIENT CROSSCONNECT FINITE STATE MACHINES
  *
  *******************************************************************************************************************/

  void PluginCalientFSC::trncsp_calient_xc_process(tnrcsp_calient_xc_state state, xc_calient_params* parameters, tnrcsp_result_t response, std::string* data)
  {
    TNRC_DBG("trncsp_calient_xc_process handled");
    tnrcsp_result_t result;
    bool call_callback = false;
    if(parameters->activity == TNRCSP_XC_MAKE)
    {
      if(call_callback = trncsp_calient_make_xc(state, parameters, response, data, &result))
        if(result == TNRCSP_RESULT_NOERROR)
        {
          parameters->activated = true;
          tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out};
          xc_map[key] = *parameters;
        }
        else if(result != TNRCSP_RESULT_BUSYRESOURCES)
        {
          call_callback = false;
          parameters->final_result = true;
          parameters->result = result;
          TNRC_WRN("Make unsuccessful -> destroying");
          parameters->activity = TNRCSP_XC_DESTROY;
          trncsp_calient_destroy_xc(TNRCSP_CALIENT_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
        }
    }
    else if(parameters->activity == TNRCSP_XC_DESTROY)
    {
      if(call_callback = trncsp_calient_destroy_xc(state, parameters, response, data, &result))
      {
        TNRC_DBG("channel removation completed");
        // updating activated param in both structures
        parameters->activated = false;
        tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out};
        xc_map[key] = *parameters;

        if(!parameters->deactivate)
        {
          call_callback = false;
          parameters->activity = TNRCSP_XC_UNRESERVE;
          TNRC_DBG("channel removation completed, unreserving");
          call_callback = trncsp_calient_unreserve_xc(TNRCSP_CALIENT_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
        }
      }
    }
    else if(parameters->activity == TNRCSP_XC_RESERVE)
    {
      if(call_callback = trncsp_calient_reserve_xc(state, parameters, response, data, &result))
      {
        if(result == TNRCSP_RESULT_NOERROR)
        {
          parameters->activated = false;
          tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out};
          xc_map[key] = *parameters;
        }
        else if(result != TNRCSP_RESULT_BUSYRESOURCES)
        {
          call_callback = false;
          parameters->final_result = true;
          parameters->result = result;
          TNRC_WRN("reservation unsuccessful -> unreserve resources\n");
          parameters->activity = TNRCSP_XC_UNRESERVE;
          call_callback = trncsp_calient_unreserve_xc(TNRCSP_CALIENT_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
        }
      }
    }
    else if(parameters->activity == TNRCSP_XC_UNRESERVE)
    {
      if(call_callback = trncsp_calient_unreserve_xc(state, parameters, response, data, &result))
      {
        tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out};
        if (xc_map.count(key) > 0)
          xc_map.erase(key);
      }
    }
    if(call_callback && parameters->response_cb)
    {
        if(parameters->final_result == true) 
          result = parameters->result; // because of rollback action result is stored in parameters object

        TNRC_DBG("Calient-SP - xc notification to AP with %s", SHOW_TNRCSP_RESULT(result));
        parameters->response_cb(parameters->handle, result, parameters->response_ctx);
    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** make crossconnection */ 
  bool PluginCalientFSC::trncsp_calient_make_xc(tnrcsp_calient_xc_state state, xc_calient_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("make state: %s", SHOW_TNRCSP_CALIENT_XC_STATE(state));
    if(state == TNRCSP_CALIENT_STATE_INIT)
    {
      if(!parameters->activate)
      {
        TNRC_DBG("Calient provision XC");
        *result = calient_provision_xc(parameters->ingress_aid, parameters->egress_aid, TNRCSP_CALIENT_STATE_ASSIGN, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
     else
      {
        TNRC_DBG("Calient restore XC");
        *result = calient_restore_xc(parameters->ingress_aid, parameters->egress_aid, TNRCSP_CALIENT_STATE_RESTORE, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
      }
    } 
    else if(state == TNRCSP_CALIENT_STATE_ASSIGN)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = calient_error_analyze2(data, "Invalid Connection", TNRCSP_RESULT_BUSYRESOURCES, response);
        if (*result == TNRCSP_RESULT_BUSYRESOURCES && parameters->_virtual == true)
	{
	  TNRC_INF("MAKE XC for resiliency: Xconn already installed");
	  *result = TNRCSP_RESULT_NOERROR;
	}
	return true;
      }
      TNRC_DBG("Calient restore XC");
      *result = calient_restore_xc(parameters->ingress_aid, parameters->egress_aid, TNRCSP_CALIENT_STATE_RESTORE, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_CALIENT_STATE_RESTORE)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = response;
        return true;
      }
      TNRC_DBG("Calient retrieve XC");
      *result = calient_retrieve_xc(parameters->ingress_aid, parameters->egress_aid, TNRCSP_CALIENT_STATE_CHECK, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_CALIENT_STATE_CHECK)
    {
      TNRC_DBG("Calient checking XC");
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = response;
        return true;
      }

      if(data == NULL)
      {
        TNRC_DBG("Making next xc retrieve");
        *result = calient_retrieve_xc(parameters->ingress_aid, parameters->egress_aid, TNRCSP_CALIENT_STATE_CHECK, parameters);
        TNRC_DBG("Message send to network"); 
        if(*result != TNRCSP_RESULT_NOERROR)
          return true;
        return false;

      }
      if(!calient_retrieve_xc_resp(data, parameters->ingress_aid, parameters->egress_aid, parameters, "IS", "IS", "OK"))
      {
        if (parameters->repeat_count < 10)
        {
          TNRC_DBG("Allocating %i time event", parameters->repeat_count);
          parameters->repeat_count++;
          set_timer_event(TNRCSP_CALIENT_STATE_CHECK, (void*) parameters, 1);
          return false;
        }
        else
        {   
          TNRC_DBG("XC state after 10sec in failure condition");
          *result = TNRCSP_RESULT_GENERICERROR;
          return true;
        }
      }
      *result = response;
      return true;

    }
    return false;
  } 

  //-------------------------------------------------------------------------------------------------------------------
  /** destroys crossconnection */
  bool PluginCalientFSC::trncsp_calient_destroy_xc(tnrcsp_calient_xc_state state, xc_calient_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("destroy state: %s", SHOW_TNRCSP_CALIENT_XC_STATE(state));
    if(state == TNRCSP_CALIENT_STATE_INIT)
    {
      TNRC_DBG("Calient remove XC");
      *result = calient_remove_xc(parameters->ingress_aid, parameters->egress_aid, TNRCSP_CALIENT_STATE_REMOVE, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_CALIENT_STATE_REMOVE)
    {
      *result = response;
      return true;
    }
    return false; 
  }
  //-------------------------------------------------------------------------------------------------------------------
  /** reserve crossconnection */
  bool PluginCalientFSC::trncsp_calient_reserve_xc(tnrcsp_calient_xc_state state, xc_calient_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("reserve state: %s", SHOW_TNRCSP_CALIENT_XC_STATE(state));
    if(state == TNRCSP_CALIENT_STATE_INIT)
    {
      *result = calient_provision_xc(parameters->ingress_aid, parameters->egress_aid, TNRCSP_CALIENT_STATE_ASSIGN, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_CALIENT_STATE_ASSIGN)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = calient_error_analyze2(data, "Invalid Connection", TNRCSP_RESULT_BUSYRESOURCES, response);
        return true;
      }

      *result = calient_retrieve_xc(parameters->ingress_aid, parameters->egress_aid, TNRCSP_CALIENT_STATE_CHECK, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;

    }
    else if(state == TNRCSP_CALIENT_STATE_CHECK)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = response;
        return true;
      }
      if(!calient_retrieve_xc_resp(data, parameters->ingress_aid, parameters->egress_aid, parameters, "UMA", "RDY", "OK"))
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
  bool PluginCalientFSC::trncsp_calient_unreserve_xc(tnrcsp_calient_xc_state state, xc_calient_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("unreserve state: %s", SHOW_TNRCSP_CALIENT_XC_STATE(state));
    if(state == TNRCSP_CALIENT_STATE_INIT)
    {
        TNRC_DBG("Calient delete XC");
        *result = calient_delete_xc(parameters->ingress_aid, parameters->egress_aid, TNRCSP_CALIENT_STATE_DELETE, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
            return true;
    }
    else if(state == TNRCSP_CALIENT_STATE_DELETE)
    {
        TNRC_DBG("Calient retrieve XC");
        *result = calient_retrieve_xc(parameters->ingress_aid, parameters->egress_aid, TNRCSP_CALIENT_STATE_CHECK, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
            return true;
    }
    else if(state == TNRCSP_CALIENT_STATE_CHECK)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = calient_error_analyze2(data, "Invalid Connection", TNRCSP_RESULT_NOERROR, response);
        return true;
      }
      *result = TNRCSP_RESULT_GENERICERROR;
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
  tnrcsp_result_t PluginCalientFSC::calient_provision_xc(std::string* ingress_aid,
                                                         std::string* egress_aid, 
                                                         tnrcsp_calient_xc_state state,
                                                         xc_calient_params* parameters)
  {
      /* send xc creation command */
    int ctag = gen_identifier();
    char buff[100];
    std::string xc_dir = (parameters->direction==XCDIRECTION_BIDIR) ? "2way" : "1way";
    sprintf(buff, "ENT-CRS::%s,%s:%i::,%s;",ingress_aid->c_str(),egress_aid->c_str(),ctag, xc_dir.c_str());
    std::string command(buff);
    return asynch_send(&command, ctag, state, parameters);
  }

  //-------------------------------------------------------------------------------------------------------------------
  tnrcsp_result_t PluginCalientFSC::calient_retrieve_xc(std::string* ingress_aid, std::string* egress_aid, tnrcsp_calient_xc_state state, xc_calient_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    std::string xc_dir = (parameters->direction==XCDIRECTION_BIDIR) ? "-" : ">";
    sprintf(buff, "RTRV-CRS::%s,%s:%i::%s%s%s;", ingress_aid->c_str(),egress_aid->c_str(),ctag,ingress_aid->c_str(),xc_dir.c_str(),egress_aid->c_str());
    std::string command(buff);
    return asynch_send(&command, ctag, state, parameters);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** send channel out-of-service command */
  tnrcsp_result_t PluginCalientFSC::calient_remove_xc(std::string* ingress_aid, std::string* egress_aid, tnrcsp_calient_xc_state state, xc_calient_params* parameters)
  {
    /* send xc out-of-service command */
    int ctag = gen_identifier();
    char buff[100];
    std::string xc_dir = (parameters->direction==XCDIRECTION_BIDIR) ? "-" : ">";
    sprintf(buff, "CANC-CRS::%s,%s:%i::%s%s%s;", ingress_aid->c_str(),egress_aid->c_str(),ctag,ingress_aid->c_str(),xc_dir.c_str(),egress_aid->c_str());
    std::string command(buff);
    return asynch_send(&command, ctag, state, parameters);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** send channel in-service command */
  tnrcsp_result_t PluginCalientFSC::calient_restore_xc(std::string* ingress_aid, std::string* egress_aid, tnrcsp_calient_xc_state state, xc_calient_params* parameters)
  {
    /* send xc in-service command */
    int ctag = gen_identifier();
    char buff[100];
    std::string xc_dir = (parameters->direction==XCDIRECTION_BIDIR) ? "-" : ">";
    sprintf(buff, "ACT-CRS::%s,%s:%i::%s%s%s;", ingress_aid->c_str(),egress_aid->c_str(), ctag,ingress_aid->c_str(),xc_dir.c_str(),egress_aid->c_str());
    std::string command(buff);
    return asynch_send(&command, ctag, state, parameters);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** send channel delete command */
  tnrcsp_result_t PluginCalientFSC::calient_delete_xc(std::string* ingress_aid, std::string* egress_aid, tnrcsp_calient_xc_state state, xc_calient_params* parameters)
  {
      /* send xc delete command */
    int ctag = gen_identifier();
    char buff[100];
    std::string xc_dir = (parameters->direction==XCDIRECTION_BIDIR) ? "-" : ">";
    sprintf(buff, "DLT-CRS::%s,%s:%i::,SYSTEM,%s%s%s;", ingress_aid->c_str(),egress_aid->c_str(), ctag,ingress_aid->c_str(),xc_dir.c_str(),egress_aid->c_str());
    std::string command(buff);
    return asynch_send(&command, ctag, state, parameters);
  }

  //-------------------------------------------------------------------------------------------------------------------
  bool PluginCalientFSC::calient_retrieve_xc_resp(std::string* data, std::string* ingress_aid, std::string* egress_aid, xc_calient_params* parameters, std::string esp_admstate, std::string esp_opstate, std::string esp_opcap)
  {
    std::string _data = *data;
    std::string line = "";
    std::string admin_state, oper_state, oper_capacity;

    while(_data.length() > 0)
    {
      line = pop_line(&_data);

      int pos = line.find(":");
      if(pos == -1)
        break;
      line.erase(0, pos+1);

      dict d;
      match_dict(&line, d);

      admin_state   = d["AS"];
      oper_state    = d["OS"];
      oper_capacity = d["OC"];

      if(admin_state==""||oper_state==""||oper_capacity=="")
        continue;

      return (admin_state == esp_admstate && oper_state == esp_opstate && oper_capacity == esp_opcap);
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------------------------
  /** search description in the message, return True if description found */
  bool calient_error_analyze(std::string* message, std::string description)
  {
    if(!message)
      return false;
    return message->find(description) != -1;
  }

  //--------------------------------------------------------------------------------------------------------------------
  /** search description in the message and replace response if description found */
  tnrcsp_result_t calient_error_analyze2(std::string* message, std::string description, tnrcsp_result_t response, tnrcsp_result_t default_response)
  {
    if(!message)
      return TNRCSP_RESULT_INTERNALERROR;

    if(calient_error_analyze(message, description))
      return response;
    else
      return default_response;
  }
  
  /*******************************************************************************************************************
  *
  *            CALIENT UTILS
  *
  *******************************************************************************************************************/

  PluginCalientFSC::PluginCalientFSC (std::string name):   PluginTl1(name)
  {
    listen_proc = new PluginCalientFSC_listen(this);
    terminator = new char[2];
    terminator[0] = '?';
    terminator[1] = ';';
    no_of_term_chars = 2;
    received_buffer = "";
    xc_bidir_support_ = true;

    int seg, fib;
    for (seg = 0; seg <= 5; seg ++)
    {
      fixed_segments[seg] = false;
      for (fib = 0; fib <= 8; fib ++)
        fixed_fibers[seg][fib] = false;

    }
  }

  //-------------------------------------------------------------------------------------------------------------------
  PluginCalientFSC::~PluginCalientFSC (void)
  {
    close_connection();
    no_of_term_chars = 0;
    delete []terminator;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** Launch communication with device or its simulator
   * @param location <ipAddress>:<port>:<path_to_configuration_file> for communication with real device or <simulator> for device simulation
   */
  tnrcapiErrorCode_t
  PluginCalientFSC::probe (std::string location)
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

    std::string strAddress;
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
      //sock = new TNRC_SP_TL1_CALIENT_SIMULATOR::TL1_calient_simulator();
    }
    else
    {
      TNRC_DBG("Plugin Calient address %s", strAddress.c_str());
      if (inet_aton(strAddress.c_str(), &remote_address) != 1)
      {
        TNRC_ERR("Plugin Calient::probe: wrong IP address %s given string is: %s", inet_ntoa(remote_address), strAddress.c_str());
        return TNRC_API_ERROR_GENERIC;
      }

      if ((stPos = endPos+1) > location.length())
      {
        TNRC_ERR("Plugin Calient::probe: wrong location description: no port");
        return TNRC_API_ERROR_GENERIC;
      }
      endPos = location.find_first_of(separators, stPos);

      strPort = location.substr(stPos, endPos-stPos);

      TNRC_DBG("Plugin Port %s", strPort.c_str());
      if (sscanf(strPort.c_str(), "%d", &remote_port) != 1)
      {
        TNRC_ERR("Plugin Calient::probe: wrong port (location = %s, readed port %d)", location.c_str(), remote_port);
        return TNRC_API_ERROR_GENERIC;
      }

      sock = new TL1_socket(remote_address, remote_port);

      TNRC_INF("Plugin Calient probe: sesion data: ip %s, port %d", inet_ntoa(remote_address), remote_port);
    }

    TL1_status = TNRC_SP_TL1_STATE_NO_SESSION;

    // getting login from config file
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("PluginAdvaLSC::probe: wrong location description: no path");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    login = location.substr(stPos, endPos-stPos);
    
    // getting password from config file
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("PluginAdvaLSC::probe: wrong location description: no path");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    password = location.substr(stPos, endPos-stPos);

    retriveT (thread_add_timer (master, tl1_retrive_func, this, 0));

    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Calient::probe: wrong location of description: no path");
      return TNRC_API_ERROR_GENERIC;
    }
    
    //getting eqpt config filename from config file 
    endPos = location.find_first_of(separators, stPos);
    strPath = location.substr(stPos, endPos-stPos);
    
    //read eqpuipment config file
    TNRC_DBG("Plugin Calient configuration file %s", strPath.c_str());
    vty_read_config ((char *)strPath.c_str(), NULL);
    TNRC_DBG("Calient extra information about ports readed");
    
    // Iterate over AP Data Model for registering eqpuipment info retrieving
    TNRC::Eqpt                    * e;
    TNRC::Board                   * b;
    TNRC::Port                    * pp;
    tnrc_port_id_t                  tnrc_port_id;
    eqpt_id_t                       eqpt_id;
    board_id_t                      board_id;
    port_id_t                       port_id;
    g2mpls_addr_t                   dl_id;
    TNRC::TNRC_AP::iterator_eqpts   iter_e;
    TNRC::Eqpt::iterator_boards     iter_b;
    TNRC::Board::iterator_ports     iter_p;

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
    
    //Iterate over AP Data Model
    for(iter_b = e->begin_boards(); iter_b != e->end_boards(); iter_b++) {
        board_id = (*iter_b).first;
        b = (*iter_b).second;
        for(iter_p = b->begin_ports();
            iter_p != b->end_ports();
            iter_p++) {
            port_id = (*iter_p).first;
            
            int fib, seg;
            seg = ((port_id>>12) & 0x0F);
            fib = ((port_id>>8) & 0x0F);

            // registering eqpt resources info retrieval
            fixed_segments[seg] = true;
            fixed_fibers[seg][fib] = true;
            TNRC_DBG("Registering (seg,fib) = (0x%x, 0x%x) for retrieving equipment", seg, fib);
        }
    }
    return TNRC_API_ERROR_NONE;
  }

  //-------------------------------------------------------------------------------------------------------------------
  wq_item_status PluginCalientFSC::wq_function (void *d)
  {
    TNRC_ERR("!!!!!!!!!!!! PluginCalientFSC::wq_function function should be never executed");
    return WQ_SUCCESS;	
  }

  //-------------------------------------------------------------------------------------------------------------------
  void
  PluginCalientFSC::del_item_data(void *d)
  {
    TNRC_ERR("!!!!!!!!!!!! PluginCalientFSC::del_item_data function should be never executed");

    tnrcsp_action_t		*data;

    //get work item data
    data = (tnrcsp_action_t *) d;
    //delete data structure
    delete data;
  }

  /*******************************************************************************************************************
  *
  *            CALIENT SP API
  *
  *******************************************************************************************************************/

  tnrcsp_result_t PluginCalientFSC::tnrcsp_make_xc(tnrcsp_handle_t * handlep, 
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
    TNRC_DBG("Make XC: portid_in = 0x%x, portid_out = 0x%x", portid_in, portid_out);
 
    if (!calient_check_resources(portid_in, portid_out))
      return TNRCSP_RESULT_PARAMERROR;
    if(TL1_status != TNRC_SP_TL1_STATE_LOGGED_IN)
      return TNRCSP_RESULT_EQPTLINKDOWN;

    xc_calient_params* xc_params = new xc_calient_params(TNRCSP_XC_MAKE, *handlep, 
                                portid_in, portid_out, 
                                direction, isvirtual, activate, 
                                response_cb, response_ctxt, async_cb, async_ctxt);

    trncsp_calient_xc_process(TNRCSP_CALIENT_STATE_INIT, xc_params);

    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  tnrcsp_result_t
  PluginCalientFSC::tnrcsp_destroy_xc (tnrcsp_handle_t *handlep,
                                     tnrc_port_id_t portid_in, 
                                     label_t labelid_in, 
                                     tnrc_port_id_t portid_out, 
                                     label_t labelid_out,
                                     xcdirection_t direction,
                                     tnrc_boolean_t isvirtual,	
                                     tnrc_boolean_t deactivate, 
                                     tnrcsp_response_cb_t response_cb,
                                     void *response_ctxt)
  {
    *handlep = gen_identifier();
    TNRC_DBG("Destroy XC: portid_in = 0x%x, portid_out = 0x%x", portid_in, portid_out);

    if (!calient_check_resources(portid_in, portid_out))
        return TNRCSP_RESULT_PARAMERROR;
    if(TL1_status != TNRC_SP_TL1_STATE_LOGGED_IN)
        return TNRCSP_RESULT_EQPTLINKDOWN;

    xc_calient_params* xc_params = new xc_calient_params(TNRCSP_XC_DESTROY, *handlep,
                                portid_in, portid_out,
                                direction,  isvirtual, deactivate,
                                response_cb, response_ctxt, NULL, NULL);
    trncsp_calient_xc_process(TNRCSP_CALIENT_STATE_INIT, xc_params);
    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  tnrcsp_result_t
  PluginCalientFSC::tnrcsp_reserve_xc (tnrcsp_handle_t *handlep, 
                                            tnrc_port_id_t portid_in, 
                                            label_t labelid_in, 
                                            tnrc_port_id_t portid_out, 
                                            label_t labelid_out, 
                                            xcdirection_t direction,
				            tnrc_boolean_t isvirtual,
                                            tnrcsp_response_cb_t response_cb,
                                            void *response_ctxt)
  {
    *handlep = gen_identifier();

    TNRC_DBG("Reserce XC: portid_in = 0x%x, portid_out = 0x%x", portid_in, portid_out);

    if (!calient_check_resources(portid_in, portid_out))
      return TNRCSP_RESULT_PARAMERROR;
    if(TL1_status != TNRC_SP_TL1_STATE_LOGGED_IN)
      return TNRCSP_RESULT_EQPTLINKDOWN;

    tnrc_boolean_t _virtual = 0;

    xc_calient_params* xc_params = new xc_calient_params (TNRCSP_XC_RESERVE, *handlep,
                                portid_in, portid_out,
                                direction, _virtual, false,
                                response_cb, response_ctxt, NULL, NULL);
    trncsp_calient_xc_process(TNRCSP_CALIENT_STATE_INIT, xc_params);
    return TNRCSP_RESULT_NOERROR;
  }
	
  //-------------------------------------------------------------------------------------------------------------------
  tnrcsp_result_t
  PluginCalientFSC::tnrcsp_unreserve_xc (tnrcsp_handle_t *handlep, 
                                            tnrc_port_id_t portid_in, 
                                            label_t labelid_in, 
                                            tnrc_port_id_t portid_out, 
                                            label_t labelid_out, 
                                            xcdirection_t direction,
					    tnrc_boolean_t isvirtual,
                                            tnrcsp_response_cb_t response_cb,
                                            void *response_ctxt)
  {
    *handlep = gen_identifier();
    TNRC_DBG("Unreserve XC: portid_in = 0x%x, portid_out = 0x%x", portid_in, portid_out);

    if (!calient_check_resources(portid_in, portid_out))
        return TNRCSP_RESULT_PARAMERROR;
    if(TL1_status != TNRC_SP_TL1_STATE_LOGGED_IN)
        return TNRCSP_RESULT_EQPTLINKDOWN;

    tnrc_boolean_t _virtual = 0;

    xc_calient_params* xc_params = new xc_calient_params(TNRCSP_XC_UNRESERVE, *handlep,
                                    portid_in, portid_out,
                                    direction, _virtual, false, 
                                    response_cb, response_ctxt, NULL, NULL);
    trncsp_calient_xc_process(TNRCSP_CALIENT_STATE_INIT, xc_params);
    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  tnrcsp_result_t PluginCalientFSC::tnrcsp_register_async_cb (tnrcsp_event_t *events)
  {
    TNRC_DBG("Register callback: portid=0x%x, operstate=%s, adminstate=%s, event=%s\n", 
             events->portid, SHOW_OPSTATE(events->oper_state), SHOW_ADMSTATE(events->admin_state),
             SHOW_TNRCSP_EVENT(events->events));

    TNRC_OPSPEC_API::update_dm_after_notify  (events->portid, events->labelid, events->events);

    TNRC_DBG ("Data model updated");
    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** make resuorce list */ 
  tnrcsp_result_t PluginCalientFSC::trncsp_calient_get_resource_list(tnrcsp_resource_id_t** resource_listp, int* num)
  {
    /* return all available resources; resource_id is composed using aid and channel */
    if(TL1_status != TNRC_SP_TL1_STATE_LOGGED_IN)
        return TNRCSP_RESULT_EQPTLINKDOWN;

    *num = resource_list.size();
    *resource_listp = new tnrcsp_resource_id_t[*num];
    
    int i=0;
    std::map<tnrc_portid_t, tnrcsp_lsc_resource_detail_t>::iterator it;
    for(it = resource_list.begin(); it != resource_list.end(); it++)
    {
        (*resource_listp)[i].portid = it->first;
        (*resource_listp)[i].resource_list=list_new();
        i++;
    }
    return TNRCSP_RESULT_NOERROR;
  }

  /*******************************************************************************************************************
  *
  *            CLASS CALIENT LISTENER
  *
  *******************************************************************************************************************/

  PluginCalientFSC_listen::PluginCalientFSC_listen() : PluginTl1_listen()
  {
  }

  //-------------------------------------------------------------------------------------------------------------------
  PluginCalientFSC_listen::PluginCalientFSC_listen(PluginCalientFSC *owner) : PluginTl1_listen()
  {
    plugin = owner;
    pluginCalientFsc = owner;
  }

  //-------------------------------------------------------------------------------------------------------------------
  PluginCalientFSC_listen::~PluginCalientFSC_listen()
  {

  }

  //-------------------------------------------------------------------------------------------------------------------
  /** process successful responses */
  void PluginCalientFSC_listen::process_successful(std::string* response_block, std::string* command, int ctag)
  {
    PluginTl1_listen::process_successful(response_block, command, ctag); //TODO
    //if(response_block->find( ">") == -1) //message is parted
    //{ 
      if(command->find("ACT-USER") != -1)
      { 
        plugin->TL1_status = TNRC_SP_TL1_STATE_LOGGED_IN; // positive response for login command
        TNRC_INF("Logged in\n");
        thread_cancel(plugin->t_retrive);
        plugin->retriveT(thread_add_timer(master, tl1_retrive_func, plugin, 1));
      }
      else if(command->find("RTRV-PORT") != -1)
        save_equipments(*response_block);
      else if(command->find("RTRV-CRS") != -1)
        save_xcs(*response_block);
    //}
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** process denied responses */
  void PluginCalientFSC_listen::process_denied(std::string* response_block, std::string* command, int ctag)
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
  void PluginCalientFSC_listen::process_autonomous(std::string* mes, std::string* mes_code)
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
  void PluginCalientFSC_listen::process_event(std::string* alarm_code, std::string* mes)
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
      pluginCalientFsc -> trncsp_calient_xc_process((tnrcsp_calient_xc_state)handler.state, (xc_calient_params*)handler.parameters);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** process autonomous message with alarm event */
  void PluginCalientFSC_listen::process_alarm(std::string* alarm_code, std::string* mes)
  {
    std::string aid, ingress, egress, ntfcncde, conditiontype, srveff;
    std::string tokens[7];
    if(match(mes, ":,,,,", 6, tokens) != 6)
      return;

    aid                = tokens[0];
    ingress            = aid.substr(4, 5); //remove 3 spaces and "
    egress             = aid.substr(10, 5); //remove 3 spaces,", ingress and - or > 
    ntfcncde           = tokens[1]; 
    conditiontype      = tokens[2];
    srveff             = tokens[3];

    if(ntfcncde == "CL") // CL - alarm cleared
    {
        pluginCalientFsc->notify_event(ingress, UP, ENABLED);
        pluginCalientFsc->notify_event(egress, UP, ENABLED);
    }
    else if(ntfcncde == "CR" || ntfcncde == "MJ" || ntfcncde == "MN" ||  ntfcncde == "DEFAULT") // CR - critical, MJ - major, MN - minor
    {
        pluginCalientFsc->notify_event(ingress, DOWN, ENABLED);
        pluginCalientFsc->notify_event(egress, DOWN, ENABLED);
    }
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
  void PluginCalientFSC_listen::process_xc(int state, void* parameters, tnrcsp_result_t result, std::string* response_block)
  {
    plugin->retrive_interwall=plugin->default_retrive_interwall;
    pluginCalientFsc->trncsp_calient_xc_process((tnrcsp_calient_xc_state)state, (xc_calient_params*)parameters, result, response_block);
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** store information with equipments details */
  void PluginCalientFSC_listen::save_equipments(std::string response_block)
  {
    std::string             line = "";
    tnrc_portid_t           port_id;
    operational_state       oper_state;
    administrative_state    admin_state;

    std::string aid;
    std::string type_id;
    std::string in_opt_cap;      ///Input  optical capable
    std::string out_opt_cap;     ///Putput optical capable
    std::string in_opt_adm;      ///Input  optical administrative
    std::string out_opt_adm;     ///Output optical administrative
    std::string in_opt_oper;     ///Input  optical operational
    std::string out_opt_oper;    ///Output optical operational

    std::string tokens[12];

    while(response_block.length() > 0)
    {
      line = pop_line(&response_block);

      int nol= match(&line, ":,,:,,,,,,,", 12, tokens);
      if(nol != 12)
        continue;

      aid            = tokens[0].substr(4);
      type_id        = tokens[1]; 
      in_opt_adm     = tokens[4];
      in_opt_oper    = tokens[5];
      in_opt_cap     = tokens[6];
      out_opt_adm    = tokens[8];
      out_opt_oper   = tokens[9];
      out_opt_cap    = tokens[10];

      port_id     = calient_compose_id(&aid);

      oper_state  = ((in_opt_cap.find("OK") != -1) && (out_opt_cap.find("OK") != -1)) ? UP : DOWN;
      admin_state = ENABLED;

      /// update AP Data model
      std::list<tnrcsp_lsc_xc_detail_t>::iterator it2;

      // sent notification if state changed
      if(pluginCalientFsc->resource_list.count(port_id) == 0
        || oper_state != pluginCalientFsc->resource_list[port_id].oper_state)
      {
        for(it2=pluginCalientFsc->xc_list.begin(); it2!=pluginCalientFsc->xc_list.end(); it2++)
          if(it2->portid_in == port_id || it2->portid_out == port_id)
          {
            if(it2->xc_state == TNRCSP_LSC_XCSTATE_BUSY)
              oper_state = DOWN; // port in busy state
            break;
          }

        pluginCalientFsc->register_event(port_id, oper_state, admin_state, 0);
      }
    }
    TNRC_INF("resource_list contains:");
    std::map<tnrc_portid_t, tnrcsp_lsc_resource_detail_t>::iterator it;
    for(it = pluginCalientFsc->resource_list.begin(); it != pluginCalientFsc->resource_list.end(); it++)
    {
        tnrcsp_lsc_resource_detail_t d = it->second;
        TNRC_NOT("   ID=0x%x, OperState=%s, AdminState=%s, Event=%s", it->first, SHOW_OPSTATE(d.oper_state),
                    SHOW_ADMSTATE(d.admin_state), SHOW_TNRCSP_EVENT(d.last_event));
    } 
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** store information with xconn details */
  void PluginCalientFSC_listen::save_xcs(std::string response_block)
  {
    std::string line = "";

    tnrc_portid_t port_in, port_out;
    tnrcsp_lsc_xc_state_t xc_state;

    std::string ingress, egress, admin_state, oper_state, oper_capacity;

    std::string tokens[18];

    std::list<tnrcsp_lsc_xc_detail_t> busy_list; // a list with external xconns
    std::list<tnrcsp_lsc_xc_detail_t>::iterator busy_iter;

    std::list<tnrcsp_lsc_xc_detail_t>::iterator it;
    for(it=pluginCalientFsc->xc_list.begin(); it!=pluginCalientFsc->xc_list.end(); it++ )
      if(it->xc_state == TNRCSP_LSC_XCSTATE_BUSY)
        busy_list.insert(busy_list.end(), *it);

    pluginCalientFsc->xc_list.clear();

    while(response_block.length() > 0)
    {
      line = pop_line(&response_block);

      int pos = line.find(":");
      if(pos == -1)
        break;
      line.erase(0, pos+1);

      dict d;
      match_dict(&line, d);

      ingress       = d["SRCPORT"];
      egress        = d["DSTPORT"]; 
      admin_state   = d["AS"];
      oper_state    = d["OS"];
      oper_capacity = d["OC"];

      if(ingress==""||egress==""||admin_state==""||oper_state==""||oper_capacity=="")
        continue;

      port_in =     calient_compose_id(&ingress);
      port_out =    calient_compose_id(&egress);

      if(admin_state == "UMA" && oper_state == "RDY" && oper_capacity == "OK")
        xc_state = TNRCSP_LSC_XCSTATE_RESERVED;
      else if(admin_state == "IS" && oper_state == "IS" && oper_capacity == "OK")
        xc_state = TNRCSP_LSC_XCSTATE_ACTIVE;
      else
        xc_state = TNRCSP_LSC_XCSTATE_FAILED;

      if (find_board_id(port_in&0xFFFF) != 0 || find_board_id(port_out&0xFFFF) != 0)
      {
          std::map<tnrc_sp_xc_key, xc_calient_params,xc_map_comp_lsc>::iterator it3;
          tnrc_sp_xc_key xc_key = {port_in, port_out};
          it3 = pluginCalientFsc->xc_map.find(xc_key);

          if(it3 == pluginCalientFsc->xc_map.end())
          {
            // externally created xconn - set busy flag
            TNRC_INF("XC not found in db: In=0x%x, Out=0x%x", port_in, port_out);
            xc_state = TNRCSP_LSC_XCSTATE_BUSY;

            for(busy_iter=busy_list.begin(); busy_iter!=busy_list.end(); busy_iter++ )
            {
              if(busy_iter->portid_in == port_in && busy_iter->portid_out == port_out)
              {
                 busy_list.erase(busy_iter);
                 break;
              }
            }

            pluginCalientFsc->register_event(port_in, UP, ENABLED, EVENT_PORT_XCONNECTED);
            pluginCalientFsc->register_event(port_out, UP, ENABLED, EVENT_PORT_XCONNECTED);
          } 
      }

      tnrcsp_lsc_xc_detail_t details = {port_in, port_out, 0, xc_state}; 
      pluginCalientFsc->xc_list.insert(pluginCalientFsc->xc_list.end(), details); 
    }

    for(busy_iter=busy_list.begin(); busy_iter!=busy_list.end(); busy_iter++ )
    {
      // list of ports for which external xconn has disappeared
      TNRC_INF("External xconn deletion detected: In=0x%x, Out=0x%x", busy_iter->portid_in, busy_iter->portid_out);
      pluginCalientFsc->register_event(busy_iter->portid_in, UP, ENABLED, EVENT_PORT_XDISCONNECTED);
      pluginCalientFsc->register_event(busy_iter->portid_out, UP, ENABLED, EVENT_PORT_XDISCONNECTED);
    };

    TNRC_INF("xc_list contains:");
    for(it=pluginCalientFsc->xc_list.begin(); it!=pluginCalientFsc->xc_list.end(); it++ )
      TNRC_INF("   In=0x%x, Out=0x%x, State=%s", it->portid_in, it->portid_out, SHOW_TNRCSP_XC_STATE(it->xc_state));
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** if synch function call wait more then 120 sec release all TL1 request waiting for response */
  void PluginCalientFSC_listen::check_func_waittime()
  { 
    std::map<const int, tnrc_sp_handle_item>::iterator iter;
    for(iter = handlers.begin(); iter != handlers.end(); iter++ )
      if (time(NULL) - iter->second.start > 120 /* secs */)
      {
        TNRC_WRN("Timeout!");
        xc_calient_params* parameters = (xc_calient_params*)iter->second.parameters;
        if(parameters->response_cb)
          parameters->response_cb(parameters->handle, TNRCSP_RESULT_GENERICERROR, parameters->response_ctx);
      }
  }

  //-------------------------------------------------------------------------------------------------------------------
  /** release all TL1 request waiting for response */
  void PluginCalientFSC_listen::delete_func_waittime()
  {
    std::map<const int, tnrc_sp_handle_item>::iterator iter;
    for(iter = handlers.begin(); iter != handlers.end(); iter++ )
    {
      delete (xc_calient_params*)iter->second.parameters;
      handlers.erase(iter->first);
    }
  }

  /*******************************************************************************************************************
  *
  *            CLASS XC CALIENTS PARAMETERS
  *
  *******************************************************************************************************************/

  xc_calient_params::xc_calient_params(trncsp_xc_activity activity, tnrcsp_handle_t handle, 
                        tnrc_portid_t port_in, tnrc_portid_t port_out,
                        xcdirection_t direction, tnrc_boolean_t _virtual,
                        tnrc_boolean_t activate, tnrcsp_response_cb_t response_cb,
                        void* response_ctx, tnrcsp_notification_cb_t async_cb, void* async_cxt)
  {
    this->activity        = activity;
    this->handle          = handle;
    this->port_in         = port_in;
    this->port_out        = port_out;
    this->direction       = direction;
    this->_virtual        = _virtual;
    this->activate        = activate;
    this->response_cb     = response_cb;
    this->response_ctx    = response_ctx;
    this->async_cb        = async_cb;
    this->async_cxt       = async_cxt;
    this->deactivate      = activate;
    this->ingress_aid     = calient_decompose_in(port_in);
    this->egress_aid      = calient_decompose_out(port_out);

    this->activated       = false;
    this->repeat_count    = 0;
    this->final_result    = false;
    this->result          = TNRCSP_RESULT_NOERROR;
  }

  xc_calient_params::xc_calient_params()
  {
    this->ingress_aid     = NULL;
    this->egress_aid      = NULL;
  }
  
  //-------------------------------------------------------------------------------------------------------------------
  xc_calient_params::~xc_calient_params()
  {
    if(ingress_aid)
        delete ingress_aid;
    ingress_aid = NULL;

    if(egress_aid)
        delete egress_aid;
    egress_aid = NULL;
  }

  //-------------------------------------------------------------------------------------------------------------------
  xc_calient_params & xc_calient_params::operator=(const xc_calient_params &from)
  {
    activity      = from.activity;
    handle        = from.handle;
    port_in       = from.port_in;
    port_out      = from.port_out;
    direction     = from.direction;
    _virtual      = from._virtual;
    activate      = from.activate;
    response_cb   = from.response_cb;
    response_ctx  = from.response_ctx;
    async_cb      = from.async_cb;
    async_cxt     = from.async_cxt;
    deactivate    = from.activate;
    ingress_aid   = calient_decompose_in(from.port_in);
    egress_aid    = calient_decompose_out(from.port_out);
    activated     = from.activated;
    repeat_count  = from.repeat_count;
    final_result  = from.final_result;
    result        = from.result;
    return *this;
  }
}


