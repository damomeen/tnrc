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
//  Jakub Gutkowski       (PSNC)             <jgutkow_at_man.poznan.pl>
//

#include <zebra.h>
#include "vty.h"

#include "tnrcd.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_plugin_telnet_at_8000.h"

namespace TNRC_SP_AT8000
{

  //================================================================================
  /**
  *  Utils
  */

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

  /** create AP port represtation by adding eqpt and board ids */
  tnrc_portid_t create_at8000_api_port_id(tnrc_portid_t portid)
  {
    board_id_t board_id = find_board_id(portid);
    return TNRC_CONF_API::get_tnrc_port_id(1/*eqpt_id*/, board_id, portid);
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

  static int at8000_read_func (struct thread *thread);

  /** search description in the message, return True if description found */
  bool at8000_find(std::string* message, std::string description)
  {
    if(!message)
        return false;
    return message->find(description) != -1;
  }

  std::string delete_first_spaces(std::string message)
  {
    int i = 0;
    char s = 115;
    char e = 101;
    while(message[i] != s && message[i] != e)
      i++;
    std::string temp = message.substr(i);
    return temp;
  }

  std::string delete_first_spaces_2(std::string message)
  {
    int i = 0;
    char space = 32;
    while(message[i] == space)
      i++;
    std::string temp = message.substr(i);
    return temp;
  }

  /** generates port id in AT8000 notation ePORT (i.e.: e10); returns NULL in case of any error */
  std::string* at8000_encode_portid(tnrc_portid_t portid)
  {
    int slot = portid;
    slot = (slot >> 8) & 0x000000FF;
    int port = portid;
    port = port & 0x000000FF;

    if((1 > port) || (port > 42))//TODO check max port id
    {
        TNRC_WRN("Port_id cannot be decomposed - out of range 01 - 42!\n");
        return NULL;
    }
    char result_portid[16];
    sprintf(result_portid, "%d", port);
    std::string* port_id = new std::string(result_portid);

    return port_id;
  }

  bool at8000_check_vlanid(vlanid_t vlanid)
  {
    if( vlanid>1 && vlanid<=4090 )//TODO check max vlanid
        return true;
    else
        return false;
  }

  /** check if given resources are correct as an vlan creation parametrers*/
  bool at8000_check_resources(tnrc_portid_t port_in, tnrc_portid_t port_out, vlanid_t vlanid)
  {
    bool success;
    std::string* ingress_id = at8000_encode_portid(port_in);
    std::string* egress_id = at8000_encode_portid(port_out);

    if(!ingress_id || !egress_id || !at8000_check_vlanid(vlanid))
    {
        success = false;
        TNRC_INF("At8000_check_resources no success, ingress_id:e%s, egress_id:e%s, vlanid:%d",ingress_id->c_str(),egress_id->c_str(), vlanid);
    }
    else
    {
        success = true;
        TNRC_INF("At8000_check_resources success, ingress_id:e%s, egress_id:e%s, vlanid:%d",ingress_id->c_str(),egress_id->c_str(), vlanid);
    }
    delete ingress_id, egress_id;
    return success;
  }

  //================================================================================
  /**  Class PluginAT8000
  *  Abstract Specyfic Part class for devices with CLI-SSH interface
  */
  PluginAT8000::PluginAT8000(): t_read(NULL), t_retrive(NULL)
  {
    handle_ = 1;
    xc_id_ = 1;
    wqueue_ = work_queue_new (TNRC_MASTER.getMaster(), AT8000_WQ);
    wqueue_->spec.workfunc = workfunction;
    wqueue_->spec.del_item_data = delete_item_data;
    wqueue_->spec.max_retries = 5;
    //wqueue_->spec.hold = 500;
    sock = NULL;
    TELNET_status = TNRC_SP_STATE_NO_SESSION;
    _activity  = GET_XC;
    listen_proc = new PluginAT8000_listen(this);
    xc_bidir_support_ = true;
    read_thread = false;
    send_space = false;
    wait_disconnect = false;
    identifier = 0;
    which_port = 1;
    check_status = 0;
  }

  PluginAT8000::PluginAT8000(std::string name): t_read(NULL), t_retrive(NULL)
  {
    name_ = name;
    handle_ = 1;
    xc_id_ = 1;
    wqueue_ = work_queue_new (TNRC_MASTER.getMaster(), AT8000_WQ);
    wqueue_->spec.workfunc = workfunction;
    wqueue_->spec.del_item_data = delete_item_data;
    wqueue_->spec.max_retries = 5;
    //wqueue_->spec.hold = 500;
    sock = NULL;
    TELNET_status = TNRC_SP_STATE_NO_SESSION;
    _activity  = GET_XC;
    listen_proc = new PluginAT8000_listen(this);
    xc_bidir_support_ = true;
    read_thread = false;
    send_space = false;
    wait_disconnect = false;
    identifier = 0;
    which_port = 1;
    check_status = 0;
  }

  PluginAT8000::~PluginAT8000()
  {
    close_connection();
  }

  /** send password 
   * @return 0 if password send successful
   */
  int PluginAT8000::at8000_password()
  {
    char buff[100];
    sprintf(buff, password.c_str());
    string str(buff);
    str.append("\n");
    asynch_send(&str, NULL, TNRC_PASS);
    return 0;
  }

  /** send login commnad 
   * @return 0 if login command send successful
   */
  int PluginAT8000::at8000_log_in()
  {
    char buff[100];
    sprintf(buff, login.c_str());
    string str(buff);
    str.append("\n");
    asynch_send(&str, NULL, TNRC_LOGIN);
    return 0;
  }

  void PluginAT8000::retriveT(thread *retrive)
  {
    t_retrive = retrive;
  }

  void PluginAT8000::readT(thread *read)
  {
    t_read = read;
  }

  /** PluginAT8000::getFd()
  * @return FileDescriptor
  */
  int PluginAT8000::getFd()
  {
    return sock->fd();
  }

  /** PluginAT8000::at8000_connect()
  * @return 0 if telnet connection is established
  */
  int PluginAT8000::at8000_connect()
  {
    int result = sock->_connect();
    if (result == 0)
    {
      read_thread = true;
      TELNET_status = TNRC_SP_STATE_DONE;
      command_type = TNRC_PRELOGIN;
      TNRC_INF("Connected OK");
    }
    else
    {
      TNRC_ERR("Connection Error");
    }
    return result;
  }

  int PluginAT8000::at8000_disconnect()
  {
    int result;
    result = close_connection();
    return result;
  }

  /** PluginAT8000::asynch_send
  * @param command 
  * @param state tnrc sp state
  * @param parameters
  * @param cmd_type
  * @return
  */
  tnrcsp_result_t PluginAT8000::asynch_send(std::string* command, void* parameters, tnrc_l2sc_cmd_type_t cmd_type)
  {
    /* send SSH2 CLI command */
    if ((TELNET_status == TNRC_SP_STATE_NOT_LOGGED_IN) || (TELNET_status == TNRC_SP_STATE_LOGGED_IN) || (TELNET_status == TNRC_SP_STATE_NOT_LOGGED_IN_PASS))
    {
        if(TELNET_status == TNRC_SP_STATE_NOT_LOGGED_IN || TELNET_status == TNRC_SP_STATE_NOT_LOGGED_IN_PASS)
          TELNET_status = TNRC_SP_STATE_DONE;
        TNRC_INF("Send command: %s\n", command->c_str());
        read_thread = true;
        if(sock->_send(command) == -1)
        {
            TNRC_WRN("Connection reset by peer\n");
            TELNET_status = TNRC_SP_STATE_NO_SESSION;
            return TNRCSP_RESULT_EQPTLINKDOWN;
        }
        at8000_response.assign("");
        command_type = cmd_type;
        sent_command = *command;
        param = (xc_at8000_params*)parameters;
        return TNRCSP_RESULT_NOERROR;
    }
    else
    {
        TNRC_WRN("Send unsuccessful\n");
        if(TELNET_status == TNRC_SP_STATE_NO_SESSION)
            return TNRCSP_RESULT_EQPTLINKDOWN;
        return TNRCSP_RESULT_INTERNALERROR;
    }
  }

  /** Read thread's rountine (thread *t_read) */
  int PluginAT8000::doRead()
  {
    std::string recMsg("");
    at8000_response = "";
    int l;
    l = sock->_recv(&recMsg, 4096);
    temp_at8000_response.append(recMsg);

    if((temp_at8000_response.find("#")) != -1 || (temp_at8000_response.find("User Name:")) != -1 || (temp_at8000_response.find("Password:")) != -1 || temp_at8000_response.find("<space>") != -1)
    {

      at8000_response = temp_at8000_response;
      temp_at8000_response = "";
      read_thread = false;
      TNRC_INF("Processing received message \n%s", at8000_response.c_str());
      listen_proc->process_message(&at8000_response);
      return -1;
    }
    return 0;
  }

  /** send ports retrieve commnad */
  void PluginAT8000::get_ports_resources()
  {
    /* send request for all available ports resources*/
    char buff[100];
    std::string str;
    if(send_space)
    {
      send_space = false;
      sprintf(buff, " ");
      str = buff;
    }
    else
    {
      sprintf(buff, "show interfaces status");
      str = buff;
      str.append("\n");
    }
    asynch_send(&str, NULL, TNRC_GET_PORTS_RESOURCES);
  }

  /** send channels retrieve commnad */
  void PluginAT8000::get_all_xc()
  {
    /* send request for all existing vlan instances*/
    char buff[100];
    std::string str;
    if(send_space)
    {
      send_space = false;
      sprintf(buff, " ");
      str = buff;
    }
    else
    {
      sprintf(buff, "show vlan");
      str = buff;
      str.append("\n");
    }
    asynch_send(&str, NULL, TNRC_GET_VLANS_RESOURCES);
  }

 /** Backgrount thread's rountine (thread *t_retrive)
  * - connection with device
  * - equipment
  * - connections
  */
  int PluginAT8000::doRetrive()
  {
    check_status++; 
    if (check_status == 300) //TODO change this value now its 5 min (300sec) 
    {
      check_status = 0;
      wait_disconnect = true;
    }
    int result = -1;
    switch(TELNET_status)
    {
      case TNRC_SP_STATE_NO_SESSION:
          TNRC_INF("Trying to connect");
          result = at8000_connect();
        break;
      case TNRC_SP_STATE_NOT_LOGGED_IN:
          TNRC_INF("Trying to logg in");
          result = at8000_log_in();
        break;
      case TNRC_SP_STATE_NOT_LOGGED_IN_PASS:
          TNRC_INF("Sending password");
          result = at8000_password();
        break;
      case TNRC_SP_STATE_LOGGED_IN:
        switch (_activity)
        {
          case GET_EQUIPMENT:
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
                xc_at8000_params* xc_params = action_list.front();
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
                            TNRC_INF("AT-SP (isvirtual response) - xc notification to AP with %s", SHOW_TNRCSP_RESULT(result));
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
                        TNRC_INF("AT-SP (isvirtual response) - xc notification to AP with %s", SHOW_TNRCSP_RESULT(xc_params->result));
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
                              TNRC_INF("AT-SP (isvirtual response) - xc notification to AP with %s", SHOW_TNRCSP_RESULT(result));
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
                          TNRC_INF("AT-SP (isvirtual response) - xc notification to AP with %s", SHOW_TNRCSP_RESULT(xc_params->result));
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
                  tnrcsp_at8000_xc_process(TNRCSP_STATE_INIT, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                  _activity = DONE;
                }
                else
                  _activity = GET_XC;

              }
              else if(action_list.front()->activity == TNRCSP_XC_DESTROY)
              {
                xc_at8000_params* xc_params = action_list.front();
                action_list.pop_front();
                tnrcsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                _activity = DONE;
              }
              else if(action_list.front()->activity == TNRCSP_XC_RESERVE)
              {
                xc_at8000_params* xc_params = action_list.front();
                action_list.pop_front();
                tnrcsp_at8000_xc_process(TNRCSP_STATE_INIT, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                _activity = DONE;
              }
              else if(action_list.front()->activity == TNRCSP_XC_UNRESERVE)
              {
                xc_at8000_params* xc_params = action_list.front();
                action_list.pop_front();
                tnrcsp_at8000_xc_process(TNRCSP_STATE_INIT, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                _activity = DONE;
              }
            }
            break;
        }
        break;
      case TNRC_SP_STATE_END_SESSION:
        TNRC_INF("Ending session");
        result = at8000_disconnect();
        TELNET_status = TNRC_SP_STATE_DISCONNECTED;
        break;
    }
    if (read_thread)
      readT(thread_add_event(master, at8000_read_func, this, 0));
    if (wait_disconnect && TELNET_status == TNRC_SP_STATE_DISCONNECTED)
    {
      wait_disconnect = false;
      TELNET_status = TNRC_SP_STATE_NO_SESSION;
    }
    return result;
  }

  int PluginAT8000::close_connection()
  {
    if(sock->close_connection() == -1)
        return -1;
    return 0;
  }

  /**
  * Thread read function
  */
  static int at8000_read_func (struct thread *thread)
  {
    int fd;
    PluginAT8000 *p;

    fd = THREAD_FD (thread);
    p = static_cast<PluginAT8000 *>(THREAD_ARG (thread));

    int result = p->doRead();
    return result;
  }

  /** 
  * Thread retrive function
  */
  int at8000_retrive_func (struct thread *thread)
  {
    PluginAT8000 *p;

    p = static_cast<PluginAT8000 *>(THREAD_ARG (thread));

    int result = p->doRetrive();
    p->retriveT(thread_add_timer(master, at8000_retrive_func, p, 1));
    return result;
  }

  /** Launch communication with device
   * @param location <ipAddress>:<port>:<path_to_configuration_file>
   */
  tnrcapiErrorCode_t
  PluginAT8000::probe (std::string location)
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

    TNRC_INF("Plugin Allied Telesis 8000 address %s", strAddress.c_str());
    if (inet_aton(strAddress.c_str(), &remote_address) != 1)
    {
      TNRC_ERR("Plugin Allied Telesis 8000::probe: wrong IP address %s given string is: %s", inet_ntoa(remote_address), strAddress.c_str());
      return TNRC_API_ERROR_GENERIC;
    }

    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 8000::probe: wrong location description: no port");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    strPort = location.substr(stPos, endPos-stPos);

    TNRC_INF("Plugin Port %s", strPort.c_str());
    if (sscanf(strPort.c_str(), "%d", &remote_port) != 1)
    {
      TNRC_ERR("Plugin Allied Telesis 8000::probe: wrong port (location = %s, readed port %d)", location.c_str(), remote_port);
      return TNRC_API_ERROR_GENERIC;
    }

    sock = new PluginAT8000_session(remote_address, remote_port, this);
    TNRC_INF("Plugin Allied Telesis 8000::probe: sesion data: ip %s, port %d", inet_ntoa(remote_address), remote_port);
    TELNET_status = TNRC_SP_STATE_NO_SESSION;

    //login
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 8000::probe: wrong location description: no login");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    login = location.substr(stPos, endPos-stPos);

    //password
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 8000::probe: wrong location description: no password");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    password = location.substr(stPos, endPos-stPos);

    //vlan number
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 8000::probe: wrong location description: no vlan number");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    vlan_number = location.substr(stPos, endPos-stPos);

    //vlan pool
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 8000::probe: wrong location description: no vlan pool");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    vlan_pool = location.substr(stPos, endPos-stPos);

    //backup vlan number
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 8000::probe: wrong location description: no backup vlan number");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    backup_vlan = location.substr(stPos, endPos-stPos);

    retriveT (thread_add_timer (master, at8000_retrive_func, this, 0));

    //getting eqpt config filename from config file
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 8000::probe: wrong location description: no path");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    strPath = location.substr(stPos, endPos-stPos);

    //read eqpuipment config file
    TNRC_INF("Plugin Allied Telesis 8000 configuration file %s", strPath.c_str());
    vty_read_config ((char *)strPath.c_str(), NULL);
    TNRC_INF("Allied Telesis 8000 extra information about ports readed");

    return TNRC_API_ERROR_NONE;
  }

  void PluginAT8000::tnrcsp_at8000_xc_process(tnrcsp_at8000_xc_state state, xc_at8000_params* parameters, tnrcsp_result_t response, string* data)
  {
    tnrcsp_result_t result;
    bool call_callback = false;
    if(parameters->activity == TNRCSP_XC_MAKE)
    {
        if(call_callback = tnrcsp_at8000_make_xc(state, parameters, response, data, &result))
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
                tnrcsp_at8000_destroy_xc(TNRCSP_STATE_DELETE_VLAN, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
            }
    }
    else if(parameters->activity == TNRCSP_XC_DESTROY)
    {
      if(call_callback = tnrcsp_at8000_destroy_xc(state, parameters, response, data, &result))
      {
        TNRC_INF("Vlan removing completed");
        tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out};
        if (xc_map.count(key) > 0)
          xc_map.erase(key);

        if(parameters->deactivate)
        {
          call_callback = false;
          parameters->activity = TNRCSP_XC_RESERVE;
          //call_callback = tnrcsp_at8000_reserve_xc(TNRCSP_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
          action_list.insert(action_list.end(), parameters);
          wait_disconnect = true;
        }
      }
    }
    else if(parameters->activity == TNRCSP_XC_RESERVE)
    {
      if(call_callback = tnrcsp_at8000_reserve_xc(state, parameters, response, data, &result))
      {
        if(result == TNRCSP_RESULT_NOERROR)
        {
          TNRC_INF("Reserve XC result NOERROR");
          parameters->activated = false;
          tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out, parameters->vlanid};
          xc_map[key] = parameters;
        }
        else if(result != TNRCSP_RESULT_BUSYRESOURCES)
        {
          call_callback = false;
          parameters->final_result = true;
          parameters->result = result;
          TNRC_WRN("Reservation unsuccessful -> unreserve resources\n");
          parameters->activity = TNRCSP_XC_UNRESERVE;
          call_callback = tnrcsp_at8000_unreserve_xc(TNRCSP_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
        }
      }
    }
    else if(parameters->activity == TNRCSP_XC_UNRESERVE)
    {
      if(call_callback = tnrcsp_at8000_unreserve_xc(state, parameters, response, data, &result))
      {
        TNRC_INF("Unreserve XC result NOERROR");
        tnrc_sp_xc_key key = {parameters->port_in, parameters->port_out};
        if (xc_map.count(key) > 0)//TODO check it  
          xc_map.erase(key);
      }
    }
    if(call_callback && parameters->response_cb)
    {
      if(parameters->final_result == true)
        result = parameters->result;
      TNRC_INF("AT-SP - xc notification to AP with %s", SHOW_TNRCSP_RESULT(result));
      parameters->response_cb(parameters->handle, result, parameters->response_ctxt);
      delete parameters;
    }
  }

  /** PluginAT8000::trncsp_at8000_make_xc
  *   make crossconnection
  */
  bool PluginAT8000::tnrcsp_at8000_make_xc(tnrcsp_at8000_xc_state state, xc_at8000_params* parameters, tnrcsp_result_t response, string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("make state: %i\n", state);
    if(state == TNRCSP_STATE_INIT)
    {
      *result = at8000_configure(parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_STATE_VLAN_DATABASE)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_vlan_database(parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_ADD_VLAN)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_add_vlan(parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_FIRST_EXIT)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_exit(TNRCSP_STATE_FIRST_EXIT, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_INTERFACE)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_interface(parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_ACCESS_VLAN)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_access_vlan(parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_SECOND_EXIT)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_exit(TNRCSP_STATE_SECOND_EXIT, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_THIRD_EXIT)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_exit(TNRCSP_STATE_THIRD_EXIT, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_CHECK)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_check(parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_FINAL)
    {
      *result = response;
      _activity = GET_XC;
      return true;
    }
    return false;
  }

 /** PluginAT8000::trncsp_at8000_destroy_xc
  *   destroy crossconnection
  */
  bool PluginAT8000::tnrcsp_at8000_destroy_xc(tnrcsp_at8000_xc_state state, xc_at8000_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("destroy state: %i\n", state);
    if(state == TNRCSP_STATE_DELETE_VLAN)
    {
      *result = at8000_configure(parameters);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_STATE_INTERFACE)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_interface(parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_ACCESS_VLAN)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_access_vlan(parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_VLAN_DATABASE)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_vlan_database(parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_VLAN_DESTROY)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_vlan_destroy(parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_SECOND_EXIT)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_exit(TNRCSP_STATE_SECOND_EXIT, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_THIRD_EXIT)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_exit(TNRCSP_STATE_THIRD_EXIT, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_FOURTH_EXIT)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_exit(TNRCSP_STATE_FOURTH_EXIT, parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_CHECK)
    {
      if(response != TNRCSP_RESULT_NOERROR)
      {
        *result = at8000_error_analyze(data, "...", TNRCSP_RESULT_GENERICERROR, response);//TODO
        return true;
      }
      else
      {
        *result = at8000_destroy_check(parameters);
        if(*result != TNRCSP_RESULT_NOERROR)
                return true;
      }
    }
    else if(state == TNRCSP_STATE_FINAL)
    {
      *result = response;
      _activity = GET_XC;
      return true;
    }
    return false;
  }

  /** PluginAT8000::trncsp_at8000_reserve_xc
  *   reserve crossconnection
  */
  bool PluginAT8000::tnrcsp_at8000_reserve_xc(tnrcsp_at8000_xc_state state, xc_at8000_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {

  }

  /** PluginAT8000::trncsp_at8000_unreserve_xc
  *   unreserve crossconnection
  */
  bool PluginAT8000::tnrcsp_at8000_unreserve_xc(tnrcsp_at8000_xc_state state, xc_at8000_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {

  }

  /** send configure command */
  tnrcsp_result_t PluginAT8000::at8000_configure(xc_at8000_params* parameters)
  {
    char buff[100];
    sprintf(buff, "configure");
    std::string command(buff);
    command.append("\n");

    return asynch_send(&command, parameters, TNRC_CONFIGURE);
  }

  /** send vlan database command */
  tnrcsp_result_t PluginAT8000::at8000_vlan_database(xc_at8000_params* parameters)
  {
    char buff[100];
    sprintf(buff, "vlan database");
    std::string command(buff);
    command.append("\n");

    return asynch_send(&command, parameters, TNRC_VLAN_DATABASE);
  }

  /** send add vlan command */
  tnrcsp_result_t PluginAT8000::at8000_add_vlan(xc_at8000_params* parameters)
  {
    char buff[100];
    sprintf(buff, "vlan %i", parameters->vlanid);
    std::string command(buff);
    command.append("\n");

    return asynch_send(&command, parameters, TNRC_ADD_VLAN);
  }

  /** send exit command */
  tnrcsp_result_t PluginAT8000::at8000_exit(tnrcsp_at8000_xc_state state, xc_at8000_params* parameters)
  {
    char buff[100];
    sprintf(buff, "exit");
    std::string command(buff);
    command.append("\n");

    if(state == TNRCSP_STATE_FIRST_EXIT)
      return asynch_send(&command, parameters, TNRC_FIRST_EXIT);
    else if(state == TNRCSP_STATE_SECOND_EXIT)
      return asynch_send(&command, parameters, TNRC_SECOND_EXIT);
    else if(state == TNRCSP_STATE_THIRD_EXIT)
      return asynch_send(&command, parameters, TNRC_THIRD_EXIT);
    else if(state == TNRCSP_STATE_FOURTH_EXIT)
      return asynch_send(&command, parameters, TNRC_FOURTH_EXIT);
  }

  /** send interface command */
  tnrcsp_result_t PluginAT8000::at8000_interface(xc_at8000_params* parameters)
  {
    char buff[100];

    if (which_port == 1)
    {
      sprintf(buff, "interface ethernet e%s", parameters->ingress_fid->c_str());
      which_port++;
    }
    else
    {
      which_port = 1;
      sprintf(buff, "interface ethernet e%s", parameters->egress_fid->c_str());
    }
    std::string command(buff);
    command.append("\n");

    return asynch_send(&command, parameters, TNRC_INTERFACE);
  }

  /** send general mode command */
  tnrcsp_result_t PluginAT8000::at8000_access_vlan(xc_at8000_params* parameters)
  {
    char buff[100];
    if(parameters->activity == TNRCSP_XC_MAKE)
      sprintf(buff, "switchport access vlan %i",parameters->vlanid);
    else
    {
      int v_i,v_e,p_i,p_e;
      v_i = atoi(backup_vlan.c_str());
      p_i = atoi(parameters->ingress_fid->c_str());
      v_i += p_i;
      v_e = atoi(backup_vlan.c_str());
      p_e = atoi(parameters->egress_fid->c_str());
      v_e += p_e;
      if(which_port == 1)
        sprintf(buff, "switchport access vlan %d",v_e);
      else
        sprintf(buff, "switchport access vlan %d",v_i);
    }
    std::string command(buff);
    command.append("\n");

    return asynch_send(&command, parameters, TNRC_ACCESS_VLAN);
  }

  /** send check command */
  tnrcsp_result_t PluginAT8000::at8000_check(xc_at8000_params* parameters)
  {
    char buff[100];
    sprintf(buff, "show vlan name %i",parameters->vlanid);
    std::string command(buff);
    command.append("\n");

    return asynch_send(&command, parameters, TNRC_CHECK);
  }

  /** send destroy command */
  tnrcsp_result_t PluginAT8000::at8000_vlan_destroy(xc_at8000_params* parameters)
  {
    char buff[100];
    sprintf(buff, "no vlan %i",parameters->vlanid);
    std::string command(buff);
    command.append("\n");

    return asynch_send(&command, parameters, TNRC_DESTROY);
  }

  /** send destroy check command */
  tnrcsp_result_t PluginAT8000::at8000_destroy_check(xc_at8000_params* parameters)
  {
    char buff[100];
    sprintf(buff, "show vlan name %i",parameters->vlanid);
    std::string command(buff);
    command.append("\n");

    return asynch_send(&command, parameters, TNRC_DESTROY_CHECK);
  }

  /** search description in the message and replace response if description found */
  tnrcsp_result_t PluginAT8000::at8000_error_analyze(string* message, std::string description, tnrcsp_result_t response, tnrcsp_result_t default_response)
  {
    if(!message)
      return TNRCSP_RESULT_INTERNALERROR;

    if(at8000_find(message, description))
      return response;
    else
      return default_response;
  }

   //TODO temporary solution
  /** search for first free vlanid*/
  int PluginAT8000::first_free_vlanid(tnrc_portid_t portid_in, tnrc_portid_t portid_out)
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

  //==============================================================================
  /** Class Xmr_session
  * Provides communication with SSH2 protocol using network connection
  */
  PluginAT8000_listen::PluginAT8000_listen(PluginAT8000 *owner)
  {
       plugin = owner;
  }

  PluginAT8000_session::PluginAT8000_session(in_addr address, int port,PluginAT8000 *owner)
  {
    plugin = owner;
    remote_address = address;
    remote_port = port;
  }

  /** PluginXmr_session::_connect()
  * @return error_code: 0: No error, -1:sock error 
  */
  int PluginAT8000_session::_connect()
  {
    s_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    TNRC_INF("Socket description = %d", s_desc);

    struct sockaddr_in remote;
    struct hostent *he;

    remote.sin_family         = AF_INET;
    remote.sin_port           = htons(remote_port);
    remote.sin_addr = remote_address;
    memset(&(remote.sin_zero), '\0', 8);  // clear the structure

    if(connect(s_desc, (struct sockaddr*)&remote, sizeof(remote)) == 0)
      return 0;
    else
    {
        TNRC_ERR("Unsuccessful telnet equipment connection: %i, %s\n", errno, strerror(errno));
        return -1;
    }
  }

  /** PluginAT8000_session::_send(std::string* mes)
  * @param mes: pointer to the string that is send to the equipment
  * @return error_code: 0: No error0, -1:sock error 
  */
  int PluginAT8000_session::_send(std::string* mes)
  {
    if(send(s_desc, mes->c_str(), mes->length(), 0) != mes->length())
      return -1;
    return 0;
  }

  /** PluginAT8000_session::_recv(std::string* mes, int buff_size)
  * @param mes: pointer to the string that the received message is written
  * @param buff_size: maximum message length
  * @return error_code: 1: No error0:timeout, -1:sock error 
  */
  int PluginAT8000_session::_recv(std::string* mes, int buff_size)
  {
    int mes_len;
    char* buffer;

    fd_set readfds;
    struct timeval tv = {1,0};

    FD_ZERO(&readfds);
    FD_SET(s_desc, &readfds);

    int ret_value = select(s_desc+1, &readfds, NULL, NULL, &tv);
    if (ret_value == -1)              /// error occurred in select()
    {
      TNRC_ERR("socket select problem: %i\n", ret_value);
      perror("select"); 
      return -1;
    }
    else if (ret_value == 0)	      /// timeout occurred
    {
      return 0;
    }
    else if (FD_ISSET(s_desc, &readfds))
    {
      buffer = new char[buff_size];
      mes_len = recv(s_desc, buffer, buff_size, 0);
      if(mes_len > 0)
      {
        mes->append(std::string(buffer, mes_len));
        delete []buffer;
        return 1;
      }
      return mes_len; 
    }
    else
      return -1;
  }

  int PluginAT8000_session::close_connection()
  {
    if(close(s_desc) == -1)
      return -1;
    return 0;
  }

  PluginAT8000_session::~PluginAT8000_session()
  {
    close_connection();
  }

  int PluginAT8000_session::fd()
  {
    return s_desc;
  }

  /** search for vlanid in label_map*/
  int PluginAT8000::find_vlanid(tnrc_portid_t portid)
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
  /**  Class PluginAT8000_listen
  *  Listen and process received AT8000 responses
  */
  void PluginAT8000_listen::process_message(std::string* mes)
  {
    if(plugin->command_type == TNRC_NO_COMMAND) process_no_command_response();
    else if (plugin->command_type == TNRC_CONFIGURE) process_configure_response();
    else if (plugin->command_type == TNRC_VLAN_DATABASE) process_vlan_database_response();
    else if (plugin->command_type == TNRC_ADD_VLAN) process_add_vlan_response();
    else if (plugin->command_type == TNRC_FIRST_EXIT) process_first_exit_response();
    else if (plugin->command_type == TNRC_INTERFACE) process_interface_response();
    else if (plugin->command_type == TNRC_ACCESS_VLAN) process_access_vlan_response();
    else if (plugin->command_type == TNRC_SECOND_EXIT) process_second_exit_response();
    else if (plugin->command_type == TNRC_THIRD_EXIT) process_third_exit_response();
    else if (plugin->command_type == TNRC_FOURTH_EXIT) process_fourth_exit_response();
    else if (plugin->command_type == TNRC_CHECK) process_check_response();
    else if (plugin->command_type == TNRC_DESTROY) process_destroy_response();
    else if (plugin->command_type == TNRC_DESTROY_CHECK) process_destroy_check_response();
    else if (plugin->command_type == TNRC_PRELOGIN) process_prelogin_response();
    else if (plugin->command_type == TNRC_PASS) process_password_response();
    else if (plugin->command_type == TNRC_LOGIN) process_login_response();
    else if (plugin->command_type == TNRC_GET_PORTS_RESOURCES) process_get_ports_response();
    else if (plugin->command_type == TNRC_GET_VLANS_RESOURCES) process_get_vlans_response();
  }

  void PluginAT8000_listen::process_no_command_response()
  {
      plugin->at8000_response = "";
      plugin->command_type = TNRC_NO_COMMAND;
  }

  void PluginAT8000_listen::process_prelogin_response()
  {
      char buff[32];
      sprintf(buff, "User Name:");
      std::string expected_str(buff);

      if(at8000_find(&plugin->at8000_response, expected_str))
        plugin->TELNET_status = TNRC_SP_STATE_NOT_LOGGED_IN;
      else
      {
        //TODO
      }
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;
  }

  void PluginAT8000_listen::process_password_response()
  {
      char buff[32];
      sprintf(buff, "AT-8000S#");
      std::string expected_str(buff);

      if(at8000_find(&plugin->at8000_response, expected_str))
      {
        plugin->TELNET_status = TNRC_SP_STATE_LOGGED_IN;
        TNRC_INF("TNRC_LISTEN - logged in\n");
        plugin->_activity = GET_XC;
      }
      else
      {
        plugin->TELNET_status = TNRC_SP_STATE_NOT_LOGGED_IN;
        TNRC_INF("TNRC_LISTEN - NOT logged in\n");
      }
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;
  }

  void PluginAT8000_listen::process_login_response()
  {
      char buff[32];
      sprintf(buff, "Password:");
      std::string expected_str(buff);

      if(at8000_find(&plugin->at8000_response, expected_str))
        plugin->TELNET_status = TNRC_SP_STATE_NOT_LOGGED_IN_PASS;
      else
      {
        //TODO
      }
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;
  }

  void PluginAT8000_listen::process_get_ports_response()
  {
    if(plugin->at8000_response.find("<space>") != -1)
    {
      plugin->at8000_response = delete_first_spaces(plugin->at8000_response);
      plugin->ports_resources_response.append(plugin->at8000_response);
      plugin->ports_resources_response.append("\n");
      plugin->send_space = true;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;
      plugin->_activity = GET_EQUIPMENT;
    }
    else
    {
      plugin->at8000_response = delete_first_spaces(plugin->at8000_response);
      plugin->ports_resources_response.append(plugin->at8000_response);
      save_equipments(plugin->ports_resources_response);
      plugin->ports_resources_response = "";
      plugin->at8000_response = "";
      plugin->command_type = TNRC_NO_COMMAND;
      if(!plugin->action_list.empty())
        plugin->_activity = CHECK_ACTION;
      else
        plugin->TELNET_status = TNRC_SP_STATE_END_SESSION;
    }
  }

  void PluginAT8000_listen::process_get_vlans_response()
  {
    if(plugin->at8000_response.find("<space>") != -1)
    {
      plugin->ports_resources_response.append(plugin->at8000_response);
      plugin->ports_resources_response.append("\n");
      plugin->send_space = true;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;
      plugin->_activity = GET_XC;
    }
    else
    {
      plugin->ports_resources_response.append(plugin->at8000_response);
      save_channels(plugin->ports_resources_response);
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;
      plugin->_activity = GET_EQUIPMENT;
    }
  }

  void PluginAT8000_listen::process_configure_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";

      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "(config)#");
      std::string expected_str(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response = "";
      plugin->command_type = TNRC_NO_COMMAND;

      if(at8000_find(&first_line, expected_str))
      {
        if(plugin->param->activity == TNRCSP_XC_MAKE)
          _trncsp_at8000_xc_process(TNRCSP_STATE_VLAN_DATABASE, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
        else
          _trncsp_at8000_xc_process(TNRCSP_STATE_INTERFACE, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      }
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::process_vlan_database_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";
      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "(config-vlan)#");
      std::string expected_str(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(at8000_find(&first_line, expected_str))
      {
        if(plugin->param->activity == TNRCSP_XC_MAKE)
          _trncsp_at8000_xc_process(TNRCSP_STATE_ADD_VLAN, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
        else if(plugin->param->activity == TNRCSP_XC_DESTROY)
          _trncsp_at8000_xc_process(TNRCSP_STATE_VLAN_DESTROY, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      }
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::process_add_vlan_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";
      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "(config-vlan)#");
      std::string expected_str(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(at8000_find(&first_line, expected_str))
        _trncsp_at8000_xc_process(TNRCSP_STATE_FIRST_EXIT, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::process_first_exit_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";
      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "(config)#");
      std::string expected_str(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(at8000_find(&first_line, expected_str))
        _trncsp_at8000_xc_process(TNRCSP_STATE_INTERFACE, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::process_interface_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";
      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "(config-if)#");
      std::string expected_str(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(at8000_find(&first_line, expected_str))
        _trncsp_at8000_xc_process(TNRCSP_STATE_ACCESS_VLAN, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::process_access_vlan_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";
      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "(config-if)#");
      std::string expected_str(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(at8000_find(&first_line, expected_str))
      {
        _trncsp_at8000_xc_process(TNRCSP_STATE_SECOND_EXIT, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      }
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::process_second_exit_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";
      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "(config)#");
      std::string expected_str(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(at8000_find(&first_line, expected_str))
      {
        if(plugin->which_port == 2)
          _trncsp_at8000_xc_process(TNRCSP_STATE_INTERFACE, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
        else if(plugin->param->activity == TNRCSP_XC_DESTROY && plugin->which_port == 1)
         _trncsp_at8000_xc_process(TNRCSP_STATE_VLAN_DATABASE, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
        else
          _trncsp_at8000_xc_process(TNRCSP_STATE_THIRD_EXIT, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      }
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::process_third_exit_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";
      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "#");
      std::string expected_str(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(at8000_find(&first_line, expected_str))
      {
        if(plugin->param->activity == TNRCSP_XC_MAKE)
          _trncsp_at8000_xc_process(TNRCSP_STATE_CHECK, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
        else
          _trncsp_at8000_xc_process(TNRCSP_STATE_FOURTH_EXIT, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      }
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::process_fourth_exit_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";
      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "#");
      std::string expected_str(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(at8000_find(&first_line, expected_str))
        _trncsp_at8000_xc_process(TNRCSP_STATE_CHECK, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::process_check_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";
      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      first_line = pop_line(&at8000_response_nl);
      first_line = pop_line(&at8000_response_nl);
      first_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "%i",plugin->param->vlanid);
      std::string expected_str_1(buff);
      sprintf(buff, "%s",plugin->param->ingress_fid->c_str());
      std::string expected_str_2(buff);
      sprintf(buff, "%s",plugin->param->egress_fid->c_str());
      std::string expected_str_3(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(at8000_find(&first_line, expected_str_1) && at8000_find(&first_line, expected_str_2) && at8000_find(&first_line, expected_str_3))
        _trncsp_at8000_xc_process(TNRCSP_STATE_FINAL, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::process_destroy_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";
      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      std::string second_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "(config-vlan)#");
      std::string expected_str(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(at8000_find(&first_line, expected_str))
        _trncsp_at8000_xc_process(TNRCSP_STATE_THIRD_EXIT, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::process_destroy_check_response()
  {
      std::string at8000_response_nl = plugin->at8000_response + "\n";
      std::string first_no_important_line = pop_line(&at8000_response_nl);
      std::string first_line = pop_line(&at8000_response_nl);
      first_line = pop_line(&at8000_response_nl);
      first_line = pop_line(&at8000_response_nl);
      first_line = pop_line(&at8000_response_nl);
      char buff[100];
      sprintf(buff, "%i",plugin->param->vlanid);
      std::string expected_str_1(buff);
      sprintf(buff, "%s",plugin->param->ingress_fid->c_str());
      std::string expected_str_2(buff);
      sprintf(buff, "%s",plugin->param->egress_fid->c_str());
      std::string expected_str_3(buff);
      at8000_response_nl = plugin->at8000_response;
      plugin->at8000_response="";
      plugin->command_type = TNRC_NO_COMMAND;

      if(!at8000_find(&first_line, expected_str_1) && !at8000_find(&first_line, expected_str_2) && !at8000_find(&first_line, expected_str_3))
        _trncsp_at8000_xc_process(TNRCSP_STATE_FINAL, plugin->param, TNRCSP_RESULT_NOERROR, &at8000_response_nl);
      else
        _trncsp_at8000_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &at8000_response_nl);
  }

  void PluginAT8000_listen::_trncsp_at8000_xc_process(int state, void* parameters, tnrcsp_result_t result, string* response_block)
  {
      plugin->tnrcsp_at8000_xc_process((tnrcsp_at8000_xc_state)state, (xc_at8000_params*)parameters, result, response_block);
  }

  void PluginAT8000_listen::save_equipments(std::string response_block)
  {
      std::string                 line = "";
      tnrc_portid_t               port_id;
      operational_state           oper_state;
      administrative_state        admin_state;
      tnrcsp_lsc_evmask_t         last_event;
      tnrc_sp_l2sp_speed          speed;
      tnrc_sp_l2sp_duplex_mode    duplex_mode;
      tnrcsp_l2sc_eqtype_t        equip_type;

      std::string                 tokens[9];
      map<tnrc_portid_t, tnrcsp_l2sc_resource_detail_t> tmp_resource_list;

      //ignofre first 3 lines with command
      line = pop_line(&response_block);
      line = pop_line(&response_block);
      line = pop_line(&response_block);

      //start process 
      while(response_block.length() > 0)
      {
        line = pop_line(&response_block);
        if(line.find("100M-Copper") == -1)
          continue;

        //Line format:
        //                                             Flow Link        Back     Mdix
        //Port     Type         Duplex Speed Neg       ctrl State       Pressure Mode
        //-------- ------------ ------ ----- --------- ---- ----------- -------- -------
        //e1       100M-Copper  Full   100   Enebled   Off  Up          Disabled On

        tokens[0] = line.substr(0,9);   //Port id
        tokens[1] = line.substr(9,13);  //Type
        tokens[2] = line.substr(22,8);  //Duplex
        tokens[3] = line.substr(30,6);  //Speed
        tokens[4] = line.substr(36,9);  //Neg
        tokens[5] = line.substr(45,5);  //Flow ctrl
        tokens[6] = line.substr(50,12); //Link State
        tokens[7] = line.substr(62,9);  //Back Pressure
        tokens[8] = line.substr(71);    //Mdix Mode

        //Generate port id;
        int first_space = tokens[0].find_first_of(' ');
        std::string portId = tokens[0].substr(0,first_space);
        char* port = (char *)portId.c_str();
        port_id = at8000_decode_portid(port);
        if(port_id == 0)
            continue;

        //Admin/Oper state setup
        if(tokens[6].find("Up") != std::string::npos)
        {
            oper_state = UP;
            admin_state = ENABLED;
        }
        else if(tokens[6].find("Down") != std::string::npos)
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
        if(tokens[2].find("Full") != std::string::npos)
            duplex_mode = TNRC_SP_L2SC_DUPLEX_FULL;
        else if(tokens[2].find("Half") != std::string::npos)
            duplex_mode = TNRC_SP_L2SC_DUPLEX_HALF;
        else
            duplex_mode = TNRC_SP_L2SC_DUPLEX_NONE;

        //Interface speed setup
        if(tokens[3].find("10") != std::string::npos)
            speed = TNRC_SP_L2SC_SPEED_10M;
        else if(tokens[3].find("100") != std::string::npos)
            speed = TNRC_SP_L2SC_SPEED_100M;
        else if(tokens[3].find("1000") != std::string::npos)
            speed = TNRC_SP_L2SC_SPEED_1G;
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

      TNRC_NOT("Resource_list contains:\n");
      std::map<tnrc_portid_t, tnrcsp_l2sc_resource_detail_t>::iterator it;
      for(it = resource_list.begin(); it != resource_list.end(); it++)
      {
        tnrcsp_l2sc_resource_detail_t d = it->second;
        TNRC_NOT("   id=0x%x, op=%i, ad=%i, ev=%i, sp=%i, du=%i, eq=%i\n", it->first, d.oper_state, d.admin_state, d.last_event, d.speed, d.duplex_mode, d.equip_type);
      }
  }

  void PluginAT8000_listen::save_channels(std::string response_block)
  {
    xc_list.clear();

    std::string line = "";
    std::string response = response_block;
    std::string tokens[5];

    tnrc_portid_t port_in, port_out;
    vlanid_t vlanid;
    tnrcsp_l2sc_xc_state_t xc_state;

    //skip first 5 lines
    for(int i=0; i<5; i++)
        line = pop_line(&response);

    std::string vlanid_str;
    std::string port_in_str;
    std::string port_out_str;
    tnrc_sp_xc_key node;
    plugin->vlan_list.clear();

    while(response.length() > 0 && response.find("Required") != -1)
    {
        line = pop_line(&response);
        if(line.find("Required") == -1)
          continue;

        //Line format:
        //
        //Vlan        Name                  Ports                Type     Authorization
        //---- ----------------- --------------------------- ------------ -------------
        // 1       phosphorus5              e(1,17)            permanent    Required

        tokens[0] = line.substr(0,5);   //Vlan id
        tokens[1] = line.substr(5,18);  //Vlan name
        tokens[2] = line.substr(23,28); //Ports
        tokens[3] = line.substr(51,13); //Type
        tokens[4] = line.substr(64);    //Authorization

        //get line with vlan id
        tokens[0] = delete_first_spaces_2(tokens[0]);
        int first_space = tokens[0].find_first_of(' ');
        vlanid_str = tokens[0].substr(0,first_space);
        vlanid = atoi(vlanid_str.c_str());

        if(vlanid < MAX_VLAN_ID)
        {
          if(tokens[2].find("e") != std::string::npos)
          {
            tokens[2] = delete_first_spaces_2(tokens[2]);
            int first_space = tokens[2].find_first_of(' ');
            int first_comma = tokens[2].find_first_of(',');
            if (first_comma == -1)
              first_comma = tokens[2].find_first_of('-');
            port_in_str = tokens[2].substr(1,first_comma);
            port_out_str = tokens[2].substr(first_comma,first_space-2);

            char* port = (char *)port_in_str.c_str();
            port_in = at8000_decode_portid(port);
            port = (char *)port_out_str.c_str();
            port_out = at8000_decode_portid(port);

            node.vlanid = vlanid;
            node.port_in = port_in;
            node.port_out = port_out;

            plugin->vlan_list.insert(plugin->vlan_list.end(), node);

            xc_state = TNRCSP_L2SC_XCSTATE_ACTIVE;

          }
          else
          {
            port_in = 0;
            port_out = 0;
            xc_state = TNRCSP_L2SC_XCSTATE_FAILED;
          }
          tnrcsp_l2sc_xc_detail_t details = {port_in, port_out, vlanid, xc_state};
          xc_list.insert(xc_list.end(), details);
        }
    }
    TNRC_NOT("xc_list contains:\n");
    list<tnrcsp_l2sc_xc_detail_t>::iterator it;
    for(it=xc_list.begin(); it!=xc_list.end(); it++ )
     {
         TNRC_NOT("   0x%x, 0x%x, %i, %i\n", it->portid_in, it->portid_out, it->vlanid, it->xc_state);
     }
  }

  tnrc_portid_t  PluginAT8000_listen::at8000_decode_portid(char* portid_str)
  {
    tnrc_portid_t portid = 0;
    int p,s = 0,i = 0;
    std::string port(portid_str);
    char space = 32;
    port = port.substr(1);
    while(port[i] != space)
      i++;
    port.substr(1,i);
    p = atoi(port.c_str());
    portid = p;
    //take board from AP
    portid = create_at8000_api_port_id(portid);

    return portid;
  }

  void PluginAT8000_listen::register_event(tnrc_portid_t portid, operational_state operstate, administrative_state adminstate,tnrc_sp_l2sp_speed speed, tnrc_sp_l2sp_duplex_mode duplex_mode, tnrcsp_l2sc_eqtype_t equip_type, tnrcsp_evmask_t e)
  {
    TNRC_INF("AT8000S notify via reqister callback: port=0x%x, operstate=%s, adminstate=%s", portid, SHOW_OPSTATE(operstate), SHOW_ADMSTATE(adminstate));

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

  tnrc_sp_xc_key PluginAT8000_listen::find_vlanid_from_portid(tnrc_portid_t portid)
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

  //==============================================================================
  /**  Class xc_at8000_params
  *  Crossconnection information
  */
  xc_at8000_params::xc_at8000_params(trncsp_xc_activity activity, tnrcsp_handle_t handle,
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
    this->ingress_fid   = at8000_encode_portid(port_in);
    this->egress_fid    = at8000_encode_portid(port_out);


    this->activated       = false;
    this->repeat_count    = 0;
    this->final_result    = false;
    this->result          = TNRCSP_RESULT_NOERROR;

  }

//###################################################################
//########### FOUNDRY SP API FUNCTIONS ##############################
//###################################################################

  tnrcsp_result_t PluginAT8000::tnrcsp_make_xc(tnrcsp_handle_t *handlep, 
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
      TNRC_INF("Make XC is starting");
      *handlep = gen_identifier();
      vlanid_t vlanid = first_free_vlanid(portid_in, portid_out);

      if (!at8000_check_resources(portid_in, portid_out, vlanid)) 
        return TNRCSP_RESULT_PARAMERROR;

      xc_at8000_params* xc_params = new xc_at8000_params(TNRCSP_XC_MAKE, *handlep, portid_in, portid_out, vlanid, direction, isvirtual, activate, response_cb, response_ctxt, async_cb, async_ctxt);

      action_list.insert(action_list.end(), xc_params);
      wait_disconnect = true;
      return TNRCSP_RESULT_NOERROR;
  }

  tnrcsp_result_t PluginAT8000::tnrcsp_destroy_xc(tnrcsp_handle_t *handlep,
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

    TNRC_INF("Destroy XC is starting");

    if (!at8000_check_resources(portid_in, portid_out, vlanid))
        return TNRCSP_RESULT_PARAMERROR;

    xc_at8000_params* xc_params = new xc_at8000_params(TNRCSP_XC_DESTROY, *handlep, portid_in, portid_out, vlanid, direction, isvirtual, deactivate, response_cb, response_ctxt, NULL, NULL);

    action_list.insert(action_list.end(), xc_params);
    wait_disconnect = true;
    return TNRCSP_RESULT_NOERROR;
  }

  tnrcsp_result_t PluginAT8000::tnrcsp_reserve_xc (tnrcsp_handle_t *handlep, 
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

    TNRC_INF("Reserve XC is starting");

    if (!at8000_check_resources(portid_in, portid_out, vlanid))
        return TNRCSP_RESULT_PARAMERROR;
    if(TELNET_status != TNRC_SP_STATE_LOGGED_IN)
        return TNRCSP_RESULT_EQPTLINKDOWN;

    //tnrc_boolean_t isvirtual = 0;

    xc_at8000_params* xc_params = new xc_at8000_params (TNRCSP_XC_RESERVE, *handlep, portid_in, portid_out, vlanid, direction, isvirtual, false, response_cb, response_ctxt, NULL, NULL);
    action_list.insert(action_list.end(), xc_params);
    wait_disconnect = true;
    return TNRCSP_RESULT_NOERROR;
  }

  tnrcsp_result_t PluginAT8000::tnrcsp_unreserve_xc (tnrcsp_handle_t *handlep, 
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

    TNRC_DBG("Unreserve XC is starting");

    if (!at8000_check_resources(portid_in, portid_out, vlanid))
        return TNRCSP_RESULT_PARAMERROR;
    if(TELNET_status != TNRC_SP_STATE_LOGGED_IN)
        return TNRCSP_RESULT_EQPTLINKDOWN;

    tnrc_boolean_t _virtual = 0;

    xc_at8000_params* xc_params = new xc_at8000_params(TNRCSP_XC_UNRESERVE, *handlep, portid_in, portid_out, vlanid, direction, _virtual, false, response_cb, response_ctxt, NULL, NULL);
    action_list.insert(action_list.end(), xc_params);
    wait_disconnect = true;

    return TNRCSP_RESULT_NOERROR;
  }

  tnrcsp_result_t PluginAT8000::tnrcsp_register_async_cb (tnrcsp_event_t *events)
  {
    TNRC_INF("Register callback: portid=0x%x, operstate=%d, adminstate=%d, event=%d", events->portid, events->oper_state, events->admin_state, events->events);

    TNRC_OPSPEC_API::update_dm_after_notify (events->portid, events->labelid, events->events);

    TNRC_INF("Data model updated\n");
    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  wq_item_status PluginAT8000::wq_function (void *d)
  {
    TNRC_ERR("!!!!!!!!!!!! PluginXmr::wq_function function should be never executed");
    return WQ_SUCCESS;	
  }

  //-------------------------------------------------------------------------------------------------------------------
  void PluginAT8000::del_item_data(void *d)
  {
    TNRC_ERR("!!!!!!!!!!!! PluginXmr::del_item_data function should be never executed");

    tnrcsp_action_t		*data;

    //get work item data
    data = (tnrcsp_action_t *) d;
    //delete data structure
    delete data;
  }

}
