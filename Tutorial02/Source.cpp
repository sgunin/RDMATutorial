#include "Source.h"

constexpr auto DeviceNumber = 0;
constexpr auto PortNumber = 1;

/* The link layer protocol used by this port */
const char* transport_str(enum ibv_transport_type transport)
{
    switch (transport) {
    case IBV_TRANSPORT_IB:			return "InfiniBand";
    case IBV_TRANSPORT_IWARP:		return "iWARP";
    case IBV_TRANSPORT_USNIC:		return "usNIC";
    case IBV_TRANSPORT_USNIC_UDP:	return "usNIC UDP";
    case IBV_TRANSPORT_UNSPECIFIED:	return "Unspecified";
    case IBV_TRANSPORT_UNKNOWN:		return "Unknown";
    }
}

/* The logical port state to string */
const char* port_state_str(enum ibv_port_state portState)
{
    switch (portState) {
    case IBV_PORT_NOP:
        /* Reserved value, which shouldn't be observed */
        return "Nop";
    case IBV_PORT_DOWN:
        /* Logical link is down.
        The physical link of the port isn't up.
        In this state, the link layer discards all packets
        presented to it for transmission */
        return "Down";
    case IBV_PORT_INIT:
        /* Logical link is Initializing.
        The physical link of the port is up, but the SM haven't yet configured the logical link.
        In this state, the link layer can only receive and transmit SMPs and flow control link packets,
        other types of packets presented to it for transmission are discarded */
        return "Init";
    case IBV_PORT_ARMED:
        /* Logical link is Armed.
        The physical link of the port is up, but the SM haven't yet fully configured the logical link.
        In this state, the link layer can receive and transmit SMPs and flow control link packets,
        other types of packets can be received, but discards non SMP packets for sending */
        return "Armed";
    case IBV_PORT_ACTIVE:
        /* Logical link is Active.
        The link layer can transmit and receive all packet types */
        return "Active";
    case IBV_PORT_ACTIVE_DEFER:
        /* Logical link is in Active Deferred.
        The logical link was Active, but the physical link suffered from a failure.
        If the error will be recovered within a timeout, the logical link will return to IBV_PORT_ACTIVE,
        otherwise it will move to IBV_PORT_DOWN */
        return "ActiveDefer";
    }
}

/* Maximum MTU supported by this port */
const uint16_t max_MTU(enum ibv_mtu maxMTU)
{
    switch (maxMTU) {
    case IBV_MTU_256:  return 256;
    case IBV_MTU_512:  return 512;
    case IBV_MTU_1024: return 1024;
    case IBV_MTU_2048: return 2048;
    case IBV_MTU_4096: return 4096;
    }
}

/* The link layer protocol used by this port */
const char* link_layer_str(uint8_t link_layer)
{
    switch (link_layer) {
    case IBV_LINK_LAYER_UNSPECIFIED:
    case IBV_LINK_LAYER_INFINIBAND:
        return "InfiniBand";
    case IBV_LINK_LAYER_ETHERNET:
        return "Ethernet";
    }
}

/* The maximum number of VLs supported by this port (VL0 - VL15) */
const uint8_t max_vl(uint8_t vl_num)
{
    switch (vl_num) {
    case 1:  return 1;
    case 2:  return 2;
    case 3:  return 4;
    case 4:  return 8;
    case 5:  return 15;
    }
}

/* The active link width of this port */
const uint8_t width_str(uint8_t width)
{
    switch (width) {
    case 1:  return 1;
    case 2:  return 4;
    case 4:  return 8;
    case 8:  return 12;
    }
}

/* The physical link status */
const char* port_phy_state_str(uint8_t phys_state)
{
    switch (phys_state) {
    case 1:
        /* The port drives its output to quiescent levels and does not respond to received data.
        In this state, the link is deactivated  without powering off the port */
        return "Sleep";
    case 2:
        /* The port transmits training sequences and responds to receive training sequences */
        return "Polling";
    case 3:
        /* The port drives its output to quiescent levels and does not respond to receive data */
        return "Disabled";
    case 4:
        /* Both transmitter and receive active and the port is attempting to configure and transition to the LinkUp state */
        return "Port Configuration Training";
    case 5:
        /* The port is available to send and receive packets */
        return "LinkUp";
    case 6:
        /* Port attempts to re-synchronize the link and return it to normal operation */
        return "Link Error Recovery";
    case 7:
        /* Port allows the transmitter and received circuitry to be tested by external test equipment
        for compliance with the transmitter and receiver specifications */
        return "Phy Test";
    }
}

/* The active link speed of this port */
const char* port_speed_str(uint8_t activeSpeed)
{
    switch (activeSpeed) {
    case 1:		return "2.5 Gbps";
    case 2:		return "5.0 Gbps";
    case 4:
    case 8:		return "10.0 Gbps";
    case 16:	return "14.0 Gbps";
    case 32:	return "25.0 Gbps";
    case 64:	return "50.0 Gbps";
    case 128:	return "100.0 Gbps";
    default:	return "Unsupported value";
    }
}

/* Destroy RDMA resource */
static void destroyRDMAResource(struct RDMAResource* res)
{
    if (res) {
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

    /* Query HCA device properties */
    if (ibv_query_device(res->context, &res->deviceAttr)) {
        fprintf(stderr, "Unable to query device attribute\n");
        destroyRDMAResource(res);
        exit(1);
    }

    fprintf(stdout, "HCA device %s is open\n", res->deviceName);
    fprintf(stdout, "\ttransport:\t\t%s\n", transport_str(res->device->transport_type));
    fprintf(stdout, "\tvendor_id:\t\t0x%04x\n", res->deviceAttr.vendor_id);
    fprintf(stdout, "\tvendor_part_id:\t%d\n", res->deviceAttr.vendor_part_id);
    fprintf(stdout, "\thw_ver:\t\t\t0x%X\n", res->deviceAttr.hw_ver);
    fprintf(stdout, "\tphys_port_cnt:\t\t%d\n", res->deviceAttr.phys_port_cnt);
    fprintf(stdout, "\tmax_qp:\t\t\t%d\n", res->deviceAttr.max_qp);
    fprintf(stdout, "\tmax_qp_wr:\t\t%d\n", res->deviceAttr.max_qp_wr);
    fprintf(stdout, "\tmax_sge:\t\t%d\n", res->deviceAttr.max_sge);
    fprintf(stdout, "\tmax_cq:\t\t\t%d\n", res->deviceAttr.max_cq);
    fprintf(stdout, "\tmax_cqe:\t\t%d\n", res->deviceAttr.max_cqe);
    fprintf(stdout, "\tmax_mr:\t\t\t%d\n", res->deviceAttr.max_mr);
    fprintf(stdout, "\tmax_mr_size:\t\t0x%llx\n", (unsigned long long) res->deviceAttr.max_mr_size);
    fprintf(stdout, "\tmax_pd:\t\t\t%d\n", res->deviceAttr.max_pd);

    /* Query HCA port properties */
    int rc = ibv_query_port(res->context, PortNumber, &res->portAttr);
    if (rc) {
        fprintf(stderr, "Failed to query port %d attributes in device '%s'\n", PortNumber, res->deviceName);
        destroyRDMAResource(res);
        exit(1);
    }
    fprintf(stdout, "HCA device %s port %d:\n", res->deviceName, PortNumber);
    fprintf(stdout, "\tstate:\t\t\t%s\n", port_state_str(res->portAttr.state));
    fprintf(stdout, "\tmax_mtu:\t\t%d\n", max_MTU(res->portAttr.max_mtu));
    fprintf(stdout, "\tactive_mtu:\t\t%d\n", max_MTU(res->portAttr.active_mtu));
    fprintf(stdout, "\tsm_lid:\t\t\t%d\n", res->portAttr.sm_lid);
    fprintf(stdout, "\tport_lid:\t\t%d\n", res->portAttr.lid);
    fprintf(stdout, "\tport_lmc:\t\t0x%02x\n", res->portAttr.lmc);
    fprintf(stdout, "\tlink_layer:\t\t%s\n", link_layer_str(res->portAttr.link_layer));
    fprintf(stdout, "\tmax_msg_sz:\t\t0x%x\n", res->portAttr.max_msg_sz);
    fprintf(stdout, "\tport_cap_flags:\t0x%08x\n", res->portAttr.port_cap_flags);
    fprintf(stdout, "\tmax_vl_num:\t\t%d\n", max_vl(res->portAttr.max_vl_num));
    fprintf(stdout, "\tbad_pkey_cntr:\t\t0x%x\n", res->portAttr.bad_pkey_cntr);
    fprintf(stdout, "\tqkey_viol_cntr:\t0x%x\n", res->portAttr.qkey_viol_cntr);
    fprintf(stdout, "\tsm_sl:\t\t\t%d\n", res->portAttr.sm_sl);
    fprintf(stdout, "\tpkey_tbl_len:\t\t%d\n", res->portAttr.pkey_tbl_len);
    fprintf(stdout, "\tgid_tbl_len:\t\t%d\n", res->portAttr.gid_tbl_len);
    fprintf(stdout, "\tsubnet_timeout:\t%d\n", res->portAttr.subnet_timeout);
    fprintf(stdout, "\tinit_type_reply:\t%d\n", res->portAttr.init_type_reply);
    fprintf(stdout, "\tactive_width:\t\t%d\n", width_str(res->portAttr.active_width));
    fprintf(stdout, "\tphys_state:\t\t%s\n", port_phy_state_str(res->portAttr.phys_state));
    fprintf(stdout, "\tspeed:\t\t\t%s\n", port_speed_str(res->portAttr.active_speed));
}

int main()
{
    struct RDMAResource res {};

    createRDMAResource(&res);
    destroyRDMAResource(&res);

    return 0;
}