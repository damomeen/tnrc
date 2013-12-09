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
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_plugin_tl1.h"

namespace TNRC_SP_TL1
{
  static int tl1_read_func (struct thread *thread);

 /*************************************************************
 *
 *        UTILS
 *
 **************************************************************/
 
  void remove_empty_lines(std::string* mes)
  {
    int pos = 0;
    while (pos < 2)
    {
      pos = mes->find("\n");
      if(pos < 2)
        mes->erase(0, pos+1);
    }
  }

  void remove_line(std::string* mes)
  {
    int pos = 0;
    pos = mes->find("\n");
    if(pos == -1)
      mes->clear();
    else
      mes->erase(0, pos+1);
  }

  std::string pop_line(std::string* mes)
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

  int match(std::string* data, std::string pattern, int token_num, std::string tokens[])
  {
    int i, pos;
    for(i=0; i<token_num; i++)
    {
      std::string ch = pattern.substr(i,1);
      int pos = data->find(ch);
      if(pos == -1 || ch == "")
      {
        tokens[i] = *data;
        data->clear();
        break;
      }
      else
      {
        tokens[i] = data->substr(0, pos);
        data->erase(0, pos+1);
      }
    }
    return i+1;
  }

  void match_dict(std::string* data, dict& d)
  {
    std::string key, value;
    int pos;
  
    while(true)
    {
      pos = data->find("=");
      if(pos == -1)
        break;
      key = data->substr(0, pos);
      data->erase(0, pos+1);
  
      pos = data->find(",");
      if(pos == -1)
        break;
      value = data->substr(0, pos);
      data->erase(0, pos+1);
  
      d[key] = value;
    }
  }

  int get_message_type(std::string* mes, std::string* mes_code)
  {
    char code[10]; 
    int ret_value;
    int code_len;
    int ctag;
    code[9] = '\x00';

    while(mes->length() > 0)
    {
      ret_value = sscanf(mes->c_str(), "%9s %i", code, &ctag);
      code_len = strlen(code);
      if(code_len < 1 || 2 < code_len || ret_value != 2)
        remove_line(mes);
      else
        break;
    }
    if(mes->length()==0)
    {
      mes_code = NULL;
      return -1;
    }
    mes_code->append(code);
    return ctag;
  }

//================================================================================
/**  Class PluginTl1
  *  Abstract Specyfic Part class for devices with TL1 interface
  */
  PluginTl1::PluginTl1(): t_read(NULL), t_retrive(NULL)
  {
    handle_ = 1;
    xc_id_ = 1;
    wqueue_ = work_queue_new (TNRC_MASTER.getMaster(), ADVA_WQ);
    wqueue_->spec.workfunc = workfunction;
    wqueue_->spec.del_item_data = delete_item_data;
    wqueue_->spec.max_retries = 5;
    //wqueue_->spec.hold = 500;
    sock = NULL;
    TL1_status    = TNRC_SP_TL1_STATE_NO_ADDRESS;
    _activity  = GET_EQUIPMENT;
    identifier = 0;

    default_retrive_interwall = TL1_RETRIVE_INTERWALL;
    retrive_interwall = default_retrive_interwall;
  }

  PluginTl1::PluginTl1(std::string name): t_read(NULL), t_retrive(NULL)
  {
    name_ = name;
    handle_ = 1;
    xc_id_ = 1;
    wqueue_ = work_queue_new (TNRC_MASTER.getMaster(), ADVA_WQ);
    wqueue_->spec.workfunc = workfunction;
    wqueue_->spec.del_item_data = delete_item_data;
    wqueue_->spec.max_retries = 5;
    //wqueue_->spec.hold = 500;
    sock = NULL;
    TL1_status    = TNRC_SP_TL1_STATE_NO_ADDRESS;
    _activity  = GET_EQUIPMENT;
    identifier = 0;

    default_retrive_interwall = TL1_RETRIVE_INTERWALL;
    retrive_interwall = TL1_RETRIVE_INTERWALL;
  }

  PluginTl1::~PluginTl1()
  {
    close_connection();
  }

  void PluginTl1::retriveT(thread *retrive)
  {
    t_retrive = retrive;
  }

  void PluginTl1::readT(thread *read)
  {
    t_read = read;
  }

/** PluginTl1::getFd()
  * @return FileDescriptor
  */
  int PluginTl1::getFd()
  {
    return sock->fd();
  }

  //-------------------------------------------------------------------------------------------------------------------

/** waits for autonomous messages with one of the texts */
  void PluginTl1::autonomous_handle(int handle, std::string* aid, std::string text, int state, void* parameters)
  {
    tnrc_sp_handle_item item = {time(NULL), *aid, text, state, parameters};
    listen_proc->add_handler(handle, item);
  }


  int PluginTl1::tl1_connect()
  {
    if (TL1_status == TNRC_SP_TL1_STATE_NO_ADDRESS)
    {
      TNRC_ERR("connect unsuccessful: no information about remote host address");
      return -1;
    }

    if (t_read != NULL)
    {
      close_connection();
      THREAD_OFF(t_read);
      t_read = NULL;
    }

    int result = sock->_connect();
    if (result == 0)
    {
      TL1_status = TNRC_SP_TL1_STATE_NOT_LOGGED_IN;
      TNRC_INF("Connected OK");
      readT(thread_add_read(master, tl1_read_func, this, getFd()));
    }
    else
    {
      TNRC_ERR("Connection Error");
    }
    return result;
  }

/** send TL1 command asynchronously  */
  tnrcsp_result_t PluginTl1::asynch_send(std::string* command, int ctag, int state, void* parameters)
  {
    if (((command->find("ACT-USER") != -1) && (TL1_status == TNRC_SP_TL1_STATE_NOT_LOGGED_IN)) || (TL1_status == TNRC_SP_TL1_STATE_LOGGED_IN))
    {
      TNRC_INF("Send command: %s", command->c_str());
      if(sock->_send(command) == -1)
      {
        TNRC_WRN("Connection reset by peer");
        TL1_status = TNRC_SP_TL1_STATE_NO_SESSION;
        return TNRCSP_RESULT_EQPTLINKDOWN;
      }
      message_map[ctag] = *command;
      tnrc_sp_handle_item item = {time(NULL), "", "", state, parameters};
      listen_proc->add_handler(ctag, item);
      return TNRCSP_RESULT_NOERROR;
    }
    else
    {
      TNRC_WRN("send unsuccessful");
      if(TL1_status == TNRC_SP_TL1_STATE_NO_SESSION)
        return TNRCSP_RESULT_EQPTLINKDOWN;
      return TNRCSP_RESULT_INTERNALERROR;
    }
  }

/** Read thread's rountine (thread *t_read) */
  int PluginTl1::doRead()
  {
    std::string recMsg("");
    sock->_recv(&recMsg, 1024);

    int read_pos       =  received_buffer.length();
    received_buffer    += recMsg;
    int len            =  received_buffer.length();


    int t_index;      //terminator index
    while(read_pos < len)
    {
      for (t_index=0; t_index < no_of_term_chars; t_index ++)
      {
        if(received_buffer[read_pos] == terminator[t_index])   // message terminators
        {
          std::string msg = received_buffer.substr(0, read_pos+1);
          received_buffer = received_buffer.substr(read_pos+1);
          read_pos = 0;
          len = received_buffer.length();
          listen_proc->process_message(&msg);
          break;
        }
      }
      read_pos++;
    }
    return 0;
  }


/** Backgrount thread's rountine (thread *t_retrive)
  * - connection with device
  * - equipment
  * - connections
  */
  int PluginTl1::doRetrive()
  {
    int result = -1;
    static bool firstLoggIn = true;
    switch(TL1_status)
    {
      case TNRC_SP_TL1_STATE_NO_SESSION:
        TNRC_INF("trying to connect");
        result = tl1_connect();
        if (result != 0)
          break;
      case TNRC_SP_TL1_STATE_NOT_LOGGED_IN:
        TNRC_INF("trying to logg in");
        result = tl1_log_in(login, password);
        break;
      case TNRC_SP_TL1_STATE_LOGGED_IN:
        if (firstLoggIn)
        {
          get_all_equipment();
          get_all_xc();
          firstLoggIn = false;
        }
        else
        {
          switch (_activity)
          {
            case GET_EQUIPMENT:
              get_all_equipment();
              _activity = GET_XC;
              result = 0;
              break;
            case GET_XC:
              get_all_xc();
              _activity = GET_EQUIPMENT;
              result = 0;
              break;
          }
        }
        break;
    }
    return result;
  }

  int PluginTl1::close_connection()
  {
    if(sock->close_connection() == -1)
        return -1;
    return 0;
  }


  static
  int do_check(struct thread* thread)
  {
     tnrcsp_xc_timer_event* event = (tnrcsp_xc_timer_event*)THREAD_ARG(thread);

     event->pluginListen->process_xc(event->state, event->parameters, TNRCSP_RESULT_NOERROR, NULL);
     
     delete event;
     return 1;
  }

  void PluginTl1::set_timer_event(int state, void* parameters, int timeout)
  {
     tnrcsp_xc_timer_event* event = new tnrcsp_xc_timer_event;
     event->state        = state;
     event->parameters   = parameters;
     event->pluginListen = this->listen_proc;
     
     thread_add_timer(master /*thread*/, do_check /*callback*/, (void*)event /*arg*/, timeout);
  }


//==============================================================================
/** Class TL1_socket */
  TL1_socket::TL1_socket()
  {
  }

  TL1_socket::TL1_socket(in_addr address, int port)
  {
    remote_address = address;
    remote_port = port;

    socket_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    TNRC_INF("Socket description = %d", socket_desc);
  }

  TL1_socket::~TL1_socket()
  {
    close_connection();
  }

  int TL1_socket::_connect()
  {
    struct sockaddr_in remote;
    struct hostent *he;

    remote.sin_family     = AF_INET;
    remote.sin_port       = htons(remote_port);
    remote.sin_addr       = remote_address;
    /// clear the rest of the struct
    memset(&(remote.sin_zero), '\0', 8);

    if(connect(socket_desc, (struct sockaddr*)&remote, sizeof(remote)) == 0)
      return 0;
    else
    {
      TNRC_ERR("connect unsuccessful: %i, %s", errno, strerror(errno));
      return -1;
    }
  }

  int TL1_socket::_send(std::string* mes)
  {
    if(send(socket_desc, mes->c_str(), mes->length(), 0) != mes->length())
      return -1;
    return 0;
  }

/** TL1_socket::_recv(std::string* mes, int buff_size)
  * @param mes: pointer to the string that the received message is written
  * @param buff_size: maximum message length
  * @return error_code: 1: No error0:timeout, -1:sock error 
  */
  int TL1_socket::_recv(std::string* mes, int buff_size)
  {
    int mes_len;
    char* buffer;

    fd_set readfds;
    struct timeval tv = {1,0};

    FD_ZERO(&readfds);
    FD_SET(socket_desc, &readfds);

    int ret_value = select(socket_desc+1, &readfds, NULL, NULL, &tv);
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
    else if (FD_ISSET(socket_desc, &readfds))
    {
      buffer = new char[buff_size];
      mes_len = recv(socket_desc, buffer, buff_size, 0);
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

  int TL1_socket::close_connection()
  {
    if(close(socket_desc) == -1)
      return -1;
    return 0;
  }

  int TL1_socket::fd()
  {
    return socket_desc;
  }

//===================================================================================
/** class PluginTl1_listen */
  PluginTl1_listen::PluginTl1_listen()
  {
  }
  PluginTl1_listen::~PluginTl1_listen()
  {
  }

  void PluginTl1_listen::add_handler(int key, tnrc_sp_handle_item value)
  {
    handlers[key]=value;
  }
/** message is multipart, save not last part, part is joined */
  void PluginTl1_listen::save_multiple_part(std::string* response_block, int ctag)
  {
    if(multipart.count(ctag) > 0)
      multipart[ctag] += *response_block;
    else
      multipart[ctag] = *response_block;
  }

/** return all saved multiparts or empty string */
  std::string PluginTl1_listen::get_saved_multiple_parts(int ctag)
  {
    std::string parts = "";
    if (multipart.count(ctag) > 0)
      parts = multipart[ctag];
    multipart.erase(ctag);
    return parts;
  }

/** process all kind of incoming TL1 messages; separate different message types */
  void PluginTl1_listen::process_message(std::string* mes)
  {
    TNRC_INF("---\n%s\n---\n", mes->c_str());
    remove_empty_lines(mes);
    int ctag;
    std::string mes_code = "";
    if((ctag = get_message_type(mes, &mes_code)) == -1)
    {
      TNRC_ERR("something wrong: message code unrecognized");
      return;
    }

    if(mes_code == "M")
    {
      //TNRC_DBG("process_response");
      process_response(mes, ctag);
    }
    else if(mes_code == "*C" || mes_code == "**" || mes_code == "*" || mes_code == "A")
    {
//      TNRC_DBG("process_autonomous");
      process_autonomous(mes, &mes_code);
    }
    else if(mes_code == "IP" || mes_code == "PF" || mes_code == "OK" || mes_code == "NA" || mes_code == "NG" || mes_code == "RL" )
    {
//      TNRC_DBG("process_acknowledgmen");
      process_acknowledgment(&mes_code, ctag);
    }
    else
    {
      TNRC_ERR("TL1 message not recognized %s", mes_code.c_str());
    }
  }

/** process TL1 response message; separate different response types */
  void PluginTl1_listen::process_response(std::string* mes, int ctag)
  {
    char compl_code[10];
    if(sscanf(mes->c_str(), "%*s  %*i %s", compl_code) != 1)
        return;
    if(plugin->message_map.count(ctag) == 0)
    {
      TNRC_WRN("lack of command in message map");
        return;
    }
    std::string comp_code(compl_code);
    std::string command = plugin->message_map[ctag];
    remove_line(mes);
    plugin->message_map.erase(ctag);
    if(comp_code == "COMPLD" || comp_code == "DELAY" || comp_code == "RTRV")
      process_successful(mes, &command, ctag);
    else if(comp_code == "DENY" || comp_code == "PRTL")
      process_denied(mes, &command, ctag);
  }

/** process TL1 acknowledgment */
  void PluginTl1_listen::process_acknowledgment(std::string* mes_code, int ctag)
  {
    if(handlers.count(ctag) > 0)
      if(*mes_code == "IP" || *mes_code == "PP" || *mes_code == "OK" || *mes_code == "RL")
        /// command is processed - extend timeout time
        handlers[ctag].start = time(NULL);
      else if(*mes_code == "NA" || *mes_code == "NG")
        /// command processing failed - generate timeout as fast as possible
        handlers[ctag].start = 0;   
  }

  /** process successful responses */
  void PluginTl1_listen::process_successful(std::string* response_block, std::string* command, int ctag)
  {
    if(response_block->find( "\n>") != -1)           /// message is parted
    {
      save_multiple_part(response_block, ctag);    /// save and wait for next part
      return;
    }

    /// get all parts of the message
    std::string _response_block = get_saved_multiple_parts(ctag) + *response_block; 

    tnrc_sp_handle_item handler;
    bool found;
    if (handlers.count(ctag) > 0)
    {
      handler = handlers[ctag];
      handlers.erase(ctag);
      found = true;
    }
    if(found && handler.parameters)
      process_xc(handler.state, handler.parameters, TNRCSP_RESULT_NOERROR, &_response_block); 
  }

  void PluginTl1_listen::process_denied(std::string* response_block, std::string* command, int ctag)
  {
    /* process denied responses */
    tnrc_sp_handle_item handler;
    bool found;
    if (handlers.count(ctag) > 0)
    {
        handler = handlers[ctag];
        handlers.erase(ctag);
        found = true;
    }
    if(found && handler.parameters)
        process_xc(handler.state, handler.parameters, TNRCSP_RESULT_GENERICERROR, response_block);
  };

  /**
  * Thread read function
  */
  static int tl1_read_func (struct thread *thread)
  {
    TNRC_INF("TL1 Plugin socket read thread");

    int fd;
    PluginTl1 *p;

    fd = THREAD_FD (thread);
    p = static_cast<PluginTl1 *>(THREAD_ARG (thread));

    int result = p->doRead();
    p->readT(thread_add_read(master, tl1_read_func, p, fd));
    return result;
  }

  /** 
  * Thread retrive function
  */
  int tl1_retrive_func (struct thread *thread)
  {
    PluginTl1 *p;

    p = static_cast<PluginTl1 *>(THREAD_ARG (thread));

    int result = p->doRetrive();
    p->retriveT(thread_add_timer(master, tl1_retrive_func, p, p->retrive_interwall));
    return result;
  }

/** class TL1_simulator.
  * Simulates TL1_device behaviour
  */
  TL1_simulator::TL1_simulator()
  {
    TNRC_ERR("Tl1 adva simulator is not yet implemented");
  }

  TL1_simulator::~TL1_simulator()
  {
  }
  int TL1_simulator::_connect()
  {
    return 0;
  }
  int TL1_simulator::_send(std::string* mes)
  {
    //TODO now not supported
    return -1;
  }
  int TL1_simulator::_recv(std::string* mes, int buff_size)
  {
    //TODO now not supported
    return -1;
  }

  int TL1_simulator::close_connection()
  {
    if(close(socket_desc_1) == -1)
      return -1;
    if(close(socket_desc_2) == -1)
      return -1;
    return 0;
  }
  int TL1_simulator::fd()
  {
    return socket_desc_2;
  }
}

