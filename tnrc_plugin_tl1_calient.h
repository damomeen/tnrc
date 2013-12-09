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



#ifndef TNRC_PLUGIN_CALIENT_H
#define TNRC_PLUGIN_CALIENT_H

#include "tnrc_sp.h"
#include "tnrc_plugin_tl1.h"
//#include "tnrc_plugin_tl1_calient_simulator.h"

namespace TNRC_SP_TL1
{
  class xc_calient_params;
  class PluginTl1;
  class PluginCalientFSC;

  //=======================================================================================================================
  /** infomation about a crossconnection */
  typedef struct {
    std::string                 ingress_aid;        /// ingress port identifier
    std::string                 egress_aid;         /// egress port identifier
    tnrcsp_notification_cb_t    async_cb;           /// asynchronous callback function
    void*                       async_cxt;          /// asynchronous callback function context
    tnrcsp_handle_t             handle;             /// handle of make xconn operation
    bool                        activated;          /// false if xconn was only reserved otherwise true
  } tnrcsp_calient_xc_item;

  //========================================================================================================================
  /** crossconnection FSM state */
  typedef enum
  {
    TNRCSP_CALIENT_STATE_INIT,        /// FSM init state
    TNRCSP_CALIENT_STATE_ASSIGN,      /// FSM waits ASG-CHANNEL response
    TNRCSP_CALIENT_STATE_RESTORE,     /// FSM waits RST-CHANNEL response
    TNRCSP_CALIENT_STATE_REMOVE,      /// FSM waits RMV-CHANNEL response
    TNRCSP_CALIENT_STATE_DELETE,      /// FSM waits DLT-CHANNEL response
    TNRCSP_CALIENT_STATE_CHECK,       /// FSM waits RTRV-CHANNEL response
  } tnrcsp_calient_xc_state;

#define SHOW_TNRCSP_CALIENT_XC_STATE(X)                   \
  (((X) == TNRCSP_CALIENT_STATE_INIT    ) ? "INIT"      : \
  (((X) == TNRCSP_CALIENT_STATE_ASSIGN  ) ? "ASSIGN"    : \
  (((X) == TNRCSP_CALIENT_STATE_RESTORE ) ? "RESTORE"   : \
  (((X) == TNRCSP_CALIENT_STATE_REMOVE  ) ? "REMOVE"    : \
  (((X) == TNRCSP_CALIENT_STATE_DELETE  ) ? "DELETE"    : \
  (((X) == TNRCSP_CALIENT_STATE_CHECK   ) ? "CHECK"     : \
             "==UNKNOWN=="))))))

  //=========================================================================================================================
  /**** CALIENT UTILS ******/
  tnrc_portid_t        calient_compose_id(std::string* aid);
  std::string*         calient_decompose_in(tnrc_portid_t port_id);
  std::string*         calient_decompose_out(tnrc_portid_t port_id);
  std::string*         calient_decompose_id(tnrc_portid_t port_id);

  bool                 calient_check_resources(tnrc_portid_t port_in, tnrc_portid_t port_out );

  bool                 calient_error_analyze(std::string* message, std::string description);
  tnrcsp_result_t      calient_error_analyze2(std::string* message, std::string description, tnrcsp_result_t response, tnrcsp_result_t default_response);

  void                 set_check_timer(xc_calient_params* parameters, int timeout); 

  //========================================================================================================================
  /** class PluginCalientFSC_listen */
  class PluginCalientFSC_listen : public PluginTl1_listen
  {
   public:
    PluginCalientFSC_listen();
    PluginCalientFSC_listen(PluginCalientFSC *owner);
    ~PluginCalientFSC_listen();

   private:
    PluginCalientFSC *pluginCalientFsc;       /// pointer to TNRC SP ADVA plugin module

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

    /** process incoming TL1 message from Calient */
    void process_xc(int state, void* parameters, tnrcsp_result_t result, std::string* response_block); 

    /** add event to last_event map */
    void add_event(std::string aid, channel_t channel, std::string* event_type, std::string* cond_desc);

    /** store information with xconn details */
    void save_xcs(std::string response_block);

    /** store information with equipments details */
    void save_equipments(std::string response_block);

    /** if synch function call wait more then 120 sec release all TL1 request waiting for response */
    void check_func_waittime();

    /** release all TL1 request waiting for response */
    void delete_func_waittime();
  };

  //========================================================================================================================
  /** TNRC Specyfic part Plugin class
  * Support Calient OXC
  */
  class PluginCalientFSC : public PluginTl1
  {
   private:
    std::map<tnrc_sp_xc_key, xc_calient_params, xc_map_comp_fsc> xc_map;      /// map of all xconn

    bool fixed_segments[6];  //reducing number of TL1 port retrive commands
    bool fixed_fibers[6][9]; //reducing number of TL1 port retrive commands

    //FIXME tnrcsp_lsc_resource_detail_t <-> tnrcsp_fsc_resource_detail_t different resource detail ???
    std::map<tnrc_portid_t, tnrcsp_lsc_resource_detail_t>  resource_list;


    //--------   CALIENT XC  -----------------------------------------------------------------------------------
    bool trncsp_calient_make_xc     (tnrcsp_calient_xc_state  state,
                                     xc_calient_params        *parameters,
                                     tnrcsp_result_t          response,
                                     std::string              *data,
                                     tnrcsp_result_t          *result);

    bool trncsp_calient_destroy_xc  (tnrcsp_calient_xc_state  state,
                                     xc_calient_params        *parameters,
                                     tnrcsp_result_t          response,
                                     std::string              *data,
                                     tnrcsp_result_t          *result);

    bool trncsp_calient_reserve_xc  (tnrcsp_calient_xc_state  state,
                                     xc_calient_params*       parameters,
                                     tnrcsp_result_t          response,
                                     std::string              *data,
                                     tnrcsp_result_t          *result);

    bool trncsp_calient_unreserve_xc(tnrcsp_calient_xc_state  state,
                                     xc_calient_params        *parameters,
                                     tnrcsp_result_t          response,
                                     std::string              *data,
                                     tnrcsp_result_t          *result);

    //--------   CALIENT TL1 MESSAGES   ----------------------------------------------------------------------------------------
    tnrcsp_result_t  calient_provision_xc (std::string* ingress_aid, 
                                           std::string* egress_aid,
                                           tnrcsp_calient_xc_state state,
                                           xc_calient_params* parameters);

    tnrcsp_result_t  calient_retrieve_xc  (std::string* ingress_aid,
                                           std::string* egress_aid,
                                           tnrcsp_calient_xc_state state,
                                           xc_calient_params* parameters);

    tnrcsp_result_t  calient_remove_xc    (std::string              *ingress_aid,
                                           std::string              *egress_aid,
                                           tnrcsp_calient_xc_state  state,
                                           xc_calient_params        *parameters);

    tnrcsp_result_t  calient_restore_xc   (std::string              *ingress_aid,
                                           std::string              *egress_aid,
                                           tnrcsp_calient_xc_state  state,
                                           xc_calient_params        *parameters);

    tnrcsp_result_t  calient_delete_xc    (std::string              *ingress_aid,
                                           std::string              *egress_aid,
                                           tnrcsp_calient_xc_state  state,
                                           xc_calient_params        *parameters);

    bool         calient_retrieve_xc_resp (std::string              *data,
                                           std::string              *ingress_aid,
                                           std::string              *egress_aid,
                                           xc_calient_params        *parameters,
                                           std::string              esp_admstate,
                                           std::string              esp_opstate,
                                           std::string              esp_opcap);

    void         calient_autonomous_oos   (std::string              *ingress_aid,
                                           std::string              *egress_aid,
                                           tnrcsp_calient_xc_state  state,
                                           xc_calient_params        *parameters);

   public:
    ~PluginCalientFSC (void);
    PluginCalientFSC  (std::string name);

    tnrcsp_result_t trncsp_calient_get_resource_list(tnrcsp_resource_id_t** resource_listp, int* num);

    //-------------------------------------------------------------------------------------------------------------------
    tnrcapiErrorCode_t    probe (std::string location);

    friend class          PluginCalientFSC_listen;

    int                   tl1_log_in(std::string user, std::string passwd);
    void                  get_all_equipment();
    void                  get_all_xc();

    //-------  unused methods  -----------------------------------------------------------------------------------------
    wq_item_status        wq_function  (void *d);
    void                  del_item_data(void *d);

    //------  CALIENT SP API   ---------------------------------------------------------------------------------------------
    void          trncsp_calient_xc_process (tnrcsp_calient_xc_state  state,
                                             xc_calient_params        *parameters,
                                             tnrcsp_result_t          result=TNRCSP_RESULT_NOERROR,
                                             std::string              *data=NULL);

    void                  notify_event      (std::string              aid,
                                             operational_state        operstate,
                                             administrative_state     adminstate);

    void                  register_event    (tnrc_portid_t            portid,
                                             operational_state        operstate,
                                             administrative_state     adminstate,
					     tnrcsp_evmask_t          e);

    tnrcsp_result_t       tnrcsp_make_xc    (tnrcsp_handle_t * handlep,
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
                                             void * async_ctxt);

    tnrcsp_result_t       tnrcsp_destroy_xc (tnrcsp_handle_t *handlep,
                                             tnrc_port_id_t portid_in, 
                                             label_t labelid_in, 
                                             tnrc_port_id_t portid_out, 
                                             label_t labelid_out, 
                                             xcdirection_t direction,
                                             tnrc_boolean_t isvirtual,	
                                             tnrc_boolean_t deactivate, 
                                             tnrcsp_response_cb_t response_cb,
                                             void *response_ctxt);

    tnrcsp_result_t       tnrcsp_reserve_xc (tnrcsp_handle_t *handlep, 
                                             tnrc_port_id_t portid_in, 
                                             label_t labelid_in, 
                                             tnrc_port_id_t portid_out, 
                                             label_t labelid_out, 
                                             xcdirection_t direction,
					     tnrc_boolean_t isvirtual,
                                             tnrcsp_response_cb_t response_cb,
                                             void *response_ctxt);

    tnrcsp_result_t     tnrcsp_unreserve_xc (tnrcsp_handle_t *handlep, 
                                             tnrc_port_id_t portid_in, 
                                             label_t labelid_in, 
                                             tnrc_port_id_t portid_out, 
                                             label_t labelid_out, 
                                             xcdirection_t direction,
					     tnrc_boolean_t isvirtual,
                                             tnrcsp_response_cb_t response_cb,
                                             void *response_ctxt);

    tnrcsp_result_t tnrcsp_register_async_cb(tnrcsp_event_t *events);
  };

  //=====================================================================================================================
  /** crossconnection information */
  class xc_calient_params
  {
   public:
    trncsp_xc_activity          activity;           /// type of xconn operaration

    ///  TNRC SP API xconn operation parameters
    tnrcsp_handle_t             handle;             /// handle of xconn operation
    tnrc_portid_t               port_in, port_out;  /// ingress and egress port id
    xcdirection_t               direction;          /// directionality of xconn
    tnrc_boolean_t              _virtual;           /// allocate existing xconn
    tnrc_boolean_t              activate;           /// xconn activation
    tnrcsp_response_cb_t        response_cb;        /// pseudo-synchronous callback function
    void*                       response_ctx;       /// pseudo-synchronous callback function context
    tnrcsp_notification_cb_t    async_cb;           /// asynchronous callback function
    void*                       async_cxt;          /// asynchronous callback function context

    /// additional parameters
    tnrc_boolean_t              deactivate;         /// xconn deactivation
    std::string*                ingress_aid;        /// ingress calient port identifier
    std::string*                egress_aid;         /// egress calient port identifier

    /// parameters important for xconn operation FSM
    bool                        activated;          /// false if xconn was only reserved otherwise true
    int                         repeat_count;       /// FSM state counter -> EFSM 
    bool                        final_result;       /// 
    tnrcsp_result_t             result;             /// 

    //-------------------------------------------------------------------------------------------------------------------
    /** Crossconnection information object contructor
    * @param activity     - type of xconn operaration
    * @param handle       - handle of xconn operation
    * @param port_in      - ingress port id
    * @param port_out     - egress port id
    * @param direction    - directionality of xconn
    * @param _virtual     - allocate existing xconn
    * @param activate     - xconn activation
    * @param response_cb  - pseudo-synchronous callback function
    * @param response_ctx - pseudo-synchronous callback function context
    * @param async_cb     - asynchronous callback function
    * @param async_cxt    - asynchronous callback function context
    */
    xc_calient_params(trncsp_xc_activity           activity,
                      tnrcsp_handle_t              handle,
                      tnrc_portid_t                port_in,
                      tnrc_portid_t                port_out,
                      xcdirection_t                direction,
                      tnrc_boolean_t               _virtual,
                      tnrc_boolean_t               activate,
                      tnrcsp_response_cb_t         response_cb,
                      void*                        response_ctx,
                      tnrcsp_notification_cb_t     async_cb,
                      void*                        async_cxt);

    xc_calient_params();
    ~xc_calient_params();
    xc_calient_params & operator=(const xc_calient_params &from);



  };

  //========================================================================================================================
  //Function to export
  int   tl1_retrive_func (struct thread *thread);
}
void  calient_init_vty();

#endif /* TNRC_PLUGIN_ADVA_H */
