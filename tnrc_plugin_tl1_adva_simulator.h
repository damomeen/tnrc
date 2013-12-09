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



#ifndef TNRC_PLUGIN_ADVA_SIMULATOR_H
#define TNRC_PLUGIN_ADVA_SIMULATOR_H

#include "tnrc_sp.h"
#include "tnrc_plugin_tl1.h"

namespace TNRC_SP_TL1_ADVA_SIMULATOR
{

  struct eqpt
  {
    std::string aid;
    std::string _typeid;
    std::string alarmadminstate;
    std::string primarystate;
    std::string secondarystate;
    std::string regenmode;
    std::string portcontrol;
    std::string channelID;
    std::string wavelength;
  };

  struct user
  {
    std::string login;
    std::string passwd;
  };

  struct xc
  {
    std::string ingress_aid;
    std::string wavelength;
    std::string connectionType;
    std::string connectionStatus;
    std::string owner;
    std::string status;
  };

  struct xc_tuple
  {
    std::string egress_aid;
    std::string channel;
  };


  struct xc_map_compa
  {
    bool operator() (const xc_tuple& lhs, const xc_tuple& rhs) const
    {
        if(lhs.egress_aid == rhs.egress_aid)
        {
            if(lhs.channel == rhs.channel)
                return false;
        }
        else return true;
    }
  };

/** Simulates behawiour of real device
  */

  class TL1_adva_simulator: public TNRC_SP_TL1::TL1_simulator
  {
   private:
    void init();
    void insert_patterns(std::string *,std::string *);
    void init_eqt_list();
    void init_xc_map();
    void init_users_list();
    int _s(std::string* mes);
    std::string _r();

    void prepare_response(std::string *, std::string *);
    void process_act_user(std::string *);
    void process_asg_channel(std::string *);
    void process_dlt_channel(std::string *);
    void process_rst_channel(std::string *);
    void process_rmv_channel(std::string *);
    void process_rtrv_channel(std::string *);
    void process_channel_all_patt(std::string *);
    void process_rtrv_eqpt(std::string *);
    void process_rtrv_eqpt_all(std::string *);
    std::string check_params(xc_tuple , std::string , std::string , std::string , std::string );

    bool logged_in;
    std::string remote_address;
    unsigned int remote_port;
    struct sockaddr_in to;
    int con;
    char buf[1024];

    std::string tokens[10];
    std::list<std::string> resp;
    std::string patterns_headers[9];
    std::string answer_patterns[2];
    std::list<eqpt> equipment;
    std::list<user> users;
    std::map<xc_tuple,xc,xc_map_compa> xc_map_2;
    std::list<std::string> old_eqpt;
    std::list<std::string> xpt_eqpt;
    std::list<user>::iterator it;

   public:
    TL1_adva_simulator();
    ~TL1_adva_simulator();

    int _send(std::string* mes);
    int _recv(std::string* mes, int buff_size);
  };
}

#endif /* TNRC_PLUGIN_ADVA_SIMULATOR_H */
