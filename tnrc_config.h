/*
 *  This file is part of phosphorus-g2mpls.
 *
 *  Copyright (C) 2006, 2007, 2008, 2009 Nextworks s.r.l.
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
 *  Giacomo Bernini       (Nextworks s.r.l.) <g.bernini_at_nextworks.it>
 *  Gino Carrozzo         (Nextworks s.r.l.) <g.carrozzo_at_nextworks.it>
 *  Nicola Ciulli         (Nextworks s.r.l.) <n.ciulli_at_nextworks.it>
 *  Giodi Giorgi          (Nextworks s.r.l.) <g.giorgi_at_nextworks.it>
 *  Francesco Salvestrini (Nextworks s.r.l.) <f.salvestrini_at_nextworks.it>
 */



#ifndef TNRC_CONFIG_H
#define TNRC_CONFIG_H

#define TNRCD_DEFAULT_CONFIG   "/usr/local/etc/tnrcd.conf"
//#define TNRC_TNRCD_PID         "/var/run/quagga/tnrcd.pid"
#define TNRCD_VTY_PORT         2613
//#define TNRC_VTYSH_PATH        "/var/run/quagga/tnrcd.vty"
#define TNRC_CODE_VERSION      "0.1"

#define EQPTID_TO_DLID_MASK    0x3F
#define BOARDID_TO_DLID_MASK   0x3FF
#define PORTID_TO_DLID_MASK    0xFFFF

#define DLID_TO_EQPTID_MASK    0xFC000000
#define DLID_TO_BOARDID_MASK   0x03FF0000
#define DLID_TO_PORTID_MASK    0x0000FFFF

#define EQPTID_SHIFT           26
#define BOARDID_SHIFT          16

#define SIMULATOR_STRING       "simulator"
#define CALIENT_STRING         "calient_fsc"
#define ADVA_LSC_STRING        "adva_lsc"
#define FOUNDRY_L2SC_STRING    "foundry_l2sc"
#define AT8000_L2SC_STRING     "at8000_l2sc"
#define AT9424_L2SC_STRING     "at9424_l2sc"
#define CLS_STRING             "cls_fsc"

#define SIMULATOR_WQ           "sim_WQ"
#define ADVA_WQ                "adva_WQ"
#define FOUNDRY_WQ             "foundry_WQ"
#define AT8000_WQ              "at8000_WQ"
#define AT9424_WQ              "at9424_WQ"
#define CLS_WQ                 "cls_WQ"

#define DEFAULT_BASIC_BW_UNIT  1
#define FSC_BASIC_BW_UNIT      1
#define LSC_BASIC_BW_UNIT      BwEnc_10GigELAN

#define MAX_AT_ACT_RETRY       5
#define RETRY_TIMER            1 //SECONDS

//advance reservation calendar utilities
#define TNRC_DEFAULT_QUANTUM    60 //SECONDS
#define TNRC_DEFAULT_NUM_EVENTS 10

//#define UBUNTU_OPERATOR_PATCH

#endif // TNRC_CONFIG_H
