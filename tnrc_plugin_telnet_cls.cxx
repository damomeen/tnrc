/**
 * TNRC CLS Plugin implementation.
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
#include "tnrc_sp.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_plugin_telnet_cls.h"


namespace TNRC_SP_TELNET {

  /*
   *******************************************************************************
   * Utilities
   *******************************************************************************
   */



  /*
   * Compose port_id from device and real port
   */
  tnrc_portid_t cls_compose_id(int eqptId, int boardId, int portId) {
	return (0x0 |
			((eqptId & EQPTID_TO_DLID_MASK) << EQPTID_SHIFT) |
			((boardId & BOARDID_TO_DLID_MASK) << BOARDID_SHIFT) |
			((portId & PORTID_TO_DLID_MASK) << 0)
			);
  }



  /*
   * Decompose device and port from port_id
   */
  int cls_decompose_id(tnrc_portid_t port_id, int *device, int *port) {
	int portId = (port_id & DLID_TO_PORTID_MASK) >> 0;
	int boardId = (port_id & DLID_TO_BOARDID_MASK) >> BOARDID_SHIFT;
	int eqptId = (port_id & DLID_TO_EQPTID_MASK) >> EQPTID_SHIFT;

	if (portId != 0 && boardId != 0 && eqptId != 0) {
	  *device = boardId;
	  *port = portId;
	  return 0;
	} else {
	  TNRC_WRN("%s %d: port_id [%d] cannot be decomposed!", __FILE__, __LINE__, port_id);
	  *device = 0;
	  *port = 0;
	  return 1;
	}
  }



  /*
   * Check if ports are 'decomposable' and have the same board (device) idetification
   */
  int cls_check_ports(tnrc_portid_t port_in, tnrc_portid_t port_out) {
	int device1, port1, device2, port2;
	if (cls_decompose_id(port_in, &device1, &port1) != 0)
	  return 1;
	if (cls_decompose_id(port_out, &device2, &port2) != 0)
	  return 2;
	if (device1 != device2)
	  return 3;
	if (port1 == port2)
	  return 4;
	return 0;
  }



  /*
   * Find needle in the data, on success return positive code otherwise negative code
   */
  tnrcsp_result_t cls_check_response(std::string *data, char *needle, tnrcsp_result_t positive, tnrcsp_result_t negative) {
	if (!data)
	  return TNRCSP_RESULT_INTERNALERROR;
	if (data->find(needle) != -1)
	  return positive;
	return negative;
  }



  /*
   *******************************************************************************
   * PluginClsFSC class
   * Main class for manipulation with CzechLight devices
   *******************************************************************************
   */

  PluginClsFSC::PluginClsFSC(std::string name) : PluginTelnet(name) {
	// Create a new listen process
	listen_proc = new PluginClsFSC_listen(this);

	// Set command prompt for CzechLight devices
	std::string prompt("> ");
	listen_proc->command_prompt_set(&prompt);
  }



  PluginClsFSC::~PluginClsFSC(void) {
	telnet_close();
  }



  /*
   * Call periodically get_device_info for getting information about real ports
   */
  void PluginClsFSC::get_device_info(void) {
	TNRC_DBG("%s %d: get_device_info() called", __FILE__, __LINE__);

	std::string cmd("show device");
	asynch_send(&cmd);
  }



  /*
   * Set parameters which are not retrievable from real device
   */
  void PluginClsFSC::tnrcsp_set_fixed_port(
										   uint32_t key,
										   int flags,
										   g2mpls_addr_t rem_eq_addr,
										   port_id_t rem_port_id,
										   uint32_t bandwidth,
										   gmpls_prottype_t protection,
										   bool managed) {



	fixed_port_map[key] = ((tnrcsp_telnet_fixed_port_t) {
						   flags, rem_eq_addr, rem_port_id, bandwidth, protection, false, managed
	});
  }



  /*
   * Work queue is not used!
   */
  wq_item_status PluginClsFSC::wq_function(void *d) {
	TNRC_ERR("%s %d: wq_function() function should be never executed", __FILE__, __LINE__);
	return WQ_SUCCESS;
  }



  void PluginClsFSC::del_item_data(void *d) {
	TNRC_ERR("%s %d: del_item_data() function should be never executed", __FILE__, __LINE__);
	tnrcsp_action_t *data;
	//get work item data
	data = (tnrcsp_action_t *) d;
	//delete data structure
	delete data;
  }



  /*
   * Cross-connect making state machine
   */
  void PluginClsFSC::trncsp_cls_xc_process(tnrcsp_cls_xc_state state, xc_cls_params *parameters, tnrcsp_result_t response, std::string* data) {
	tnrcsp_result_t result;
	bool call_callback = false;

	TNRC_DBG("%s %d: cls_xc_process() activity=%d, state=%d, portIn=%d, portOut=%d", __FILE__, __LINE__, parameters->activity, state, parameters->ingress, parameters->egress);
	if (parameters->activity == TNRCSP_XC_MAKE) {
	  if (call_callback = trncsp_cls_make_xc(state, parameters, response, data, &result)) {
		if (result == TNRCSP_RESULT_NOERROR) {
		  parameters->activated = true;
		}
		//        else if(result != TNRCSP_RESULT_BUSYRESOURCES) {
		//          call_callback = false;
		//          parameters->final_result = true;
		//          parameters->result = result;
		//          TNRC_WRN("Make unsuccessful -> destroying resources");
		//          parameters->activity = TNRCSP_XC_DESTROY;
		//          trncsp_cls_destroy_xc(TNRCSP_CLS_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
		//        }
	  }
	} else if (parameters->activity == TNRCSP_XC_DESTROY) {
	  if (call_callback = trncsp_cls_destroy_xc(state, parameters, response, data, &result)) {
		parameters->activated = false;

		if (!parameters->deactivate) {
		  parameters->activity = TNRCSP_XC_UNRESERVE;
		  trncsp_cls_unreserve_xc(TNRCSP_CLS_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
		  call_callback = false;
		}
	  }
	} else if (parameters->activity == TNRCSP_XC_RESERVE) {
	  if (call_callback = trncsp_cls_reserve_xc(state, parameters, response, data, &result)) {
		if (result == TNRCSP_RESULT_NOERROR) {
		  parameters->activated = false;
		}
		//      else if(result != TNRCSP_RESULT_BUSYRESOURCES) {
		//        call_callback = false;
		//        parameters->final_result = true;
		//        parameters->result = result;
		//        TNRC_WRN("reservation unsuccessful -> unreserve resources");
		//        parameters->activity = TNRCSP_XC_UNRESERVE;
		//        call_callback = trncsp_cls_unreserve_xc(TNRCSP_CLS_STATE_INIT, parameters, TNRCSP_RESULT_NOERROR, NULL, &result);
		//      }

	  }
	} else if (parameters->activity == TNRCSP_XC_UNRESERVE) {
	  if (call_callback = trncsp_cls_unreserve_xc(state, parameters, response, data, &result))
		parameters->activated = false;
	}

	// Call callback function - call only when occurs a final or an error state
	if (call_callback) {
	  if (parameters->response_cb) {
		if (parameters->final_result == false)
		  parameters->result = result;
		TNRC_DBG("%s %d: Calling a sync callback with the result: %s", __FILE__, __LINE__, SHOW_TNRCSP_RESULT(parameters->result));
		parameters->response_cb(parameters->handle, parameters->result, parameters->response_ctx);
	  }
	  delete parameters;
	}
  }



  /*
   * Make XC function
   */
  bool PluginClsFSC::trncsp_cls_make_xc(tnrcsp_cls_xc_state state,
										xc_cls_params *parameters,
										tnrcsp_result_t response,
										std::string *data,
										tnrcsp_result_t *result) {
	TNRC_DBG("%s %d: Make state: %i", __FILE__, __LINE__, state);

	if (state == TNRCSP_CLS_STATE_INIT) {
	  if (!parameters->activate) {
		*result = cls_cmd_xc_set(TNRCSP_CLS_STATE_SET, parameters, parameters->device, parameters->ingress, parameters->egress);
		if (*result != TNRCSP_RESULT_NOERROR)
		  return true;
	  } else {
		*result = cls_cmd_xc_show(TNRCSP_CLS_STATE_CHECK, parameters, parameters->device);
		if (*result != TNRCSP_RESULT_NOERROR)
		  return true;
	  }
	} else if (state == TNRCSP_CLS_STATE_SET) {
	  *result = cls_check_response(data, "Error: Port is already mapped", TNRCSP_RESULT_BUSYRESOURCES, response);
	  if (*result != TNRCSP_RESULT_NOERROR)
		return true;
	  *result = cls_cmd_xc_show(TNRCSP_CLS_STATE_CHECK, parameters, parameters->device);
	  if (*result != TNRCSP_RESULT_NOERROR)
		return true;
	} else if (state == TNRCSP_CLS_STATE_CHECK) {
	  char buf[256];
	  sprintf(buf, "%d=%d", parameters->ingress, parameters->egress);
	  *result = cls_check_response(data, buf, response, TNRCSP_RESULT_INTERNALERROR);
	  return true;
	} else { /* unknown state */
	  *result = TNRCSP_RESULT_INTERNALERROR;
	  return true;
	}
	return false;
  }



  /*
   * Destroy XC function
   */
  bool PluginClsFSC::trncsp_cls_destroy_xc(tnrcsp_cls_xc_state state,
										   xc_cls_params *parameters,
										   tnrcsp_result_t response,
										   std::string *data,
										   tnrcsp_result_t *result) {
	TNRC_DBG("%s %d: Destroy state: %i", __FILE__, __LINE__, state);
	return true; /* CLS hasn't any method how to destroy mapping, unreserve do exact unmapping */
  }



  /*
   * Reserve XC function
   */
  bool PluginClsFSC::trncsp_cls_reserve_xc(tnrcsp_cls_xc_state state,
										   xc_cls_params *parameters,
										   tnrcsp_result_t response,
										   std::string *data,
										   tnrcsp_result_t *result) {
	TNRC_DBG("%s %d: Reserve state: %i", __FILE__, __LINE__, state);

	if (state == TNRCSP_CLS_STATE_INIT) {
	  *result = cls_cmd_xc_set(TNRCSP_CLS_STATE_SET, parameters, parameters->device, parameters->ingress, parameters->egress);
	  if (*result != TNRCSP_RESULT_NOERROR)
		return true;
	} else if (state == TNRCSP_CLS_STATE_SET) {
	  *result = cls_check_response(data, "Error: Port is already mapped", TNRCSP_RESULT_BUSYRESOURCES, response);
	  if (*result != TNRCSP_RESULT_NOERROR)
		return true;
	  *result = cls_cmd_xc_show(TNRCSP_CLS_STATE_CHECK, parameters, parameters->device);
	  if (*result != TNRCSP_RESULT_NOERROR)
		return true;
	} else if (state == TNRCSP_CLS_STATE_CHECK) {
	  char buf[256];
	  sprintf(buf, "%d=%d", parameters->ingress, parameters->egress);
	  *result = cls_check_response(data, buf, response, TNRCSP_RESULT_INTERNALERROR);
	  return true;
	} else { /* unknown state */
	  *result = TNRCSP_RESULT_INTERNALERROR;
	  return true;
	}
	return false;
  }



  /*
   * Unreserve XC function
   */
  bool PluginClsFSC::trncsp_cls_unreserve_xc(tnrcsp_cls_xc_state state,
											 xc_cls_params *parameters,
											 tnrcsp_result_t response,
											 std::string *data,
											 tnrcsp_result_t *result) {
	TNRC_DBG("%s %d: Unreserve state: %i", __FILE__, __LINE__, state);
	if (state == TNRCSP_CLS_STATE_INIT) {
	  *result = cls_cmd_xc_clear(TNRCSP_CLS_STATE_CLEAR, parameters, parameters->device, parameters->ingress, parameters->egress);
	  if (*result != TNRCSP_RESULT_NOERROR)
		return true;
	} else if (state == TNRCSP_CLS_STATE_CLEAR) {
	  *result = cls_cmd_xc_show(TNRCSP_CLS_STATE_CHECK, parameters, parameters->device);
	  if (*result != TNRCSP_RESULT_NOERROR)
		return true;
	} else if (state == TNRCSP_CLS_STATE_CHECK) {
	  char buf[256];
	  sprintf(buf, "%d=%d", parameters->ingress, parameters->egress);
	  *result = cls_check_response(data, buf, TNRCSP_RESULT_INTERNALERROR, response);
	  return true;
	} else { /* unknown state */
	  *result = TNRCSP_RESULT_INTERNALERROR;
	  return true;
	}
	return false;
  }



  /*
   * Run 'set xc' command on a real device -> make port mapping
   */
  tnrcsp_result_t PluginClsFSC::cls_cmd_xc_set(tnrcsp_cls_xc_state state, xc_cls_params *parameters, int device, int ingress, int egress) {
	int ctag = gen_identifier();
	char buff[256];

	sprintf(buff, "set xc %d %d=%d", device, ingress, egress);
	std::string cmd(buff);
	return asynch_send(&cmd, state, parameters);
  }



  /*
   * Run 'clear xc' command on a real device -> destroy port mapping
   */
  tnrcsp_result_t PluginClsFSC::cls_cmd_xc_clear(tnrcsp_cls_xc_state state, xc_cls_params *parameters, int device, int ingress, int egress) {
	int ctag = gen_identifier();
	char buff[256];

	sprintf(buff, "clear xc %d %d=%d", device, ingress, egress);
	std::string cmd(buff);
	return asynch_send(&cmd, state, parameters);
  }



  /*
   * Run 'show xc' command on a real device -> show current port mapping
   */
  tnrcsp_result_t PluginClsFSC::cls_cmd_xc_show(tnrcsp_cls_xc_state state, xc_cls_params *parameters, int device) {
	int ctag = gen_identifier();
	char buff[256];

	sprintf(buff, "show xc %d", device);
	std::string cmd(buff);
	return asynch_send(&cmd, state, parameters);
  }



  /*
   * Probe - connect to the device and regiter them in DM
   * location string: <IP_address>[:<port>][:<config_path>]
   */
  tnrcapiErrorCode_t PluginClsFSC::probe(std::string location) {
	std::string strAddress = "127.0.0.1"; // default value - loopback
	std::string strPort = "23"; // telnet port
	std::string strPath = ""; // no extra configuration

	TNRC::TNRC_AP * t;
	std::string err;
	tnrcapiErrorCode_t rc;
	in_addr remote_address;
	int remote_port;
	std::string tmp;
	std::string separators(":");
	int stPos;
	int endPos;
	int len;

	// Check if a TNRC instance is running. If not, create a new one
	t = TNRC_MASTER.getTNRC();
	if (t == NULL)
	  TNRC_CONF_API::tnrcStart(err); //create a new instance

	len = location.length();
	if (len) {
	  stPos = 0;
	  endPos = location.find_first_of(separators);
	  if (endPos == -1)
		endPos = len;
	  if (endPos - stPos > 0) {
		tmp = location.substr(stPos, endPos - stPos);
		if (tmp.length() != 0)
		  strAddress = tmp;
	  }

	  stPos = endPos + 1;
	  endPos = location.find_first_of(separators, stPos);
	  if (endPos == -1)
		endPos = len;
	  if (endPos - stPos > 0) {
		tmp = location.substr(stPos, endPos - stPos);
		if (tmp.length() != 0)
		  strPort = tmp;
	  }

	  stPos = endPos + 1;
	  endPos = location.find_first_of(separators, stPos);
	  if (endPos == -1)
		endPos = len;
	  if (endPos - stPos > 0) {
		tmp = location.substr(stPos, endPos - stPos);
		if (tmp.length() != 0)
		  strPath = tmp;
	  }
	}

	TNRC_DBG("%s %d: Called probe (address:%s, port:%s, path:%s)", __FILE__, __LINE__, strAddress.c_str(), strPort.c_str(), strPath.c_str());

	if (inet_aton(strAddress.c_str(), &remote_address) != 1) {
	  TNRC_ERR("%s %d: Wrong IP address [%s]", __FILE__, __LINE__, strAddress.c_str());
	  return TNRC_API_ERROR_GENERIC;
	}

	if (sscanf(strPort.c_str(), "%d", &remote_port) != 1 || remote_port == 0) {
	  TNRC_ERR("%s %d: Wrong port [%s]", __FILE__, __LINE__, strPort.c_str());
	  return TNRC_API_ERROR_GENERIC;
	}

	///AP Data model support
	data_model_id = 1;
	inet_aton(strAddress.c_str(), &data_model_address.value.ipv4);
	data_model_type = EQPT_CLS_FSC;
	data_model_opstate = UP;
	data_model_admstate = ENABLED;
	data_model_location = location;

	///adding equipment to the AP Data model
	TNRC_DBG("%s %d: Adding equipment to AP Data model", __FILE__, __LINE__);
	rc = TNRC_CONF_API::add_Eqpt(
								data_model_id,
								data_model_address,
								data_model_type,
								data_model_opstate,
								data_model_admstate,
								data_model_location);
	if (rc != TNRC_API_ERROR_NONE)
	  return rc;

	if (strPath.length() > 0) {
	  TNRC_INF("%s %d: Loading configuration file [%s]", __FILE__, __LINE__, strPath.c_str());
	  vty_read_config((char *) strPath.c_str(), NULL);
	}

	TELNET_status = TNRC_SP_TELNET_STATE_NO_SESSION;
	sock = new TELNET_socket(remote_address, remote_port);

	TNRC_INF("%s %d: Session data: IP=%s, port=%d", __FILE__, __LINE__, inet_ntoa(remote_address), remote_port);

	// Run new thread for retriving data
	retriveT(thread_add_timer(master, telnet_retrive_func, this, 0));

	return TNRC_API_ERROR_NONE;
  }



  /*
   * CLS SP API make function
   */
  tnrcsp_result_t PluginClsFSC::tnrcsp_make_xc(
											   tnrcsp_handle_t *handlep,
											   tnrc_port_id_t portid_in,
											   label_t labelid_in,
											   tnrc_port_id_t portid_out,
											   label_t labelid_out,
											   xcdirection_t direction,
											   tnrc_boolean_t isvirtual,
											   tnrc_boolean_t activate,
											   tnrcsp_response_cb_t response_cb,
											   void *response_ctxt,
											   tnrcsp_notification_cb_t async_cb,
											   void *async_ctxt) {
	*handlep = gen_identifier();
	TNRC_DBG("%s %d: ***MAKE XC***", __FILE__, __LINE__);
	if (cls_check_ports(portid_in, portid_out) != 0)
	  return TNRCSP_RESULT_PARAMERROR;

	if (TELNET_status != TNRC_SP_TELNET_STATE_CONNECTED)
	  return TNRCSP_RESULT_EQPTLINKDOWN;

	xc_cls_params *xc_params = new xc_cls_params(TNRCSP_XC_MAKE, *handlep,
												portid_in, portid_out,
												direction, isvirtual, activate,
												response_cb, response_ctxt, async_cb, async_ctxt);

	trncsp_cls_xc_process(TNRCSP_CLS_STATE_INIT, xc_params);

	return TNRCSP_RESULT_NOERROR;
  }



  /*
   * CLS SP API destroy function
   */
  tnrcsp_result_t PluginClsFSC::tnrcsp_destroy_xc(
												  tnrcsp_handle_t *handlep,
												  tnrc_port_id_t portid_in,
												  label_t labelid_in,
												  tnrc_port_id_t portid_out,
												  label_t labelid_out,
												  xcdirection_t direction,
												  tnrc_boolean_t isvirtual,
												  tnrc_boolean_t deactivate,
												  tnrcsp_response_cb_t response_cb,
												  void *response_ctxt) {
	*handlep = gen_identifier();
	TNRC_DBG("%s %d: ***DESTROY XC***", __FILE__, __LINE__);
	if (cls_check_ports(portid_in, portid_out) != 0)
	  return TNRCSP_RESULT_PARAMERROR;

	if (TELNET_status != TNRC_SP_TELNET_STATE_CONNECTED)
	  return TNRCSP_RESULT_EQPTLINKDOWN;

	xc_cls_params *xc_params = new xc_cls_params(TNRCSP_XC_DESTROY, *handlep,
												portid_in, portid_out,
												direction, isvirtual, deactivate,
												response_cb, response_ctxt, NULL, NULL);

	trncsp_cls_xc_process(TNRCSP_CLS_STATE_INIT, xc_params);

	return TNRCSP_RESULT_NOERROR;
  }



  /*
   * CLS SP API reserve function
   */
  tnrcsp_result_t PluginClsFSC::tnrcsp_reserve_xc(
												  tnrcsp_handle_t *handlep,
												  tnrc_port_id_t portid_in,
												  label_t labelid_in,
												  tnrc_port_id_t portid_out,
												  label_t labelid_out,
												  xcdirection_t direction,
                                                                                                  tnrc_boolean_t isvirtual,
												  tnrcsp_response_cb_t response_cb,
												  void *response_ctxt) {
	*handlep = gen_identifier();
	TNRC_DBG("%s %d: ***RESERVE XC***", __FILE__, __LINE__);
	if (cls_check_ports(portid_in, portid_out) != 0)
	  return TNRCSP_RESULT_PARAMERROR;

	if (TELNET_status != TNRC_SP_TELNET_STATE_CONNECTED)
	  return TNRCSP_RESULT_EQPTLINKDOWN;

	xc_cls_params *xc_params = new xc_cls_params(TNRCSP_XC_RESERVE, *handlep,
												portid_in, portid_out,
												direction, 0, false,
												response_cb, response_ctxt, NULL, NULL);

	trncsp_cls_xc_process(TNRCSP_CLS_STATE_INIT, xc_params);

	return TNRCSP_RESULT_NOERROR;
  }



  /*
   * CLS SP API unreserve function
   */
  tnrcsp_result_t PluginClsFSC::tnrcsp_unreserve_xc(
													tnrcsp_handle_t *handlep,
													tnrc_port_id_t portid_in,
													label_t labelid_in,
													tnrc_port_id_t portid_out,
													label_t labelid_out,
													xcdirection_t direction,
tnrc_boolean_t isvirtual,
													tnrcsp_response_cb_t response_cb,
													void *response_ctxt) {
	*handlep = gen_identifier();
	TNRC_DBG("%s %d: ***UNRESERVE XC***", __FILE__, __LINE__);
	if (cls_check_ports(portid_in, portid_out) != 0)
	  return TNRCSP_RESULT_PARAMERROR;

	if (TELNET_status != TNRC_SP_TELNET_STATE_CONNECTED)
	  return TNRCSP_RESULT_EQPTLINKDOWN;

	xc_cls_params *xc_params = new xc_cls_params(TNRCSP_XC_UNRESERVE, *handlep,
												portid_in, portid_out,
												direction, 0, false,
												response_cb, response_ctxt, NULL, NULL);

	trncsp_cls_xc_process(TNRCSP_CLS_STATE_INIT, xc_params);

	return TNRCSP_RESULT_NOERROR;
  }



  /*
   * CLS SP API asynch function - CLS doesn't support asynchronous actions
   */
  tnrcsp_result_t PluginClsFSC::tnrcsp_register_async_cb(tnrcsp_event_t *events) {
	return TNRCSP_RESULT_NOERROR;
  }



  /*
   *******************************************************************************
   * Class PluginClsFSC_listen
   * Used for manipulation with a real device responses
   *******************************************************************************
   */

  PluginClsFSC_listen::PluginClsFSC_listen() : PluginTelnet_listen() {
	
  }



  PluginClsFSC_listen::PluginClsFSC_listen(PluginClsFSC *owner) : PluginTelnet_listen() {
	plugin = owner;
	pluginClsFsc = owner;
  }



  PluginClsFSC_listen::~PluginClsFSC_listen() {
	
  }



  /*
   * Response dispatcher
   */
  void PluginClsFSC_listen::do_response(tnrc_sp_handle_item *item, std::string *response) {
	if (item->command.find("show device") == 0) {
	  save_equipments(item, response);
	} else if (item->command.find("show port") == 0) {
	  TNRC_INF("%s %d: Doing response for show port", __FILE__, __LINE__); // NOT USED
	} else if (item->parameters != NULL) {
	  // CALL state machine func
	  pluginClsFsc->trncsp_cls_xc_process((tnrcsp_cls_xc_state) item->state, (xc_cls_params *) item->parameters, TNRCSP_RESULT_NOERROR, response);
	} else
	  TNRC_WRN("%s %d: Unknown command response!", __FILE__, __LINE__);
  }



  /*
   * Save equipment in to the data model
   */
  void PluginClsFSC_listen::save_equipments(tnrc_sp_handle_item *item, std::string *response) {
	std::string line = "";
	int eqptId = 1;
	int boardId;
	int portId;
	int inputs;
	int outputs;
	int in_type;
	int out_type;
	int port_num;
	int i;

	while (response->length() > 0) {
	  line = pop_line(response);
	  if (sscanf(line.c_str(), "%d %d %d %d %d %*s", &boardId, &inputs, &outputs, &in_type, &out_type) == 5) {
		tnrcsp_telnet_map_fixed_board_t::iterator it;
		it = pluginClsFsc->fixed_board_map.find(boardId);
		if (it == pluginClsFsc->fixed_board_map.end()) {
		  pluginClsFsc->fixed_board_map[boardId] = true;
		  TNRC_CONF_API::add_Board(pluginClsFsc->data_model_id, boardId, SWCAP_FSC, ENCT_FIBER, UP, ENABLED);
		}

		port_num = (inputs > outputs) ? inputs : outputs;
		for (i = 1; i <= port_num; i++) {
		  portId = cls_compose_id(eqptId, boardId, i);

		  tnrcsp_telnet_map_fixed_port_t::iterator it;
		  it = pluginClsFsc->fixed_port_map.find(portId);
		  if (it != pluginClsFsc->fixed_port_map.end()) {
			if (it->second.added_to_DM == false) {
			  if (TNRC_CONF_API::add_Port(
										  pluginClsFsc->data_model_id,
										  boardId,
										  portId,
										  it->second.flags,
										  it->second.rem_eq_addr,
										  it->second.rem_port_id,
										  UP,
										  ENABLED,
										  0,
										  0,
										  it->second.bandwidth,
										  it->second.protection) == TNRC_API_ERROR_NONE)
				it->second.added_to_DM = true;
			}
		  } else {
			TNRC_WRN("%s %d: Port 0x%x is not configured", __FILE__, __LINE__, portId);
		  }
		}
	  }
	}
  }



  /*
   * Check for command timeout
   * If asynch function call waits more then 120 sec, release all requests waiting for response
   */
  void PluginClsFSC_listen::check_func_waittime() {
	std::map<const int, tnrc_sp_handle_item>::iterator it;
	for (it = handlers.begin(); it != handlers.end(); it++) {
	  if (time(NULL) - it->second.start > 120) {
		TNRC_WRN("%s %d: Command [%s] timeouted!", __FILE__, __LINE__, it->second.command.c_str());

		tnrcsp_cls_xc_state state = (tnrcsp_cls_xc_state) it->second.state;
		xc_cls_params *parameters = (xc_cls_params*) it->second.parameters;
		if (parameters != NULL)
		  pluginClsFsc->trncsp_cls_xc_process(state, parameters, TNRCSP_RESULT_GENERICERROR); // free parameters
		handlers.erase(it);
	  }
	}
  }



  /*
   *******************************************************************************
   * Class xc_cls_params
   * Used for manipulation with cross-connect parameters (state machine)
   *******************************************************************************
   */
  xc_cls_params::xc_cls_params(trncsp_xc_activity activity, tnrcsp_handle_t handle,
							   tnrc_portid_t port_in, tnrc_portid_t port_out,
							   xcdirection_t direction, tnrc_boolean_t _virtual,
							   tnrc_boolean_t activate, tnrcsp_response_cb_t response_cb,
							   void* response_ctx, tnrcsp_notification_cb_t async_cb, void* async_cxt) {
	this->activity = activity;
	this->handle = handle;
	this->port_in = port_in;
	this->port_out = port_out;
	this->direction = direction;
	this->_virtual = _virtual;
	this->activate = activate;
	this->response_cb = response_cb;
	this->response_ctx = response_ctx;
	this->async_cb = async_cb;
	this->async_cxt = async_cxt;
	this->deactivate = activate;
	this->activated = false;
	this->repeat_count = 0;
	this->final_result = false;
	this->result = TNRCSP_RESULT_NOERROR;

	cls_decompose_id(port_in, &(this->device), &(this->ingress));
	cls_decompose_id(port_out, &(this->device), &(this->egress));
  }



  xc_cls_params::xc_cls_params() {
  }



  xc_cls_params::~xc_cls_params() {
  }

}

/* EOF TNRC_PLUGIN_TELNET_CLS_CXX */
