#pragma once

#include <cstdio>
#include <string>

#include <verbs.h>
#include <arch.h>

struct RDMAResource {
	struct ibv_device_attr	deviceAttr;	/* HCA device attribute */
	struct ibv_port_attr	portAttr;	/* IB port attributes */
	struct ibv_context*		context;	/* HCA device context handle */
	struct ibv_device*		device;		/* HCA device handle */
	const char* deviceName;				/* HCA kernel device name */
};
