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
#include "tnrc_sp.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "tnrc_plugin_tl1.h"
#include "tnrc_plugin_tl1_adva_simulator.h"


namespace TNRC_SP_TL1_ADVA_SIMULATOR
{

  int match_2(std::string* data, std::string pattern, int token_num, std::string tokens[])
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

  void find_and_replace(std::string &source, const std::string find, std::string replace )
  {
    size_t j;
    for(;(j = source.find(find)) != std::string::npos;)
    {
      source.replace( j, find.length(), replace );
    }
  }

  bool _find(std::string str, char ch)
  {
    std::string::iterator it;
    it = str.end();
    it = it - 2;
    if (*it == '-')
    {
      it++;
      if (*it == ch) return true;
    }
    return false;
  }

  std::string _last(std::string str)
  {
    std::string temp;
    int size = str.size();
    temp = str.substr(0,size-2);
    return temp;
  }

/** class TL1_simulator.
  * Simulates TL1_device behaviour
  */
  TL1_adva_simulator::TL1_adva_simulator()
  {
    init();
    TNRC_DBG("***TL1_adva_simulator***");

    //remote_address = "150.254.170.153";
    remote_address = "127.0.0.1";
    remote_port = 3082;

    socket_desc_1 = socket(PF_INET, SOCK_DGRAM, 0);
    to.sin_family         = PF_INET;
    to.sin_port           = htons(remote_port);
    inet_aton(remote_address.c_str(), &to.sin_addr);


    socket_desc_2 = socket(PF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family=PF_INET;
    addr.sin_port=htons(remote_port);
    addr.sin_addr.s_addr=INADDR_ANY;

    con = bind(socket_desc_2, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
  }

  TL1_adva_simulator::~TL1_adva_simulator()
  {
  }

  int TL1_adva_simulator::_send(std::string* mes)
  {
    std::string mess = *mes;
    if (mess.size() > 1)
    {
      prepare_response(&mess,patterns_headers);
      return 0;
    }
    return -1;
  }
  std::string TL1_adva_simulator::_r()
  {
    std::string mess;
    mess.append("\n");
    while (!resp.empty())
    {
      mess.append(resp.front().c_str());
      resp.pop_front();
    }
    mess.append("\n;");
    return mess;
  }

  int TL1_adva_simulator::_recv(std::string* mes, int buff_size)
  {
    struct sockaddr_in from;
    socklen_t fromlen;
    con = recvfrom(socket_desc_2, buf, 1024, 0, (struct sockaddr *) &from, &fromlen);
    *mes = buf;
  }

  void TL1_adva_simulator::init()
  {
    logged_in = false;
    insert_patterns(patterns_headers, answer_patterns);
    init_eqt_list();
    init_xc_map();
    init_users_list();
  }

  void TL1_adva_simulator::insert_patterns(std::string *p_h, std::string *a_p)
  {
    p_h[0] = "ACT-USER";
    p_h[1] = "ASG-CHANNEL";
    p_h[2] = "DLT-CHANNEL";
    p_h[3] = "RST-CHANNEL";
    p_h[4] = "RMV-CHANNEL";
    p_h[5] = "RTRV-CHANNEL";
    p_h[6] = "RTRV-CHANNEL";
    p_h[7] = "RTRV-EQPT";
    p_h[8] = "RTRV-EQPT-ALL";

    a_p[0] = "M  %s COMPLD";
    a_p[1] = "M  %s DENY";
  }

  void TL1_adva_simulator::init_eqt_list()
  {
    eqpt _equipment1 = {"1-1-1","PWRFLTRA","ENABLED","IS-NR","","","NORMAL","",""};
    equipment.push_back(_equipment1);

    eqpt _equipment2 = {"1-1-2","USP","ENABLED","IS-NR","ACT","","NORMAL","",""};
    equipment.push_back(_equipment2);

    eqpt _equipment3 = {"1-1-3","R2UTIL","ENABLED","IS-NR","","","NORMAL","",""};
    equipment.push_back(_equipment3);

    eqpt _equipment4 = {"1-1-5","PWRFLTRB","ENABLED","IS-NR","","","NORMAL","",""};
    equipment.push_back(_equipment4);

    eqpt _equipment5 = {"1-1-8","OLDUEV20 1310","ENABLED","IS-NR","","","NORMAL","",""};
    equipment.push_back(_equipment5);

    eqpt _equipment6 = {"1-1-9","XPT10GLO 1532.68","ENABLED,IS-NR","","DISABLED","NORMAL","56","1532.68 nm"};
    equipment.push_back(_equipment6);

    eqpt _equipment7 = {"1-1-13","OLDUEV20 1310","ENABLED","IS-NR","","","NORMAL","",""};
    equipment.push_back(_equipment7);

    eqpt _equipment8 = {"1-1-14","XPT10GLO 1534.25","ENABLED","IS-NR","","DISABLED","NORMAL","54","1534.25 nm"};
    equipment.push_back(_equipment8);

    eqpt _equipment9 = {"1-1-16","XPT10GLO 1546.12","ENABLED","IS-NR","","DISABLED","NORMAL","39","1546.12 nm"};
    equipment.push_back(_equipment9);

    eqpt _equipment10 = {"1-2-1","PWRFLTRA","ENABLED","IS-NR","","","NORMAL","",""};
    equipment.push_back(_equipment10);

    eqpt _equipment11 = {"1-2-2","USP","ENABLED","IS-NR","ACT","","NORMAL","",""};
    equipment.push_back(_equipment11);

    eqpt _equipment12 = {"1-2-3","R2UTIL","ENABLED","IS-NR","","","NORMAL","",""};
    equipment.push_back(_equipment12);

    eqpt _equipment13 = {"1-2-5","PWRFLTRB","ENABLED","IS-NR","","","NORMAL","",""};
    equipment.push_back(_equipment13);

    eqpt _equipment14 = {"1-2-13","VG2020PA","ENABLED","IS-NR","","","NORMAL","",""};
    equipment.push_back(_equipment14);

    eqpt _equipment15 = {"1-3-1","EROADM","ENABLED","IS-NR","ACT","","NORMAL","",""};
    equipment.push_back(_equipment15);

    eqpt _equipment16 = {"1-4-1","EROADM","ENABLED","IS-NR","ACT","","NORMAL","",""};
    equipment.push_back(_equipment16);

  }

  void TL1_adva_simulator::init_xc_map()
  {
    xc_tuple xt = {"1-1-13-2","23"};
    xc x = {"1-1-8-1","1558.98","PASSTHRU","Connected-OOS","TL1","OOS"};
    xc_map_2[xt] = x;

    xc_tuple xt2 = {"1-1-13-2","45"};
    xc x2 = {"1-1-8-1","1541.35","PASSTHRU","EQ-Failure","TL1","IS"};
    xc_map_2[xt2] = x2;
  }

  void TL1_adva_simulator::init_users_list()
  {
    user _user;
    _user.login = "user";
    _user.passwd = "****";
    users.push_back(_user);
    user _user2;
    _user2.login = "admin";
    _user2.passwd = "admin";
    users.push_back(_user2);
  }

  void TL1_adva_simulator::prepare_response(std::string *message, std::string *p_h)
  {
    std::string temp = *message;
    if(match_2(message, ":::", 4, tokens) != 4)
          return;
    std::string command = tokens[0];
    std::string all = tokens[2];
    if (p_h[0] == command) process_act_user(&temp);
    else if(p_h[1] == command) process_asg_channel(&temp);
    else if(p_h[2] == command) process_dlt_channel(&temp);
    else if(p_h[3] == command) process_rst_channel(&temp);
    else if(p_h[4] == command) process_rmv_channel(&temp);
    else if(p_h[5] == command && all != "ALL") process_rtrv_channel(&temp);
    else if(p_h[6] == command && all == "ALL") process_channel_all_patt(&temp);
//  else if(p_h[7] == command) process_rtrv_eqpt(&temp);
    else if(p_h[8] == command) process_rtrv_eqpt_all(&temp);
    else
    {
      printf("unknow message\n");
      return;
    }
  }

  void TL1_adva_simulator::process_act_user(std::string *message)
  {
    std::string temp = *message;
    if(match_2(message, ":::::;", 7, tokens) != 7)
        return;
    std::string user = tokens[2];
    std::string ctag = tokens[3];
    std::string pass_opt = tokens[5];
    bool pos_log = false;

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char buff[1000];

    for (it=users.begin(); it!=users.end(); it++)
    {
      if((it->login) == user && (it->passwd) == pass_opt)
      {
        pos_log = true;
        break;
      }
    }

    if (pos_log)
    {
      sprintf(buff,"  CRACOW %s%s\n  \"%s:lastlog, attempts\"\n  /*\n  System Vendor:  ...\n  System Name:    ...\n  System Type:    ...\n  Active SP Load: ...\n  User Privilege: ... (%s)\n  */",asctime (timeinfo),answer_patterns[0].c_str(),user.c_str(),user.c_str());
      std::string mes = "";
      mes.append(buff);
      find_and_replace(mes,"%s",ctag);
      resp.push_back(mes);
      logged_in = true;
    }
    else
    {
      sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
      std::string mes = "";
      mes.append(buff);
      find_and_replace(mes,"%s",ctag);
      resp.push_back(mes);
    }
    std::string send = _r();
    const char* s = (const char*)send.c_str();
    int con = sendto(socket_desc_1, s, strlen(s), 0, (struct sockaddr *) &to, sizeof(struct sockaddr_in));
  }

  void TL1_adva_simulator::process_asg_channel(std::string *message)
  {
    if(match_2(message, ":::::,,,;", 10,tokens) != 10)
        return;
    std::string ctag = tokens[3];
    std::string ingress_aid = tokens[5];
    std::string egress_aid = tokens[6];
    std::string channel = tokens[7];
    std::string conn_type = tokens[8];

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char buff[1000];

    if (logged_in)
    {
      xc_tuple xt;
      xt.egress_aid = egress_aid;
      xt.channel = channel;
      std::string error = check_params(xt,ingress_aid, egress_aid, channel, conn_type);
      if (error.size() > 0)
      {
        sprintf(buff,"  CRACOW %s%s\n   SROF\n",asctime (timeinfo),answer_patterns[1].c_str());
        std::string mes = "";
        mes.append(buff);
        find_and_replace(mes,"%s",ctag);
        resp.push_back(mes+error);
      }
      else
      {
        sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[0].c_str());
        std::string mes = "";
        mes.append(buff);
        find_and_replace(mes,"%s",ctag);
        resp.push_back(mes);

        xc x;
        x.ingress_aid = ingress_aid;
        x.connectionStatus = "OOS-EqOOS";
        x.status = "OOS";
        x.connectionType = conn_type;
        xc_map_2[xt] = x;
      }
    }
    else
    {
      sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
      std::string mes = "";
      mes.append(buff);
      find_and_replace(mes,"%s",ctag);
      resp.push_back(mes);
    }
    std::string send = _r();
    const char* s = (const char*)send.c_str();
    int con = sendto(socket_desc_1, s, strlen(s), 0, (struct sockaddr *) &to, sizeof(struct sockaddr_in));
  }

  void TL1_adva_simulator::process_dlt_channel(std::string *message)
  {
    if(match_2(message, ":::::,,;", 9,tokens) != 9)
        return;
    std::string ctag = tokens[3];
    std::string egress_aid = tokens[6];
    std::string channel = tokens[7];

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char buff[1000];
    bool f = true;

    if (logged_in)
    {
      xc_tuple xt;
      xt.egress_aid = egress_aid;
      xt.channel = channel;

      std::map<xc_tuple,xc>::iterator ite;
      for (ite=xc_map_2.begin(); ite!=xc_map_2.end(); ite++)
      {
        if (ite->first.egress_aid == xt.egress_aid && ite->first.channel == xt.channel )
        {
          if (ite->second.status == "OOS")
          {
            sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[0].c_str());
            std::string mes = "";
            mes.append(buff);
            find_and_replace(mes,"%s",ctag);
            resp.push_back(mes);
            xc_map_2.erase(ite);
            f = false;
          }
          else
          {
            sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
            std::string mes = "";
            mes.append(buff);
            find_and_replace(mes,"%s",ctag);
            std::string error = "\n   /* Cannot delete channel when IS status */";
            resp.push_back(mes + error);
            f = false;
          }
        }
      }
      if (f)
      {
        sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
        std::string mes = "";
        mes.append(buff);
        find_and_replace(mes,"%s",ctag);
        resp.push_back(mes);
      }
    }
    else
    {
      sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
      std::string mes = "";
      mes.append(buff);
      find_and_replace(mes,"%s",ctag);
      resp.push_back(mes);
    }
    std::string send = _r();
    const char* s = (const char*)send.c_str();
    int con = sendto(socket_desc_1, s, strlen(s), 0, (struct sockaddr *) &to, sizeof(struct sockaddr_in));
  }

  void TL1_adva_simulator::process_rst_channel(std::string *message)
  {
    if(match_2(message, ":::::;", 7,tokens) != 7)
        return;
    std::string ctag = tokens[3];
    std::string egress_aid = tokens[2];
    std::string channel = tokens[5];

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char buff[1000];
    bool f = true;

    if (logged_in)
    {
      xc_tuple xt;
      xt.egress_aid = egress_aid;
      xt.channel = channel;

      std::map<xc_tuple,xc>::iterator ite;
      for (ite=xc_map_2.begin(); ite!=xc_map_2.end(); ite++)
      {
        if(ite->first.egress_aid == xt.egress_aid && ite->first.channel == xt.channel)
        {
          for(int j=0;j<1000;j++) buff[j] = 0;
          sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[0].c_str());
          std::string mes = "";
          mes.append(buff);
          find_and_replace(mes,"%s",ctag);
          resp.push_back(mes);
          ite->second.status = "IS";
          ite->second.connectionStatus = "Equalized-IS";
          f = false;
        }
      }
      if (f)
      {
        for(int j=0;j<1000;j++) buff[j] = 0;
        sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
        std::string mes = "";
        mes.append(buff);
        find_and_replace(mes,"%s",ctag);
        resp.push_back(mes);
      }
    }
    else
    {
      for(int j=0;j<1000;j++) buff[j] = 0;
      sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
      std::string mes = "";
      mes.append(buff);
      find_and_replace(mes,"%s",ctag);
      resp.push_back(mes);
    }
    std::string send = _r();
    const char* s = (const char*)send.c_str();
    int con = sendto(socket_desc_1, s, strlen(s), 0, (struct sockaddr *) &to, sizeof(struct sockaddr_in));
  }

  void TL1_adva_simulator::process_rmv_channel(std::string *message)
  {
    if(match_2(message, ":::::;", 7,tokens) != 7)
        return;
    std::string ctag = tokens[3];
    std::string egress_aid = tokens[2];
    std::string channel = tokens[5];

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char buff[1000];
    bool f = true;

    if (logged_in)
    {
      xc_tuple xt;
      xt.egress_aid = egress_aid;
      xt.channel = channel;

      std::map<xc_tuple,xc>::iterator ite;
      for (ite=xc_map_2.begin(); ite!=xc_map_2.end(); ite++)
      {
        if(ite->first.egress_aid == xt.egress_aid && ite->first.channel == xt.channel)
        {
          for(int j=0;j<1000;j++) buff[j] = 0;
          sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[0].c_str());
          std::string mes = "";
          mes.append(buff);
          find_and_replace(mes,"%s",ctag);
          resp.push_back(mes);
          ite->second.status = "OOS";
          ite->second.connectionStatus = "OOS-EqOOS";
          f = false;
        }
      }
      if (f)
      {
        for(int j=0;j<1000;j++) buff[j] = 0;
        sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
        std::string mes = "";
        mes.append(buff);
        find_and_replace(mes,"%s",ctag);
        resp.push_back(mes);
      }
    }
    else
    {
      for(int j=0;j<1000;j++) buff[j] = 0;
      sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
      std::string mes = "";
      mes.append(buff);
      find_and_replace(mes,"%s",ctag);
      resp.push_back(mes);
    }
    std::string send = _r();
    const char* s = (const char*)send.c_str();
    int con = sendto(socket_desc_1, s, strlen(s), 0, (struct sockaddr *) &to, sizeof(struct sockaddr_in));
  }

  void TL1_adva_simulator::process_rtrv_channel(std::string *message)
  {
    if(match_2(message, ":::::;", 7,tokens) != 7)
        return;
    std::string ctag = tokens[3];
    std::string egress_aid = tokens[2];
    std::string channel = tokens[5];

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char buff[1000];
    bool f = true;

    if (logged_in)
    {
      xc_tuple xt;
      xt.egress_aid = egress_aid;
      xt.channel = channel;

      std::map<xc_tuple,xc>::iterator ite;
      for (ite=xc_map_2.begin(); ite!=xc_map_2.end(); ite++)
      {
        if(ite->first.egress_aid == xt.egress_aid && ite->first.channel == xt.channel)
        {
          for(int j=0;j<1000;j++) buff[j] = 0;
          sprintf(buff,"  CRACOW %s%s\n   \"%s,%s:%s,%s,%s,%s,%s,%s\"",asctime (timeinfo),answer_patterns[0].c_str(),ite->second.ingress_aid.c_str(), ite->first.egress_aid.c_str(), ite->first.channel.c_str(), ite->second.wavelength.c_str(), ite->second.connectionType.c_str(), ite->second.connectionStatus.c_str(), ite->second.owner.c_str(), ite->second.status.c_str());
          std::string mes = "";
          mes.append(buff);
          find_and_replace(mes,"%s",ctag);
          resp.push_back(mes);
          f = false;
        }
      }
      if (f)
      {
        sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[0].c_str());
        std::string mes = "";
        mes.append(buff);
        find_and_replace(mes,"%s",ctag);
        resp.push_back(mes + "\n   /* No channels found */");
      }
    }
    else
    {
      sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
      std::string mes = "";
      mes.append(buff);
      find_and_replace(mes,"%s",ctag);
      resp.push_back(mes);
    }
    std::string send = _r();
    const char* s = (const char*)send.c_str();
    int con = sendto(socket_desc_1, s, strlen(s), 0, (struct sockaddr *) &to, sizeof(struct sockaddr_in));
  }

  void TL1_adva_simulator::process_channel_all_patt(std::string *message)
  {
    if(match_2(message, ":::;", 5,tokens) != 5)
        return;
    std::string ctag = tokens[3];

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char buff[1000];

    if (logged_in)
    {
      if (xc_map_2.size() > 0)
      {
        std::string mes = "";
        sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[0].c_str());
        mes.append(buff);
        find_and_replace(mes,"%s",ctag);

        std::string m;
        std::map<xc_tuple,xc>::iterator ite;
        for (ite=xc_map_2.begin(); ite!=xc_map_2.end(); ite++)
        {
          for(int j=0;j<1000;j++) buff[j] = 0;
          sprintf(buff,"\n   \"%s,%s:%s,%s,%s,%s,%s,%s\"",ite->second.ingress_aid.c_str(), ite->first.egress_aid.c_str(), ite->first.channel.c_str(), ite->second.wavelength.c_str(), ite->second.connectionType.c_str(), ite->second.connectionStatus.c_str(), ite->second.owner.c_str(), ite->second.status.c_str());
          m.append(buff);
        }
        resp.push_back(mes + m);
      }
      else
      {
        sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[0].c_str());
        std::string mes = "";
        mes.append(buff);
        find_and_replace(mes,"%s",ctag);
        resp.push_back(mes + "\n   /* No channels found */");
      }
    }
    else
    {
      sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
      std::string mes = "";
      mes.append(buff);
      find_and_replace(mes,"%s",ctag);
      resp.push_back(mes);
    }
    std::string send = _r();
    const char* s = (const char*)send.c_str();
    int con = sendto(socket_desc_1, s, strlen(s), 0, (struct sockaddr *) &to, sizeof(struct sockaddr_in));
  }

  void TL1_adva_simulator::process_rtrv_eqpt(std::string *message)
  {

  }

  void TL1_adva_simulator::process_rtrv_eqpt_all(std::string *message)
  {
    if(match_2(message, ":::;", 5,tokens) != 5)
        return;
    std::string ctag = tokens[3];

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char buff[1000];

    if (logged_in)
    {
      std::string mes = "";
      sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[0].c_str());
      mes.append(buff);
      find_and_replace(mes,"%s",ctag);

      std::list<eqpt>::iterator i;
      std::string m;
      for (i=equipment.begin(); i!=equipment.end(); i++)
      {
        for(int j=0;j<1000;j++) buff[j] = 0;
        sprintf(buff,"\n   \"%s:%s,%s,%s,%s,%s,%s,%s,%s\"",i->aid.c_str() ,i->_typeid.c_str() ,i->alarmadminstate.c_str() ,i->primarystate.c_str() ,i->secondarystate.c_str() ,i->regenmode.c_str() ,i->portcontrol.c_str() ,i->channelID.c_str() ,i->wavelength.c_str());
        m.append(buff);
      }
      resp.push_back(mes + m);
    }
    else
    {
      sprintf(buff,"  CRACOW %s%s",asctime (timeinfo),answer_patterns[1].c_str());
      std::string mes = "";
      mes.append(buff);
      find_and_replace(mes,"%s",ctag);
      resp.push_back(mes);
    }
    std::string send = _r();
    const char* s = (const char*)send.c_str();
    int con = sendto(socket_desc_1, s, strlen(s), 0, (struct sockaddr *) &to, sizeof(struct sockaddr_in));
  }

  std::string TL1_adva_simulator::check_params(xc_tuple xt, std::string ingress_aid, std::string egress_aid, std::string channel, std::string conn_type)
  {
    int tr;
    if(atoi(channel.c_str()) < 19 || atoi(channel.c_str()) > 61) return "   /* Incorrect channel ID */";
    std::map<xc_tuple,xc>::iterator ite;
    for (ite=xc_map_2.begin(); ite!=xc_map_2.end(); ite++)
    {
      if(ite->first.egress_aid == xt.egress_aid && ite->first.channel == xt.channel) return "   /* Channel has already been assigned */";
    }
    std::list<eqpt>::iterator i;
    std::list<std::string>::iterator ii;
    for (i=equipment.begin(); i!=equipment.end(); i++)
    {
      if(i->_typeid.find("OLD") != -1) old_eqpt.push_back(i->aid);
      else if (i->_typeid.find("XPT") != -1) xpt_eqpt.push_back(i->aid);
    }
    if (conn_type == "PASSTHRU")
    {
      if (_find(ingress_aid,'1') && _find(egress_aid,'2'))
      {
        for (ii=old_eqpt.begin(); ii!=old_eqpt.end(); ii++)
        {
          if(*ii == _last(ingress_aid))
          {
            for (ii=old_eqpt.begin(); ii!=old_eqpt.end(); ii++)
            {
              if (*ii == _last(egress_aid)) return "";
            }
            break;
          }
        }
      }
    }
    else if (conn_type == "ADD")
    {
      if (_find(ingress_aid,'1') && _find(egress_aid,'2'))
      {
        for (ii=xpt_eqpt.begin(); ii!=xpt_eqpt.end(); ii++)
        {
          if(*ii == _last(ingress_aid))
          {
            for (ii=old_eqpt.begin(); ii!=old_eqpt.end(); ii++)
            {
              if(*ii == _last(egress_aid)) return "";
            }
            break;
          }
        }
      }
    }
    else if (conn_type == "DROP")
    {
      if (_find(ingress_aid,'1') && _find(egress_aid,'2'))
      {
        for (ii=old_eqpt.begin(); ii!=old_eqpt.end(); ii++)
        {
          if(*ii == _last(ingress_aid))
          {
            for (ii=xpt_eqpt.begin(); ii!=xpt_eqpt.end(); ii++)
            {
              if(*ii == _last(egress_aid)) return "";
            }
            break;
          }
        }
      }
    }
    return "   /* Incorrect AID or channel ID*/";
  }

}
