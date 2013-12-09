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

#ifndef PLUGIN_AT9424_HEADER_FILE
#define PLUGIN_AT9424_HEADER_FILE

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#ifdef HAVE_LSSH2
#include <libssh2.h>
#endif
#include "thread.h"

#include "tnrc_dm.h"
#include "tnrc_sp.h"
#include "tnrc_plugin.h"

#ifndef OUT
#define OUT
#endif

extern struct thread_master *master;

namespace TNRC_SP_AT9424
{
  class PluginAT9424_session;
  class PluginAT9424_listen;
  class PluginAT9424;

  //========================================================================================================================
  /** crossconnection FSM state */
  typedef enum {
    TNRCSP_STATE_INIT,
    TNRCSP_STATE_DELETE_VLAN,
    TNRCSP_STATE_ADD_INGRESS_PORT,
    TNRCSP_STATE_ADD_EGRESS_PORT,
    TNRCSP_STATE_FINAL
  } tnrcsp_at9424_xc_state;

  typedef unsigned int          tnrc_portid_t;
  typedef unsigned int          vlanid_t;
  typedef unsigned int          tnrcsp_lsc_evmask_t;

  #define MAX_VLAN_ID           3500

  typedef enum {
    TNRC_SP_STATE_NO_SESSION,
    TNRC_SP_STATE_END_SESSION,
    TNRC_SP_STATE_NOT_LOGGED_IN,
    TNRC_SP_STATE_NOT_LOGGED_IN_PASS,
    TNRC_SP_STATE_LOGGED_IN,
    TNRC_SP_STATE_DONE,
    TNRC_SP_STATE_DISCONNECTED
  } tnrc_sp_state_at9424;

  typedef enum {
    TNRC_NO_COMMAND,
    TNRC_FIRST_TELNET_OPTION,
    TNRC_SECOND_TELNET_OPTION,
    TNRC_THIRD_TELNET_OPTION,
    TNRC_LOGIN,
    TNRC_PASS,
    TNRC_CREATE_VLAN,
    TNRC_DESTROY_VLAN,
    TNRC_ADD_INGRESS_PORT,
    TNRC_ADD_EGRESS_PORT,
    TNRC_GET_PORTS_RESOURCES,
    TNRC_GET_VLANS_RESOURCES
  } tnrc_l2sc_cmd_type_t;

  typedef enum {
    TNRCSP_XC_MAKE,
    TNRCSP_XC_DESTROY,
    TNRCSP_XC_RESERVE,
    TNRCSP_XC_UNRESERVE
  } trncsp_xc_activity;

  typedef struct {
    tnrc_portid_t               port_in;
    tnrc_portid_t               port_out;
    vlanid_t                    vlanid;
  } tnrc_sp_xc_key;

  typedef enum {
    TNRC_OPERSTATE_UP,
    TNRC_OPERSTATE_DOWN
  } tnrc_operstate_t;

  typedef enum {
    TNRC_ADMINSTATE_ENABLED,
    TNRC_ADMINSTATE_DISABLED
  } tnrc_adminstate_t;

  typedef enum {
    TNRC_SP_L2SC_SPEED_10M,
    TNRC_SP_L2SC_SPEED_100M,
    TNRC_SP_L2SC_SPEED_1G,
    TNRC_SP_L2SC_SPEED_NONE
  } tnrc_sp_l2sp_speed;

  typedef enum {
    TNRC_SP_L2SC_DUPLEX_FULL,
    TNRC_SP_L2SC_DUPLEX_HALF,
    TNRC_SP_L2SC_DUPLEX_NONE
  } tnrc_sp_l2sp_duplex_mode;

  typedef enum {
    TNRC_SP_L2SC_ETH,
    TNRC_SP_L2SC_TOKEN_RING,
    TNRC_SP_L2SC_FDDI
  } tnrcsp_l2sc_eqtype_t;

  typedef struct {
    operational_state           oper_state;
    administrative_state        admin_state;
    tnrcsp_lsc_evmask_t         last_event;
    tnrc_sp_l2sp_speed          speed;
    tnrc_sp_l2sp_duplex_mode    duplex_mode;
    tnrcsp_l2sc_eqtype_t        equip_type;
  } tnrcsp_l2sc_resource_detail_t;

  typedef struct {
    opstate_t*                  operstate_filter;
    admstate_t*                 adminstate_filter;
    std::list<std::string>      events_filter;
    vlanid_t                    vlanid;
    std::string                 event_type;
    std::string                 descr;
    bool                        notify;
  } tnrc_sp_last_events_item;

  typedef enum {
    TNRCSP_L2SC_XCSTATE_RESERVED,
    TNRCSP_L2SC_XCSTATE_ACTIVE,
    TNRCSP_L2SC_XCSTATE_FAILED,
    TNRCSP_L2SC_XCSTATE_BUSY
  } tnrcsp_l2sc_xc_state_t;

  typedef struct {
    tnrc_portid_t               portid_in;
    tnrc_portid_t               portid_out;
    vlanid_t                    vlanid;
    tnrcsp_l2sc_xc_state_t      xc_state;
  } tnrcsp_l2sc_xc_detail_t;

  typedef struct {
    std::string*                portid_in;
    std::string*                portid_out;
  } tnrcsp_l2sc_xc_res;

  typedef enum {
    GET_EQUIPMENT,
    GET_XC,
    CHECK_ACTION,
    DONE
  } tnrc_sp_retrive_activity_t;

  struct xc_map_comp {
    bool operator() (const tnrc_sp_xc_key& lhs, const tnrc_sp_xc_key& rhs) const
    {
        if(lhs.port_in < rhs.port_in)
            return true;
        if(lhs.port_in > rhs.port_in)
            return false;
        if(lhs.port_in == rhs.port_in)
        {
            if(lhs.port_out < rhs.port_out)
                return true;
            if(lhs.port_out > rhs.port_out)
                return false;
            if(lhs.port_out == rhs.port_out)
                return lhs.vlanid < rhs.vlanid;
        }
    }
  };

  //========================================================================================================================
  /** Class PluginAT_9424_session
  * Provides communication with SSH2 protocol using network connection
  */
  class PluginAT9424_session
  {
    private:

      PluginAT9424*               plugin;
      in_addr                     remote_address;           /// address of the real remote device
      int                         remote_port;              /// port of the real remote device
      int                         s_desc;                   /// socket descriptor

    public:

      PluginAT9424_session(in_addr adress, int port, PluginAT9424 *);
      ~PluginAT9424_session();

      #ifdef HAVE_LSSH2 
      LIBSSH2_SESSION*            session;                 /// SSH2 session object
      LIBSSH2_CHANNEL*            channel;                 /// SSH2 channel object
      #endif

      int                         _connect();
      int                         _send(std::string* mes);
      int                         _recv(OUT std::string* mes, int buff_size);
      int                         close_connection();
      int                         fd();
  };

  //======================================================================================================================== 
  /**  Class PluginAT9424_listen
  *  Listen and process received Allied Telesis responses
  */
  class PluginAT9424_listen
  {
    public:

      PluginAT9424*             plugin;
      PluginAT9424_listen(PluginAT9424 *);
      ~PluginAT9424_listen();

      void                      process_message(std::string* mes);

      std::map<tnrc_portid_t, vlanid_t>  label_map;
      std::map<tnrc_portid_t, tnrcsp_l2sc_resource_detail_t> resource_list;
      std::list<tnrcsp_l2sc_xc_detail_t> xc_list;
      std::list<tnrc_sp_xc_key> external_xc;
      std::list<tnrc_portid_t> xdisconnected_ports;

    protected:

      /** store information with equipments details */
      void                      save_equipments(std::string response_block);
      /** store information with vlans details */
      void                      save_channels(std::string response_block);
      /** create unique port_id from "port" */
      tnrc_portid_t             at9424_decode_portid(char* portid_str);
      tnrc_sp_xc_key            find_vlanid_from_portid(tnrc_portid_t portid);

      void                      register_event(tnrc_portid_t              portid,
                                               operational_state          operstate,
                                               administrative_state       adminstate,
                                               tnrc_sp_l2sp_speed         speed,
                                               tnrc_sp_l2sp_duplex_mode   duplex_mode,
                                               tnrcsp_l2sc_eqtype_t       equip_type,
                                               tnrcsp_evmask_t e);

      void                      process_no_command_response();
      void                      process_telnet_options_response();
      void                      process_login_response();
      void                      process_password_response();
      void                      process_get_ports_response();
      void                      process_get_vlans_response();
      void                      process_create_vlan_response();
      void                      process_destroy_vlan_response();
      void                      process_add_ports_response();

      void                      _trncsp_at9424_xc_process(int state, void* parameters, tnrcsp_result_t result, string* response_block);

  };

  //=====================================================================================================================
  /**  Class xc_at9424_params
  *  Crossconnection information
  */
  class xc_at9424_params
  {
  public:
    /**  TNRC SP API xconn operation parameters */
    trncsp_xc_activity          activity;            /// type of xc operaration
    tnrcsp_handle_t             handle;              /// handle of xc operation
    tnrc_portid_t               port_in, port_out;   /// ingress and egress port id
    vlanid_t                    vlanid;              /// vlan id
    xcdirection_t               direction;           /// directionality of xc
    tnrc_boolean_t              _virtual;            /// allocate existing xc
    tnrc_boolean_t              activate;            /// xc activation
    tnrcsp_response_cb_t        response_cb;         /// pseudo-synchronous callback function
    void*                       response_ctxt;       /// pseudo-synchronous callback function context
    tnrcsp_notification_cb_t    async_cb;            /// asynchronous callback function
    void*                       async_ctxt;          /// asynchronous callback function context

    /** additional parameters */
    tnrc_boolean_t              deactivate;          /// xc deactivation
    std::string*                ingress_fid;         /// ingress port identifier
    std::string*                egress_fid;          /// egress port identifier

    /** parameters important for xconn operation FSM */
    bool                        activated;           /// false if xc was only reserved otherwise true -> we can't reserve xmr
    int                         repeat_count;        /// FSM state counter -> not used
    bool                        final_result;
    tnrcsp_result_t             result;

    //-------------------------------------------------------------------------------------------------------------------
    /** Crossconnection information object contructor
    * @param activity - type of xc operaration
    * @param handle - handle of xc operation
    * @param port_in - ingress port id
    * @param port_out - egress port id
    * @param channel - channel id
    * @param direction - directionality of xc
    * @param _virtual - allocate existing xco
    * @param activate - xc activation
    * @param response_cb - pseudo-synchronous callback function
    * @param response_ctx - pseudo-synchronous callback function context
    * @param async_cb - asynchronous callback function
    * @param async_cxt - asynchronous callback function context
    */
    xc_at9424_params(trncsp_xc_activity            activity,
                     tnrcsp_handle_t               handle,
                     tnrc_portid_t                 port_in,
                     tnrc_portid_t                 port_out,
                     vlanid_t                      vlanid,
                     xcdirection_t                 direction,
                     tnrc_boolean_t                _virtual,
                     tnrc_boolean_t                activate,
                     tnrcsp_response_cb_t          response_cb,
                     void*                         response_ctxt,
                     tnrcsp_notification_cb_t      async_cb,
                     void*                         async_ctxt);

    ~xc_at9424_params(){};

  };

  //=====================================================================================================================
  /**  Class PluginAT9424
  *  Specyfic Part for Allied Telesis 9424T
  */
  class PluginAT9424: public Plugin
  {
   public:

    friend class PluginAT9424_listen;
    friend class PluginAT9424_session;

    PluginAT9424();
    PluginAT9424(std::string name);
    ~PluginAT9424();

    /**Communication with device
     * @param location <ipAddress>:<port>:<username>:<password>:<enable_password>:<path_to_configuration_file> real device
     */
    tnrcapiErrorCode_t          probe (std::string location);

    /** Thread functions*/
    void                        readT(thread *read);
    void                        retriveT(thread *retrive);
    int                         doRetrive();
    int                         doRead();
    int                         getFd();

   protected:

    PluginAT9424_session*       sock;
    PluginAT9424_listen*        listen_proc;
    xc_at9424_params*           param;

    thread*                     t_read;
    thread*                     t_retrive;

    std::string                 login;                 ///connectrion login    (ssh)
    std::string                 password;              ///connection  password (ssh)
    std::string                 enable_password;       ///enable password
    std::string                 vlan_number;           ///vlan number
    std::string                 vlan_pool;             ///how many vlans do we have to each machine
    std::string                 backup_vlan;           ///
    list<tnrc_sp_xc_key>        vlan_list;             ///List with vlans numbers
    bool                        read_thread;           ///Start read thread
    int                         _connect;              ///State of connection

    /** AP data model parameters */
    eqpt_id_t                   data_model_id;         /// Equipment id
    g2mpls_addr_t               data_model_address;    /// Equipment address displayed in data model
    eqpt_type_t                 data_model_type;       /// Equipment type
    opstate_t                   data_model_opstate;    /// Equipment operationall state dispalayed in data model
    admstate_t                  data_model_admstate;   /// Equipment administrative state dispalayed in data model
    std::string                 data_model_location;   /// Equipment location displayed in data model

    std::map<tnrc_sp_xc_key, xc_at9424_params*, xc_map_comp> xc_map;   /// map of all xconn retrived by SSH
    std::map<std::string, tnrc_sp_last_events_item> last_events;
    std::list<xc_at9424_params*> action_list;          /// Action list (make, destroy)


    int                         xc_id_;
    tnrc_sp_state_at9424        TELNET_status;          /// SP state
    std::string                 at9424_response;        /// Message recived from device
    std::string                 temp_at9424_response;   /// Temporary message recived from device
    std::string                 key_word;               /// Key word in response
    std::string                 second_key_word;        /// Second key word in response
    bool                        no_key_word;            /// No key word in response
    std::string                 sent_command;           /// Response sent to device
    tnrc_l2sc_cmd_type_t        command_type;           /// Parsing command type
    std::string                 ports_resources_response;///Temporary ports response
    bool                        send_space;             /// Send space when response is parted
    bool                        wait_disconnect;        /// Wait for disconnection
    int                         check_status;
    tnrc_sp_retrive_activity_t  _activity;              /// Background process activity: GET_EQUIPMENT, GET_XC




    int                         at9424_log_in();
    int                         at9424_password();
    void                        get_ports_resources();
    void                        get_all_xc();
    int                         at9424_connect();
    int                         at9424_disconnect();
    int                         close_connection();
    int                         first_free_vlanid(tnrc_portid_t portid_in, tnrc_portid_t portid_out);
    int                         find_vlanid(tnrc_portid_t portid);
    int                         gen_identifier() {return identifier++;}

    /** Send SSH command asynchronously */
    tnrcsp_result_t             asynch_send(std::string*                command,
                                            void*                       parameters=NULL,
                                            tnrc_l2sc_cmd_type_t        cmd_type=TNRC_NO_COMMAND);

   /** FOUNDRY XC FSM functions */
    void                        tnrcsp_at9424_xc_process(tnrcsp_at9424_xc_state           state,
                                                         xc_at9424_params*                parameters,
                                                         tnrcsp_result_t                  response,
                                                         string*                          data);

    bool                        tnrcsp_at9424_unreserve_xc(tnrcsp_at9424_xc_state         state,
                                                           xc_at9424_params*              parameters,
                                                           tnrcsp_result_t                response,
                                                           string*                        data,
                                                           tnrcsp_result_t*               result);

    bool                        tnrcsp_at9424_reserve_xc(tnrcsp_at9424_xc_state           state,
                                                         xc_at9424_params*                parameters,
                                                         tnrcsp_result_t                  response,
                                                         string*                          data,
                                                         tnrcsp_result_t*                 result);

    bool                        tnrcsp_at9424_destroy_xc(tnrcsp_at9424_xc_state           state,
                                                         xc_at9424_params*                parameters,
                                                         tnrcsp_result_t                  response,
                                                         string*                          data,
                                                         tnrcsp_result_t*                 result);

    bool                        tnrcsp_at9424_make_xc(tnrcsp_at9424_xc_state              state,
                                                      xc_at9424_params*                   parameters,
                                                      tnrcsp_result_t                     response,
                                                      string*                             data,
                                                      tnrcsp_result_t*                    result);

    tnrcsp_result_t             at9424_create_vlan(xc_at9424_params*                      parameters);
    tnrcsp_result_t             at9424_destroy_vlan(xc_at9424_params*                     parameters);
    tnrcsp_result_t             at9424_add_ports(xc_at9424_params*                        parameters,
                                                 tnrcsp_at9424_xc_state                   state);


    /** unused methods */
    wq_item_status              wq_function  (void *d);
    void                        del_item_data(void *d);

    /** FOUNDRY XMR SP API */
    tnrcsp_result_t             tnrcsp_reserve_xc (tnrcsp_handle_t*                         handlep,
                                                   tnrc_port_id_t                           portid_in,
                                                   label_t                                  labelid_in,
                                                   tnrc_port_id_t                           portid_out,
                                                   label_t                                  labelid_out,
                                                   xcdirection_t                            direction,
						   tnrc_boolean_t                           isvirtual,
                                                   tnrcsp_response_cb_t                     response_cb,
                                                   void*                                    response_ctxt);

    tnrcsp_result_t             tnrcsp_unreserve_xc (tnrcsp_handle_t*                       handlep,
                                                     tnrc_port_id_t                         portid_in,
                                                     label_t                                labelid_in,
                                                     tnrc_port_id_t                         portid_out,
                                                     label_t                                labelid_out,
                                                     xcdirection_t                          direction,
						     tnrc_boolean_t                         isvirtual,
                                                     tnrcsp_response_cb_t                   response_cb,
                                                     void*                                  response_ctxt);

    tnrcsp_result_t             tnrcsp_make_xc (tnrcsp_handle_t*                            handlep,
                                                tnrc_port_id_t                              portid_in,
                                                label_t                                     labelid_in,
                                                tnrc_port_id_t                              portid_out,
                                                label_t                                     labelid_out,
                                                xcdirection_t                               direction,
                                                tnrc_boolean_t                              isvirtual,
                                                tnrc_boolean_t                              activate,
                                                tnrcsp_response_cb_t                        response_cb,
                                                void*                                       response_ctxt,
                                                tnrcsp_notification_cb_t                    async_cb,
                                                void*                                       async_ctxt);

    tnrcsp_result_t             tnrcsp_destroy_xc (tnrcsp_handle_t*                         handlep,
                                                   tnrc_portid_t                            portid_in,
                                                   label_t                                  labelid_in,
                                                   tnrc_portid_t                            portid_out,
                                                   label_t                                  labelid_out,
                                                   xcdirection_t                            direction,
                                                   tnrc_boolean_t                           isvirtual,
                                                   tnrc_boolean_t                           deactivate,
                                                   tnrcsp_response_cb_t                     response_cb,
                                                   void*                                    response_ctxt);

    tnrcsp_result_t             tnrcsp_register_async_cb (tnrcsp_event_t *events);

   private:

    int                         identifier;

  };
}
#endif /* PLUGIN_AT9424_HEADER_FILE */
