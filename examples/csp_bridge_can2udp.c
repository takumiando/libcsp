#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/interfaces/csp_if_udp.h>

#define RUN_DURATION_IN_SEC	(3)
#define DEFAULT_CAN_NAME	"can0"
#define DEFAULT_UDP_HOST	"127.0.0.1"
#define DEFAULT_UDP_LPORT	(0)
#define DEFAULT_UDP_RPORT	(0)

/* These three functions must be provided in arch specific way */
int bridge_start(void);

extern csp_conf_t csp_conf;

static struct option long_options[] = {
	{"can", required_argument, 0, 'c'},
	{"host", required_argument, 0, 'H'},
	{"lport", required_argument, 0, 'l'},
	{"rport", required_argument, 0, 'r'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};

void csp_input_hook(csp_iface_t * iface, csp_packet_t * packet) {
	char dst_name[] = "UDP";

	if (strncmp(iface->name, "UDP", 3)) {
		strncpy(dst_name, "CAN", sizeof(dst_name));
	}

	csp_print("%s: %u(%u) --> %s: %u(%u), priority: %u, flags: 0x%02X, size: %" PRIu16 "\n",
			  iface->name, packet->id.src, packet->id.sport,
			  dst_name, packet->id.dst, packet->id.sport,
			  packet->id.pri, packet->id.flags, packet->length);
}

static void print_help() {
	csp_print("Usage: csp_bridge_can2udp [options]\n");
	csp_print(" -c <can>   set CAN interface\n");
	csp_print(" -H <host>  set UDP address\n"
			  " -l <lport> set UDP listen port\n"
			  " -r <rport> set UDP remote port\n"
			  " -h         print help\n");
}

static csp_iface_t * add_can_iface(const char * can_name)
{
	csp_iface_t * iface = NULL;

	int error = csp_can_socketcan_open_and_add_interface(can_name, CSP_IF_CAN_DEFAULT_NAME,
														 0, 1000000, true, &iface);
	if (error != CSP_ERR_NONE) {
		csp_print("Failed to add CAN interface [%s], error: %d\n", can_name, error);
		exit(1);
	}

	return iface;
}

static csp_iface_t * add_udp_iface(char * host, int lport, int rport)
{
	csp_iface_t * iface = malloc(sizeof(csp_iface_t));
	csp_if_udp_conf_t * conf = malloc(sizeof(csp_if_udp_conf_t));

	conf->host = host;
	conf->lport = lport;
	conf->rport = rport;
	csp_if_udp_init(iface, conf);

	return iface;
}

int main(int argc, char * argv[]) {
	char default_can_name[] = DEFAULT_CAN_NAME;
	char * can_name = default_can_name;

	char default_host[] = DEFAULT_UDP_HOST;
	char * host = default_host;
	int lport = DEFAULT_UDP_LPORT;
	int rport = DEFAULT_UDP_RPORT;

	csp_iface_t * can_iface;
	csp_iface_t * udp_iface;

	int opt;

	/* Use CSP version 1 */
	csp_conf.version = 1;

	while ((opt = getopt_long(argc, argv, "c:H:l:r:h", long_options, NULL)) != -1) {
		switch (opt) {
			case 'c':
				can_name = optarg;
				break;
			case 'H':
				host = optarg;
				break;
			case 'l':
				lport = atoi(optarg);
				break;
			case 'r':
				rport = atoi(optarg);
				break;
			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
			case '?':
				/* Invalid option or missing argument */
				print_help();
				exit(EXIT_FAILURE);
		}
	}

	/* Init CSP */
	csp_init();

	/* Add interfaces */
	can_iface = add_can_iface(can_name);
	udp_iface = add_udp_iface(host, lport, rport);

	/* Start bridge */
	csp_bridge_set_interfaces(can_iface, udp_iface);
	bridge_start();

	/* Print interfaces list */
	csp_iflist_print();

	/* Wait for execution to end (ctrl+c) */
	while(1) {
		sleep(RUN_DURATION_IN_SEC);
	}

	return 0;
}
