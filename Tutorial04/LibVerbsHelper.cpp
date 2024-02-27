#include "LibVerbsHelper.h"

/* Destroy RDMA resource */
void destroyRDMAResource(struct RDMAResource* res)
{
    if (res) {
        if (res->queuePair) {
            ibv_destroy_qp(res->queuePair);
            res->queuePair = NULL;
        }
        if (res->memoryHandle) {
            ibv_dereg_mr(res->memoryHandle);
            res->memoryHandle = NULL;
        }
        if (res->buffer) {
            free(res->buffer);
            res->buffer = NULL;
        }
        if (res->compQueue) {
            ibv_destroy_cq(res->compQueue);
            res->compQueue = NULL;
        }
        if (res->protectedDomain) {
            ibv_dealloc_pd(res->protectedDomain);
            res->protectedDomain = NULL;
        }
        if (res->context) {
            ibv_close_device(res->context);
            res->context = NULL;
        }
    }
}

/* Create RDMA resource structure and filled in */
void createRDMAResource(struct RDMAResource* res)
{
    int num_devices;

    struct ibv_device** device_list = ibv_get_device_list(&num_devices);
    if (!device_list) {
        fprintf(stderr, "Unable to get HCA device list\n");
        exit(1);
    }

    if (num_devices == 0) {
        fprintf(stderr, "Unable to find any HCA devices\n");
        ibv_free_device_list(device_list);
        exit(1);
    }

    for (int i = 0; i < num_devices; i++) {
        if (!strcmp(res->deviceName, ibv_get_device_name(device_list[i]))) {
            fprintf(stdout, "Select HCA device %s\n", ibv_get_device_name(device_list[i]));
            res->device = device_list[i];
            res->context = ibv_open_device(device_list[i]);
            break;
        }
    }
    ibv_free_device_list(device_list);

    if (res->context == 0) {
        fprintf(stderr, "Unable to get the device: %s\n", res->deviceName);
        exit(1);
    }

    /* Optional. Query HCA device properties */
    if (ibv_query_device(res->context, &res->deviceAttr)) {
        fprintf(stderr, "Unable to query device attribute\n");
        destroyRDMAResource(res);
        exit(1);
    }

    /* Optional. Query HCA port properties */
    int rc = ibv_query_port(res->context, res->devicePort, &res->portAttr);
    if (rc) {
        fprintf(stderr, "Failed to query port %d attributes in device '%s'\n", res->devicePort, res->deviceName);
        destroyRDMAResource(res);
        exit(1);
    }

    /* Verify enable port and active connection */
    if (res->portAttr.phys_state != 5)
    {
        fprintf(stderr, "Port %d in device '%s' not in LinkUp state\n", res->devicePort, res->deviceName);
        destroyRDMAResource(res);
        exit(1);
    }

    /* Allocate Protection Domain */
    res->protectedDomain = ibv_alloc_pd(res->context);
    if (!res->protectedDomain) {
        fprintf(stderr, "Allocate protection domain error\n");
        exit(1);
    }
    fprintf(stdout, "Protection Domain allocated\n");

    /* Create Completion Queue with 10 entries */
    res->compQueue = ibv_create_cq(res->context, QueueSize, nullptr, nullptr, 0);
    if (!res->compQueue) {
        fprintf(stderr, "Failed to create CQ woth %u entries\n", QueueSize);
        exit(1);
    }
    fprintf(stdout, "Create Completion Queue with %d entries\n", QueueSize);

    /* Allocate memory buffer that will hold the data */
    res->buffer = (char*)malloc(BufferSize);
    if (!res->buffer) {
        fprintf(stderr, "Failed to malloc %Zu bytes memory buffer\n", BufferSize);
        exit(1);
    }
    fprintf(stdout, "Allocate %Zu bytes memory buffer\n", BufferSize);

    memset(res->buffer, 0, BufferSize);

    /* Register memory buffer */
    int mrFlags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
    res->memoryHandle = ibv_reg_mr(res->protectedDomain, res->buffer, BufferSize, mrFlags);
    if (!res->memoryHandle) {
        fprintf(stderr, "Register memory buffer failed with mr_flags=0x%x\n", mrFlags);
        exit(1);
    }
    fprintf(stdout, "Register memory buffer with addr=%p, lkey=0x%x, rkey=0x%x, flags=0x%x\n",
        res->buffer, res->memoryHandle->lkey, res->memoryHandle->rkey, mrFlags);

    /* Create the Queue Pair */
    struct ibv_qp_init_attr qpInitAttr;
    memset(&qpInitAttr, 0, sizeof(ibv_qp_init_attr));
    qpInitAttr.qp_type = IBV_QPT_RC;
    qpInitAttr.sq_sig_all = 1;
    qpInitAttr.send_cq = res->compQueue;
    qpInitAttr.recv_cq = res->compQueue;
    qpInitAttr.cap.max_send_wr = 1;
    qpInitAttr.cap.max_recv_wr = 1;
    qpInitAttr.cap.max_send_sge = 1;
    qpInitAttr.cap.max_recv_sge = 1;

    res->queuePair = ibv_create_qp(res->protectedDomain, &qpInitAttr);
    if (!res->queuePair) {
        fprintf(stderr, "Failed to create Queue Pair\n");
        exit(1);
    }
    fprintf(stdout, "QP with number 0x%x was created\n", res->queuePair->qp_num);
}

/* Modify QP to INIT state */
int modifyQPtoInit(struct RDMAResource* res)
{
    struct ibv_qp_attr qpInitAttr;
    memset(&qpInitAttr, 0, sizeof(ibv_qp_attr));
    
    qpInitAttr.qp_state = IBV_QPS_INIT;
    qpInitAttr.port_num = res->devicePort;
    qpInitAttr.pkey_index = 0;
    qpInitAttr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

    int flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    int result = 0;
    
    result = ibv_modify_qp(res->queuePair, &qpInitAttr, flags);
    if (result)
        fprintf(stderr, "Failed to modify Queue Pair to Init state\n");
    return result;
}

/* Modify QP to RTR state */
int modifyQPtoRTR(struct RDMAResource* res)
{
    struct ibv_qp_attr rtrAttr;
    memset(&rtrAttr, 0, sizeof(ibv_qp_attr));
    
    rtrAttr.qp_state = IBV_QPS_RTR;
    rtrAttr.path_mtu = IBV_MTU_1024;
    rtrAttr.rq_psn = 0;
    rtrAttr.max_dest_rd_atomic = 1;
    rtrAttr.min_rnr_timer = 0x12;
    rtrAttr.ah_attr.is_global = 0;
    rtrAttr.ah_attr.sl = 0;
    rtrAttr.ah_attr.src_path_bits = 0;
    rtrAttr.ah_attr.port_num = res->devicePort;

    rtrAttr.dest_qp_num = res->remoteQueueName;
    rtrAttr.ah_attr.dlid = res->remoteId;

    int flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
    int result = 0;

    result = ibv_modify_qp(res->queuePair, &rtrAttr, flags);
    if (result)
        fprintf(stderr, "Failed to modify Queue Pair to RTR state\n");
    return result;
}

/* Modify QP to RTS state */
int modifyQPtoRTS(struct RDMAResource* res)
{
    struct ibv_qp_attr rtsAttr;
    memset(&rtsAttr, 0, sizeof(ibv_qp_attr));

    rtsAttr.qp_state = IBV_QPS_RTS;
    rtsAttr.timeout = 0x12;
    rtsAttr.retry_cnt = 7;
    rtsAttr.rnr_retry = 7;
    rtsAttr.sq_psn = 0;
    rtsAttr.max_rd_atomic = 1;

    int flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
    int result = 0;

    result = ibv_modify_qp(res->queuePair, &rtsAttr, flags);
    if (result)
        fprintf(stderr, "Failed to modify Queue Pair to RTS state\n");
    return result;
}