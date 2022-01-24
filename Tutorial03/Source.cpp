#include "Source.h"

constexpr auto DeviceNumber = 0;
constexpr auto PortNumber = 1;
constexpr auto QueueSize = 0x10;
constexpr auto BufferSize = 1024;

/* Destroy RDMA resource */
static void destroyRDMAResource(struct RDMAResource* res)
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
static void createRDMAResource(struct RDMAResource* res)
{
    int num_devices;

    if (res) {
        destroyRDMAResource(res);
        /*free(res);*/
    }

    memset(res, 0, sizeof * res);

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

    res->device = device_list[DeviceNumber];
    res->context = ibv_open_device(res->device);
    res->deviceName = ibv_get_device_name(res->device);

    ibv_free_device_list(device_list);
    fprintf(stdout, "Select HCA device %s\n", res->deviceName);

    if (res->context == 0) {
        fprintf(stderr, "Unable to get the device %d\n", DeviceNumber);
        exit(1);
    }

    /* Optional. Query HCA device properties */
    if (ibv_query_device(res->context, &res->deviceAttr)) {
        fprintf(stderr, "Unable to query device attribute\n");
        destroyRDMAResource(res);
        exit(1);
    }

    /* Optional. Query HCA port properties */
    int rc = ibv_query_port(res->context, PortNumber, &res->portAttr);
    if (rc) {
        fprintf(stderr, "Failed to query port %d attributes in device '%s'\n", PortNumber, res->deviceName);
        destroyRDMAResource(res);
        exit(1);
    }

    /* Verify enable port and active connection */
    if (res->portAttr.phys_state != 5)
    {
        fprintf(stderr, "Port %d in device '%s' not in LinkUp state\n", PortNumber, res->deviceName);
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
    memset(&qpInitAttr, 0, sizeof(qpInitAttr));
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

int main()
{
    struct RDMAResource res {};

    createRDMAResource(&res);
    destroyRDMAResource(&res);

    return 0;
}