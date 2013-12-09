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



#ifndef PLUGIN_TL1_HEADER_FILE
#define PLUGIN_TL1_HEADER_FILE

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "thread.h"

#include "tnrc_plugin.h"

#include "tnrc_dm.h"
#include "tnrc_sp.h"

#ifndef OUT
#define OUT
#endif

#define TL1_RETRIVE_INTERWALL 60

extern struct thread_master *master;

namespace TNRC_SP_TL1
{
  typedef std::map<std::string, std::string> dict;

  void           remove_empty_lines(std::string* mes);
  void           remove_line(std::string* mes);
  int            get_message_type(std::string* mes, std::string* mes_code);
  std::string    pop_line(std::string* mes);
  int            match(std::string* data, std::string pattern, int token_num, std::string tokens[]);
  void           match_dict(std::string* data, dict& d);

  class PluginTl1;
  class PluginTl1_listen;

  typedef unsigned int   tnrc_portid_t;
  typedef unsigned int   tnrcsp_lsc_evmask_t;
  typedef unsigned short tnrcsp_lsc_eqplane_t;
  typedef unsigned int   channel_t;

  typedef enum {
    TNRCSP_XC_MAKE,
    TNRCSP_XC_DESTROY,
    TNRCSP_XC_RESERVE,
    TNRCSP_XC_UNRESERVE
  } trncsp_xc_activity;

  /** infomation about port parameters unretliviable by TL1 */
  typedef struct {
    int                         flags;              /// flags
    g2mpls_addr_t               rem_eq_addr;        /// remote address
    port_id_t                   rem_port_id;        /// remote port id
    uint32_t                    no_of_wavelen;      /// number of wawelengths
    uint32_t                    max_bandwidth;      /// maximum bandwidth
    gmpls_prottype_t            protection;         /// protection type
    bool                        added_to_DM;        /// added to the AP Data model
  } tnrcsp_adva_fixed_port_t;

  typedef std::map<const uint32_t, tnrcsp_adva_fixed_port_t> tnrcsp_adva_map_fixed_port_t;

  typedef enum {
    TNRCSP_LSC_PASSTHRU,
    TNRCSP_LSC_ADD,
    TNRCSP_LSC_DROP,
    TNRCSP_LSC_UNKNOWN
  } tnrcsp_lsp_xc_type;

#define SHOW_TNRCSP_LSP_XC_TYPE(X)                    \
  (((X) == TNRCSP_LSC_PASSTHRU) ? "PASSTHRU"   : \
  (((X) == TNRCSP_LSC_ADD     ) ? "ADD"        : \
  (((X) == TNRCSP_LSC_DROP    ) ? "DROP"       : \
             "==UNKNOWN==")))

  typedef enum {
    TNRCSP_LSC_XCSTATE_RESERVED,
    TNRCSP_LSC_XCSTATE_ACTIVE,
    TNRCSP_LSC_XCSTATE_FAILED,
    TNRCSP_LSC_XCSTATE_BUSY
  } tnrcsp_lsc_xc_state_t;

#define SHOW_TNRCSP_XC_STATE(X)                          \
  (((X) == TNRCSP_LSC_XCSTATE_RESERVED) ? "RESERVED"   : \
  (((X) == TNRCSP_LSC_XCSTATE_ACTIVE  ) ? "ACTIVE"     : \
  (((X) == TNRCSP_LSC_XCSTATE_FAILED  ) ? "FAILED"     : \
  (((X) == TNRCSP_LSC_XCSTATE_BUSY    ) ? "BUSY"       : \
             "==UNKNOWN=="))))

  typedef enum {
    TNRCSP_LISTTYPE_UNSPECIFIED,
    TNRCSP_LISTTYPE_RESOURCES
  } tnrcsp_list_type_t;

  typedef enum {
    TNRCSP_LSC_OLD,
    TNRCSP_LSC_XCVR
  } tnrcsp_lsc_eqtype_t;

#define SHOW_TNRCSP_LSP_EQPT_PLANE(X)                   \
  (((X) == TNRCSP_LSC_OLD        ) ? "LSC_OLD"   : \
  (((X) == TNRCSP_LSC_XCVR       ) ? "LSC_XCVR"  : \
             "==UNKNOWN=="))

  typedef enum {
    TNRC_OPERSTATE_UP,
    TNRC_OPERSTATE_DOWN,
  } tnrc_operstate_t;

  typedef enum {
    TNRC_ADMINSTATE_ENABLED,
    TNRC_ADMINSTATE_DISABLED,
  } tnrc_adminstate_t;

  typedef enum {
    TNRC_SP_TL1_STATE_NO_ADDRESS,
    TNRC_SP_TL1_STATE_NO_SESSION,
    TNRC_SP_TL1_STATE_NOT_LOGGED_IN,
    TNRC_SP_TL1_STATE_LOGGED_IN
  } tnrc_sp_state;

  typedef enum {
    GET_EQUIPMENT,
    GET_XC
  } tnrc_sp_tl1_retrive_activity_t;

  typedef struct {
    label_t                 labelid;
    operational_state       oper_state;
    administrative_state    admin_state;
    tnrcsp_lsc_evmask_t     last_event;
    tnrcsp_lsc_eqtype_t     equip_type;
    tnrcsp_lsc_eqplane_t    equip_plane;
  } tnrcsp_lsc_resource_detail_t;

  typedef struct
  {
    tnrc_portid_t           portid_in;
    tnrc_portid_t           portid_out;
    channel_t               channel;
    tnrcsp_lsc_xc_state_t   xc_state;
  } tnrcsp_lsc_xc_detail_t;

  typedef struct
  {
    tnrc_portid_t            port_in;
    tnrc_portid_t            port_out;
    channel_t                channel;
  } tnrc_sp_xc_key;

  typedef struct
  {
    time_t                   start;
    std::string              aid;
    std::string              text;
    int                      state;
    void*                    parameters;
  } tnrc_sp_handle_item;

  /* structure enabling timer raised events in XC FSMs */
  typedef struct
  {
    int                      state;        /// next state of XC FSM
    void*                    parameters;   /// XC parameters
    PluginTl1_listen*        pluginListen; /// object containg XC FSM: process_xc function
  } tnrcsp_xc_timer_event;

  struct xc_map_comp_lsc
  {
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
                return lhs.channel < rhs.channel;
        }
    }
  };

  struct xc_map_comp_fsc
  {
    bool operator() (const tnrc_sp_xc_key& lhs, const tnrc_sp_xc_key& rhs) const
    {
        if(lhs.port_in < rhs.port_in)
            return true;
        if(lhs.port_in > rhs.port_in)
            return false;
        if(lhs.port_in == rhs.port_in)
            return lhs.port_out < rhs.port_out;
    }
  };

/** Abstract Class TL1
  * Provides communication with TL1 protocol
  */
  class TL1
  {
   public:
    TL1() {};
    ~TL1() {};

    virtual int _connect() = 0;
    virtual int _send(std::string* mes) = 0;
    virtual int _recv(OUT std::string* mes, int buff_size) = 0;
    virtual int close_connection() = 0;
    virtual int fd() = 0;
  };

/** Class TL1_socket
  * Provides communication with TL1 protocol using network connection
  */
  class TL1_socket: public TL1
  {
   private:
    in_addr    remote_address;           /// address of the real remote device
    int        remote_port;              /// port of the real remote device
    int        socket_desc;              /// socket descriptor

   public:
    TL1_socket();
    TL1_socket(in_addr adress, int port);
    ~TL1_socket();

    int _connect();
    int _send(std::string* mes);
    int _recv(OUT std::string* mes, int buff_size);

    int close_connection();

    int fd();
  };

/**  Abstract class PluginTl1_listen
  *  Listen and process received TL1 responses
  */
  class PluginTl1_listen
  {
   public:
    PluginTl1                                 *plugin;

    PluginTl1_listen();
    ~PluginTl1_listen();

    void           add_handler(int key, tnrc_sp_handle_item value);
    void           process_message(std::string* mes);

    virtual void   process_xc(int state, void* parameters, tnrcsp_result_t result, std::string* response_block) = 0;


   protected:
    bool                                      active;
    pthread_t                                 thread;
    std::map<const int, tnrc_sp_handle_item>  handlers;
    std::map<const int, std::string>          multipart;


    void           start();
    void           stop();

    void           save_multiple_part(std::string* response_block, int ctag);
    std::string    get_saved_multiple_parts(int ctag);

    void           process_response(std::string* mes, int ctag);
    void           process_acknowledgment(std::string* mes_code, int ctag);
    virtual void   process_autonomous(std::string* mes, std::string* mes_code) = 0;
    virtual void   process_successful(std::string* response_block, std::string* command, int ctag);
    virtual void   process_denied(std::string* response_block, std::string* command, int ctag);

   // virtual void   process_xc(int state, void* parameters, tnrcsp_result_t result, std::string* response_block) = 0; 
    virtual void   check_func_waittime() = 0;
    virtual void   delete_func_waittime() = 0;
    virtual void   save_equipments(std::string response_block) = 0;
    virtual void   save_xcs(std::string response_block) = 0;

    static  void*  entry_point(PluginTl1_listen* self);
  };

/** Abstract Specyfic Part class
  * for the devices that communicates using TL1 protocol
  */
  class PluginTl1: public Plugin
  {
   public:

    /**  map of port with fixed parameters (useful for AP Data model)
     *   map<const uint32_t, tnrcsp_tl1_fixed_port_t>
     */

    friend class PluginTl1_listen;
    friend class PluginAdvaLSC_listen;
    friend class PluginCalientFSC_listen;

    PluginTl1();
    PluginTl1(std::string name);
    ~PluginTl1();

    int                          getFd();
    int                          default_retrive_interwall;
    int                          retrive_interwall;

    virtual tnrcapiErrorCode_t   probe (std::string location) = 0;

    ///add new fixed link parameters or modify existing fixed link parameters (usefull for AP Data model)

    void                         readT(thread *read);
    void                         retriveT(thread *retrive);
    int                          doRetrive();
    int                          doRead();

    void                         autonomous_handle(int handle, std::string* aid, std::string text, int state, void* parameters=NULL);


   protected:
    std::string      login;                       ///TL1 login
    std::string      password;                    ///TL1 password

    //AP data model parameters
    eqpt_id_t        data_model_id;               ///Equipment ID FIXIT now is equal to 1
    g2mpls_addr_t    data_model_address;          ///Equipment address displayed in data model
    eqpt_type_t      data_model_type;
    opstate_t        data_model_opstate;
    admstate_t       data_model_admstate;         ///Equipment administrative state dispalayed in data model
    std::string      data_model_location;         ///Equipment location displayed in data model

    TNRC::Eqpt                                             *e;
    /** TL1 receiced message map CTAG, MESSAGE */
    std::map<const int, std::string>                       message_map;

    /** label map TNRC_PORTIT_T CHANNEL_T */
    std::map<tnrc_portid_t, channel_t>                     label_map;
    /** list of cross connection */
    std::list<tnrcsp_lsc_xc_detail_t>                      xc_list;

    /** real device (TL1_socket) or simulator (TL1_simulator) */
    TL1                                                    *sock;

    std::string received_buffer;                  ///TL1 received buffer
    char *terminator;                             ///TL1 message terminators
    int  no_of_term_chars;                        ///number of TL1 terminator chars

    PluginTl1_listen                                       *listen_proc;
    int                                                    xc_id_;
    tnrc_sp_state                                          TL1_status;
    /**bacground process retrive activity: GET_EQUIPMENT, GET_XC */
    tnrc_sp_tl1_retrive_activity_t                         _activity;

    thread                                                 *t_read;
    thread                                                 *t_retrive;


    virtual int        tl1_log_in(std::string user, std::string passwd) =0;
    virtual void       get_all_equipment() = 0;
    virtual void       get_all_xc() = 0;

    int                tl1_connect();
    int                close_connection();
    tnrcsp_result_t    asynch_send(std::string* command, int ctag, int state, void* parameters=NULL);
    
    void               set_timer_event(int state, void* xc_params, int timeout);

    int                gen_identifier() {return identifier++;}

   private:
    int identifier;    ///TL1 CTAG
  };

/** Simulates behawiour of real TL1 device
  */
  class TL1_simulator: public TL1
  {
   public:
    int socket_desc_1;
    int socket_desc_2;
    TL1_simulator();
    ~TL1_simulator();

    int _connect();
    virtual int _send(std::string* mes);
    virtual int _recv(std::string* mes, int buff_size);

    int fd();
    int close_connection();
  };
}
#endif /* SOCKET_TL1_HEADER_FILE */
