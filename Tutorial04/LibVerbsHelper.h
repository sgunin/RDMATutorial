#pragma once

#include <string>

#include <verbs.h>
#include <arch.h>

constexpr auto QueueSize = 0x10;
constexpr auto BufferSize = 1024;

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
	const char*				deviceName;			/* HCA kernel device name */
	int						devicePort;			/* HCA device port */
	uint64_t				remoteBuffer;		/* Remote buffer address */
	uint32_t				remoteKey;			/* Remote key */
	uint32_t				remoteQueueNum;		/* Remote Queue Pair number */
	uint16_t				remoteId;			/* Remote ID */
};

/* Destroy RDMA resource */
void destroyRDMAResource(struct RDMAResource* res);

/* Create RDMA resource structure and filled in */
void createRDMAResource(struct RDMAResource* res);

/* Modify QP to INIT state */
int modifyQPtoInit(struct RDMAResource* res);

/* Modify QP to RTR state */
int modifyQPtoRTR(struct RDMAResource* res);

/* Modify QP to RTS state */
int modifyQPtoRTS(struct RDMAResource* res);

/* Get Local ID */
uint16_t getLocalId(struct RDMAResource* res);

/* Get Queue Pair Number*/
uint32_t getQueuePairNumber(struct RDMAResource* res);