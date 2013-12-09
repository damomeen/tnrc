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

#ifndef TNRC_PLUGIN_TELNET_CLS_H
#define TNRC_PLUGIN_TELNET_CLS_H

#include "tnrc_sp.h"
#include "tnrc_plugin_telnet.h"

namespace TNRC_SP_TELNET {

  class xc_cls_params;
  class PluginClsFSC;


  /*
   * Infomation about port parameters unretrivable from CLS
   */
  typedef struct {
	int flags; // flags
	g2mpls_addr_t rem_eq_addr; // remote address
	port_id_t rem_port_id; // remote port id
	uint32_t bandwidth; // bandwidth
	gmpls_prottype_t protection; // protection type
	bool added_to_DM; // added to the AP Data model
	bool managed; // port under TNRC control (TODO...)
  } tnrcsp_telnet_fixed_port_t;


  /*
   * List of configured boards
   */
  typedef std::map<const uint32_t, bool> tnrcsp_telnet_map_fixed_board_t;

  /*
   * List of configured ports
   */
  typedef std::map<const uint32_t, tnrcsp_telnet_fixed_port_t> tnrcsp_telnet_map_fixed_port_t;


  /*
   * State of cross connection
   */
  typedef enum {
	TNRCSP_CLS_STATE_INIT, // Init state
	TNRCSP_CLS_STATE_SET, // Waits for 'set xc' response
	TNRCSP_CLS_STATE_CHECK, // Waits for 'show xc' response
	TNRCSP_CLS_STATE_CLEAR, // Waits for 'clear xc' response
  } tnrcsp_cls_xc_state;


  /*
   * Utilities
   */
  tnrc_portid_t cls_compose_id(int eqptId, int boardId, int portId);
  int cls_decompose_id(tnrc_portid_t port_id, int *device, int *port);
  int cls_check_ports(tnrc_portid_t port_in, tnrc_portid_t port_out);
  tnrcsp_result_t cls_check_response(
									 std::string *data,
									 char *needle,
									 tnrcsp_result_t positive,
									 tnrcsp_result_t negative);


  /*
   *******************************************************************************
   *  Class PluginClsFSC_listen
   *  Listen and process received TELNET responses from CLS
   *******************************************************************************
   */
  class PluginClsFSC_listen : public PluginTelnet_listen {
  public:
	PluginClsFSC_listen();
	PluginClsFSC_listen(PluginClsFSC *owner);
	~PluginClsFSC_listen();

	void check_func_waittime();

  protected:
	void do_response(tnrc_sp_handle_item *item, std::string *response);
	void save_equipments(tnrc_sp_handle_item *item, std::string *response);

  private:
	PluginClsFSC *pluginClsFsc; // Pointer to TNRC SP
  };


  /*
   *******************************************************************************
   * Class PluginClsFSC
   * Send xc messages and communicate with a real CLS device
   *******************************************************************************
   */
  class PluginClsFSC : public TNRC_SP_TELNET::PluginTelnet {
	friend class PluginClsFSC_listen;

  public:
	tnrcsp_telnet_map_fixed_port_t fixed_port_map;
	tnrcsp_telnet_map_fixed_board_t fixed_board_map;

	PluginClsFSC(std::string name);
	~PluginClsFSC();

	void get_device_info(void);
	void tnrcsp_set_fixed_port(
							uint32_t key,
							int flags,
							g2mpls_addr_t rem_eq_addr,
							port_id_t rem_port_id,
							uint32_t bandwidth,
							gmpls_prottype_t protection,
							bool managed);
	void trncsp_cls_xc_process(
							tnrcsp_cls_xc_state state,
							xc_cls_params *parameters,
							tnrcsp_result_t result = TNRCSP_RESULT_NOERROR,
							std::string *data = NULL);

	wq_item_status wq_function(void *d);
	void del_item_data(void *d);

	tnrcapiErrorCode_t probe(std::string location);

	tnrcsp_result_t tnrcsp_make_xc(
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
								void *async_ctxt);

	tnrcsp_result_t tnrcsp_destroy_xc(
									tnrcsp_handle_t *handlep,
									tnrc_port_id_t portid_in,
									label_t labelid_in,
									tnrc_port_id_t portid_out,
									label_t labelid_out,
									xcdirection_t direction,
									tnrc_boolean_t isvirtual,
									tnrc_boolean_t deactivate,
									tnrcsp_response_cb_t response_cb,
									void *response_ctxt);

	tnrcsp_result_t tnrcsp_reserve_xc(
									tnrcsp_handle_t *handlep,
									tnrc_port_id_t portid_in,
									label_t labelid_in,
									tnrc_port_id_t portid_out,
									label_t labelid_out,
									xcdirection_t direction,
                                                                        tnrc_boolean_t isvirtual,
									tnrcsp_response_cb_t response_cb,
									void *response_ctxt);

	tnrcsp_result_t tnrcsp_unreserve_xc(
										tnrcsp_handle_t *handlep,
										tnrc_port_id_t portid_in,
										label_t labelid_in,
										tnrc_port_id_t portid_out,
										label_t labelid_out,
										xcdirection_t direction,
                                                                                tnrc_boolean_t isvirtual,
										tnrcsp_response_cb_t response_cb,
										void *response_ctxt);

	tnrcsp_result_t tnrcsp_register_async_cb(tnrcsp_event_t *events);

  protected:
	// AP data model parameters
	eqpt_id_t data_model_id; // Equipment ID FIXIT now is equal to 1
	g2mpls_addr_t data_model_address; // Equipment address displayed in data model
	eqpt_type_t data_model_type; // Equipment type - FSC or LSC
	opstate_t data_model_opstate; // Equipment state - UP or DOWN
	admstate_t data_model_admstate; // Equipment administrative state dispalayed in data model
	std::string data_model_location; // Equipment location displayed in data model

  private:
	bool trncsp_cls_make_xc(
							tnrcsp_cls_xc_state state,
							xc_cls_params *parameters,
							tnrcsp_result_t response,
							std::string *data,
							tnrcsp_result_t *result);

	bool trncsp_cls_destroy_xc(
							tnrcsp_cls_xc_state state,
							xc_cls_params *parameters,
							tnrcsp_result_t response,
							std::string *data,
							tnrcsp_result_t *result);

	bool trncsp_cls_reserve_xc(
							tnrcsp_cls_xc_state state,
							xc_cls_params *parameters,
							tnrcsp_result_t response,
							std::string *data,
							tnrcsp_result_t *result);

	bool trncsp_cls_unreserve_xc(
								tnrcsp_cls_xc_state state,
								xc_cls_params *parameters,
								tnrcsp_result_t response,
								std::string *data,
								tnrcsp_result_t *result);

	tnrcsp_result_t cls_cmd_xc_set(
								tnrcsp_cls_xc_state state,
								xc_cls_params *parameters,
								int device,
								int ingress,
								int egress);

	tnrcsp_result_t cls_cmd_xc_clear(
									tnrcsp_cls_xc_state state,
									xc_cls_params *parameters,
									int device,
									int ingress,
									int egress);

	tnrcsp_result_t cls_cmd_xc_show(
									tnrcsp_cls_xc_state state,
									xc_cls_params *parameters,
									int device);

	int CheckPort(tnrc_port_id_t portId); // Check if portId is registered in DM
  };


  /*
   *******************************************************************************
   * Class xc_cls_params
   * Storage for xc parameters - used in a state machine
   *******************************************************************************
   */
  class xc_cls_params {
  public:
	trncsp_xc_activity activity; // type of xconn operaration

	// TNRC SP API xconn operation parameters
	tnrcsp_handle_t handle; // handle of xconn operation
	tnrc_portid_t port_in; // ingress port id
	tnrc_portid_t port_out; // egress port id
	xcdirection_t direction; // directionality of xconn
	tnrc_boolean_t _virtual; // allocate existing xconn
	tnrc_boolean_t activate; // xconn activation
	tnrcsp_response_cb_t response_cb; // pseudo-synchronous callback function
	void *response_ctx; // pseudo-synchronous callback function context
	tnrcsp_notification_cb_t async_cb; // asynchronous callback function
	void *async_cxt; // asynchronous callback function context

	// Additional parameters
	tnrc_boolean_t deactivate; // xconn deactivation
	int device; // cls device ID
	int ingress; // ingress cls port identifier
	int egress; // egress cls port identifier

	// Parameters important for xconn operation
	bool activated; // false if xconn was only reserved otherwise true
	int repeat_count; // NOT USED: state counter
	bool final_result; //
	tnrcsp_result_t result; //

	xc_cls_params(
				trncsp_xc_activity activity,
				tnrcsp_handle_t handle,
				tnrc_portid_t port_in,
				tnrc_portid_t port_out,
				xcdirection_t direction,
				tnrc_boolean_t _virtual,
				tnrc_boolean_t activate,
				tnrcsp_response_cb_t response_cb,
				void *response_ctx,
				tnrcsp_notification_cb_t async_cb,
				void *async_cxt);

	xc_cls_params();
	~xc_cls_params();
  };

  int telnet_retrive_func(struct thread *thread);
}

#endif /* TNRC_PLUGIN_TELNET_CLS_H */
