#pragma once

#include <string>

#include <verbs.h>
#include <arch.h>

constexpr auto DeviceNumber = 0;
constexpr auto PortNumber = 1;
constexpr auto QueueSize = 0x10;
constexpr auto BufferSize = 1024;

struct RDMAResource {
	struct ibv_device_attr	deviceAttr;			/* HCA device attribute */
	struct ibv_port_attr	portAttr;			/* IB port attributes */
	struct ibv_context* context;			/* HCA device context handle */
	struct ibv_device* device;				/* HCA device handle */
	struct ibv_pd* protectedDomain;	/* Protected domain handle */
	struct ibv_cq* compQueue;			/* Completion queue handle */
	struct ibv_qp* queuePair;			/* Queue pair handle */
	struct ibv_mr* memoryHandle;		/* Memory registration for buffer */
	char* buffer;				/* Memory buffer handle */
	const char* deviceName;						/* HCA kernel device name */
};

/* Destroy RDMA resource */
void destroyRDMAResource(struct RDMAResource* res);

/* Create RDMA resource structure and filled in */
void createRDMAResource(struct RDMAResource* res);

/* Connetc to Queue pair. Transition server side to RTR, client side to RTS */
int connectQP(struct RDMAResource* res);