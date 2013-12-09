#
#   This file is part of phosphorus-g2mpls.
#
#   Copyright (C) 2006, 2007, 2008, 2009 Nextworks s.r.l.
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#   Authors:
#
#   Giacomo Bernini       (Nextworks s.r.l.) <g.bernini_at_nextworks.it>
#   Gino Carrozzo         (Nextworks s.r.l.) <g.carrozzo_at_nextworks.it>
#   Nicola Ciulli         (Nextworks s.r.l.) <n.ciulli_at_nextworks.it>
#   Giodi Giorgi          (Nextworks s.r.l.) <g.giorgi_at_nextworks.it>
#   Francesco Salvestrini (Nextworks s.r.l.) <f.salvestrini_at_nextworks.it>
#


## Process this file with automake to produce Makefile.in.

INCLUDES = @INCLUDES@ -I.. -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/idl @LIBG2MPLS_INCLUDES@ @SNMP_INCLUDES@
DEFS = @DEFS@ $(LOCAL_OPTS) -DSYSCONFDIR=\"$(sysconfdir)/\"
INSTALL_SDATA=@INSTALL@ -m 600

lib_LTLIBRARIES      = libtnrc.la
libtnrc_la_LDFLAGS   = -version 0:0:0

sbin_PROGRAMS = tnrcd

if CORBA
noinst_LIBRARIES = libcorba.a
libcorba_a_CFLAGS   = $(OMNIORB_CFLAGS)
libcorba_a_SOURCES  =				\
	../idl/typesSK.cc			\
	../idl/g2mplsTypesSK.cc			\
	../idl/g2rsvpteSK.cc			\
	../idl/lrmSK.cc				\
	../idl/tnrcSK.cc			\
	tnrc_corba.cxx
endif

tnrc_action_FSM =				\
	tnrc_action.cxx				\
	tnrc_action_gen.cxx

tnrc_action_headers_FSM =			\
	tnrc_action.h				\
	tnrc_action_gen.h

tnrcdheaderdir = $(pkgincludedir)/tnrcd

noinst_HEADERS =				\
	tnrc_action_objects.h			\
	tnrc_apis.h				\
	tnrc_common_types.h			\
	tnrc_config.h				\
	tnrc_dm.h				\
	tnrc_g2rsvpte_api.h			\
	tnrc_lrm_api.h				\
	tnrc_plugin.h				\
	tnrc_plugin_ssh_foundry.h		\
        tnrc_plugin_telnet_at_8000.h		\
	tnrc_plugin_telnet_at_9424.h		\
	tnrc_plugin_simulator.h			\
	tnrc_plugin_tl1.h			\
	tnrc_plugin_tl1_adva.h			\
	tnrc_plugin_tl1_adva_simulator.h	\
	tnrc_plugin_tl1_calient.h		\
        tnrc_plugin_telnet.h                    \
        tnrc_plugin_telnet_cls.h                \
	tnrc_sp.h				\
	tnrc_trace.h				\
	tnrc_utils.h				\
	tnrcd.h					\
	$(tnrc_action_headers_FSM)

if CORBA
noinst_HEADERS += tnrc_corba.h
endif

libtnrc_la_SOURCES =				\
	tnrc_plugin.cxx				\
	tnrc_plugin_simulator.cxx		\
	tnrc_plugin_ssh_foundry.cxx		\
        tnrc_plugin_telnet_at_8000.cxx		\
	tnrc_plugin_telnet_at_9424.cxx		\
	tnrc_plugin_tl1.cxx			\
	tnrc_plugin_tl1_adva.cxx		\
	tnrc_plugin_tl1_calient.cxx		\
	tnrc_plugin_tl1_adva_simulator.cxx	\
        tnrc_plugin_telnet.cxx                  \
        tnrc_plugin_telnet_cls.cxx              \
      	$(tnrc_action_FSM)



tnrcd_SOURCES  = 				\
	tnrcd.cxx				\
	tnrc_common_types.cxx			\
	tnrc_dm.cxx				\
	tnrc_vty.cxx				\
	tnrc_apis.cxx				\
	tnrc_action_objects.cxx			\
	tnrc_lrm_api.cxx			\
	tnrc_g2rsvpte_api.cxx			\
	tnrc_main.cxx

tnrcd_LDADD    =				\
	libtnrc.la				\
	../lib/libzebra.la			\
	$(LIBG2MPLS)				\
	$(G2MPLS_LIBS)				\
	@LIBCAP@

if HAVE_LSSH2
tnrcd_LDADD +=                                  \
        -lssh2
endif

if CORBA
tnrcd_LDADD  +=					\
	libcorba.a
endif

do_subst = sed					\
  -e 's,[@]LOGFILEDIR[@],${quagga_statedir},g'	\
  -e 's,[@]CONFDIR[@],${sysconfdir},g'

tnrcd.conf.sample: Makefile tnrcd.conf.sample.in
	( echo "Creating $@" && \
	  rm -f $@ $@.tmp && \
	  $(do_subst) < tnrcd.conf.sample.in > $@.tmp && \
	  mv -f $@.tmp $@ ) || { \
		rm -f $@ $@.tmp ; \
		exit 1 ; \
	}

examplesdir = $(exampledir)
dist_examples_DATA = 				\
	tnrcd.conf.sample			\
	tnrcd.eqpt.sim.sample

CLEANFILES = *.pid *.log tnrcd.conf.sample *.vty
