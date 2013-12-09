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
//  Jakub Gutkowski       (PSNC)             <jgutkow_at_man.poznan.pl>
//



#include <zebra.h>
#include "vty.h"

#include "tnrcd.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_plugin_ssh_foundry.h"

namespace TNRC_SP_XMR
{
  static int xmr_read_func (struct thread *thread);

  /*************************************************************
  *
  *        UTILS
  *
  **************************************************************/

  /** iterate over AP Data Model and return board id for first maching port 
      - assuming that there are not present the same ports on different boards */
  static board_id_t find_board_id(tnrc_portid_t portid)
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
    if (t == NULL)
      return TNRC_API_ERROR_NO_INSTANCE;

    //Check the presence of an EQPT object
    if (t->n_eqpts() == 0)
      return TNRC_API_ERROR_GENERIC;

    //Fetch all data links in the data model
    iter_e = t->begin_eqpts();
    eqpt_id = (*iter_e).first;
    e = (*iter_e).second;
    if (e == NULL)
      return TNRC_API_ERROR_GENERIC;

    //Iterate over Abstract Part Data Model: boards and ports
    for(iter_b = e->begin_boards(); iter_b != e->end_boards(); iter_b++)
    {
      board_id = (*iter_b).first;
      b = (*iter_b).second;
      for(iter_p = b->begin_ports();iter_p != b->end_ports();iter_p++)
      {
        port_id = (*iter_p).first;
        pp = (*iter_p).second;

        //Check if port found
        if (port_id == portid)
          return board_id;
      }
    }
    return 0;
  }

  std::string pop_line(string* mes)
  {
    int pos = mes->find("\n");
    if(pos == -1)
    {
        std::string _mes = *mes;
        mes->clear();
        return *mes;
    }
    std::string first_line = mes->substr(0, pos);
    mes->erase(0, pos+1);
    return first_line;
  }

  /** search description in the message, return True if description found */
  bool foundry_find(std::string* message, std::string description)
  {
    if(!message)
        return false;
    return message->find(description) != -1;
  }

  /** generates port id in XMR notation slot/port (max. 99/99 -> 9999); returns NULL in case of any error */
  std::string* foundry_encode_portid(tnrc_portid_t portid)
  {
    int slot = portid;
    slot = (slot >> 8) & 0x000000FF;
    int port = portid;
    port = port & 0x000000FF;

    if((1 > slot) || (1 > port) || (slot > 99) || (port > 99))
    {
        TNRC_WRN("port_id cannot be decomposed - out of range 101 - 9999!\n");
        return NULL;
    }
    char result_portid[16];
    sprintf(result_portid, "%d/%d", slot,port);
    std::string* port_id = new std::string(result_portid);

    return port_id;
  }

  bool foundry_check_vlanid(vlanid_t vlanid)
  {
    if( vlanid>1 && vlanid<=4090 )
        return true;
    else
        return false;
  }

  /** check if given resources are correct as an vlan creation parametrers*/
  bool foundry_check_resources(tnrc_portid_t port_in, tnrc_portid_t port_out, vlanid_t vlanid)
  {
    bool success;
    std::string* ingress_id = foundry_encode_portid(port_in);
    std::string* egress_id = foundry_encode_portid(port_out);

    if(!ingress_id || !egress_id || !foundry_check_vlanid(vlanid))
    {
        success = false;
        TNRC_INF("foundry_check_resources no success, ingress_id:%s, egress_id:%s, vlanid:%d",ingress_id->c_str(),egress_id->c_str(), vlanid);
    }
    else
    {
        success = true;
        TNRC_INF("foundry_check_resources success, ingress_id:%s, egress_id:%s, vlanid:%d",ingress_id->c_str(),egress_id->c_str(), vlanid);
    }
    delete ingress_id, egress_id;
    return success;
  }

  //================================================================================
  /**  Class PluginXmr
  *  Abstract Specyfic Part class for devices with CLI-SSH interface
  */
  PluginXmr::PluginXmr(): t_read(NULL), t_retrive(NULL)
  {
    handle_ = 1;
    xc_id_ = 1;
    wqueue_ = work_queue_new (TNRC_MASTER.getMaster(), FOUNDRY_WQ);
    wqueue_->spec.workfunc = workfunction;
    wqueue_->spec.del_item_data = delete_item_data;
    wqueue_->spec.max_retries = 5;
    //wqueue_->spec.hold = 500;
    sock = NULL;
    SSH2_status = TNRC_SP_STATE_NO_SESSION;
    _activity  = GET_XC;
    identifier = 0;
    listen_proc = new PluginXmr_listen(this);
    xc_bidir_support_ = true;
    write_count = 1;
    repeat_mes = false;
    wait_logging = true;
    start_count = false;
    read_thread = false;
    wait_disconnect = false;
    check_status = 0;
  }

  PluginXmr::PluginXmr(std::string name): t_read(NULL), t_retrive(NULL)
  {
    name_ = name;
    handle_ = 1;
    xc_id_ = 1;
    wqueue_ = work_queue_new (TNRC_MASTER.getMaster(), FOUNDRY_WQ);
    wqueue_->spec.workfunc = workfunction;
    wqueue_->spec.del_item_data = delete_item_data;
    wqueue_->spec.max_retries = 5;
    //wqueue_->spec.hold = 500;
    sock = NULL;
    SSH2_status = TNRC_SP_STATE_NO_SESSION;
    _activity  = GET_XC;
    identifier = 0;
    listen_proc = new PluginXmr_listen(this);
    xc_bidir_support_ = true;
    write_count = 1;
    repeat_mes = false;
    wait_logging = true;
    start_count = false;
    read_thread = false;
    wait_disconnect =false;
    check_status = 0;
  }

  PluginXmr::~PluginXmr()
  {
    close_connection();
  }

  /** send login commnad 
   * @param user   device login
   * @param passwd device password
   * @return 0 if login command send successful
   */
  int PluginXmr::xmr_log_in(std::string user, std::string passwd)
  {
    /* send enable commnad to get the admin privilages*/
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "terminal length 0\n%s\n%s\nconfigure terminal\n\n", user.c_str(), passwd.c_str());
    string str(buff);
    asynch_send(&str, ctag, TNRCSP_STATE_INIT, NULL, TNRC_LOGIN);
    return 0;
  }

  void PluginXmr::retriveT(thread *retrive)
  {
    t_retrive = retrive;
  }

  void PluginXmr::readT(thread *read)
  {
    t_read = read;
  }

  /** PluginXmr::getFd()
  * @return FileDescriptor
  */
  int PluginXmr::getFd()
  {
    return sock->fd();
  }

  /** waits for autonomous messages with one of the texts *//*
  void PluginXmr::autonomous_handle(int handle, std::string* aid, std::string text, int state, void* parameters)
  {
    tnrc_sp_handle_item item = {time(NULL), *aid, text, state, parameters};
    listen_proc->add_handler(handle, item);
  }*/

  /** PluginXmr::xmr_connect()
  * @return 0 if ssh connection is established
  */
  int PluginXmr::xmr_connect()
  {
    int result = sock->_connect();
    if (result == 0)
    {
      SSH2_status = TNRC_SP_STATE_NOT_LOGGED_IN;
      TNRC_INF("Connected OK");
    }
    else
    {
      TNRC_ERR("Connection Error");
    }
    return result;
  }

  int PluginXmr::xmr_disconnect()
  {
    int result;
    result = close_connection();
    return result;
  }

  /** PluginXmr::asynch_send
  * @param command 
  * @param ctag not used
  * @param state tnrc sp state
  * @param parameters
  * @param cmd_type
  * @return
  */
  tnrcsp_result_t PluginXmr::asynch_send(std::string* command, int ctag, int state, void* parameters, tnrc_l2sc_cmd_type_t cmd_type)
  {
    /* send SSH2 CLI command */
    if (((command->find("enable") != -1) && (SSH2_status == TNRC_SP_STATE_NOT_LOGGED_IN))
    || (SSH2_status == TNRC_SP_STATE_LOGGED_IN))
    {
        TNRC_INF("Send command: %s\n", command->c_str());
        read_thread = true;
        if(sock->_send(command) == -1)
        {
            TNRC_WRN("Connection reset by peer\n");
            SSH2_status = TNRC_SP_STATE_NO_SESSION;
            return TNRCSP_RESULT_EQPTLINKDOWN;
        }
        xmr_response.assign("");
        command_type = cmd_type;
        sent_command = *command;
        param = (xc_foundry_params*)parameters;
        return TNRCSP_RESULT_NOERROR;
    }
    else
    {
        TNRC_WRN("send unsuccessful\n");
        if(SSH2_status == TNRC_SP_STATE_NO_SESSION)
            return TNRCSP_RESULT_EQPTLINKDOWN;
        return TNRCSP_RESULT_INTERNALERROR;
    }
  }

  /** Read thread's rountine (thread *t_read) */
  int PluginXmr::doRead()
  {
    std::string recMsg("");
    xmr_response = "";
    if(write_count == 6)
      repeat_mes = true;

    int l,last_l;
    l = sock->_recv(&recMsg, 4096);
    if(l > -1)
      start_count = true;
    if(start_count)
      write_count = write_count + 1;

    temp_xmr_response.append(recMsg);
    
    if((temp_xmr_response.find(")#")) != -1 || repeat_mes)
    {
      xmr_response = temp_xmr_response;
      temp_xmr_response = "";
      write_count = 1;
      start_count = false;
      read_thread = false;

      if(command_type == TNRC_GET_VLANS_RESOURCES)
      {
        if((xmr_response.find("Configured")) == -1)
        {
          read_thread = true;
          return 0;
        }
      }

      TNRC_INF("Processing received message \n%s", xmr_response.c_str());
      listen_proc->process_message(&xmr_response);
      return -1;
    }
    return 0;
  }

  /** send ports retrieve commnad */
  void PluginXmr::get_ports_resources()
  {
    /* send request for all available ports resources*/
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "show interfaces brief | include level\n\n");
    std::string str(buff);
    asynch_send(&str, ctag, TNRCSP_STATE_INIT, NULL, TNRC_GET_PORTS_RESOURCES);
  }

  /** send channels retrieve commnad */
  void PluginXmr::get_all_xc()
  {
    /* send request for all existing vlan instances*/
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "show vlan\n\n");
    std::string str(buff);
    asynch_send(&str, ctag, TNRCSP_STATE_INIT, NULL, TNRC_GET_VLANS_RESOURCES);
  }

 /** Backgrount thread's rountine (thread *t_retrive)
  * - connection with device
  * - equipment
  * - connections
  */
  int PluginXmr::doRetrive()
  {
    check_status++;
    if (check_status == 300) //TODO change this value now its 5 min (300sec)
    {
      check_status = 0;
      wait_disconnect = true;
    }
    int result = -1;
    switch(SSH2_status)
    {
      case TNRC_SP_STATE_NO_SESSION:
        TNRC_INF("trying to connect");
        result = xmr_connect();
        if (result != 0)
          break;
      case TNRC_SP_STATE_NOT_LOGGED_IN:
        if (wait_logging)
        {
          wait_logging = false;
          TNRC_INF("trying to logg in");
          result = xmr_log_in("enable", enable_password);
        }
        break;
      case TNRC_SP_STATE_LOGGED_IN:
        switch (_activity)
        {
          case GET_EQUIPMENT:
              wait_logging = true;
              get_ports_resources();
              _activity = DONE;
              result = 0;
            break;
          case GET_XC:
              get_all_xc();
              _activity = DONE;
              result = 0;
            break;
          case CHECK_ACTION:
            if(!action_list.empty())
            {
              if(action_list.front()->activity == TNRCSP_XC_MAKE)
              {
                bool make = true, second = true;
                xc_foundry_params* xc_params = action_list.front();
                action_list.pop_front();
                if(xc_params->_virtual)
                {
                  list<tnrcsp_l2sc_xc_detail_t>::iterator it;
                  for(it=listen_proc->xc_list.begin(); it!=listen_proc->xc_list.end(); it++ )
                  {
                    if(it->portid_in == xc_params->port_in || it->portid_in == xc_params->port_out)
                    {
                      if(it->portid_out == xc_params->port_out || it->portid_out == xc_params->port_in)
                      {
                        int min = atoi(vlan_number.c_str());
                        int max = min + atoi(vlan_pool.c_str());
                          if(it->vlanid >= min && it->vlanid < max)
                          {
                            xc_params->activated = true;
                            tnrcsp_result_t result = TNRCSP_RESULT_NOERROR;
                            xc_params->vlanid = it->vlanid;
                            tnrc_sp_xc_key key = {xc_params->port_in, xc_params->port_out, xc_params->vlanid};
                            xc_map[key] = xc_params;
                            list<tnrc_sp_xc_key>::iterator it;
                            for(it = listen_proc->external_xc.begin(); it!= listen_proc->external_xc.end(); it++)
                            {
                              if(it->port_in == xc_params->port_in || it->port_out == xc_params->port_in)
                              {
                                listen_proc->external_xc.erase(it);
                                break;
                              }
                            }
                            TNRC_INF("Foundry-SP (isvirtual response) - xc notification to AP with %s", SHOW_TNRCSP_RESULT(result));
                            xc_params->response_cb(xc_params->handle, result, xc_params->response_ctxt);
                            delete xc_params;
                            make = false;
                            second = false;
                            break;
                          }
                      }
                      else if(make)
                      {
                        tnrcsp_result_t result = TNRCSP_RESULT_BUSYRESOURCES;
                        xc_params->result = TNRCSP_RESULT_BUSYRESOURCES;
                        TNRC_INF("Foundry-SP (isvirtual response) - xc notification to AP with %s", SHOW_TNRCSP_RESULT(xc_params->result));
                        xc_params->response_cb(xc_params->handle, result, xc_params->response_ctxt);
                        delete xc_params;
                        make = false;
                        second = false;
                        break;
                      }
                    }
                    if(second)
                    {
                      if(it->portid_out == xc_params->port_out || it->portid_out == xc_params->port_in)
                      {
                        if(it->portid_in == xc_params->port_in || it->portid_in == xc_params->port_out)
                        {
                          int min = atoi(vlan_number.c_str());
                          int max = min + atoi(vlan_pool.c_str());
                          if(it->vlanid >= min && it->vlanid < max)
                          {
                              xc_params->activated = true;
                              tnrcsp_result_t result = TNRCSP_RESULT_NOERROR;
                              xc_params->vlanid = it->vlanid;
                              tnrc_sp_xc_key key = {xc_params->port_in, xc_params->port_out, xc_params->vlanid};
                              xc_map[key] = xc_params;
                              list<tnrc_sp_xc_key>::iterator it;
                              for(it = listen_proc->external_xc.begin(); it!= listen_proc->external_xc.end(); it++)
                              {
                                if(it->port_in == xc_params->port_in || it->port_out == xc_params->port_in)
                                {
                                  listen_proc->external_xc.erase(it);
                                  break;
                                }
                              }
                              TNRC_INF("Foundry-SP (isvirtual response) - xc notification to AP with %s", SHOW_TNRCSP_RESULT(result));
                              xc_params->response_cb(xc_params->handle, result, xc_params->response_ctxt);
                              delete xc_params;
                              make = false;
                              break;
                            }
                        }
                        else if(make)
                        {
                          tnrcsp_result_t result = TNRCSP_RESULT_BUSYRESOURCES;
                          xc_params->result = TNRCSP_RESULT_BUSYRESOURCES;
                          TNRC_INF("Foundry-SP (isvirtual response) - xc notification to AP with %s", SHOW_TNRCSP_RESULT(xc_params->result));
                          xc_params->response_cb(xc_params->handle, result, xc_params->response_ctxt);
                          delete xc_params;
                          make = false;
                          break;
                        }
                      }
                   }
                  }
                }
                if(make)
                {
                  trncsp_foundry_xc_process(TNRCSP_STATE_INIT, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                  _activity = DONE;
                }
                else
                  _activity = GET_XC;
              }
              else if(action_list.front()->activity == TNRCSP_XC_DESTROY)
              {
                xc_foundry_params* xc_params = action_list.front();
                action_list.pop_front();
                trncsp_foundry_xc_process(TNRCSP_STATE_INIT, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                _activity = DONE;
              }
              else if(action_list.front()->activity == TNRCSP_XC_RESERVE)
              {
                xc_foundry_params* xc_params = action_list.front();
                action_list.pop_front();
                trncsp_foundry_xc_process(TNRCSP_STATE_INIT, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                _activity = DONE;
              }
              else if(action_list.front()->activity == TNRCSP_XC_UNRESERVE)
              {
                xc_foundry_params* xc_params = action_list.front();
                action_list.pop_front();
                trncsp_foundry_xc_process(TNRCSP_STATE_INIT, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                _activity = DONE;
              }
            }
            break;
        }
        break;
      case TNRC_SP_STATE_END_SESSION:
        TNRC_INF("ending session");
        result = xmr_disconnect();
        SSH2_status = TNRC_SP_STATE_DONE;
        break;
    }
    if (read_thread)
      readT(thread_add_event(master, xmr_read_func, this, 0));
    if (wait_disconnect && SSH2_status == TNRC_SP_STATE_DONE)
    {
      wait_disconnect = false;
      SSH2_status = TNRC_SP_STATE_NO_SESSION;
    }
    return result;
  }

  int PluginXmr::close_connection()
  {
    if(sock->close_connection() == -1)
        return -1;
    return 0;
  }


  /**
  * Thread read function
  */
  static int xmr_read_func (struct thread *thread)
  {
    int fd;
    PluginXmr *p;

    fd = THREAD_FD (thread);
    p = static_cast<PluginXmr *>(THREAD_ARG (thread));

    int result = p->doRead();
    return result;
  }

  /** 
  * Thread retrive function
  */
  int xmr_retrive_func (struct thread *thread)
  {
    PluginXmr *p;

    p = static_cast<PluginXmr *>(THREAD_ARG (thread));

    int result = p->doRetrive();
    p->retriveT(thread_add_timer(master, xmr_retrive_func, p, 1));
    return result;
  }

  /** Launch communication with device
   * @param location <ipAddress>:<port>:<path_to_configuration_file>
   */
  tnrcapiErrorCode_t
  PluginXmr::probe (std::string location)
  {
    TNRC::TNRC_AP * t;
    std::string     err;

    //check if a TNRC instance is running. If not, create e new one
    t = TNRC_MASTER.getTNRC();
    if (t == NULL) 
    { //create a new instance
      TNRC_CONF_API::tnrcStart(err);
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

    TNRC_INF("Plugin Foundry Xmr address %s", strAddress.c_str());
    if (inet_aton(strAddress.c_str(), &remote_address) != 1)
    {
      TNRC_ERR("Plugin Foundry Xmr::probe: wrong IP address %s given string is: %s", inet_ntoa(remote_address), strAddress.c_str());
      return TNRC_API_ERROR_GENERIC;
    }
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Foundry Xmr::probe: wrong location descriotion: no port");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);

    strPort = location.substr(stPos, endPos-stPos);

    TNRC_INF("Plugin Port %s", strPort.c_str());
    if (sscanf(strPort.c_str(), "%d", &remote_port) != 1)
    {
      TNRC_ERR("Plugin Foundry Xmr::probe: wrong port (location = %s, readed port %d)", location.c_str(), remote_port);
      return TNRC_API_ERROR_GENERIC;
    }

    sock = new PluginXmr_session(remote_address, remote_port, this);

    TNRC_INF("Plugin Foundry Xmr::probe: sesion data: ip %s, port %d", inet_ntoa(remote_address), remote_port);

    SSH2_status = TNRC_SP_STATE_NO_SESSION;

    //login
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Foundry Xmr::probe: wrong location descriotion: no login");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    login = location.substr(stPos, endPos-stPos);

    //password
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Foundry Xmr::probe: wrong location descriotion: no password");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    password = location.substr(stPos, endPos-stPos);

    //enable password
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Foundry Xmr::probe: wrong location descriotion: no enable password");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    enable_password = location.substr(stPos, endPos-stPos);

    //vlan number
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Foundry Xmr::probe: wrong location descriotion: no vlan number");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    vlan_number = location.substr(stPos, endPos-stPos);

    //vlan pool
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Foundry Xmr::probe: wrong location descriotion: no vlan pool");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    vlan_pool = location.substr(stPos, endPos-stPos);

    retriveT (thread_add_timer (master, xmr_retrive_func, this, 0));

    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Foundry Xmr::probe: wrong location description: no path");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    strPath = location.substr(stPos, endPos-stPos);

    //read eqpuipment config file
    TNRC_INF("Plugin Foundry Xmr configuration file %s", strPath.c_str());
    vty_read_config ((char *)strPath.c_str(), NULL);
    TNRC_INF("Foundry Xmr extra information about ports readed");

    return TNRC_API_ERROR_NONE;
  }


  void PluginXmr::trncsp_foundry_xc_process(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters, tnrcsp_result_t response, string* data)
  {
    tnrcsp_result_t result;
    bool call_callback = false;
    if(parameters->activity == TNRCSP_XC_MAKE)
    {
        if(call_callback = trncsp_foundry_make_xc(state, parameters, response, data, &result))
            if(result == TNRCSP_RESULT_NOERROR)
            {
                parameters->activated = true;
                tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out, parameters->vlanid};
                xc_map[key] = parameters;
            }
            else if(result != TNRCSP_RESULT_BUSYRESOURCES)
            {
                call_callback = false;
                parameters->final_result = true;
                parameters->result = result;
                TNRC_WRN("Make unsuccessful -> destroying resources\n");
                parameters->activity = TNRCSP_XC_DESTROY;
                trncsp_foundry_destroy_xc(TNRCSP_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
            }
    }
    else if(parameters->activity == TNRCSP_XC_DESTROY)
    {
      if(call_callback = trncsp_foundry_destroy_xc(state, parameters, response, data, &result))
      {
        TNRC_INF("Vlan removation completed");
        tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out};
        if (xc_map.count(key) > 0)
          xc_map.erase(key);

        if(parameters->deactivate)
        {
          call_callback = false;
          parameters->activity = TNRCSP_XC_RESERVE;
          //call_callback = trncsp_foundry_reserve_xc(TNRCSP_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
          action_list.insert(action_list.end(), parameters);
          wait_disconnect = true;
        }
      }
    }
    else if(parameters->activity == TNRCSP_XC_RESERVE)
    {
      if(call_callback = trncsp_foundry_reserve_xc(state, parameters, response, data, &result))
      {
        if(result == TNRCSP_RESULT_NOERROR)
        {
          TNRC_INF("Foundry reserve XC result NOERROR");
          parameters->activated = false;
          tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out, parameters->vlanid};
          xc_map[key] = parameters;
        }
        else if(result != TNRCSP_RESULT_BUSYRESOURCES)
        {
          call_callback = false;
          parameters->final_result = true;
          parameters->result = result;
          TNRC_WRN("reservation unsuccessful -> unreserve resources\n");
          parameters->activity = TNRCSP_XC_UNRESERVE;
          call_callback = trncsp_foundry_unreserve_xc(TNRCSP_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
        }
      }
    }
    else if(parameters->activity == TNRCSP_XC_UNRESERVE)
    {
      if(call_callback = trncsp_foundry_unreserve_xc(state, parameters, response, data, &result))
      {
         TNRC_INF("Foundry unreserve XC result NOERROR");
         tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out};
         if (xc_map.count(key) > 0)//TODO check it 
           xc_map.erase(key);
      }
    }
    if(call_callback && parameters->response_cb)
    {
      if(parameters->final_result == true)
        result = parameters->result;
      TNRC_INF("Foundry-SP - xc notification to AP with %s", SHOW_TNRCSP_RESULT(result));
      parameters->response_cb(parameters->handle, result, parameters->response_ctxt);
      delete parameters;
    }
  }

  /** PluginXmr::trncsp_foundry_make_xc
  *   make crossconnection
  */ 
  bool PluginXmr::trncsp_foundry_make_xc(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters, tnrcsp_result_t response, string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("make state: %i\n", state);
    if(state == TNRCSP_STATE_INIT)
    {
        *result = foundry_create_vlan(parameters->conn_type, TNRCSP_STATE_CREATE_VLAN, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
            return true;

    }
    else if(state == TNRCSP_STATE_CREATE_VLAN)
    {
        if(response != TNRCSP_RESULT_NOERROR)
        {
            *result = foundry_error_analyze(data, "Invalid input", TNRCSP_RESULT_GENERICERROR, response);
            return true;
        }
        else
        {
            *result = foundry_vlan_add_port(parameters->ingress_fid, TNRCSP_STATE_ADD_PORT_IN, parameters);
            if(*result != TNRCSP_RESULT_NOERROR)
                return true;
        }
    }
    else if(state == TNRCSP_STATE_ADD_PORT_IN)
    {
        if(response != TNRCSP_RESULT_NOERROR)
        {
            *result = foundry_error_analyze(data, "not belong to default vlan", TNRCSP_RESULT_BUSYRESOURCES, response);
            return true;
        }
        else
        {
            *result = foundry_vlan_add_port(parameters->egress_fid, TNRCSP_STATE_ADD_PORT_OUT, parameters);
            if(*result != TNRCSP_RESULT_NOERROR)
                return true;
        }
    }
    else if(state == TNRCSP_STATE_ADD_PORT_OUT)
    {
        if(response != TNRCSP_RESULT_NOERROR)
        {
            *result = foundry_error_analyze(data, "not belong to default vlan", TNRCSP_RESULT_BUSYRESOURCES, response);
            return true;
        }
        else
        {
            *result = foundry_vlan_add_flooding(TNRCSP_STATE_ADD_FLOODING, parameters);
            if(*result != TNRCSP_RESULT_NOERROR)
                return true;
        }
    }
    else if(state == TNRCSP_STATE_ADD_FLOODING)
    {
        if(response != TNRCSP_RESULT_NOERROR)
        {
            *result = foundry_error_analyze(data, "cannot set flooding", TNRCSP_RESULT_BUSYRESOURCES, response);
            return true;
        }
        else
        {
            *result = foundry_vlan_precommit(TNRCSP_STATE_EXIT, parameters);
            if(*result != TNRCSP_RESULT_NOERROR)
                return true;
        }
    }
    else if(state == TNRCSP_STATE_EXIT)
    {
        if(response != TNRCSP_RESULT_NOERROR)
        {
            *result = foundry_error_analyze(data, "cannot set exit", TNRCSP_RESULT_BUSYRESOURCES, response);
            return true;
        }
        else
        {
            *result = foundry_vlan_commit(TNRCSP_STATE_CHECK, parameters);
            if(*result != TNRCSP_RESULT_NOERROR)
                return true;
        }
    }
    else if(state == TNRCSP_STATE_CHECK)
    {
        *result = foundry_vlan_check(TNRCSP_STATE_FINAL, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
            return true;
    }
    else if(state == TNRCSP_STATE_FINAL)
    {
        *result = response;
        _activity = GET_XC;
        return true;
    } 
    return false;
  }

  /** PluginXmr::trncsp_foundry_destroy_xc
  *   destroy crossconnection
  */
  bool PluginXmr::trncsp_foundry_destroy_xc(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("destroy state: %i\n", state);
    if(state == TNRCSP_STATE_INIT)
    {
        *result = foundry_delete_vlan(TNRCSP_STATE_DELETE_VLAN, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
            return true;
    }
    else if(state == TNRCSP_STATE_DELETE_VLAN)
    {
        if(response != TNRCSP_RESULT_NOERROR)
        {
            *result = response;
            return true;
        }
        else
        {
            // there is no specified vlan configured, nothing to commit and check
            if(foundry_find(data, "not configured"))
            {
                *result = TNRCSP_RESULT_PARAMERROR;
                return true;
            }
            else
            {
                *result = foundry_vlan_destroy_commit(TNRCSP_STATE_CHECK, parameters);
                if(*result != TNRCSP_RESULT_NOERROR)
                    return true;
            }
        }

    }
    else if(state == TNRCSP_STATE_CHECK)
    {
        *result = foundry_destroy_vlan_check(TNRCSP_STATE_FINAL, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
            return true;
    }
    else if(state == TNRCSP_STATE_FINAL)
    {
        *result = response;
        _activity = GET_XC;        return true;
    }
    return false;
  }

  /** PluginXmr::trncsp_foundry_reserve_xc
  *   reserve crossconnection
  */
  bool PluginXmr::trncsp_foundry_reserve_xc(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("reserve state: %i", state);
    if(state == TNRCSP_STATE_INIT)
    {
      *result = foundry_reservation_xc(parameters->ingress_fid, parameters->egress_fid, TNRCSP_L2SC_XCSTATE_UNRESERVE, TNRCSP_STATE_FINAL, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_STATE_FINAL)
    {
      *result = TNRCSP_RESULT_NOERROR;
      _activity = GET_XC;
      return true;
    }
    return false;
  }

  /** PluginXmr::trncsp_foundry_unreserve_xc
  *   unreserve crossconnection
  */
  bool PluginXmr::trncsp_foundry_unreserve_xc(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("unreserve state: %i\n", state);
    if(state == TNRCSP_STATE_INIT)
    {
      *result = foundry_unreserve_vlan(parameters->ingress_fid, parameters->egress_fid, TNRCSP_L2SC_XCSTATE_RESERVE, TNRCSP_STATE_FINAL, parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_STATE_FINAL)
    {
      *result = TNRCSP_RESULT_NOERROR;
      _activity = GET_XC;
      return true;
    }
    return false;
  }

  /** send vlan creation command */
  tnrcsp_result_t PluginXmr::foundry_create_vlan(tnrc_sp_lsp_xc_type conn_type, tnrcsp_foundry_xc_state state, xc_foundry_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "vlan %i name phosphorus%i\n\n", parameters->vlanid, parameters->vlanid);
    std::string command(buff);

    return asynch_send(&command, ctag, state, parameters, TNRC_CREATE_VLAN);
  }

  /** search description in the message and replace response if description found */
  tnrcsp_result_t PluginXmr::foundry_error_analyze(string* message, std::string description, tnrcsp_result_t response, tnrcsp_result_t default_response)
  {

    if(!message)
        return TNRCSP_RESULT_INTERNALERROR;

    if(foundry_find(message, description))
        return response;
    else
        return default_response;
  }

  /** send add port command */
  tnrcsp_result_t PluginXmr::foundry_vlan_add_port(std::string* port_fid, tnrcsp_foundry_xc_state state, xc_foundry_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "untagged ethernet %s\n\n", port_fid->c_str());
    std::string command(buff);

    if (state == TNRCSP_STATE_ADD_PORT_OUT)
        return asynch_send(&command, ctag, state, parameters, TNRC_ADD_PORT_OUT);
    else
        return asynch_send(&command, ctag, state, parameters, TNRC_ADD_PORT_IN);
  }

  tnrcsp_result_t PluginXmr::foundry_vlan_add_flooding(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "transparent-hw-flooding\n\n");
    std::string command(buff);

    return asynch_send(&command, ctag, state, parameters, TNRC_ADD_FLOODING);
  }

  /** send exit commands*/
  tnrcsp_result_t PluginXmr::foundry_vlan_precommit(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "exit\n\n");
    std::string command(buff);

    return asynch_send(&command, ctag, state, parameters, TNRC_PRECOMMIT);
  }

  /** send commit commands*/
  tnrcsp_result_t PluginXmr::foundry_vlan_commit(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "write memory\n\n");
    std::string command(buff);

    return asynch_send(&command, ctag, state, parameters, TNRC_COMMIT);
  }

  tnrcsp_result_t PluginXmr::foundry_vlan_destroy_commit(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "write memory\n\n");
    std::string command(buff);

    return asynch_send(&command, ctag, state, parameters, TNRC_DESTROY_COMMIT);
  }

   /** send check command*/
  tnrcsp_result_t PluginXmr::foundry_vlan_check(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "show vlan | include PORT-VLAN %i\n\n", parameters->vlanid);
    std::string command(buff);

    return asynch_send(&command, ctag, state, parameters, TNRC_CHECK);
  }

   /** send check command*/
  tnrcsp_result_t PluginXmr::foundry_destroy_vlan_check(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "show vlan | include PORT-VLAN %i\n\n", parameters->vlanid);
    std::string command(buff);

    return asynch_send(&command, ctag, state, parameters, TNRC_CHECK_DESTROY);
  }

  /** send vlan deletion command */
  tnrcsp_result_t PluginXmr::foundry_delete_vlan(tnrcsp_foundry_xc_state state, xc_foundry_params* parameters)
  {
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "no vlan %i\n\n", parameters->vlanid);
    std::string command(buff);

    return asynch_send(&command, ctag, state, parameters, TNRC_DELETE_VLAN);
  }

  /** remove reservation from reservation list*/
  tnrcsp_result_t PluginXmr::foundry_unreserve_vlan(string* port_in, string* port_out, tnrcsp_l2sc_xc_state_t xc_state, tnrcsp_foundry_xc_state state, xc_foundry_params* parameters)
  {
    tnrcsp_l2sc_xc_res temp = {port_in, port_out};
    list<tnrcsp_l2sc_xc_res>::iterator ite;
    for(ite=listen_proc->xc_res_list.begin(); ite!=listen_proc->xc_res_list.end(); ++ite)
    {
      if(ite->portid_in == temp.portid_in && ite->portid_out == temp.portid_out || ite->portid_in == temp.portid_out && ite->portid_out == temp.portid_in)
      {
        listen_proc->xc_res_list.erase(ite);
      }
    }
    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "show vlan\n\n");
    std::string command(buff);

    return asynch_send(&command, ctag, state, parameters, TNRC_UNRESERVE_FINAL);
  }

  /** reserve ports*/
  tnrcsp_result_t PluginXmr::foundry_reservation_xc(string* port_in, string* port_out, tnrcsp_l2sc_xc_state_t xc_state, tnrcsp_foundry_xc_state state, xc_foundry_params* parameters)
  {
    tnrcsp_l2sc_xc_res temp = {port_in, port_out};
    listen_proc->xc_res_list.insert(listen_proc->xc_res_list.end(),temp);

    int ctag = gen_identifier();
    char buff[100];
    sprintf(buff, "show vlan \n\n");
    std::string command(buff);

    return asynch_send(&command, ctag, state, parameters, TNRC_RESERVE_FINAL);
  }

  //==============================================================================
  /** Class Xmr_session
  * Provides communication with SSH2 protocol using network connection
  */
  PluginXmr_listen::PluginXmr_listen(PluginXmr *owner)
  {
       plugin = owner;
  }

  PluginXmr_session::PluginXmr_session(in_addr address, int port,PluginXmr *owner)
  {
    plugin = owner;
    remote_address = address;
    remote_port = port;
  }

  /** PluginXmr_session::_connect()
  * @return error_code: 0: No error, -1:sock error 
  */
  int PluginXmr_session::_connect()
  {
    s_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    TNRC_INF("Socket description = %d", s_desc);
    #if HAVE_LSSH2
    struct sockaddr_in remote;
    struct hostent *he;

    remote.sin_family         = AF_INET;
    remote.sin_port           = htons(remote_port);
    remote.sin_addr = remote_address;
    memset(&(remote.sin_zero), '\0', 8);  // clear the structure

    if(connect(s_desc, (struct sockaddr*)&remote, sizeof(remote)) == 0)
    {
        //create SSH2 session
        session = libssh2_session_init();
        if (libssh2_session_startup(session, s_desc)) 
        {
            TNRC_ERR("unsuccessful ssh2 session startup: %i, %s\n", errno, strerror(errno));
            return -1;
        }
        else
        {
            /* Authenticate */
            if (libssh2_userauth_password(session, plugin->login.c_str(), plugin->password.c_str()))
            {
                /* Authentication failure */
                TNRC_ERR("unsuccessful session authentication.\n");
                return -1;
            }
            else
            {
                /* Authentication success - session is ready to create a channel*/
                if (!(channel = libssh2_channel_open_session(session)))
                {
                    /* Failure opening channel */
                    TNRC_ERR("Failure opening channel.\n");
                    return -1;
                } 
                else 
                {
                    /* Try to launch a shell in this channel */
                    if(libssh2_channel_shell(channel))
                    {
                        /* Failure starting shell */
                        TNRC_ERR("Failure starting shell.\n");
                        return -1;
                    }
                    else
                    {
                        TNRC_INF("Success, shell in the channel is started!\n");
                        //clear channel output
                        char buffer[512];
                        libssh2_channel_read(channel, buffer, 512);
                        return 0;
                    }
                }
            }
        }
    }
    else
    {
        TNRC_ERR("unsuccessful ssh2 equipment connection: %i, %s\n", errno, strerror(errno));
        return -1;
    }
    #endif
    #ifndef HAVE_LSSH2
    TNRC_ERR("There is no libssh2 installed");
    return -1;
    #endif
  }

  /** PluginXmr_session::_send(std::string* mes)
  * @param mes: pointer to the string that is send to the equipment
  * @return error_code: 0: No error0, -1:sock error 
  */
  int PluginXmr_session::_send(std::string* mes)
  {
    #if HAVE_LSSH2
    if(libssh2_channel_write(channel, mes->c_str(), mes->length()) != mes->length())
        return -1;
    return 0;
    #endif
    #ifndef HAVE_LSSH2
    TNRC_ERR("There is no libssh2 installed");
    return -1;
    #endif
  }

  /** PluginXmr_session::_recv(std::string* mes, int buff_size)
  * @param mes: pointer to the string that the received message is written
  * @param buff_size: maximum message length
  * @return error_code: 1: No error0:timeout, -1:sock error 
  */
  int PluginXmr_session::_recv(std::string* mes, int buff_size)
  {
    #if HAVE_LSSH2
    int mes_len = 0;
    char* buffer = (char*)malloc(buff_size);

    libssh2_channel_set_blocking(channel, 0);
    mes_len = libssh2_channel_read(channel, buffer, buff_size);

    if(mes_len > 0)
    {
        mes->append(string((const char*)buffer, mes_len));
        free(buffer);
        return mes_len;
    }
    else
        return -1;
    #endif
    #ifndef HAVE_LSSH2
    TNRC_ERR("There is no libssh2 installed");
    return -1;
    #endif
  }

  int PluginXmr_session::close_connection()
  {
    #if HAVE_LSSH2
    libssh2_session_disconnect(session,"Normal Shutdown");
    libssh2_session_free(session); 
    if(close(s_desc) == -1)
        return -1;
    return 0;
    #endif
    #ifndef HAVE_LSSH2
    TNRC_ERR("There is no libssh2 installed");
    return -1;
    #endif
  }

  PluginXmr_session::~PluginXmr_session()
  {
    close_connection();
  }

  int PluginXmr_session::fd()
  {
    return s_desc;
  }

  /** add new fixed link parameters or modify existing fixed link parameters (usefull for AP Data model)*/
  void PluginXmr::tnrcsp_set_fixed_port(uint32_t key, int flags, g2mpls_addr_t rem_eq_addr, port_id_t rem_port_id, uint32_t no_of_wavelen, uint32_t max_bandwidth, gmpls_prottype_t protection)
  {
    fixed_port_map[key] = ((tnrcsp_foundry_fixed_port_t){flags, rem_eq_addr, rem_port_id, no_of_wavelen, max_bandwidth, protection, false});
  }

  /** search for first free vlanid*/
  int PluginXmr::first_free_vlanid(tnrc_portid_t portid_in, tnrc_portid_t portid_out)
  {
    int min = atoi(vlan_number.c_str());
    int i = min;
    bool find = true;
    list<tnrc_sp_xc_key>::iterator it;
    while(find)
    {
      find = false;
      for(it=vlan_list.begin(); it!=vlan_list.end(); it++ )
      {
        if(it->vlanid == i)
        {
          i++;
          find = true;
          break;
        }
      }
      if(i>min+atoi(vlan_pool.c_str()))
      {
        find = false;
        i = -1;
      }
    }

    return i;
  }

  /** search for vlanid in label_map*/
  int PluginXmr::find_vlanid(tnrc_portid_t portid)
  {
    int vlanid;
    bool find = false;
    list<tnrc_sp_xc_key>::iterator it;
    for(it=vlan_list.begin(); it!=vlan_list.end(); it++ )
    {
      if (it->port_in == portid || it->port_out == portid)
      {
        vlanid = it->vlanid;
        find = true;
        break;
      }
    }

    return vlanid;
  }

  //==============================================================================
  /**  Class PluginXmr_listen
  *  Listen and process received Xmr responses
  */
  void PluginXmr_listen::process_message(std::string* mes)
  {
    if(plugin->command_type == TNRC_NO_COMMAND) process_no_command_response();
    else if (plugin->command_type == TNRC_CREATE_VLAN) process_create_response();
    else if (plugin->command_type == TNRC_DELETE_VLAN) process_delete_response();
    else if (plugin->command_type == TNRC_ADD_PORT_IN) process_add_port_in_response();
    else if (plugin->command_type == TNRC_ADD_PORT_OUT) process_add_port_out_response();
    else if (plugin->command_type == TNRC_LOGIN) process_login_response();
    else if (plugin->command_type == TNRC_COMMIT) process_commit_response();
    else if (plugin->command_type == TNRC_PRECOMMIT) process_precommit_response();
    else if (plugin->command_type == TNRC_DESTROY_COMMIT) process_commit_response();
    else if (plugin->command_type == TNRC_ADD_FLOODING) process_add_flooding_response();
    else if (plugin->command_type == TNRC_CHECK) process_check_response();
    else if (plugin->command_type == TNRC_CHECK_DESTROY) process_destroy_check_response();
    else if (plugin->command_type == TNRC_EXIT) process_exit_response();
    else if (plugin->command_type == TNRC_GET_PORTS_RESOURCES) process_get_ports_response();
    else if (plugin->command_type == TNRC_GET_VLANS_RESOURCES) process_get_vlans_response();
    else if (plugin->command_type == TNRC_RESERVE_FINAL) process_reserve_show_response();
    else if (plugin->command_type == TNRC_UNRESERVE_FINAL) process_unreserve_show_response();
  }

  void PluginXmr_listen::process_no_command_response()
  {
      plugin->xmr_response = "";
      plugin->command_type = TNRC_NO_COMMAND;
  }

  void PluginXmr_listen::process_create_response()
  {
      string xmr_response_nl = plugin->xmr_response + "\n";

      string first_no_important_line = pop_line(&xmr_response_nl);
      string first_line = pop_line(&xmr_response_nl);
      char buff[100];
      sprintf(buff, "(config-vlan-%i)#", plugin->param->vlanid);
      string expected_str(buff);
      xmr_response_nl = plugin->xmr_response;
      plugin->xmr_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(foundry_find(&first_line, expected_str))
        _trncsp_foundry_xc_process(TNRCSP_STATE_CREATE_VLAN, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      else if(plugin->repeat_mes)
      {
        plugin->repeat_mes = false;
        _trncsp_foundry_xc_process(TNRCSP_STATE_INIT, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      }
      else
        _trncsp_foundry_xc_process(TNRCSP_STATE_CHECK, plugin->param, TNRCSP_RESULT_GENERICERROR, &xmr_response_nl);
  }

  void PluginXmr_listen::process_delete_response()
  {
      std::string xmr_response_nl = plugin->xmr_response + "\n";
      std::string first_no_important_line = pop_line(&xmr_response_nl);
      std::string first_line = pop_line(&xmr_response_nl);
      char buff[100];
      sprintf(buff, "(config)#");
      std::string expected_str(buff);
      sprintf(buff, "not configured");
      std::string expected_str_no_vlan(buff);
      xmr_response_nl = plugin->xmr_response;
      plugin->xmr_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(foundry_find(&first_line, expected_str) || foundry_find(&first_line, expected_str_no_vlan))
        _trncsp_foundry_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      else if(plugin->repeat_mes)
      {
        plugin->repeat_mes = false;
        _trncsp_foundry_xc_process(TNRCSP_STATE_INIT, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      }
      else
        _trncsp_foundry_xc_process(TNRCSP_STATE_CHECK, plugin->param, TNRCSP_RESULT_GENERICERROR, &xmr_response_nl);
  }

  void PluginXmr_listen::process_add_port_in_response()
  {
      std::string xmr_response_nl = plugin->xmr_response + "\n";
      std::string first_no_important_line = pop_line(&xmr_response_nl);
      std::string first_line = pop_line(&xmr_response_nl);
      char buff[100];
      sprintf(buff, "(config-vlan-%i)#", plugin->param->vlanid);
      std::string expected_str(buff);
      xmr_response_nl = plugin->xmr_response;
      plugin->xmr_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(foundry_find(&first_line, expected_str))
        _trncsp_foundry_xc_process(TNRCSP_STATE_ADD_PORT_IN, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      else if(plugin->repeat_mes)
      {
        plugin->repeat_mes = false;
        _trncsp_foundry_xc_process(TNRCSP_STATE_CREATE_VLAN, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      }
      else
        _trncsp_foundry_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &xmr_response_nl);
  }

  void PluginXmr_listen::process_add_flooding_response()
  {
      std::string xmr_response_nl = plugin->xmr_response + "\n";
      std::string first_no_important_line = pop_line(&xmr_response_nl);
      std::string first_line = pop_line(&xmr_response_nl);
      char buff[100];
      sprintf(buff, "(config-vlan-%i)#", plugin->param->vlanid);
      std::string expected_str(buff);
      xmr_response_nl = plugin->xmr_response;
      plugin->xmr_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(foundry_find(&first_line, expected_str))
        _trncsp_foundry_xc_process(TNRCSP_STATE_ADD_FLOODING, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      else if(plugin->repeat_mes)
      {
        plugin->repeat_mes = false;
        _trncsp_foundry_xc_process(TNRCSP_STATE_ADD_PORT_OUT, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      }
      else
        _trncsp_foundry_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &xmr_response_nl);
  }

  void PluginXmr_listen::process_add_port_out_response()
  {
      std::string xmr_response_nl = plugin->xmr_response + "\n";
      std::string first_no_important_line = pop_line(&xmr_response_nl);
      std::string first_line = pop_line(&xmr_response_nl);
      char buff[100];
      sprintf(buff, "(config-vlan-%i)#", plugin->param->vlanid);
      std::string expected_str(buff);
      xmr_response_nl = plugin->xmr_response;
      plugin->xmr_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(foundry_find(&first_line, expected_str))
        _trncsp_foundry_xc_process(TNRCSP_STATE_ADD_PORT_OUT, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      else if(plugin->repeat_mes)
      {
        plugin->repeat_mes = false;
        _trncsp_foundry_xc_process(TNRCSP_STATE_ADD_PORT_IN, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      }
      else
        _trncsp_foundry_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &xmr_response_nl);
  }

  void PluginXmr_listen::process_login_response()
  {

      char buff[32];
      sprintf(buff, "(config)#");
      std::string expected_str(buff);

      if(foundry_find(&plugin->xmr_response, expected_str))
      {
        plugin->SSH2_status = TNRC_SP_STATE_LOGGED_IN;
        TNRC_INF("TNRC_LISTEN - logged in\n");
        plugin->_activity = GET_XC;
      }
      else
      {
        plugin->SSH2_status = TNRC_SP_STATE_NOT_LOGGED_IN;
        TNRC_INF("TNRC_LISTEN - NOT logged in\n");
      }
      plugin->xmr_response="";
      plugin->command_type = TNRC_NO_COMMAND;
  }

  void PluginXmr_listen::process_precommit_response()
  {
      std::string xmr_response_nl = plugin->xmr_response;
      char buff[100];
      sprintf(buff, "(config)#");
      std::string expected_str(buff);
      plugin->command_type = TNRC_NO_COMMAND;
      plugin->xmr_response="";

      if(foundry_find(&xmr_response_nl, expected_str))
        _trncsp_foundry_xc_process(TNRCSP_STATE_EXIT, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      else if(plugin->repeat_mes)
      {
        plugin->repeat_mes = false;
        _trncsp_foundry_xc_process(TNRCSP_STATE_ADD_FLOODING, plugin->param, TNRCSP_RESULT_NOERROR, &xmr_response_nl);
      }
      else
        _trncsp_foundry_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &xmr_response_nl);
  }

  void PluginXmr_listen::process_commit_response()
  {
      std::string tmp_xmr_response = plugin->xmr_response;
      char buff[100];
      sprintf(buff, "Write startup-config done.");
      std::string expected_str(buff);
      plugin->xmr_response="";

      if(foundry_find(&tmp_xmr_response, expected_str))
      {
        plugin->command_type = TNRC_NO_COMMAND;
        _trncsp_foundry_xc_process(TNRCSP_STATE_CHECK, plugin->param, TNRCSP_RESULT_NOERROR, &tmp_xmr_response);
      }
      else if(plugin->command_type == TNRC_COMMIT || (plugin->command_type == TNRC_COMMIT && plugin->repeat_mes))
      {
        plugin->command_type = TNRC_NO_COMMAND;
        if(plugin->repeat_mes)
          plugin->repeat_mes = false;
        _trncsp_foundry_xc_process(TNRCSP_STATE_EXIT, plugin->param, TNRCSP_RESULT_NOERROR, &tmp_xmr_response);
      }
      else if(plugin->command_type == TNRC_DESTROY_COMMIT || (plugin->command_type == TNRC_DESTROY_COMMIT && plugin->repeat_mes))
      {
        plugin->command_type = TNRC_NO_COMMAND;
        if(plugin->repeat_mes)
          plugin->repeat_mes = false;
        _trncsp_foundry_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_NOERROR, &tmp_xmr_response);
      }
      else
      {
        plugin->command_type = TNRC_NO_COMMAND;
        _trncsp_foundry_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &tmp_xmr_response);
      }
  }

  void PluginXmr_listen::process_check_response()
  {
      std::string tmp_xmr_response = plugin->xmr_response;
      char buff[100];
      sprintf(buff, "PORT-VLAN %i, Name phosphorus%i,", plugin->param->vlanid, plugin->param->vlanid);
      std::string expected_str(buff);
      plugin->xmr_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(foundry_find(&tmp_xmr_response, expected_str))
        _trncsp_foundry_xc_process(TNRCSP_STATE_FINAL, plugin->param, TNRCSP_RESULT_NOERROR, &tmp_xmr_response);
      else if(plugin->repeat_mes)
      {
        plugin->repeat_mes = false;
        _trncsp_foundry_xc_process(TNRCSP_STATE_CHECK, plugin->param, TNRCSP_RESULT_NOERROR, &tmp_xmr_response);
      }
      else
        _trncsp_foundry_xc_process(TNRCSP_STATE_FINAL, plugin->param, TNRCSP_RESULT_GENERICERROR, &tmp_xmr_response);
  }

  void PluginXmr_listen::process_destroy_check_response()
  {
      std::string tmp_xmr_response = plugin->xmr_response;
      char buff[100];
      sprintf(buff, "PORT-VLAN %i, Name phosphorus%i,", plugin->param->vlanid, plugin->param->vlanid);
      std::string expected_str(buff);
      plugin->xmr_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(!foundry_find(&tmp_xmr_response, expected_str))
        _trncsp_foundry_xc_process(TNRCSP_STATE_FINAL, plugin->param, TNRCSP_RESULT_NOERROR, &tmp_xmr_response);
      else if(plugin->repeat_mes)
      {
        plugin->repeat_mes = false;
        _trncsp_foundry_xc_process(TNRCSP_STATE_CHECK, plugin->param, TNRCSP_RESULT_NOERROR, &tmp_xmr_response);
      }
      else
        _trncsp_foundry_xc_process(TNRCSP_STATE_FINAL, plugin->param, TNRCSP_RESULT_GENERICERROR, &tmp_xmr_response);
  }

  void PluginXmr_listen::process_exit_response()
  {

  }

  void PluginXmr_listen::process_get_ports_response()
  {
      if(plugin->repeat_mes)
      {
        plugin->repeat_mes = false;
        plugin->xmr_response="";
        plugin->command_type = TNRC_NO_COMMAND;
        plugin->_activity = GET_EQUIPMENT;
      }
      else
      {
        save_equipments(plugin->xmr_response);
        plugin->xmr_response="";
        plugin->command_type = TNRC_NO_COMMAND;
        if(!plugin->action_list.empty())
          plugin->_activity = CHECK_ACTION;
        else
          plugin->SSH2_status = TNRC_SP_STATE_END_SESSION;
      }
  }

  void PluginXmr_listen::process_get_vlans_response()
  {
    if(plugin->repeat_mes)
    {
      plugin->repeat_mes = false;
      plugin->xmr_response="";
      plugin->command_type = TNRC_NO_COMMAND;
      plugin->_activity = GET_XC;
    }
    else
    {
      save_channels(plugin->xmr_response);
      plugin->xmr_response="";
      plugin->command_type = TNRC_NO_COMMAND;
      plugin->_activity = GET_EQUIPMENT;
    }
  }

  void PluginXmr_listen::process_reserve_show_response()
  {
    //TODO Plugin have to check if ports are "free"

    std::string tmp_xmr_response = plugin->xmr_response;
    plugin->xmr_response="";
    plugin->command_type = TNRC_NO_COMMAND;

    _trncsp_foundry_xc_process(TNRCSP_STATE_FINAL, plugin->param, TNRCSP_RESULT_NOERROR, &tmp_xmr_response);
  }

  void PluginXmr_listen::process_unreserve_show_response()
  {
    std::string tmp_xmr_response = plugin->xmr_response;
    plugin->xmr_response="";
    plugin->command_type = TNRC_NO_COMMAND;

    _trncsp_foundry_xc_process(TNRCSP_STATE_FINAL, plugin->param, TNRCSP_RESULT_NOERROR, &tmp_xmr_response);
  }

  void PluginXmr_listen::_trncsp_foundry_xc_process(int state, void* parameters, tnrcsp_result_t result, string* response_block)
  {
      plugin->trncsp_foundry_xc_process((tnrcsp_foundry_xc_state)state, (xc_foundry_params*)parameters, result, response_block);
  }

  void PluginXmr_listen::save_equipments(std::string response_block)
  {
      std::string                 line = "";
      tnrc_portid_t               port_id;
      operational_state           oper_state;
      administrative_state        admin_state;
      tnrcsp_lsc_evmask_t         last_event;
      tnrc_sp_l2sp_speed          speed;
      tnrc_sp_l2sp_duplex_mode    duplex_mode;
      tnrcsp_l2sc_eqtype_t        equip_type;

      std::string                 tokens[10];
      map<tnrc_portid_t, tnrcsp_l2sc_resource_detail_t> tmp_resource_list;

      //ignofre first line withc command
      line = pop_line(&response_block);

      //start process 
      while(response_block.length() > 0)
      {
        line = pop_line(&response_block);
        if(line.length()<64)
            continue;
        //Line format:
        //Port  Link L2 State  Dupl Speed Trunk Tag Priori MAC            Name
        //1/1   Up   Forward   Full 10G   None  No  level0 000c.dbdf.d400 do_XMR2

        tokens[0] = line.substr(0,6); //Port id
        tokens[1] = line.substr(6,5); //Admin/Oper state
        tokens[2] = line.substr(11,10); //L2 state - not used in this version
        tokens[3] = line.substr(21,5); //Duplex
        tokens[4] = line.substr(26,6); //Speed
        tokens[5] = line.substr(32,6); //Trunk - not used in this version
        tokens[6] = line.substr(38,4); //Tag (Yes/No)- not used in this version
        tokens[7] = line.substr(42,7); //Prioritate - not used in this version
        tokens[8] = line.substr(49,14); //Mac address - not used in this version
        tokens[9] = line.substr(63); //Name - not used in this version


        //Generate port id;
        int first_space = tokens[0].find_first_of(' ');
        std::string portId = tokens[0].substr(0,first_space);
        char* port = (char *)portId.c_str();
        port_id = foundry_decode_portid(port);
        if(port_id == 0)
            continue;

        //Admin/Oper state setup
        if(tokens[1].find("Up") != std::string::npos)
        {
            oper_state = UP;
            admin_state = ENABLED;
        }
        else if(tokens[1].find("Down") != std::string::npos)
        {
            oper_state = DOWN;
            admin_state = ENABLED;
        }
        else
        {
            oper_state = DOWN;
            admin_state = DISABLED;
        }

        //Duplex type setup
        if(tokens[3].find("Full") != std::string::npos)
            duplex_mode = TNRC_SP_L2SC_DUPLEX_FULL;
        else if(tokens[3].find("Half") != std::string::npos)
            duplex_mode = TNRC_SP_L2SC_DUPLEX_HALF;
        else if(tokens[3].find("Auto") != std::string::npos)
            duplex_mode = TNRC_SP_L2SC_DUPLEX_AUTO;
        else
            duplex_mode = TNRC_SP_L2SC_DUPLEX_NONE;

        //Interface speed setup
        if(tokens[4].find("10M") != std::string::npos)
            speed = TNRC_SP_L2SC_SPEED_10M;
        else if(tokens[4].find("100M") != std::string::npos)
            speed = TNRC_SP_L2SC_SPEED_100M;
        else if(tokens[4].find("1G") != std::string::npos)
            speed = TNRC_SP_L2SC_SPEED_1G;
        else if(tokens[4].find("10G") != std::string::npos)
            speed = TNRC_SP_L2SC_SPEED_10G;
        else
            speed = TNRC_SP_L2SC_SPEED_NONE;

        //Interface type setup (Ethernet by default)
        equip_type = TNRC_SP_L2SC_ETH;

        //if port is part of a external VLAN XCONNECTED event is send
        tnrcsp_evmask_t e = 0;
        tnrc_sp_xc_key key = find_vlanid_from_portid(port_id);
        tnrc_sp_xc_key key2 = {key.port_out, key.port_in, key.vlanid};
        bool found = false;
        if (plugin->xc_map.count(key) == 0 && plugin->xc_map.count(key2) == 0 && key.port_in != 0 && key.port_out != 0)
        {
          e = EVENT_PORT_XCONNECTED;
          list<tnrc_sp_xc_key>::iterator it;
          for(it = external_xc.begin(); it!= external_xc.end(); it++)
            if ((it->port_in == key.port_in && it->port_out == key.port_out) || (it->port_in == key.port_out && it->port_out == key.port_in))
              found = true;
          if (!found)
            external_xc.insert(external_xc.end(),key);
        }

        list<tnrc_sp_xc_key>::iterator it;
        tnrc_portid_t port_i_o;
        if(external_xc.size() != 0)
        {
          for(it = external_xc.begin(); it!= external_xc.end(); it++)
          {
            if(it->port_in == port_id || it->port_out == port_id)
            {
              if(it->port_in == port_id)
                port_i_o = it->port_out;
              else
                port_i_o = it->port_in;
              bool found = true;
              list<tnrcsp_l2sc_xc_detail_t>::iterator it2;
              for(it2=xc_list.begin(); it2!=xc_list.end(); it2++ )
              {
                if(it2->portid_in == port_id || it2->portid_out == port_id)
                {
                  found = false;
                  break;
                }
              }
              if(found)
              {
                list<tnrc_portid_t>::iterator it3;
                bool f = true;
                for(it3 = xdisconnected_ports.begin(); it3!= xdisconnected_ports.end(); it3++)
                {
                  if(port_i_o == *it3)
                  {
                    external_xc.erase(it);
                    xdisconnected_ports.erase(it3);
                    f = false;
                    break;
                  }
                }
                e = EVENT_PORT_XDISCONNECTED;
                if(f)
                  xdisconnected_ports.push_back(port_id);
              }
              break;
            }
          }
        }

        //check if port_id is present in conf file
        if (find_board_id(port_id & 0xFFFF) != 0)
          // sent notification if state changed
          if(resource_list.count(port_id) == 0 || oper_state != resource_list[port_id].oper_state || e != 0)
          {
            register_event(port_id, oper_state, admin_state, speed, duplex_mode, equip_type, e);
          }
      }

      TNRC_INF("Resource_list contains:\n");
      map<tnrc_portid_t, tnrcsp_l2sc_resource_detail_t>::iterator it;
      for(it = resource_list.begin(); it != resource_list.end(); it++)
      {
        tnrcsp_l2sc_resource_detail_t d = it->second;
        TNRC_INF("   id=0x%x, op=%i, ad=%i, ev=%i, sp=%i, du=%i, eq=%i\n", it->first, d.oper_state, d.admin_state, d.last_event, d.speed, d.duplex_mode, d.equip_type);
      }
  }

  void PluginXmr_listen::register_event(tnrc_portid_t portid, operational_state operstate, administrative_state adminstate,tnrc_sp_l2sp_speed speed, tnrc_sp_l2sp_duplex_mode duplex_mode, tnrcsp_l2sc_eqtype_t equip_type, tnrcsp_evmask_t e)
  {
    TNRC_INF("Foundry XMR notify via reqister callback: port=0x%x, operstate=%s, adminstate=%s", portid, SHOW_OPSTATE(operstate), SHOW_ADMSTATE(adminstate));

    tnrcsp_event_t event;
    event.event_list = list_new();
    event.portid = portid;

    if (find_board_id(event.portid&0xFFFF) == 0)
    {
      TNRC_INF("Port not configured: port=0x%x, notification skipped", portid);
      return; //portid is not present in conf file
    }

    event.oper_state = operstate;
    event.admin_state = adminstate;
    if (e == 0)
      event.events = (operstate == UP) ? EVENT_PORT_OPSTATE_UP : EVENT_PORT_OPSTATE_DOWN;
    else
      event.events = e;
    plugin->tnrcsp_register_async_cb(&event);

    tnrcsp_l2sc_resource_detail_t details = {operstate, adminstate, event.events, speed, duplex_mode, equip_type};
    resource_list[portid] = details;
  }

  tnrc_portid_t  PluginXmr_listen::foundry_decode_portid(char* portid_str)
  {
    tnrc_portid_t portid = 0;
    int s,p;
    char *slot = strtok(portid_str,"/");
    if(slot)
    {
     s = atoi(slot);
     char *port = strtok(NULL,"/");
     if(port)
      p = atoi(port);
     else return 0;
    } 
    else return 0;

    int eqptid  = 1;
    int boardid = 1;
    portid = ((eqptid << 26) & 0xFC000000) | ((boardid << 16) & 0x03FF0000) | ((s << 8) & 0x0000FF00) | p;
    if((1 <= s) && (1 <= p) && (s <= 99) && (p <= 99))
     return portid;
    else
     return 0;
  }

  tnrc_sp_xc_key PluginXmr_listen::find_vlanid_from_portid(tnrc_portid_t portid)
  {
    tnrc_sp_xc_key key = {};
    list<tnrcsp_l2sc_xc_detail_t>::iterator it;
    for(it=xc_list.begin(); it!=xc_list.end(); it++ )
    {
      if ((portid & 0xFFFF) == (it->portid_in & 0xFFFF) )
      {
        if (it->vlanid > 1)
        {
          TNRC_INF("Port 0x%x is assigned to VLAN %d\n", portid, it->vlanid);
          key.port_in = it->portid_in;
          key.port_out = it->portid_out;
          key.vlanid = it->vlanid;
        }
        else
        {
          key.port_in = 0;
          key.port_out = 0;
          key.vlanid = 0;
        }
        return key;
      }
      else if((portid & 0xFFFF) == (it->portid_out & 0xFFFF))
      {
        if (it->vlanid > 1)
        {
          TNRC_INF("Port 0x%x is assigned to VLAN %d\n", portid, it->vlanid);
          key.port_in = it->portid_in;
          key.port_out = it->portid_out;
          key.vlanid = it->vlanid;
        }
        else
        {
          key.port_in = 0;
          key.port_out = 0;
          key.vlanid = 0;
        }
        return key;
      }
    }
    key.port_in = 0;
    key.port_out = 0;
    key.vlanid = 0;
    return key;
  }

  void PluginXmr_listen::save_channels(std::string response_block)
  {
    xc_list.clear();

    std::string line = "";
    std::string response = "";

    tnrc_portid_t port_in, port_out;
    vlanid_t vlanid;
    tnrcsp_l2sc_xc_state_t xc_state;

    //skip first 6 lines
    for(int i=0; i<6; i++)
        line = pop_line(&response_block);

    // and then remove all not used lines
    while(response_block.length() > 0)
    {
        line = pop_line(&response_block);
        if((line.find("PORT-VLAN") != std::string::npos) || (line.find("Untagged Ports") != std::string::npos)) {
            response.append(line);
            response.append("\n");
        }
    }

    std::string vlanid_str;
    std::string port_in_str;
    std::string port_out_str;
    tnrc_sp_xc_key node;
    plugin->vlan_list.clear();

    bool newLine = true;
    while(response.length() > 0)
    {
        //get line with vlan id
        if(newLine)
            line = pop_line(&response);
        int first_comma = line.find_first_of(',');
        vlanid_str = line.substr(10,first_comma);
        vlanid = atoi(vlanid_str.c_str());

        // get next line with ports or with next vlan id
        /*
        Important NOTE!
        1. only two untagged ports for each vlan are paresed
        (it is valid in case of vlans created with this software but may be not in case of other vlans)
        2. there is no possibility to determine which is an ingress or egress ports 
        */

        line = pop_line(&response);
        //line format:
        //Untagged Ports : ethe 8/1 ethe 8/4

        if(line.find("Untagged Ports") != std::string::npos)
        {
            line.erase(0,22);
            int p = line.find(" ");
            port_in_str = line.substr(0,p);
            line.erase(0,p+1);

            p = line.find(" ");
            line.erase(0,p+1);
            if((p = line.find(" ")) == std::string::npos)
                p = line.find("\n");
            port_out_str = line.substr(0,p);

            port_in = foundry_decode_portid((char*)port_in_str.c_str());
            port_out = foundry_decode_portid((char*)port_out_str.c_str());

            node.vlanid = vlanid;
            node.port_in = port_in;
            node.port_out = port_out;

            plugin->vlan_list.insert(plugin->vlan_list.end(), node);

            xc_state = TNRCSP_L2SC_XCSTATE_UNKNOWN;

            newLine = true;
        }
        // there are no untagged ports assigned to the vlan
        else
        {
            port_in = 0;
            port_out = 0;
            xc_state = TNRCSP_L2SC_XCSTATE_UNKNOWN;
            newLine = false;
        }

        tnrcsp_l2sc_xc_detail_t details = {port_in, port_out, vlanid, xc_state}; //TODO: last_event
        xc_list.insert(xc_list.end(), details);
    }
    TNRC_INF("xc_list contains:\n");
    list<tnrcsp_l2sc_xc_detail_t>::iterator it;
    for(it=xc_list.begin(); it!=xc_list.end(); it++ )
    {
      TNRC_INF("   0x%x, 0x%x, %i, %i\n", it->portid_in, it->portid_out, it->vlanid, it->xc_state);
    }
  }

  //==============================================================================
  /**  Class xc_foundry_params
  *  Crossconnection information
  */
  xc_foundry_params::xc_foundry_params(trncsp_xc_activity activity, tnrcsp_handle_t handle, 
                        tnrc_portid_t port_in, tnrc_portid_t port_out, vlanid_t vlanid,
                        xcdirection_t direction, tnrc_boolean_t _virtual,
                        tnrc_boolean_t activate, tnrcsp_response_cb_t response_cb,
                        void* response_ctxt, tnrcsp_notification_cb_t async_cb, void* async_ctxt)
  {
    this->activity      = activity;
    this->handle        = handle;
    this->port_in       = port_in;
    this->port_out      = port_out;
    this->vlanid        = vlanid;
    this->direction     = direction;
    this->_virtual      = _virtual;
    this->activate      = activate;
    this->response_cb   = response_cb;
    this->response_ctxt  = response_ctxt;
    this->async_cb      = async_cb;
    this->async_ctxt     = async_ctxt;
    this->deactivate    = activate;
    this->ingress_fid   = foundry_encode_portid(port_in);
    this->egress_fid    = foundry_encode_portid(port_out);


    this->activated       = false;
    this->repeat_count    = 0;
    this->final_result    = false;
    this->result          = TNRCSP_RESULT_NOERROR;

  }

  xc_foundry_params::~xc_foundry_params()
  {
    delete ingress_fid, egress_fid;
  }

//###################################################################
//########### FOUNDRY SP API FUNCTIONS ##############################
//###################################################################

  tnrcsp_result_t PluginXmr::tnrcsp_make_xc(tnrcsp_handle_t *handlep, 
                                            tnrc_portid_t portid_in, 
                                            label_t labelid_in,
                                            tnrc_portid_t portid_out, 
                                            label_t labelid_out, 
                                            xcdirection_t direction, 
                                            tnrc_boolean_t isvirtual,	
                                            tnrc_boolean_t activate, 
                                            tnrcsp_response_cb_t response_cb,
                                            void *response_ctxt, 
                                            tnrcsp_notification_cb_t async_cb, 
                                            void *async_ctxt)
  {
      TNRC_INF("Make xc is starting");
      *handlep = gen_identifier();
      vlanid_t vlanid = first_free_vlanid(portid_in, portid_out);

      if (!foundry_check_resources(portid_in, portid_out, vlanid)) 
        return TNRCSP_RESULT_PARAMERROR;

      xc_foundry_params* xc_params = new xc_foundry_params(TNRCSP_XC_MAKE, *handlep, portid_in, portid_out, vlanid, direction, isvirtual, activate, response_cb, response_ctxt, async_cb, async_ctxt);

      action_list.insert(action_list.end(), xc_params);
      wait_disconnect = true;
      return TNRCSP_RESULT_NOERROR;      
  }

  tnrcsp_result_t PluginXmr::tnrcsp_destroy_xc(tnrcsp_handle_t *handlep,
                                               tnrc_portid_t portid_in, 
                                               label_t labelid_in, 
                                               tnrc_portid_t portid_out, 
                                               label_t labelid_out,
                                               xcdirection_t direction,
                                               tnrc_boolean_t  isvirtual,	
                                               tnrc_boolean_t deactivate, 
                                               tnrcsp_response_cb_t response_cb,
                                               void *response_ctxt)
  {
    *handlep = gen_identifier();
    vlanid_t vlanid = find_vlanid(portid_in);

    TNRC_INF("Destroy is starting");

    if (!foundry_check_resources(portid_in, portid_out, vlanid))
        return TNRCSP_RESULT_PARAMERROR;

    xc_foundry_params* xc_params = new xc_foundry_params(TNRCSP_XC_DESTROY, *handlep, portid_in, portid_out, vlanid, direction, isvirtual, deactivate, response_cb, response_ctxt, NULL, NULL);

    action_list.insert(action_list.end(), xc_params);
    wait_disconnect = true;
    return TNRCSP_RESULT_NOERROR;
  }

  tnrcsp_result_t PluginXmr::tnrcsp_reserve_xc (tnrcsp_handle_t *handlep, 
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
    vlanid_t vlanid = first_free_vlanid(portid_in, portid_out);

    TNRC_INF("Reserve xc is starting");

    if (!foundry_check_resources(portid_in, portid_out, vlanid))
        return TNRCSP_RESULT_PARAMERROR;

    //tnrc_boolean_t isvirtual = 0;

    xc_foundry_params* xc_params = new xc_foundry_params (TNRCSP_XC_RESERVE, *handlep, portid_in, portid_out, vlanid, direction, isvirtual, false, response_cb, response_ctxt, NULL, NULL);
    action_list.insert(action_list.end(), xc_params);
    wait_disconnect = true;

    return TNRCSP_RESULT_NOERROR;
  }

  tnrcsp_result_t PluginXmr::tnrcsp_unreserve_xc (tnrcsp_handle_t *handlep, 
                                                  tnrc_port_id_t portid_in, 
                                                  label_t labelid_in, 
                                                  tnrc_port_id_t portid_out, 
                                                  label_t labelid_out, 
                                                  xcdirection_t        direction,
						  tnrc_boolean_t isvirtual,
                                                  tnrcsp_response_cb_t response_cb,
                                                  void *response_ctxt)
  {
    *handlep = gen_identifier();
    vlanid_t vlanid = first_free_vlanid(portid_in, portid_out);

    TNRC_INF("Unreserve xc is starting");

    if (!foundry_check_resources(portid_in, portid_out, vlanid))
        return TNRCSP_RESULT_PARAMERROR;

    tnrc_boolean_t _virtual = 0;

    xc_foundry_params* xc_params = new xc_foundry_params(TNRCSP_XC_UNRESERVE, *handlep, portid_in, portid_out, vlanid, direction, _virtual, false, response_cb, response_ctxt, NULL, NULL);
    action_list.insert(action_list.end(), xc_params);
    wait_disconnect = true;

    return TNRCSP_RESULT_NOERROR;
  }

  /** set observator for listed resources and some set of its states */
  tnrcsp_result_t PluginXmr::trncsp_l2sc_foundryxmr_register_async_cb(tnrcsp_event_t* events, unsigned int num)
  {
    for(int i=0; i<num; i++)
    {
        std::string* fid_in = foundry_encode_portid(events[i].portid);
        std::string* fid_out = foundry_encode_portid(events[i].portid);
        vlanid_t vlanid = events[i].labelid.value.fsc.portId;

        if(!fid_in || !fid_out)
            return TNRCSP_RESULT_PARAMERROR;

        opstate_t oper_state = events[i].oper_state;
        admstate_t admin_state = events[i].admin_state;
        tnrc_sp_last_events_item item = {&oper_state, &admin_state, list<string>(), vlanid, "", "", true};
        if(last_events.count(*fid_in) == 0)
            last_events[*fid_in] = item;
        else
        {
            last_events[*fid_in].operstate_filter = &oper_state;
            last_events[*fid_in].adminstate_filter = &admin_state;
            last_events[*fid_in].vlanid = vlanid;
            last_events[*fid_in].events_filter = list<string>();
            last_events[*fid_in].notify = true;
        }   
        if(last_events.count(*fid_out) == 0)
            last_events[*fid_out] = item;
        else
        {
            last_events[*fid_out].operstate_filter = &oper_state;
            last_events[*fid_out].adminstate_filter = &admin_state;
            last_events[*fid_out].vlanid = vlanid;
            last_events[*fid_out].events_filter = list<string>();
            last_events[*fid_out].notify = true;
        } 
        delete fid_in, fid_out;
    }
    return TNRCSP_RESULT_NOERROR;
  }

  tnrcsp_result_t PluginXmr::tnrcsp_register_async_cb (tnrcsp_event_t *events)
  {
    TNRC_INF("Register callback: portid=0x%x, labelid=0x%x, operstate=%d, adminstate=%d, event=%d", events->portid, events->labelid.value.lsc.wavelen, events->oper_state, events->admin_state, events->events);

    TNRC_OPSPEC_API::update_dm_after_notify (events->portid, events->labelid, events->events);

    TNRC_INF ("Data model updated\n");
    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  wq_item_status PluginXmr::wq_function (void *d)
  {
    TNRC_ERR("!!!!!!!!!!!! PluginXmr::wq_function function should be never executed");
    return WQ_SUCCESS;	
  }

  //-------------------------------------------------------------------------------------------------------------------
  void PluginXmr::del_item_data(void *d)
  {
    TNRC_ERR("!!!!!!!!!!!! PluginXmr::del_item_data function should be never executed");

    tnrcsp_action_t		*data;

    //get work item data
    data = (tnrcsp_action_t *) d;
    //delete data structure
    delete data;
  }

  tnrcsp_result_t PluginXmr::trncsp_l2sc_foundryxmr_get_resource_list(tnrcsp_resource_id_t** resource_listp, int* num)
  {

    if(SSH2_status != TNRC_SP_STATE_LOGGED_IN)
        return TNRCSP_RESULT_EQPTLINKDOWN;

    *num = listen_proc->resource_list.size();
    *resource_listp = new tnrcsp_resource_id_t[*num];
    int i = 0;

    map<tnrc_portid_t, tnrcsp_l2sc_resource_detail_t>::iterator it;
    for(it = listen_proc->resource_list.begin(); it != listen_proc->resource_list.end(); it++)
    {
        (*resource_listp)[i].portid = it->first;
        i++;
    }
    return TNRCSP_RESULT_NOERROR;
  }
}
