/*
 *  This file is part of phosphorus-g2mpls.
 *
 *  Copyright (C) 2006, 2007, 2008, 2009 PSNC
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Authors:
 *
 *  Adam Kaliszan         (PSNC)             <kaliszan_at_man.poznan.pl>
 *  Damian Parniewicz     (PSNC)             <damianp_at_man.poznan.pl>
 */



#ifndef TNRC_PLUGIN_ADVA_H
#define TNRC_PLUGIN_ADVA_H


#include "tnrc_sp.h"
#include "tnrc_plugin_tl1.h"
#include "tnrc_plugin_tl1_adva_simulator.h"
#include "vty.h"

#define DWDM_LAMBDA_BASE_ADVA  0x2900000B
#define DWDM_LAMBDA_COUNT_ADVA 40

namespace TNRC_SP_TL1
{
  class xc_adva_params;
  class PluginTl1;
  class PluginAdvaLSC;

  //=======================================================================================================================
  /** infomation about a crossconnection */
  typedef struct {
    std::string                 ingress_aid;        /// ingress port identifier
    std::string                 egress_aid;         /// egress port identifier
    std::string                 rev_ingress_aid;    /// ingress port identifier for second direction of bidirectional xconn
    std::string                 rev_egress_aid;     /// egress port identifier for second direction of bidirectional xconn
    tnrcsp_notification_cb_t    async_cb;           /// asynchronous callback function
    void*                       async_cxt;          /// asynchronous callback function context
    tnrcsp_handle_t             handle;             /// handle of make xconn operation
    bool                        activated;          /// false if xconn was only reserved otherwise true
  } tnrcsp_adva_xc_item;

  //========================================================================================================================
  /** crossconnection FSM state */
  typedef enum
  {
    TNRCSP_ADVA_STATE_INIT,          /// FSM init state
    TNRCSP_ADVA_STATE_ASSIGN_1,      /// FSM waits ASG-CHANNEL response
    TNRCSP_ADVA_STATE_ASSIGN_2,      /// FSM waits ASG-CHANNEL response for second direction of bidirectional xconn
    TNRCSP_ADVA_STATE_RESTORE_1,     /// FSM waits RST-CHANNEL response
    TNRCSP_ADVA_STATE_RESTORE_2,     /// FSM waits RST-CHANNEL response for second direction of bidirectional xconn
    TNRCSP_ADVA_STATE_REMOVE_1,      /// FSM waits RMV-CHANNEL response
    TNRCSP_ADVA_STATE_REMOVE_2,      /// FSM waits RMV-CHANNEL response for second direction of bidirectional xconn
    TNRCSP_ADVA_STATE_DELETE_1,      /// FSM waits DLT-CHANNEL response
    TNRCSP_ADVA_STATE_DELETE_2,      /// FSM waits DTL-CHANNEL response for second direction of bidirectional xconn
    TNRCSP_ADVA_STATE_OOS_1,         /// FSM waits Out-Of-Service xconn state
    TNRCSP_ADVA_STATE_OOS_2,         /// FSM waits Out-Of-Service xconn state for second direction of bidirectional xconn
    TNRCSP_ADVA_STATE_EQUALIZE_1,    /// FSM waits INIT-EQ response
    TNRCSP_ADVA_STATE_EQUALIZE_2,    /// FSM waits INIT-EQ response for second direction of bidirectional xconn 
    TNRCSP_ADVA_STATE_CHECK_1,       /// FSM waits RTRV-CHANNEL response
    TNRCSP_ADVA_STATE_CHECK_2,       /// FSM waits RTRV-CHANNEL response for second direction of bidirectional xconn
  } tnrcsp_adva_xc_state;

#define SHOW_TNRCSP_ADVA_XC_STATE(X)                         \
  (((X) == TNRCSP_ADVA_STATE_INIT       ) ? "INIT"         : \
  (((X) == TNRCSP_ADVA_STATE_ASSIGN_1   ) ? "ASSIGN_1"     : \
  (((X) == TNRCSP_ADVA_STATE_ASSIGN_2   ) ? "ASSIGN_2"     : \
  (((X) == TNRCSP_ADVA_STATE_RESTORE_1  ) ? "RESTORE_1"    : \
  (((X) == TNRCSP_ADVA_STATE_RESTORE_2  ) ? "RESTORE_2"    : \
  (((X) == TNRCSP_ADVA_STATE_REMOVE_1   ) ? "REMOVE_1"     : \
  (((X) == TNRCSP_ADVA_STATE_REMOVE_2   ) ? "REMOVE_2"     : \
  (((X) == TNRCSP_ADVA_STATE_DELETE_1   ) ? "DELETE_1"     : \
  (((X) == TNRCSP_ADVA_STATE_DELETE_2   ) ? "DELETE_2"     : \
  (((X) == TNRCSP_ADVA_STATE_OOS_1      ) ? "OOS_1"        : \
  (((X) == TNRCSP_ADVA_STATE_OOS_2      ) ? "OOS_2"        : \
  (((X) == TNRCSP_ADVA_STATE_EQUALIZE_1       ) ? "EQUALIZE_1"   : \
  (((X) == TNRCSP_ADVA_STATE_EQUALIZE_2       ) ? "EQUALIZE_2"   : \
  (((X) == TNRCSP_ADVA_STATE_CHECK_1    ) ? "CHECK_1"      : \
  (((X) == TNRCSP_ADVA_STATE_CHECK_2    ) ? "CHECK_2"      : \
             "==UNKNOWN==")))))))))))))))

  //=========================================================================================================================
  /**** ADVA UTILS ******/
  tnrc_portid_t        adva_compose_id(std::string* aid);
  bool                 adva_decompose_chaid(std::string* chaid, channel_t* channel);
  std::string*         adva_decompose_in(tnrc_portid_t port_id);
  std::string*         adva_decompose_out(tnrc_portid_t port_id);
  std::string*         adva_decompose_id(tnrc_portid_t port_id);
  channel_t            adva_label_to_channel(label_t label);
  label_t              adva_channel_to_label(channel_t channel);

  tnrcsp_lsc_eqtype_t  adva_get_port_type(tnrc_portid_t port_id);
  int                  adva_get_plane(tnrc_portid_t port_id);
  int                  adva_get_slot(tnrc_portid_t port_id);
  tnrcsp_lsp_xc_type   adva_check_xc_type(tnrc_portid_t port_in, tnrc_portid_t port_out);

  bool                 adva_check_channel(channel_t channel);
  bool                 adva_check_resources(tnrc_portid_t port_in, channel_t channel_in, tnrc_portid_t port_out, channel_t channel_out);

  bool                 adva_error_analyze(std::string* message, std::string description);
  tnrcsp_result_t      adva_error_analyze2(std::string* message, std::string description, tnrcsp_result_t response, tnrcsp_result_t default_response);
  char*                tnrcsp_adva_xc_type_desc(tnrcsp_lsp_xc_type type);
 

  //========================================================================================================================
  /** class PluginAdvaLSC_listen */
  class PluginAdvaLSC_listen : public PluginTl1_listen
  {
   public:
    PluginAdvaLSC_listen();
    PluginAdvaLSC_listen(PluginAdvaLSC *owner);
    ~PluginAdvaLSC_listen();

   private:
    PluginAdvaLSC *pluginAdvaLsc;       /// pointer to TNRC SP ADVA plugin module

    /** process successful responses */
    void process_successful(std::string* response_block, std::string* command, int ctag);

    /** process denied responses */
    void process_denied(std::string* response_block, std::string* command, int ctag);

    /** process TL1 autonomous message; information searching */
    void process_autonomous(std::string* mes, std::string* mes_code);

    /** process autonomous message with no alarm event */
    void process_event(std::string* alarm_code, std::string* mes);

    /** process autonomous message with alarm event */
    void process_alarm(std::string* alarm_code, std::string* mes);

    /** process incoming TL1 message from ADVA */
    void process_xc(int state, void* parameters, tnrcsp_result_t result, std::string* response_block); 

    /** add event to last_event map */
    void add_event(std::string aid, channel_t channel, std::string* event_type, std::string* cond_desc);

    /** store information with xconn details */
    void save_xcs(std::string response_block);

    /** delete unrecognized crossconnections listed on device */
    void destroy_alien_xcs();

    /** store information with equipments details */
    void save_equipments(std::string response_block);

    /** if synch function call wait more then 120 sec release all TL1 request waiting for response */
    void check_func_waittime();

    /** release all TL1 request waiting for response */
    void delete_func_waittime();
  };

  //========================================================================================================================
  /** TNRC Specyfic part Plugin class
  * Support Adva Lambda Switch
  */
  class PluginAdvaLSC : public TNRC_SP_TL1::PluginTl1
  {
   private:
    std::map<tnrc_sp_xc_key, xc_adva_params, xc_map_comp_lsc> xc_map;           /// map of all xconn retrived by TL1

    /** map of port's resources TNRC_PORTID_T TNRCSP_LSC_RESOURCE_DETAIL_T */
    std::map<tnrc_portid_t, tnrcsp_lsc_resource_detail_t>  resource_list;


    /**--------   ADVA XC FSMs        -----------------------------------------------------------------------------------*/
    bool trncsp_adva_make_xc     (tnrcsp_adva_xc_state state, xc_adva_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result);
    bool trncsp_adva_destroy_xc  (tnrcsp_adva_xc_state state, xc_adva_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result);
    bool trncsp_adva_reserve_xc  (tnrcsp_adva_xc_state state, xc_adva_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result);
    bool trncsp_adva_unreserve_xc(tnrcsp_adva_xc_state state, xc_adva_params* parameters, tnrcsp_result_t response, std::string* data, tnrcsp_result_t* result);

    /**--------   ADVA TL1 MESSAGES   -------------------------------------------------------------------------------------*/
    tnrcsp_result_t  assign_channel       (std::string* ingress_aid, std::string* egress_aid, tnrcsp_lsp_xc_type conn_type,
                                           tnrcsp_adva_xc_state state, xc_adva_params* parameters);
    tnrcsp_result_t  adva_retrieve_channel(std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters);
    tnrcsp_result_t  adva_remove_channel  (std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters);
    tnrcsp_result_t  adva_restore_channel (std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters);
    tnrcsp_result_t  adva_delete_channel  (std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters);
    tnrcsp_result_t  adva_equalize_channel(std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters);
        
    bool adva_retrieve_channel_resp       (std::string* data, std::string* egress_aid, xc_adva_params* parameters, std::string esp_conn_status[], std::string esp_status);
    bool adva_retrieve_channel_status(std::string* data, std::string* egress_aid, xc_adva_params* parameters);
    void adva_autonomous_oos              (std::string* egress_aid, tnrcsp_adva_xc_state state, xc_adva_params* parameters);

   //-------------------------------------------------------------------------------------------------------------------
   public:
    friend class          PluginAdvaLSC_listen;
    friend class          PluginCalientFSC_listen;

    ~PluginAdvaLSC (void);
    PluginAdvaLSC (std::string name);


    tnrcsp_adva_map_fixed_port_t   fixed_port_map;
    bool                           ignore_alarms;
    bool                           ignore_equalization;

    ///determines if destroying alien XCS
    bool do_destroy_alien_xcs;

    void tnrcsp_set_fixed_port(uint32_t key, int flags, g2mpls_addr_t rem_eq_addr, port_id_t rem_port_id, uint32_t no_of_wavelen, uint32_t max_bandwidth, gmpls_prottype_t protection);

    /** send login TL1 commnad 
     * @param user   TL1 device login
     * @param passwd TL1 device password
     * @return 0 if login command send successful
     */
    int                   tl1_log_in(std::string user, std::string passwd);
    void                  get_all_equipment();                              /// send equipment retrieve TL1 commnad
    void                  get_all_xc();                                     /// send channels retrieve TL1 commnad 

    /**Launch communication with device or its simulator
     * @param location <ipAddress>:<port>:<username>:<password>:<path_to_configuration_file> real device
     * or              "simulator":<username>:<password>:<path_to_configuration_file>        device simulator
     */
    tnrcapiErrorCode_t    probe (std::string location);

    tnrcsp_result_t       trncsp_adva_get_resource_list(tnrcsp_resource_id_t** resource_listp, int* num);

    /** TODO fixit */
    void                  trncsp_adva_xc_process(tnrcsp_adva_xc_state state, xc_adva_params* parameters, 
                                                 tnrcsp_result_t result=TNRCSP_RESULT_NOERROR,
                                                 std::string* data=NULL);

    void                  notify_event(std::string aid, channel_t channel, operational_state operstate,
                                       administrative_state adminstate);

    void                  register_event(tnrc_portid_t portid, channel_t channel, operational_state operstate,
                                         administrative_state adminstate, tnrcsp_evmask_t e);

    /**--------   unused methods  -----------------------------------------------------------------------------------------*/
    wq_item_status        wq_function  (void *d);
    void                  del_item_data(void *d);

    /**--------   ADVA SP API     -----------------------------------------------------------------------------------------*/
    tnrcsp_result_t       tnrcsp_make_xc    (tnrcsp_handle_t *handlep, 
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

    tnrcsp_result_t       tnrcsp_destroy_xc (tnrcsp_handle_t *handlep,
                                             tnrc_port_id_t portid_in, 
                                             label_t labelid_in, 
                                             tnrc_port_id_t portid_out, 
                                             label_t              labelid_out,
                                             xcdirection_t        direction,
                                             tnrc_boolean_t  isvirtual,	
                                             tnrc_boolean_t deactivate, 
                                             tnrcsp_response_cb_t response_cb,
                                             void *response_ctxt);

    tnrcsp_result_t       tnrcsp_reserve_xc (tnrcsp_handle_t *handlep, 
                                             tnrc_port_id_t portid_in, 
                                             label_t labelid_in, 
                                             tnrc_port_id_t portid_out, 
                                             label_t labelid_out, 
                                             xcdirection_t        direction,
					     tnrc_boolean_t       isvirtual,
                                             tnrcsp_response_cb_t response_cb,
                                             void *response_ctxt);

    tnrcsp_result_t       tnrcsp_unreserve_xc (tnrcsp_handle_t *handlep, 
                                             tnrc_port_id_t portid_in, 
                                             label_t labelid_in, 
                                             tnrc_port_id_t portid_out, 
                                             label_t labelid_out, 
                                             xcdirection_t        direction,
					     tnrc_boolean_t       isvirtual,
                                             tnrcsp_response_cb_t response_cb,
                                             void *response_ctxt);

    tnrcsp_result_t       tnrcsp_register_async_cb (tnrcsp_event_t *events);
  };

  //=====================================================================================================================
  /** crossconnection information */
  class xc_adva_params
  {
   public:
    trncsp_xc_activity          activity;           /// type of xconn operaration

    ///  TNRC SP API xconn operation parameters
    tnrcsp_handle_t             handle;             /// handle of xconn operation
    tnrc_portid_t               port_in, port_out;  /// ingress and egress port id
    channel_t                   channel;            /// channel id
    xcdirection_t               direction;          /// directionality of xconn
    tnrc_boolean_t              _virtual;           /// allocate existing xconn
    tnrc_boolean_t              activate;           /// xconn activation
    tnrcsp_response_cb_t        response_cb;        /// pseudo-synchronous callback function
    void*                       response_ctx;       /// pseudo-synchronous callback function context
    tnrcsp_notification_cb_t    async_cb;           /// asynchronous callback function
    void*                       async_cxt;          /// asynchronous callback function context

    /// additional parameters
    tnrc_boolean_t              deactivate;         /// xconn deactivation
    std::string*                ingress_aid;        /// ingress adva port identifier
    std::string*                egress_aid;         /// egress adva port identifier
    tnrcsp_lsp_xc_type          conn_type;          /// xconn type

    /// additional parameters used for second part of the bidirectional xconn
    std::string*                rev_ingress_aid;    /// ingress port identifier for second direction of bidirectional xconn
    std::string*                rev_egress_aid;     /// egress port identifier for second direction of bidirectional xconn
    tnrcsp_lsp_xc_type          rev_conn_type;      /// xconn type for second direction of bidirectional xconn

    /// parameters important for xconn operation FSM
    bool                        activated;          /// false if xconn was only reserved otherwise true
    bool                        processed;          /// true is xconn is created or destroyed
    int                         repeat_count;       /// FSM state counter -> EFSM
    bool                        equalize_failed;    ///  
    bool                        final_result;       /// 
    tnrcsp_result_t             result;             /// 

    //-------------------------------------------------------------------------------------------------------------------
    /** Crossconnection information object contructor
    * @param activity - type of xconn operaration
    * @param handle - handle of xconn operation
    * @param port_in - ingress port id
    * @param port_out - egress port id
    * @param channel - channel id
    * @param direction - directionality of xconn
    * @param _virtual - allocate existing xconn
    * @param activate - xconn activation
    * @param response_cb - pseudo-synchronous callback function
    * @param response_ctx - pseudo-synchronous callback function context
    * @param async_cb - asynchronous callback function
    * @param async_cxt - asynchronous callback function context
    */
    xc_adva_params(trncsp_xc_activity           activity,
                   tnrcsp_handle_t              handle,
                   tnrc_portid_t                port_in,
                   tnrc_portid_t                port_out,
                   channel_t                    channel,
                   xcdirection_t                direction,
                   tnrc_boolean_t               _virtual,
                   tnrc_boolean_t               activate,
                   tnrcsp_response_cb_t         response_cb,
                   void*                        response_ctx,
                   tnrcsp_notification_cb_t     async_cb,
                   void*                        async_cxt);
    
    xc_adva_params();
    ~xc_adva_params();
    xc_adva_params& operator=(const xc_adva_params &from);
  };

  //========================================================================================================================
  //Function to export
  int   tl1_retrive_func (struct thread *thread);
}
void delete_adva_resource_node(void *node);


#endif /* TNRC_PLUGIN_ADVA_H */
