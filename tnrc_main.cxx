//
//  This file is part of phosphorus-g2mpls.
//
//  Copyright (C) 2006, 2007, 2008, 2009 Nextworks s.r.l.
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
//  Giacomo Bernini       (Nextworks s.r.l.) <g.bernini_at_nextworks.it>
//  Gino Carrozzo         (Nextworks s.r.l.) <g.carrozzo_at_nextworks.it>
//  Nicola Ciulli         (Nextworks s.r.l.) <n.ciulli_at_nextworks.it>
//  Giodi Giorgi          (Nextworks s.r.l.) <g.giorgi_at_nextworks.it>
//  Francesco Salvestrini (Nextworks s.r.l.) <f.salvestrini_at_nextworks.it>
//




#include <zebra.h>

#include "version.h"
#include <getopt.h>
#include "command.h"
#include "thread.h"
#include "sigevent.h"
#include "privs.h"
#include "memory.h"

#include "tnrcd.h"
#include "tnrc_plugin_simulator.h"
#include "tnrc_plugin_tl1_adva.h"
#include "tnrc_plugin_tl1_calient.h"
#include "tnrc_plugin_ssh_foundry.h"
#include "tnrc_plugin_telnet_at_8000.h"
#include "tnrc_plugin_telnet_at_9424.h"
#include "tnrc_plugin_telnet_cls.h"
#include "tnrc_trace.h"
#include "tnrc_utils.h"
#include "tnrc_config.h"
#include "lib/corba.h"
#include "tnrc_corba.h"

// tnrcd privileges
zebra_capabilities_t _caps_p [] = {
	//ZCAP_NET_RAW,
	//ZCAP_BIND,
	//ZCAP_NET_ADMIN,
};

// GLOBALS
struct zebra_privs_t   tnrcd_privs;
struct quagga_signal_t tnrc_signals[4];
char *                 progname;

struct thread_master * master = NULL; // Master of threads

const char *           pid_file         = PATH_TNRCD_PID;
char                   config_default[] = TNRCD_DEFAULT_CONFIG;

struct option longopts[] = {
	{ "daemon",      no_argument,       NULL, 'd'},
	{ "config_file", required_argument, NULL, 'f'},
	{ "pid_file",    required_argument, NULL, 'i'},
	{ "iors_dir",    required_argument, NULL, 'o'},
	{ "dryrun",      no_argument,       NULL, 'C'},
	{ "help",        no_argument,       NULL, 'h'},
	{ "vty_port",    required_argument, NULL, 'P'},
	{ "user",        required_argument, NULL, 'u'},
	{ "group",       required_argument, NULL, 'g'},
	{ "version",     no_argument,       NULL, 'v'},
	{ 0 }
};


// SIG handlers
static void
sighup (void)
{
	TNRC_INF ("SIGHUP received");
}

static int terminate_execution = 0;

static void
sigint (void)
{
	TNRC_NOT ("Terminating on SIGINT");
	// tnrc_terminate();

	terminate_execution = 1;
	//exit(0);
}

// SIGUSR1 handler
static void
sigusr1 (void)
{
	zlog_rotate (NULL);
}



// Help information display
static void __attribute__ ((noreturn))
     usage (char *progname, int status)
{
	if (status != 0) {
		fprintf (stderr,
			 "Try `%s --help' for more information.\n",
			 progname);
	} else {
		printf ("Usage : %s [OPTION...]\n"
			"Daemon which manages TNRC.\n\n"
			"-d, --daemon       Runs in daemon mode\n"
			"-f, --config_file  Set configuration file name\n"
			"-i, --pid_file     Set process identifier file name\n"
			"-o, --iors_dir     Set IORs directory\n"
			"-C, --dryrun       Check configuration for validity and exit\n"
			"-P, --vty_port     Set vty's port number\n"
			"-u, --user         User to run as\n"
			"-g, --group        Group to run as\n"
			"-v, --version      Print program version\n"
			"-h, --help         Display this help and exit\n"
			"\n"
			"Report bugs to %s\n",
			progname, ZEBRA_BUG_ADDRESS);
	}

	exit(status);
}

int main(int argc, char ** argv)
{
	char *             p;
	int                vty_port;
	char *             vty_addr;
	int                daemon_mode;
	int                dryrun;
	char *             config_file;
	char *             iors_dir;
	struct thread      thread;
	Pcontainer         PC;
	tnrcapiErrorCode_t res;

	vty_port    = TNRCD_VTY_PORT;
	vty_addr    = NULL;
	daemon_mode = 0;
	dryrun      = 0;
	config_file = NULL;
	iors_dir    = NULL;

	// INITIALIZATION
#if defined(QUAGGA_USER) && defined(QUAGGA_GROUP)
	tnrcd_privs.user      = QUAGGA_USER;
	tnrcd_privs.group     = QUAGGA_GROUP;
#endif
#if defined(VTY_GROUP)
	tnrcd_privs.vty_group = VTY_GROUP;
#endif
	tnrcd_privs.caps_p    = _caps_p;
	tnrcd_privs.cap_num_p = sizeof(_caps_p)/sizeof(_caps_p[0]);
	tnrcd_privs.cap_num_i = 0;

	tnrc_signals[0].signal  = SIGHUP;
	tnrc_signals[0].handler = &sighup;
	tnrc_signals[1].signal  = SIGUSR1;
	tnrc_signals[1].handler = &sigusr1;
	tnrc_signals[2].signal  = SIGINT;
	tnrc_signals[2].handler = &sigint;
	tnrc_signals[3].signal  = SIGTERM;
	tnrc_signals[3].handler = &sigint;

	// Set umask before anything for security
	umask(0027);

	// Get program name
	progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);
	// Initialize the log subsystem
	zlog_default = openzlog(progname,
				ZLOG_TNRC,
				LOG_CONS | LOG_NDELAY | LOG_PID,
				LOG_DAEMON);

	// Create Master
	if (!TNRC_Master::init()) {
		TNRC_NOT("Cannot allocate a TNRC master object");
		exit (-1);
	}
	master = TNRC_MASTER.getMaster();

	// Declare all Plug-ins and fill Plug-ins' container
	Psimulator Sim(SIMULATOR_STRING);
	Plugin *S = &Sim;
	PC.attach (S->name(), S);

	S = new TNRC_SP_TL1::PluginAdvaLSC(ADVA_LSC_STRING);
	PC.attach (S->name(), S);

	S = new TNRC_SP_TL1::PluginCalientFSC(CALIENT_STRING);
	PC.attach (S->name(), S);

	S = new TNRC_SP_XMR::PluginXmr(FOUNDRY_L2SC_STRING);
	PC.attach (S->name(), S);

	S = new TNRC_SP_AT8000::PluginAT8000(AT8000_L2SC_STRING);
	PC.attach (S->name(), S);

	S = new TNRC_SP_AT9424::PluginAT9424(AT9424_L2SC_STRING);
	PC.attach (S->name(), S);

	S = new TNRC_SP_TELNET::PluginClsFSC(CLS_STRING);
	PC.attach (S->name(), S);

	// just reads the options from stdin
	while (1) {
		int opt;

		opt = getopt_long(argc, argv, "dChvf:P:u:g:i:o:", longopts, 0);

		if (opt == EOF) {
			break;
		}

		switch (opt) {
			case 0:
				break;
			case 'd':
				daemon_mode = 1;
				break;
			case 'C':
				dryrun = 1;
				break;
			case 'f':
				config_file = optarg;
				break;
			case 'i':
				pid_file = optarg;
				break;
			case 'o':
				iors_dir = optarg;
				break;
			case 'P':
				vty_port = atoi(optarg);
				break;
			case 'u':
				tnrcd_privs.user = optarg;
				break;
			case 'g':
				tnrcd_privs.group = optarg;
				break;
			case 'v':
				printf("TNRC daemon v.%s: vty@%d\n",
				       TNRC_CODE_VERSION, vty_port);
				printf("This program is part of "
				       "phosphorus-g2mpls\n");
				//print_version(progname);
				exit (0);
				break;
			case 'h':
				usage(progname, 0);
				break;
			default:
				usage(progname, 1);
				break;
		}
	}

	// Library inits.
	zprivs_init(&tnrcd_privs);
	signal_init(TNRC_MASTER.getMaster(),
		    Q_SIGC(tnrc_signals),
		    tnrc_signals);
	cmd_init(1);

	// these are all from libzebra
	vty_init(TNRC_MASTER.getMaster());
	memory_init();

	TNRC_MASTER.init_vty();
	TNRC_MASTER.pc(&PC);

	// sorting is needed by the command subsystem to finalize init
	sort_node();

	vty_read_config(config_file, config_default); // Get .conf file

	// Don't start execution if we are in dry-run mode
	if (dryrun) {
		return(0);
	}
	// Change to the daemon program.
	if (daemon_mode) {
		daemon(0, 0);
	}
	// Create VTY socket and start the TCP and unix socket listeners
	vty_serv_sock(vty_addr, vty_port, TNRC_VTYSH_PATH);

	TNRC_NOT ("TNRC daemon v.%s: starting... vty@%d",
		  TNRC_CODE_VERSION, vty_port);

	if (TNRC_MASTER.test_mode()) {
		thread_add_event (TNRC_MASTER.getMaster(),
				  read_test_file,
				  NULL,
				  0);
	}

#if HAVE_OMNIORB
	if (!corba_init(iors_dir,
			CORBA_SERVANT_TNRC,
			TNRC_MASTER.getMaster())) {
		TNRC_ERR("Cannot initialize CORBA subsystem");
		exit(1);
	}
	if (!corba_server_setup()) {
		TNRC_ERR("Cannot setup CORBA subsystem");
		exit(1);
	}
	TNRC_DBG("TNRC CORBA server setup ... OK");
#endif

	// Process id file create.
	pid_output(pid_file);

	while (!terminate_execution) {
		if (thread_fetch(TNRC_MASTER.getMaster(), &thread)) {
			thread_call(&thread);
		}
	}

	zlog_debug("Termination in progress ...");

#if HAVE_OMNIORB
	(void) corba_server_shutdown();
	(void) corba_fini();
#endif

	return 0;
}

/* tnrcd_main.cxx ends */
