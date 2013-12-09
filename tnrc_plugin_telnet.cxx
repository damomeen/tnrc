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


#include <zebra.h>
#include "vty.h"

#include "tnrcd.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_plugin_telnet.h"

namespace TNRC_SP_TELNET {

  static int telnet_read_func(struct thread *thread);

  /*
   *******************************************************************************
   * Utilities
   *******************************************************************************
   */



  /*
   * Remove empty line from string
   */
  void remove_empty_lines(std::string *msg) {
	int pos = 0;
	while (pos != -1) {
	  pos = msg->find("\n\n");
	  if (pos >= 0)
		msg->erase(pos, 1);
	}
  }



  /*
   * Remove the first line from the string
   */
  void remove_line(std::string *msg) {
	int pos = 0;
	pos = msg->find("\n");
	if (pos == -1)
	  msg->clear();
	else
	  msg->erase(0, pos + 1);
  }



  /*
   * Return the first line or the rest of string
   */
  std::string pop_line(std::string *msg) {
	int pos = msg->find("\n");
	if (pos == -1) {
	  std::string _msg = *msg;
	  msg->clear();
	  return _msg;
	}
	std::string _msg = msg->substr(0, pos);
	msg->erase(0, pos + 1);
	return _msg;
  }



  /*
   * Remove crlf and control characters from the string
   */
  void remove_crlf(std::string *msg) {
	string::iterator it;
	bool trimmed;
	do {
	  trimmed = false;
	  for (it = msg->begin(); it < msg->end(); it++) {
		if (*it < 20) {
		  msg->erase(it);
		  trimmed = true;
		}
	  }
	} while (trimmed == true);
  }



  /*
   * Find pattern in the data
   */
  int match(std::string *data, std::string pattern, int token_num, std::string tokens[]) {
	int i;
	for (i = 0; i < token_num; i++) {
	  std::string ch = pattern.substr(i, 1);
	  int pos = data->find(ch);
	  if (pos == -1 || ch == "") {
		tokens[i] = *data;
		data->clear();
		break;
	  } else {
		tokens[i] = data->substr(0, pos);
		data->erase(0, pos + 1);
	  }
	}
	return i + 1;
  }



  /*
   *******************************************************************************
   *  Class PluginTelnet
   *  Abstract Specific Part class for devices with TELNET interface
   *******************************************************************************
   */

  PluginTelnet::PluginTelnet() : t_read(NULL), t_retrive(NULL) {
	handle_ = 1;
	sock = NULL;
	TELNET_status = TNRC_SP_TELNET_STATE_NO_ADDRESS;
	telnet_newline = "\r\n";
  }



  PluginTelnet::PluginTelnet(std::string name) : t_read(NULL), t_retrive(NULL) {
	name_ = name;
	handle_ = 1;
	sock = NULL;
	TELNET_status = TNRC_SP_TELNET_STATE_NO_ADDRESS;
	telnet_newline = "\r\n";
  }



  PluginTelnet::~PluginTelnet() {
	telnet_close();
  }



  void PluginTelnet::retriveT(thread *retrive) {
	t_retrive = retrive;
  }



  void PluginTelnet::readT(thread *read) {
	t_read = read;
  }



  int PluginTelnet::getFd() {
	return sock->fd();
  }



  int PluginTelnet::telnet_connect() {
	int result;

	if (TELNET_status == TNRC_SP_TELNET_STATE_NO_ADDRESS) {
	  TNRC_ERR("%s %d: Connect failed - no information about remote host address",__FILE__,__LINE__);
	  return -1;
	}

	if (t_read != NULL) {
	  telnet_close();
	  THREAD_OFF(t_read);
	  t_read = NULL;
	}

	if ((result = sock->connect()) == 0) {
	  TELNET_status = TNRC_SP_TELNET_STATE_CONNECTED;
	  TNRC_INF("%s %d: Connected", __FILE__, __LINE__);
	  readT(thread_add_read(master, telnet_read_func, this, getFd()));
	} else
	  TNRC_ERR("%s %d: Connection failed", __FILE__, __LINE__);
	return result;
  }



  /*
   * Send a TELNET command
   */
  tnrcsp_result_t PluginTelnet::asynch_send(std::string *command, int state, void *parameters) {
	if (TELNET_status == TNRC_SP_TELNET_STATE_CONNECTED) {
	  tnrc_sp_handle_item item;
	  int handle = new_handle();

	  // Set start time
	  item.start = time(NULL);
	  item.command = *command;
	  item.state = state;
	  item.parameters = parameters;

	  // Remove crlf from the command
	  remove_crlf(&item.command);

	  // Add item to the handlers array
	  listen_proc->add_handler(handle, item);

	  TNRC_DBG("%s %d: Sending command: [%s]", __FILE__, __LINE__, item.command.c_str());

	  std::string cmd = item.command;
	  cmd.append(telnet_newline);
	  if (sock->send(&cmd) == -1) {
		TNRC_WRN("%s %d: Connection reset by peer", __FILE__, __LINE__);
		TELNET_status = TNRC_SP_TELNET_STATE_NO_SESSION;
		listen_proc->del_handler(handle);
		return TNRCSP_RESULT_EQPTLINKDOWN;
	  }
	  return TNRCSP_RESULT_NOERROR;
	} else {
	  TNRC_WRN("%s %d: Send unsuccessful", __FILE__, __LINE__);
	  if (TELNET_status == TNRC_SP_TELNET_STATE_NO_SESSION)
		return TNRCSP_RESULT_EQPTLINKDOWN;
	  return TNRCSP_RESULT_INTERNALERROR;
	}

  }



  /*
   * Set newline delimiter; default is \r\n
   */
  void PluginTelnet::newline_set(std::string *newline) {
	telnet_newline = *newline;
  }



  /*
   * Read thread's rountine (thread *t_read)
   */
  int PluginTelnet::doRead() {
	std::string recMsg("");
	sock->recv(&recMsg, 1024);

	int read_pos = received_buffer.length();
	received_buffer += recMsg;
	int len = received_buffer.length();

	while (read_pos < len) {
	  if (received_buffer[read_pos] == '\n') {
		std::string msg = received_buffer.substr(0, read_pos + 1);
		received_buffer = received_buffer.substr(read_pos + 1);
		read_pos = 0;
		len = received_buffer.length();

		// If a prompt is found on the line, submit previous response_block
		listen_proc->process_message(&msg);

		// Add a line to response_block
		listen_proc->add_message(&msg);
	  } else
		read_pos++;
	}

	// If a prompt is found on the line, submit previous response_block
	if (len)
	  listen_proc->process_message(&received_buffer);
	return 0;
  }



  /*
   * Background thread's rountine, (get device info)
   */
  int PluginTelnet::doRetrive() {
	int result = -1;

	if (TELNET_status == TNRC_SP_TELNET_STATE_NO_SESSION) {
	  result = telnet_connect();
	}
	if (TELNET_status == TNRC_SP_TELNET_STATE_CONNECTED) {
	  listen_proc->check_func_waittime();
	  get_device_info();
	  result = 0;
	}
	return result;
  }



  int PluginTelnet::telnet_close() {
	return sock->close();
  }



  int PluginTelnet::gen_identifier() {
	return identifier++;
  }



  /*
   **************************************************************************
   * Class PluginTelnet_listen
   **************************************************************************
   */

  PluginTelnet_listen::PluginTelnet_listen() {
	command_prompt = "Switch> "; // Default command prompt
  }



  PluginTelnet_listen::~PluginTelnet_listen() {
  }



  void PluginTelnet_listen::add_handler(int key, tnrc_sp_handle_item value) {
	handlers[key] = value;
  }



  void PluginTelnet_listen::del_handler(int key) {
	if (handlers.count(key))
	  handlers.erase(key);
  }



  /*
   * Set command prompt string
   */
  void PluginTelnet_listen::command_prompt_set(std::string *msg) {
	command_prompt = *msg;
  }



  /*
   * Add a line to the response_block
   */
  void PluginTelnet_listen::add_message(std::string *msg) {
	response_block += *msg;
  }



  /*
   * Process complete block of response, on the first line is original command
   */
  void PluginTelnet_listen::process_message(std::string *msg) {
	// Check if command prompt is in the block
	if (msg->find(command_prompt) == -1)
	  return;

	// Remove double \n\n
	remove_empty_lines(&response_block);

	// Check if something to do
	if (response_block.length() == 0)
	  return;

	// Get the first line = full command line
	std::string command_line = pop_line(&response_block);

	// Check if command prompt is on the line
	int pos = command_line.find(command_prompt);
	if (pos == -1) {
	  response_block.clear();
	  return;
	}

	// Trim string (remove \r\n ...)
	remove_crlf(&command_line);

	// Remove all characters before prompt
	command_line = command_line.substr(pos + command_prompt.length());

	// Find a command in the command queue
	int key = -1;
	std::map<const int, tnrc_sp_handle_item>::iterator it;
	for (it = handlers.begin(); it != handlers.end() && key == -1; it++) {
	  if (it->second.command == command_line)
		key = it->first;
	}

	// If key is found
	if (key != -1) {
	  TNRC_DBG("%s %d: Found cmd=[%s], time=%d ctag=%d", __FILE__, __LINE__, command_line.c_str(), handlers[key].start, key);
	  do_response(&handlers[key], &response_block);
	  del_handler(key);
	} else
	  TNRC_WRN("%s %d: Command [%s] not found in the queue - ignoring response block...", __FILE__, __LINE__, command_line.c_str());
	response_block.clear();
  }



  /*
   * Thread read function
   */
  static int telnet_read_func(struct thread *thread) {
	PluginTelnet *p;
	int fd;

	fd = THREAD_FD(thread);
	p = static_cast<PluginTelnet *> (THREAD_ARG(thread));

	int result = p->doRead();
	p->readT(thread_add_read(master, telnet_read_func, p, fd));
	return result;
  }



  /*
   * Thread retrieve function
   */
  int telnet_retrive_func(struct thread *thread) {
	PluginTelnet *p;

	p = static_cast<PluginTelnet *> (THREAD_ARG(thread));
	int result = p->doRetrive();
	p->retriveT(thread_add_timer(master, telnet_retrive_func, p, 10));//TODO zmienilem z 10 na 60
	return result;
  }



  /*
   *******************************************************************************
   * Class TELNET
   *******************************************************************************
   */
  TELNET::TELNET() {
  };



  TELNET::~TELNET() {
  };



  /*
   *******************************************************************************
   * Class TELNET_socket
   *******************************************************************************
   */

  TELNET_socket::TELNET_socket() {
  }



  TELNET_socket::TELNET_socket(in_addr address, int port) {
	struct timeval tv = {2, 0}; /* 2 sec timeout */

	remote_address = address;
	remote_port = port;
	telnet_mode = 0;

	// Create a new TCP socket
	socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	TNRC_DBG("%s %d: Socket handle = %d", __FILE__, __LINE__, socket_handle);

	// Set timeout for connection
	setsockopt(socket_handle, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv));
  }



  TELNET_socket::~TELNET_socket() {
	close();
  }



  int TELNET_socket::connect() {
	struct sockaddr_in remote;
	unsigned char telnet_init[3] = {255, 253, 1}; //magic sequence
	struct timeval tv = {2, 0}; /* 2 sec timeout */
	int rc;

	memset(&remote, 0, sizeof (remote));

	remote.sin_family = AF_INET;
	remote.sin_addr = remote_address;
	remote.sin_port = htons(remote_port);

	TNRC_INF("%s %d: Connecting to %s:%d", __FILE__, __LINE__, inet_ntoa(remote_address), remote_port);

	if (::connect(socket_handle, (struct sockaddr*) & remote, sizeof (remote)) != 0) {
	  if (errno == EBADF) { /* bad file descriptor (9) */
		::close(socket_handle);
		socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		setsockopt(socket_handle, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv));
		if (::connect(socket_handle, (struct sockaddr*) & remote, sizeof (remote)) != 0) {
		  TNRC_ERR("%s %d: Connect unsuccessful: [%i] %s", __FILE__, __LINE__, errno, strerror(errno));
		  return -1;
		} else {
		  TNRC_DBG("%s %d: Socket handle = %d", __FILE__, __LINE__, socket_handle);
		}
	  } else {
		TNRC_ERR("%s %d: Connect unsuccessful: [%i] %s", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	  }
	}

	// Send init telnet sequence - request to echoing all characters
	::send(socket_handle, (void *) telnet_init, 3, 0);
	return 0;
  }



  int TELNET_socket::send(std::string *msg) {
	int msg_len = msg->length();
	if (::send(socket_handle, msg->c_str(), msg_len, 0) != msg_len)
	  return -1;
	return 0;
  }



  int TELNET_socket::recv(std::string *msg, int buff_size) {
	int msg_len;
	char *buffer;
	unsigned char *ptr;
	fd_set readfds;
	struct timeval tv = {1, 0};
	int ret_value;
	int i, j;

	FD_ZERO(&readfds);
	FD_SET(socket_handle, &readfds);

	if ((ret_value = ::select(socket_handle + 1, &readfds, NULL, NULL, &tv)) == -1) {
	  TNRC_ERR("%s %d: Socket select error: [%i] [%s]", __FILE__, __LINE__, errno, strerror(errno));
	  return -1;
	}

	if (ret_value == 0) // timeout occurred
	  return 0;

	if (FD_ISSET(socket_handle, &readfds)) {
	  buffer = new char[buff_size];
	  msg_len = ::recv(socket_handle, buffer, buff_size, 0);
	  if (msg_len > 0) {
		i = 0;
		j = 0;
		ptr = (unsigned char *) buffer;
		while (i < msg_len) {
		  if (telnet_mode == 2) {
			switch (telnet_option) {
			  case 250: if (*ptr == 255) telnet_mode = 1;
				break;
			  case 251:
			  case 253:
				telnet_mode = 0;
				break;
			  default: telnet_mode = 0;
			}
		  } else if (telnet_mode == 1) {
			switch (*ptr) {
			  case 250: /* SB */
			  case 251: /* WILL */
			  case 252: /* WONT */
			  case 253: /* DO */
			  case 254: /* DONT */
				telnet_option = *ptr;
				telnet_mode = 2;
				break;
			  default: telnet_mode = 0;
				break;
			}
		  } else if (*ptr == 0xFF)
			telnet_mode = 1;
		  else {
			telnet_mode = 0;
			if (*ptr != 0) {
			  if (i != j)
				buffer[j] = *ptr;
			  j++;
			}
		  }
		  ptr++;
		  i++;
		}
		if (j > 0)
		  msg->append(std::string(buffer, j));
		delete []buffer;
		return 1;
	  }
	  return msg_len;
	} else
	  return -1;
  }



  int TELNET_socket::close() {
	return ::close(socket_handle);
  }



  int TELNET_socket::fd() {
	return socket_handle;
  }



  /*
   *******************************************************************************
   * Class TELNET_simulator
   * Simulates TELNET_device behaviour
   *******************************************************************************
   */

  TELNET_simulator::TELNET_simulator() {
	TNRC_ERR("%s %d: Telnet simulator is not yet implemented", __FILE__, __LINE__);
  }



  TELNET_simulator::~TELNET_simulator() {
  }



  int TELNET_simulator::connect() {
	return 0;
  }



  int TELNET_simulator::send(std::string *msg) {
	return -1;
  }



  int TELNET_simulator::recv(std::string *msg, int buff_size) {
	return -1;
  }



  int TELNET_simulator::close() {
	return 0;
  }



  int TELNET_simulator::fd() {
	return 0;
  }
}

/* EOF TNRC_PLUGIN_TELNET_CXX */
