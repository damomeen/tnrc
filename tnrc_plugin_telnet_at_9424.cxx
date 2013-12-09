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
#include "tnrc_plugin_telnet_at_9424.h"

namespace TNRC_SP_AT9424
{
  //================================================================================
  /**
  *  Utils
  */

  /**
  * Thread read function
  */
  static int at9424_read_func (struct thread *thread)
  {
    int fd;
    PluginAT9424 *p;

    fd = THREAD_FD (thread);
    p = static_cast<PluginAT9424 *>(THREAD_ARG (thread));

    int result = p->doRead();
    return result;
  }

  /**
  * Thread retrive function
  */
  int at9424_retrive_func (struct thread *thread)
  {
    PluginAT9424 *p;

    p = static_cast<PluginAT9424 *>(THREAD_ARG (thread));

    int result = p->doRetrive();
    p->retriveT(thread_add_timer(master, at9424_retrive_func, p, 1));
    return result;
  }

  /** search description in the message, return True if description found */
  bool at9424_find(std::string* message, std::string description)
  {
    if(!message)
        return false;
    return message->find(description) != -1;
  }

  /** compare two tables, return True if there are the same */
  bool at9424_compare(char* table_1, char* table_2, int symbols)
  {
    bool comp = true;
    for(int i = 0; i < symbols; i++)
      if(table_1[i] != table_2[i])
      {
        comp = false;
        break;
      }
    return comp;
  }

  /** delete first spaces from massage */
  std::string delete_first_spaces(std::string message)
  {
    int k = 0;
    char i = 105;
    while(message[k] != i)
      k++;
    k = k - 2;
    std::string temp = message.substr(k);
    return temp;
  }

  /** take first line from message */
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

  std::string delete_first_spaces_2(std::string message)
  {
    message.append("\n");
    std::string line = pop_line(&message);
    std::string temp,temp_2;
    int k = 0;
    char V = 86;
    char P = 80;
    char U = 85;
    char C = 67;
    char A = 65;
    char T = 84;
    char dash = 45;
    char cr = 13;
    while(line.find("#") == -1)
    {
      if(line.size() > 1)
      {
        while(line[k] != V && line[k] != P && line[k] != U && line[k] != C && line[k] != A && line[k] != T && line[k] != dash && k < line.size())
          k++;
      }
      if(line[k] != dash)
      {
        temp.append(line.substr(k));
        temp.append("\n");
      }
      k = 0;
      line = pop_line(&message);
    }
    temp.append("#\n");

    line = pop_line(&temp);
    while(line.find("#") == -1)
    {
      if(line[0] == V || line[0] == P || line[0] == U || line[0] == C || line[0] == A || line[0] == T)
      {
        temp_2.append(line.c_str());
        temp_2.append("\n");
      }
      line = pop_line(&temp);
    }
    return temp_2;
  }

  /** clear message from useless lines */
  std::string delete_useless_lines(std::string message)
  {
    std::string line = pop_line(&message);
    std::string mes;
    int counter = 0;
    while(line.find("#") == -1)
    {
      counter++;
      if(line.find("if") != -1)
      {
        mes.append(line.c_str());
        mes.append("\n");
        counter = 0;
      }
      if(counter == 4)
        break;
      line = pop_line(&message);
    }
    return mes;
  }

  /** delete everything to first space */
  std::string delete_to_first_space(std::string message)
  {
    char space = 32;
    int k = 0;
    while(message[k] != space)
      k++;
    return message.substr(k+1);
  }

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
  tnrc_portid_t create_at9424_api_port_id(tnrc_portid_t portid)
  {
    board_id_t board_id = find_board_id(portid);
    return TNRC_CONF_API::get_tnrc_port_id(1/*eqpt_id*/, board_id, portid);
  }

  /** search for first free vlanid*/
  int PluginAT9424::first_free_vlanid(tnrc_portid_t portid_in, tnrc_portid_t portid_out)
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

  bool at9424_check_vlanid(vlanid_t vlanid)
  {
    if( vlanid>1 && vlanid<=4090 )//TODO check max vlanid
        return true;
    else
        return false;
  }

  /** generates port id in AT9424 notation; returns NULL in case of any error */
  std::string* at9424_encode_portid(tnrc_portid_t portid)
  {
    int slot = portid;
    slot = (slot >> 8) & 0x000000FF;
    int port = portid;
    port = port & 0x000000FF;

    if((1 > port) || (port > 24))//TODO check max port id
    {
        TNRC_WRN("Port_id cannot be decomposed - out of range 01 - 24!\n");
        return NULL;
    }
    char result_portid[16];
    sprintf(result_portid, "%d", port);
    std::string* port_id = new std::string(result_portid);

    return port_id;
  }

  /** check if given resources are correct as an vlan creation parametrers*/
  bool at9424_check_resources(tnrc_portid_t port_in, tnrc_portid_t port_out, vlanid_t vlanid)
  {
    bool success;
    std::string* ingress_id = at9424_encode_portid(port_in);
    std::string* egress_id = at9424_encode_portid(port_out);

    if(!ingress_id || !egress_id || !at9424_check_vlanid(vlanid))
    {
        success = false;
        TNRC_INF("At9424_check_resources no success, ingress_id:%s, egress_id:%s, vlanid:%d",ingress_id->c_str(),egress_id->c_str(), vlanid);
    }
    else
    {
        success = true;
        TNRC_INF("At9424_check_resources success, ingress_id:%s, egress_id:%s, vlanid:%d",ingress_id->c_str(),egress_id->c_str(), vlanid);
    }
    delete ingress_id, egress_id;
    return success;
  }

  //================================================================================
  /**  Class PluginAT9424
  *  Abstract Specyfic Part class for devices with CLI-Telnet interface
  */
  PluginAT9424::PluginAT9424(): t_read(NULL), t_retrive(NULL)
  {
    handle_ = 1;
    xc_id_ = 1;
    wqueue_ = work_queue_new (TNRC_MASTER.getMaster(), AT9424_WQ);
    wqueue_->spec.workfunc = workfunction;
    wqueue_->spec.del_item_data = delete_item_data;
    wqueue_->spec.max_retries = 5;
    //wqueue_->spec.hold = 500;
    sock = NULL;
    TELNET_status = TNRC_SP_STATE_NO_SESSION;
    _activity  = GET_XC;
    listen_proc = new PluginAT9424_listen(this);
    xc_bidir_support_ = true;
    wait_disconnect = false;
    identifier = 0;
    _connect = 1;
    no_key_word = false;
    send_space = false;
    read_thread = false;
    key_word = "$abc&-_";
    second_key_word = "$abc&-_";
    check_status = 0;
  }

  PluginAT9424::PluginAT9424(std::string name): t_read(NULL), t_retrive(NULL)
  {
    name_ = name;
    handle_ = 1;
    xc_id_ = 1;
    wqueue_ = work_queue_new (TNRC_MASTER.getMaster(), AT9424_WQ);
    wqueue_->spec.workfunc = workfunction;
    wqueue_->spec.del_item_data = delete_item_data;
    wqueue_->spec.max_retries = 5;
    //wqueue_->spec.hold = 500;
    sock = NULL;
    TELNET_status = TNRC_SP_STATE_NO_SESSION;
    _activity  = GET_XC;
    listen_proc = new PluginAT9424_listen(this);
    xc_bidir_support_ = true;
    wait_disconnect = false;
    identifier = 0;
    _connect = 1;
    no_key_word = false;
    send_space = false;
    read_thread = false;
    key_word = "$abc&-_";
    second_key_word = "$abc&-_";
    check_status = 0;
  }

  PluginAT9424::~PluginAT9424()
  {
    close_connection();
  }

  void PluginAT9424::retriveT(thread *retrive)
  {
    t_retrive = retrive;
  }

  void PluginAT9424::readT(thread *read)
  {
    t_read = read;
  }

  /** PluginAT9424::getFd()
  * @return FileDescriptor
  */
  int PluginAT9424::getFd()
  {
    return sock->fd();
  }

  /** PluginAT9424::at9424_connect()
  * @return 0 if telnet connection is established
  */
  int PluginAT9424::at9424_connect()
  {
    if(_connect == 1)
    {
      int result = sock->_connect();
      TNRC_INF("Trying to connect");
      if (result == 0)
      {
        read_thread = true;
        no_key_word = true;
        TELNET_status = TNRC_SP_STATE_DONE;
        command_type = TNRC_FIRST_TELNET_OPTION;
        TNRC_INF("Connected OK");
      }
      else
      {
        TNRC_ERR("Connection Error");
      }
      return result;
    }
    else if (_connect == 2)
    {
      std::string str;
      char tab[3];

      //Will Suppress Go Ahead
      tab[0] = 0xff;
      tab[1] = 0xfb;
      tab[2] = 0x03;
      str.append(tab);
      no_key_word = true;
      TELNET_status = TNRC_SP_STATE_DONE;
      asynch_send(&str, NULL, TNRC_SECOND_TELNET_OPTION);
    }
    else if (_connect == 3)
    {
      _connect = 1;
      std::string str;
      char tab[3];

      //Won't Negotiate About Window Size
      tab[0] = 0xff;
      tab[1] = 0xfc;
      tab[2] = 0x1f;
      str.append(tab);
      TELNET_status = TNRC_SP_STATE_DONE;
      key_word = "Login:";
      asynch_send(&str, NULL, TNRC_THIRD_TELNET_OPTION);
    }
  }

  /** send login commnad 
   * @return 0 if login command send successful
   */
  int PluginAT9424::at9424_log_in()
  {
    char buff[100];
    sprintf(buff, login.c_str());
    string str(buff);
    str.append("\r\n");
    key_word = "Password:";
    asynch_send(&str, NULL, TNRC_LOGIN);
    return 0;
  }

  /** send password 
   * @return 0 if password send successful
   */
  int PluginAT9424::at9424_password()
  {
    char buff[100];
    sprintf(buff, password.c_str());
    string str(buff);
    str.append("\r\n");
    key_word = "#";
    asynch_send(&str, NULL, TNRC_PASS);
    return 0;
  }

  /** send ports retrieve commnad */
  void PluginAT9424::get_ports_resources()
  {
    /* send request for all available ports resources*/
    char buff[100];
    std::string str;
    if(send_space)
    {
      send_space = false;
      sprintf(buff, "c");
      str = buff;
      key_word = "quit";
      second_key_word = "#";
    }
    else
    {
      sprintf(buff, "show interface");
      str = buff;
      str.append("\r\n");
      key_word = "quit";
      second_key_word = "#";
    }
    asynch_send(&str, NULL, TNRC_GET_PORTS_RESOURCES);
  }

  /** send channels retrieve commnad */
  void PluginAT9424::get_all_xc()
  {
    /* send request for all existing vlan instances*/
    char buff[100];
    std::string str;
    if(send_space)
    {
      send_space = false;
      sprintf(buff, "c");
      str = buff;
      key_word = "quit";
      second_key_word = "#";
    }
    else
    {
      sprintf(buff, "show vlan");
      str = buff;
      str.append("\r\n");
      key_word = "quit";
      second_key_word = "#";
    }
    asynch_send(&str, NULL, TNRC_GET_VLANS_RESOURCES);
  }

  int PluginAT9424::at9424_disconnect()
  {
    int result;
    result = close_connection();
    return result;
  }

  /** PluginAT9424::asynch_send
  * @param command
  * @param state tnrc sp state
  * @param parameters
  * @param cmd_type
  * @return
  */
  tnrcsp_result_t PluginAT9424::asynch_send(std::string* command, void* parameters, tnrc_l2sc_cmd_type_t cmd_type)
  {
    /* send SSH2 CLI command */
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
    at9424_response.assign("");
    command_type = cmd_type;
    sent_command = *command;
    param = (xc_at9424_params*)parameters;
    return TNRCSP_RESULT_NOERROR;
  }

  /** Read thread's rountine (thread *t_read) */
  int PluginAT9424::doRead()
  {
    std::string recMsg("");
    at9424_response = "";
    int l;
    l = sock->_recv(&recMsg, 4096);
    temp_at9424_response.append(recMsg);

    if(temp_at9424_response.find(key_word.c_str()) != -1 || no_key_word == true || temp_at9424_response.find(second_key_word.c_str()) != -1)
    {

      at9424_response = temp_at9424_response;
      temp_at9424_response = "";
      read_thread = false;
      no_key_word = false;
      key_word = "$abc&-_";
      second_key_word = "$abc&-_";
      TNRC_INF("Processing received message \n%s", at9424_response.c_str());
      listen_proc->process_message(&at9424_response);
      return -1;
    }
    return 0;
  }

 /** Backgrount thread's rountine (thread *t_retrive)
  * - connection with device
  * - equipment
  * - connections
  */
  int PluginAT9424::doRetrive()
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
          result = at9424_connect();
        break;
      case TNRC_SP_STATE_NOT_LOGGED_IN:
          TNRC_INF("Trying to logg in");
          result = at9424_log_in();
        break;
      case TNRC_SP_STATE_NOT_LOGGED_IN_PASS:
          TNRC_INF("Sending password");
          result = at9424_password();
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
                xc_at9424_params* xc_params = action_list.front();
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
                  tnrcsp_at9424_xc_process(TNRCSP_STATE_INIT, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                  _activity = DONE;
                }
                else
                  _activity = GET_XC;
              }
              else if(action_list.front()->activity == TNRCSP_XC_DESTROY)
              {
                xc_at9424_params* xc_params = action_list.front();
                action_list.pop_front();
                tnrcsp_at9424_xc_process(TNRCSP_STATE_DELETE_VLAN, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                _activity = DONE;
              }
              else if(action_list.front()->activity == TNRCSP_XC_RESERVE)
              {
                xc_at9424_params* xc_params = action_list.front();
                action_list.pop_front();
                tnrcsp_at9424_xc_process(TNRCSP_STATE_INIT, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                _activity = DONE;
              }
              else if(action_list.front()->activity == TNRCSP_XC_UNRESERVE)
              {
                xc_at9424_params* xc_params = action_list.front();
                action_list.pop_front();
                tnrcsp_at9424_xc_process(TNRCSP_STATE_INIT, xc_params, TNRCSP_RESULT_NOERROR, NULL);
                _activity = DONE;
              }
            }
            break;
        }
        break;
      case TNRC_SP_STATE_END_SESSION:
        TNRC_INF("Ending session");
        result = at9424_disconnect();
        TELNET_status = TNRC_SP_STATE_DISCONNECTED;
        break;
    }
    if (read_thread)
      readT(thread_add_event(master, at9424_read_func, this, 0));
    if (wait_disconnect && TELNET_status == TNRC_SP_STATE_DISCONNECTED)
    {
      wait_disconnect = false;
      TELNET_status = TNRC_SP_STATE_NO_SESSION;
    }
    return result;
  }

  int PluginAT9424::close_connection()
  {
    if(sock->close_connection() == -1)
        return -1;
    return 0;
  }

  /** Launch communication with device
   * @param location <ipAddress>:<port>:<path_to_configuration_file>
   */
  tnrcapiErrorCode_t
  PluginAT9424::probe (std::string location)
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

    TNRC_INF("Plugin Allied Telesis 9424 address %s", strAddress.c_str());
    if (inet_aton(strAddress.c_str(), &remote_address) != 1)
    {
      TNRC_ERR("Plugin Allied Telesis 9424::probe: wrong IP address %s given string is: %s", inet_ntoa(remote_address), strAddress.c_str());
      return TNRC_API_ERROR_GENERIC;
    }

    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 9424::probe: wrong location description: no port");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    strPort = location.substr(stPos, endPos-stPos);

    TNRC_INF("Plugin Port %s", strPort.c_str());
    if (sscanf(strPort.c_str(), "%d", &remote_port) != 1)
    {
      TNRC_ERR("Plugin Allied Telesis 9424::probe: wrong port (location = %s, readed port %d)", location.c_str(), remote_port);
      return TNRC_API_ERROR_GENERIC;
    }

    sock = new PluginAT9424_session(remote_address, remote_port, this);
    TNRC_INF("Plugin Allied Telesis 9424::probe: sesion data: ip %s, port %d", inet_ntoa(remote_address), remote_port);
    TELNET_status = TNRC_SP_STATE_NO_SESSION;

    //login
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 9424::probe: wrong location description: no login");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    login = location.substr(stPos, endPos-stPos);

    //password
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 9424::probe: wrong location description: no password");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    password = location.substr(stPos, endPos-stPos);

    //vlan number
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 9424::probe: wrong location description: no vlan number");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    vlan_number = location.substr(stPos, endPos-stPos);

    //vlan pool
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 9424::probe: wrong location description: no vlan pool");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    vlan_pool = location.substr(stPos, endPos-stPos);

    //backup vlan number
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 9424::probe: wrong location description: no backup vlan number");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    backup_vlan = location.substr(stPos, endPos-stPos);

    retriveT (thread_add_timer (master, at9424_retrive_func, this, 0));

    //getting eqpt config filename from config file
    if ((stPos = endPos+1) > location.length())
    {
      TNRC_ERR("Plugin Allied Telesis 9424::probe: wrong location description: no path");
      return TNRC_API_ERROR_GENERIC;
    }
    endPos = location.find_first_of(separators, stPos);
    strPath = location.substr(stPos, endPos-stPos);

    //read eqpuipment config file
    TNRC_INF("Plugin Allied Telesis 9424 configuration file %s", strPath.c_str());
    vty_read_config ((char *)strPath.c_str(), NULL);
    TNRC_INF("Allied Telesis 9424 extra information about ports readed");

    return TNRC_API_ERROR_NONE;
  }

  void PluginAT9424::tnrcsp_at9424_xc_process(tnrcsp_at9424_xc_state state, xc_at9424_params* parameters, tnrcsp_result_t response, string* data)
  {
    tnrcsp_result_t result;
    bool call_callback = false;
    if(parameters->activity == TNRCSP_XC_MAKE)
    {
        if(call_callback = tnrcsp_at9424_make_xc(state, parameters, response, data, &result))
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
                tnrcsp_at9424_destroy_xc(TNRCSP_STATE_DELETE_VLAN, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
            }
    }
    else if(parameters->activity == TNRCSP_XC_DESTROY)
    {
      if(call_callback = tnrcsp_at9424_destroy_xc(state, parameters, response, data, &result))
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
      if(call_callback = tnrcsp_at9424_reserve_xc(state, parameters, response, data, &result))
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
          call_callback = tnrcsp_at9424_unreserve_xc(TNRCSP_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
        }
      }
    }
    else if(parameters->activity == TNRCSP_XC_UNRESERVE)
    {
      if(call_callback = tnrcsp_at9424_unreserve_xc(state, parameters, response, data, &result))
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

  /** PluginAT9424::trncsp_at9424_make_xc
  *   make crossconnection
  */
  bool PluginAT9424::tnrcsp_at9424_make_xc(tnrcsp_at9424_xc_state state, xc_at9424_params* parameters, tnrcsp_result_t response, string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("make state: %i\n", state);
    if(state == TNRCSP_STATE_INIT)
    {
      *result = at9424_create_vlan(parameters);
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

 /** PluginAT9424::trncsp_at9424_destroy_xc
  *   destroy crossconnection
  */
  bool PluginAT9424::tnrcsp_at9424_destroy_xc(tnrcsp_at9424_xc_state state, xc_at9424_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {
    TNRC_INF("destroy state: %i\n", state);
    if(state == TNRCSP_STATE_DELETE_VLAN)
    {
      *result = at9424_add_ports(parameters, state);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_STATE_ADD_INGRESS_PORT)
    {
      *result = at9424_add_ports(parameters, state);
      if(*result != TNRCSP_RESULT_NOERROR)
        return true;
    }
    else if(state == TNRCSP_STATE_ADD_EGRESS_PORT)
    {
      *result = at9424_destroy_vlan(parameters);
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

  /** PluginAT9424::trncsp_at9424_reserve_xc
  *   reserve crossconnection
  */
  bool PluginAT9424::tnrcsp_at9424_reserve_xc(tnrcsp_at9424_xc_state state, xc_at9424_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {

  }

  /** PluginAT9424::trncsp_at9424_unreserve_xc
  *   unreserve crossconnection
  */
  bool PluginAT9424::tnrcsp_at9424_unreserve_xc(tnrcsp_at9424_xc_state state, xc_at9424_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result)
  {

  }

  /** send create vlan command */
  tnrcsp_result_t PluginAT9424::at9424_create_vlan(xc_at9424_params* parameters)
  {
    char buff[100];
    sprintf(buff, "create vlan=phosphorus%i vid=%i untaggedports=%s,%s", parameters->vlanid, parameters->vlanid, parameters->ingress_fid->c_str(), parameters->egress_fid->c_str());
    std::string command(buff);
    command.append("\r\n");
    key_word = "#";

    return asynch_send(&command, parameters, TNRC_CREATE_VLAN);
  }

  /** send destroy vlan command */
  tnrcsp_result_t PluginAT9424::at9424_destroy_vlan(xc_at9424_params* parameters)
  {
    char buff[100];
    sprintf(buff, "destroy vlan %i ", parameters->vlanid);
    std::string command(buff);
    command.append("\r\n");
    key_word = "#";

    return asynch_send(&command, parameters, TNRC_DESTROY_VLAN);
  }

  /** send add ports command */
  tnrcsp_result_t PluginAT9424::at9424_add_ports(xc_at9424_params* parameters, tnrcsp_at9424_xc_state state)
  {
    char buff[100];
    int v_i,v_e,p_i,p_e;
    v_i = atoi(backup_vlan.c_str());
    p_i = atoi(parameters->ingress_fid->c_str());
    v_i += p_i;
    v_e = atoi(backup_vlan.c_str());
    p_e = atoi(parameters->egress_fid->c_str());
    v_e += p_e;
    tnrc_l2sc_cmd_type_t cmd_type;

    if(state == TNRCSP_STATE_DELETE_VLAN)
    {
      sprintf(buff, "add vlan v%d ports=%d", v_i, p_i);
      cmd_type = TNRC_ADD_INGRESS_PORT;
    }
    else
    {
      sprintf(buff, "add vlan v%d ports=%d", v_e, p_e);
      cmd_type = TNRC_ADD_EGRESS_PORT;
    }
    std::string command(buff);
    command.append("\r\n");
    key_word = "#";

    return asynch_send(&command, parameters, cmd_type);
  }

  /** search for vlanid in label_map*/
  int PluginAT9424::find_vlanid(tnrc_portid_t portid)
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
  /** Class Xmr_session
  * Provides communication with SSH2 protocol using network connection
  */

  PluginAT9424_session::PluginAT9424_session(in_addr address, int port,PluginAT9424 *owner)
  {
    plugin = owner;
    remote_address = address;
    remote_port = port;
  }

  /** PluginAT9424_session::_connect()
  * @return error_code: 0: No error, -1:sock error 
  */
  int PluginAT9424_session::_connect()
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

  /** PluginAT9424_session::_send(std::string* mes)
  * @param mes: pointer to the string that is send to the equipment
  * @return error_code: 0: No error0, -1:sock error 
  */
  int PluginAT9424_session::_send(std::string* mes)
  {
    if(send(s_desc, mes->c_str(), mes->length(), 0) != mes->length())
      return -1;
    return 0;
  }

  /** PluginAT9424_session::_recv(std::string* mes, int buff_size)
  * @param mes: pointer to the string that the received message is written
  * @param buff_size: maximum message length
  * @return error_code: 1: No error0:timeout, -1:sock error 
  */
  int PluginAT9424_session::_recv(std::string* mes, int buff_size)
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

  int PluginAT9424_session::close_connection()
  {
    if(close(s_desc) == -1)
      return -1;
    return 0;
  }

  PluginAT9424_session::~PluginAT9424_session()
  {
    close_connection();
  }

  int PluginAT9424_session::fd()
  {
    return s_desc;
  }

 //==============================================================================
  /**  Class PluginAT9424_listen
  *  Listen and process received AT9424 responses
  */

  PluginAT9424_listen::PluginAT9424_listen(PluginAT9424 *owner)
  {
       plugin = owner;
  }

  void PluginAT9424_listen::process_message(std::string* mes)
  {
    if(plugin->command_type == TNRC_NO_COMMAND) process_no_command_response();
    else if (plugin->command_type == TNRC_FIRST_TELNET_OPTION) process_telnet_options_response();
    else if (plugin->command_type == TNRC_SECOND_TELNET_OPTION) process_telnet_options_response();
    else if (plugin->command_type == TNRC_THIRD_TELNET_OPTION) process_telnet_options_response();
    else if (plugin->command_type == TNRC_LOGIN) process_login_response();
    else if (plugin->command_type == TNRC_PASS) process_password_response();
    else if (plugin->command_type == TNRC_GET_PORTS_RESOURCES) process_get_ports_response();
    else if (plugin->command_type == TNRC_GET_VLANS_RESOURCES) process_get_vlans_response();
    else if (plugin->command_type == TNRC_CREATE_VLAN) process_create_vlan_response();
    else if (plugin->command_type == TNRC_DESTROY_VLAN) process_destroy_vlan_response();
    else if (plugin->command_type == TNRC_ADD_INGRESS_PORT) process_add_ports_response();
    else if (plugin->command_type == TNRC_ADD_EGRESS_PORT) process_add_ports_response();
  }

  void PluginAT9424_listen::process_no_command_response()
  {
      plugin->at9424_response = "";
      plugin->command_type = TNRC_NO_COMMAND;
  }

  void PluginAT9424_listen::process_telnet_options_response()
  {
    if(plugin->command_type == TNRC_FIRST_TELNET_OPTION)
    {
      plugin->_connect++;
      char tab[12];

      // Will Echo
      tab[0] = 0xff;
      tab[1] = 0xfb;
      tab[2] = 0x01;

      // Do Suppress Go Ahead
      tab[3] = 0xff;
      tab[4] = 0xfd;
      tab[5] = 0x03;

      // Will Suppress Go Ahead
      tab[6] = 0xff;
      tab[7] = 0xfb;
      tab[8] = 0x03;

      // Do Terminal Type
      tab[9] = 0xff;
      tab[10] = 0xfb;
      tab[11] = 0x01;

      std::string expected_str(tab);
      char buff[100];
      for(int i =0 ; i < 100; i++)
        buff[i] = NULL;
      sprintf(buff, plugin->at9424_response.c_str());
      char buff_2[100];
      for(int i =0 ; i < 100; i++)
        buff_2[i] = NULL;
      sprintf(buff_2, expected_str.c_str());

      if(at9424_compare(buff, buff_2, 9))
        plugin->TELNET_status = TNRC_SP_STATE_NO_SESSION;
      else
      {
        //TODO
      }
      plugin->at9424_response="";
      plugin->command_type = TNRC_NO_COMMAND;
    }
    else if(plugin->command_type == TNRC_SECOND_TELNET_OPTION)
    {
      plugin->_connect++;
      char tab[3];

      // Do Suppress Go Ahead
      tab[0] = 0xff;
      tab[1] = 0xfd;
      tab[2] = 0x03;

      std::string expected_str(tab);
      char buff[100];
      for(int i =0 ; i < 100; i++)
        buff[i] = NULL;
      sprintf(buff, plugin->at9424_response.c_str());
      char buff_2[100];
      for(int i =0 ; i < 100; i++)
        buff_2[i] = NULL;
      sprintf(buff_2, expected_str.c_str());

      if(at9424_compare(buff, buff_2, 3))
        plugin->TELNET_status = TNRC_SP_STATE_NO_SESSION;
      else
      {
        //TODO
      }
      plugin->at9424_response="";
      plugin->command_type = TNRC_NO_COMMAND;
    }
    else if(plugin->command_type == TNRC_THIRD_TELNET_OPTION)
    {
      char buff[32];
      sprintf(buff, "Login:");
      std::string expected_str(buff);

      if(at9424_find(&plugin->at9424_response, expected_str))
        plugin->TELNET_status = TNRC_SP_STATE_NOT_LOGGED_IN;
      else
      {
        //TODO
      }
      plugin->at9424_response="";
      plugin->command_type = TNRC_NO_COMMAND;
    }
  }

  void PluginAT9424_listen::process_login_response()
  {
    char buff[32];
    sprintf(buff, "Password:");
    std::string expected_str(buff);

    if(at9424_find(&plugin->at9424_response, expected_str))
      plugin->TELNET_status = TNRC_SP_STATE_NOT_LOGGED_IN_PASS;
    else
    {
      //TODO
    }
    plugin->at9424_response="";
    plugin->command_type = TNRC_NO_COMMAND;
  }

  void PluginAT9424_listen::process_password_response()
  {
      char buff[32];
      sprintf(buff, "#");
      std::string expected_str(buff);

      if(at9424_find(&plugin->at9424_response, expected_str))
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
      plugin->at9424_response="";
      plugin->command_type = TNRC_NO_COMMAND;
  }

  void PluginAT9424_listen::process_get_ports_response()
  {
    if(plugin->at9424_response.find("<Space>") != -1)
    {
      plugin->at9424_response = delete_first_spaces(plugin->at9424_response);
      plugin->ports_resources_response.append(plugin->at9424_response);
      plugin->ports_resources_response.append("\n");
      plugin->send_space = true;
      plugin->at9424_response="";
      plugin->command_type = TNRC_NO_COMMAND;
      plugin->_activity = GET_EQUIPMENT;
    }
    else
    {
      plugin->at9424_response = delete_first_spaces(plugin->at9424_response);
      plugin->ports_resources_response.append(plugin->at9424_response);
      plugin->ports_resources_response = delete_useless_lines(plugin->ports_resources_response);
      save_equipments(plugin->ports_resources_response);
      plugin->ports_resources_response = "";
      plugin->at9424_response = "";
      plugin->command_type = TNRC_NO_COMMAND;
      if(!plugin->action_list.empty())
        plugin->_activity = CHECK_ACTION;
      else
        plugin->TELNET_status = TNRC_SP_STATE_END_SESSION;
    }
  }

  void PluginAT9424_listen::process_get_vlans_response()
  {
    if(plugin->at9424_response.find("<Space>") != -1)
    {
      plugin->ports_resources_response.append(plugin->at9424_response);
      plugin->ports_resources_response.append("\n");
      plugin->send_space = true;
      plugin->at9424_response="";
      plugin->command_type = TNRC_NO_COMMAND;
      plugin->_activity = GET_XC;
    }
    else
    {
      plugin->ports_resources_response.append(plugin->at9424_response);
      plugin->ports_resources_response = delete_first_spaces_2(plugin->ports_resources_response);
      save_channels(plugin->ports_resources_response);
      plugin->ports_resources_response = "";
      plugin->at9424_response = "";
      plugin->command_type = TNRC_NO_COMMAND;
      plugin->_activity = GET_EQUIPMENT;
    }
  }

  void PluginAT9424_listen::process_create_vlan_response()
  {
    char buff[100];
    sprintf(buff, "Create VLAN %i, please wait ...\n\r#",plugin->param->vlanid);
    std::string expected_str(buff);
    plugin->command_type = TNRC_NO_COMMAND;

    if(at9424_find(&plugin->at9424_response, expected_str))
      _trncsp_at9424_xc_process(TNRCSP_STATE_FINAL, plugin->param, TNRCSP_RESULT_NOERROR, &plugin->at9424_response);
    else
      _trncsp_at9424_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &plugin->at9424_response);
  }

  void PluginAT9424_listen::process_destroy_vlan_response()
  {
    char buff[100];
    sprintf(buff, "Delete VLAN %i, please wait ...\n\r#",plugin->param->vlanid);
    std::string expected_str(buff);
    plugin->command_type = TNRC_NO_COMMAND;

    if(at9424_find(&plugin->at9424_response, expected_str))
      _trncsp_at9424_xc_process(TNRCSP_STATE_FINAL, plugin->param, TNRCSP_RESULT_NOERROR, &plugin->at9424_response);
    else
      _trncsp_at9424_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &plugin->at9424_response);
  }

  void PluginAT9424_listen::process_add_ports_response()
  {
    char buff[100];
    tnrcsp_at9424_xc_state state;
    int v_i,v_e,p_i,p_e;
    v_i = atoi(plugin->backup_vlan.c_str());
    p_i = atoi(plugin->param->ingress_fid->c_str());
    v_i += p_i;
    v_e = atoi(plugin->backup_vlan.c_str());
    p_e = atoi(plugin->param->egress_fid->c_str());
    v_e += p_e;

    if(plugin->command_type == TNRC_ADD_INGRESS_PORT)
    {
      state = TNRCSP_STATE_ADD_INGRESS_PORT;
      sprintf(buff, "Adding ports to VLAN %d, please wait ...\n\r#",v_i);
    }
    else
    {
      state = TNRCSP_STATE_ADD_EGRESS_PORT;
      sprintf(buff, "Adding ports to VLAN %d, please wait ...\n\r#",v_e);
    }
    std::string expected_str(buff);
    plugin->command_type = TNRC_NO_COMMAND;

    if(at9424_find(&plugin->at9424_response, expected_str))
      _trncsp_at9424_xc_process(state, plugin->param, TNRCSP_RESULT_NOERROR, &plugin->at9424_response);
    else
      _trncsp_at9424_xc_process(TNRCSP_STATE_DELETE_VLAN, plugin->param, TNRCSP_RESULT_GENERICERROR, &plugin->at9424_response);
  }

  void PluginAT9424_listen::_trncsp_at9424_xc_process(int state, void* parameters, tnrcsp_result_t result, string* response_block)
  {
      plugin->tnrcsp_at9424_xc_process((tnrcsp_at9424_xc_state)state, (xc_at9424_params*)parameters, result, response_block);
  }

  void PluginAT9424_listen::save_equipments(std::string response_block)
  {
      std::string                 line = "";
      tnrc_portid_t               port_id;
      operational_state           oper_state;
      administrative_state        admin_state;
      tnrcsp_lsc_evmask_t         last_event;
      tnrc_sp_l2sp_speed          speed;
      tnrc_sp_l2sp_duplex_mode    duplex_mode;
      tnrcsp_l2sc_eqtype_t        equip_type;

      std::string                 tokens[6];
      map<tnrc_portid_t, tnrcsp_l2sc_resource_detail_t> tmp_resource_list;

      //start process
      while(response_block.find("if") != -1)
      {

        //Line format:
        //
        //  ifIndex.............................. 1
        //  ifMtu................................ 9198
        //  ifSeed............................... 1000000000
        //  ifAdminStatus........................ Up
        //  ifOperStatus......................... Up
        //  ifLinkUpDownTrapEnable............... Enabled

        line = pop_line(&response_block);
        line = line.substr(2);
        string temp = delete_to_first_space(line);
        tokens[0] = temp;                           //Port id

        line = pop_line(&response_block);
        line = line.substr(2);
        temp = delete_to_first_space(line);
        tokens[1] = temp;                           //Packet size

        line = pop_line(&response_block);
        line = line.substr(2);
        temp = delete_to_first_space(line);
        tokens[2] = temp;                           //Speed

        line = pop_line(&response_block);
        line = line.substr(2);
        temp = delete_to_first_space(line);
        tokens[3] = temp;                           //State of the port (Up/Down)

        line = pop_line(&response_block);
        line = line.substr(2);
        temp = delete_to_first_space(line);
        tokens[4] = temp;                           //Operational status of the port (Up/Down/unknown)

        line = pop_line(&response_block);
        line = line.substr(2);
        temp = delete_to_first_space(line);
        tokens[5] = temp;                           //(Enebled/Disabled)

        //Generate port id;
        char* port = (char *)tokens[0].c_str();
        port_id = at9424_decode_portid(port);
        if(port_id == 0)
            continue;

        //Admin/Oper state setup
        if(tokens[3].find("Up") != std::string::npos)
          admin_state = ENABLED;
        else
          admin_state = DISABLED;
        if(tokens[4].find("Up") != std::string::npos)
          oper_state = UP;
        else if (tokens[4].find("Down") != std::string::npos)
          oper_state = DOWN;
        else
          oper_state = DOWN;

        //Interface speed setup
        if(tokens[3].find("1000000000") != std::string::npos)
            speed = TNRC_SP_L2SC_SPEED_1G;
        else
            speed = TNRC_SP_L2SC_SPEED_NONE;

        //Interface type setup (Ethernet by default)
        equip_type = TNRC_SP_L2SC_ETH;

        //Duplex type setup
        duplex_mode = TNRC_SP_L2SC_DUPLEX_NONE;

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

        /// update AP Data model
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
        TNRC_NOT("   id=0x%x, op=%i, ad=%i, sp=%i, du=%i, eq=%i\n", it->first, d.oper_state, d.admin_state, d.speed, d.duplex_mode, d.equip_type);
      }
  }

  void PluginAT9424_listen::save_channels(std::string response_block)
  {
    xc_list.clear();

    std::string line = "";
    std::string response = response_block;
    std::string tokens[6];

    tnrc_portid_t port_in, port_out;
    vlanid_t vlanid;
    tnrcsp_l2sc_xc_state_t xc_state;

    //skip first 11 lines
    for(int i=0; i<11; i++)
        line = pop_line(&response);

    //std::string vlanid_str;
    std::string port_in_str;
    std::string port_out_str;
    tnrc_sp_xc_key node;
    plugin->vlan_list.clear();

    while(response.find("VLAN") != -1)
    {
        //Line format:
        //
        //VLAN Mode: User Configure
        //VLAN Information:
        //
        // VLAN Name ............................ Default_VLAN
        // VLAN ID .............................. 1
        // VLAN Type ............................ Port Based
        // Protected Ports ...................... No
        // Untagged Port(s)
        //   Configured ......................... 1-19
        //   Actual ............................. 1-19
        // Tagged Port(s) ....................... None
        //
        // VLAN Name ............................ phosphorus5
        // VLAN ID .............................. 5
        // VLAN Type ............................ Port Based
        // Protected Ports ...................... No
        // Untagged Port(s)
        //   Configured ......................... None
        //   Actual ............................. None
        // Tagged Port(s) ....................... 1,17

        line = pop_line(&response);
        string temp = delete_to_first_space(line);
        temp = delete_to_first_space(temp);
        temp = delete_to_first_space(temp);
        tokens[0] = temp;                           //Vlan name

        line = pop_line(&response);
        temp = delete_to_first_space(line);
        temp = delete_to_first_space(temp);
        temp = delete_to_first_space(temp);
        tokens[1] = temp;                           //Vlan id

        line = pop_line(&response);
        temp = delete_to_first_space(line);
        temp = delete_to_first_space(temp);
        temp = delete_to_first_space(temp);
        tokens[2] = temp;                           //Vlan type

        line = pop_line(&response);
        temp = delete_to_first_space(line);
        temp = delete_to_first_space(temp);
        temp = delete_to_first_space(temp);
        tokens[3] = temp;                           //Protected ports

        line = pop_line(&response);
        line = pop_line(&response);
        temp = delete_to_first_space(line);
        temp = delete_to_first_space(temp);
        tokens[4] = temp;                           //Untagged ports

        line = pop_line(&response);
        line = pop_line(&response);
        temp = delete_to_first_space(line);
        temp = delete_to_first_space(temp);
        temp = delete_to_first_space(temp);
        tokens[5] = temp;                           //Tagged ports

        //get vlan id
        vlanid = atoi(tokens[1].c_str());

        //get ports
        int mark = tokens[4].find_first_of(',');
        if(mark == -1)
          mark = tokens[4].find_first_of('-');
        port_in_str = tokens[4].substr(0,mark);
        port_out_str = tokens[4].substr(mark+1);
        char* port = (char *)port_in_str.c_str();
        port_in = at9424_decode_portid(port);
        port = (char *)port_out_str.c_str();
        port_out = at9424_decode_portid(port);

        node.vlanid = vlanid;
        node.port_in = port_in;
        node.port_out = port_out;

        if(node.vlanid < MAX_VLAN_ID)
        {
          plugin->vlan_list.insert(plugin->vlan_list.end(), node);

          xc_state = TNRCSP_L2SC_XCSTATE_ACTIVE;

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

  tnrc_portid_t  PluginAT9424_listen::at9424_decode_portid(char* portid_str)
  {
    tnrc_portid_t portid = 0;
    int p = 0;
    std::string port(portid_str);
    p = atoi(port.c_str());
    portid = p;

    //take board from AP
    portid = create_at9424_api_port_id(portid);

    return portid;
  }

  void PluginAT9424_listen::register_event(tnrc_portid_t portid, operational_state operstate, administrative_state adminstate,tnrc_sp_l2sp_speed speed, tnrc_sp_l2sp_duplex_mode duplex_mode, tnrcsp_l2sc_eqtype_t equip_type, tnrcsp_evmask_t e)
  {
    TNRC_INF("AT9424T notify via reqister callback: port=0x%x, operstate=%s, adminstate=%s", portid, SHOW_OPSTATE(operstate), SHOW_ADMSTATE(adminstate));

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

  tnrc_sp_xc_key PluginAT9424_listen::find_vlanid_from_portid(tnrc_portid_t portid)
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
  /**  Class xc_at9424_params
  *  Crossconnection information
  */
  xc_at9424_params::xc_at9424_params(trncsp_xc_activity activity, tnrcsp_handle_t handle,
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
    this->ingress_fid   = at9424_encode_portid(port_in);
    this->egress_fid    = at9424_encode_portid(port_out);


    this->activated       = false;
    this->repeat_count    = 0;
    this->final_result    = false;
    this->result          = TNRCSP_RESULT_NOERROR;

  }

//###################################################################
//########### FOUNDRY SP API FUNCTIONS ##############################
//###################################################################

  tnrcsp_result_t PluginAT9424::tnrcsp_make_xc(tnrcsp_handle_t *handlep, 
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

      if (!at9424_check_resources(portid_in, portid_out, vlanid)) 
        return TNRCSP_RESULT_PARAMERROR;

      xc_at9424_params* xc_params = new xc_at9424_params(TNRCSP_XC_MAKE, *handlep, portid_in, portid_out, vlanid, direction, isvirtual, activate, response_cb, response_ctxt, async_cb, async_ctxt);

      action_list.insert(action_list.end(), xc_params);
      wait_disconnect = true;
      return TNRCSP_RESULT_NOERROR;
  }

  tnrcsp_result_t PluginAT9424::tnrcsp_destroy_xc(tnrcsp_handle_t *handlep,
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

    if (!at9424_check_resources(portid_in, portid_out, vlanid))
        return TNRCSP_RESULT_PARAMERROR;

    xc_at9424_params* xc_params = new xc_at9424_params(TNRCSP_XC_DESTROY, *handlep, portid_in, portid_out, vlanid, direction, isvirtual, deactivate, response_cb, response_ctxt, NULL, NULL);

    action_list.insert(action_list.end(), xc_params);
    wait_disconnect = true;
    return TNRCSP_RESULT_NOERROR;
  }

  tnrcsp_result_t PluginAT9424::tnrcsp_reserve_xc (tnrcsp_handle_t *handlep, 
                                                   tnrc_port_id_t portid_in, 
                                                   label_t labelid_in, 
                                                   tnrc_port_id_t portid_out, 
                                                   label_t labelid_out, 
                                                   xcdirection_t direction,
                                                   tnrc_boolean_t isvirtual,
                                                   tnrcsp_response_cb_t response_cb,
                                                   void *response_ctxt)
  {
    return TNRCSP_RESULT_EQPTLINKDOWN;
  }

  tnrcsp_result_t PluginAT9424::tnrcsp_unreserve_xc (tnrcsp_handle_t *handlep, 
                                                     tnrc_port_id_t portid_in, 
                                                     label_t labelid_in, 
                                                     tnrc_port_id_t portid_out, 
                                                     label_t labelid_out, 
                                                     xcdirection_t        direction,
                                                     tnrc_boolean_t isvirtual,
                                                     tnrcsp_response_cb_t response_cb,
                                                     void *response_ctxt)
  {
    return TNRCSP_RESULT_EQPTLINKDOWN;
  }

  tnrcsp_result_t PluginAT9424::tnrcsp_register_async_cb (tnrcsp_event_t *events)
  {
    TNRC_INF("Register callback: portid=0x%x, operstate=%d, adminstate=%d, event=%d", events->portid, events->oper_state, events->admin_state, events->events);

    TNRC_OPSPEC_API::update_dm_after_notify (events->portid, events->labelid, events->events);

    TNRC_INF("Data model updated\n");
    return TNRCSP_RESULT_NOERROR;
  }

  //-------------------------------------------------------------------------------------------------------------------
  wq_item_status PluginAT9424::wq_function (void *d)
  {
    TNRC_ERR("!!!!!!!!!!!! PluginXmr::wq_function function should be never executed");
    return WQ_SUCCESS;	
  }

  //-------------------------------------------------------------------------------------------------------------------
  void PluginAT9424::del_item_data(void *d)
  {
    TNRC_ERR("!!!!!!!!!!!! PluginXmr::del_item_data function should be never executed");

    tnrcsp_action_t		*data;

    //get work item data
    data = (tnrcsp_action_t *) d;
    //delete data structure
    delete data;
  }


}
