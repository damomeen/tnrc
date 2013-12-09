/**
 * TNRC Telnet Plugin implementation.
 *
 * This file is part of GNU phosphorus-g2mpls.
 *
 * GNU phosphorus-g2mpls is free software, built on top fo GNU Quagga.
 * You can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation.
 *
 * GNU  phosphorus-g2mpls is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 *   @author Jan Nejman (CESNET)  <nejman_at_netflow_dot_cesnet_dot_cz>
 */

#ifndef TNRC_PLUGIN_TELNET_H
#define TNRC_PLUGIN_TELNET_H

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "thread.h"

#include "tnrc_plugin.h"

#include "tnrc_dm.h"
#include "tnrc_sp.h"


extern struct thread_master *master;

namespace TNRC_SP_TELNET {
  class PluginTelnet;

  typedef unsigned int tnrc_portid_t;


  typedef enum {
	TNRCSP_XC_MAKE,
	TNRCSP_XC_DESTROY,
	TNRCSP_XC_RESERVE,
	TNRCSP_XC_UNRESERVE
  } trncsp_xc_activity;


  typedef enum {
	TNRC_OPERSTATE_UP,
	TNRC_OPERSTATE_DOWN,
  } tnrc_operstate_t;


  typedef enum {
	TNRC_ADMINSTATE_ENABLED,
	TNRC_ADMINSTATE_DISABLED,
  } tnrc_adminstate_t;


  typedef enum {
	TNRC_SP_TELNET_STATE_NO_ADDRESS,
	TNRC_SP_TELNET_STATE_NO_SESSION,
	TNRC_SP_TELNET_STATE_CONNECTED
	// TODO: LOGGED_IN
  } tnrc_sp_state;


  typedef struct {
	tnrc_portid_t port_in;
	tnrc_portid_t port_out;
  } tnrc_sp_xc_key;


  typedef struct {
	time_t start;
	std::string command;
	int state;
	void *parameters;
  } tnrc_sp_handle_item;


  void remove_empty_lines(std::string *msg);
  void remove_line(std::string *msg);
  std::string pop_line(std::string *msg);
  int match(std::string *data, std::string pattern, int token_num, std::string tokens[]);


  /*
   *******************************************************************************
   * Abstract Class TELNET
   * Provides communication with TELNET protocol
   *******************************************************************************
   */
  class TELNET {
  public:
	TELNET();
	~TELNET();

	virtual int connect() = 0;
	virtual int send(std::string *msg) = 0;
	virtual int recv(std::string *msg, int buff_size) = 0;
	virtual int close() = 0;
	virtual int fd() = 0;
  };


  /*
   *******************************************************************************
   *  Abstract class PluginTelnet_listen
   *  Listen and process received TELNET responses
   *******************************************************************************
   */
  class PluginTelnet_listen {
  public:
	PluginTelnet *plugin;

	PluginTelnet_listen();
	~PluginTelnet_listen();

	void add_handler(int key, tnrc_sp_handle_item value);
	void del_handler(int key);
	void add_message(std::string *msg);
	void process_message(std::string *msg);
	void command_prompt_set(std::string *msg);
	virtual void check_func_waittime() = 0;

  protected:
	std::string command_prompt;
	std::string response_block;
	std::map<const int, tnrc_sp_handle_item> handlers;

	virtual void do_response(tnrc_sp_handle_item *item, std::string *response) = 0;
	static void *entry_point(PluginTelnet_listen *self);
  };


  /*
   *******************************************************************************
   * Abstract Specific Part class
   * for the devices that communicates using TELNET protocol
   *******************************************************************************
   */
  class PluginTelnet : public Plugin {
  public:
	friend class PluginTelnet_listen;
	friend class PluginClsFSC_listen;


	PluginTelnet();
	PluginTelnet(std::string name);
	~PluginTelnet();

	int getFd(); // Get socket handle
	void readT(thread *read); // Read from socket thread
	void retriveT(thread *retrive); // "cron" for equipment state
	int doRetrive();
	int doRead();
	int gen_identifier();

  protected:
	std::string telnet_newline; // New line string, usually "\r\n"
	std::string received_buffer; // TELNET received buffer
	TELNET *sock; // Real device (TELNET_socket) or simulator (TELNET_simulator)
	PluginTelnet_listen *listen_proc;
	tnrc_sp_state TELNET_status;
	thread *t_read;
	thread *t_retrive;

	int telnet_connect();
	int telnet_close();
	void newline_set(std::string *newline);
	tnrcsp_result_t asynch_send(std::string *command, int state = 0, void *parameters = NULL);
	virtual void get_device_info() = 0;

  private:
	int identifier; // aka CTAG for TL1
  };


  /*
   *******************************************************************************
   * Provides communication with TELNET protocol using network connection
   *******************************************************************************
   */
  class TELNET_socket : public TELNET {
  public:
	TELNET_socket();
	TELNET_socket(in_addr address, int port);
	~TELNET_socket();

	int connect();
	int send(std::string *msg);
	int recv(std::string *msg, int buff_size);
	int fd();
	int close();

  private:
	in_addr remote_address; // address of the real remote device
	int remote_port; // port of the real remote device
	int socket_handle; // socket handle
	int telnet_mode; // state of telnet protocol - data/options/suboptions
	int telnet_option; // options bytes in telnet protocol
  };


  /*
   *******************************************************************************
   * Simulates behaviour of real TELNET device - not implemented
   *******************************************************************************
   */
  class TELNET_simulator : public TELNET {
  public:
	TELNET_simulator();
	~TELNET_simulator();

	int connect();
	int send(std::string *msg);
	int recv(std::string *msg, int buff_size);
	int fd();
	int close();
  };

}

#endif /*  TNRC_PLUGIN_TELNET_H */
