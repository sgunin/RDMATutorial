#pragma once

#include <cstdio>
#include <string>
#include <stdio.h>

#include <verbs.h>
#include <arch.h>

struct RDMAResource {
	struct ibv_device_attr	deviceAttr;			/* HCA device attribute */
	struct ibv_port_attr	portAttr;			/* IB port attributes */
	struct ibv_context*		context;			/* HCA device context handle */
	struct ibv_device*		device;				/* HCA device handle */
	struct ibv_pd*			protectedDomain;	/* Protected domain handle */
	struct ibv_cq*			compQueue;			/* Completion queue handle */
	struct ibv_qp*			queuePair;			/* Queue pair handle */
	struct ibv_mr*			memoryHandle;		/* Memory registration for buffer */
	char*					buffer;				/* Memory buffer handle */
	const char* deviceName;						/* HCA kernel device name */
};