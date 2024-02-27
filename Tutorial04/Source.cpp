#include "Source.h"

/* Отображает на экране информацию об использовании программы */
void usage(const char* argv0)
{
    fprintf(stdout, "Usage:\n");
    fprintf(stdout, " %s start a program and wait for remote RDMA connection\n", argv0);
    fprintf(stdout, "\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, " -d, --device <name> kernel name of IB device\n");
    fprintf(stdout, " -p, --port <number> IB device port number (default 1)\n");
    fprintf(stdout, " -n, --queueName <number> optional remote queue pair name\n");
    fprintf(stdout, " -i, --id <number> optional remote queue pair id\n");
}

/* Заполняет аргументами командной строки структуру с конфигурацией */
int fillOptions(struct config_t* config, int argc, char* argv[])
{
    /* parse the command line parameters */
    struct option options[] =
    {
        {"ib-device", required_argument, NULL, 'd'},
        {"ib-port", required_argument, NULL, 'i'},
        {"server", optional_argument, NULL, 's'},
        {"port", optional_argument, NULL, 'p'},
        {NULL, no_argument, NULL, '\0'}
    };

    int c = 0;
    while (c = getopt_long(argc, argv, "d:i:s:p:", options, NULL) != -1)
    {
        switch (c)
        {
        case 'd': {
            config->deviceName = strdup(optarg);
            break;
        };
        case 'i': {
            config->devicePort = strtoul(optarg, NULL, 0);
            if (config->devicePort < 0)
                return 1;
        };
        case 's': {
            config->serverAddress = strdup(optarg);
            break;
        };
        case 'p': {
            config->listenPort = strtoul(optarg, NULL, 0);
            if (config->listenPort < 0)
                return 1;
        };
        case '?': default: {
            return 1;
        };
        }
    }

    return 0;
}

/* Получает информацию от удаленной системы через сетевой сокет */
int getRemoteQPInfo(struct RDMAResource* res)
{
    struct qpInfo_t localQPInfo;
    struct qpInfo_t remoteQPInfo;

    localQPInfo.addr = htonll((uintptr_t)res->buffer);
    localQPInfo.rkey = htonl(res->memoryHandle->rkey);
    localQPInfo.qpNum = htonl(res->queuePair->qp_num);
    localQPInfo.lid = htons(res->portAttr.lid);

    if (sockSyncData(socket, sizeof(qpInfo_t), (char*)&localQPInfo, (char*)&remoteQPInfo) < 0)
    {
        fprintf(stderr, "Could not get remote QP information\n");
        return 0;
    }

    res->remoteBuffer = ntohll(remoteQPInfo.addr);
    res->remoteKey = ntohl(remoteQPInfo.rkey);
    res->remoteQueueNum = ntohl(remoteQPInfo.qpNum);
    res->remoteId = ntohs(remoteQPInfo.lid);

    return 1;
}

/* Записывает в сокет локальные данные, читает удаленные */
int sockSyncData(int sock, int xfer_size, char* local_data, char* remote_data)
{
    int rc = 0;
    int readBytes = 0;
    int totalReadBytes = 0;

    rc = write(socket, local_data, xfer_size);
    if (rc < xfer_size)
    {
        fprintf(stderr, "Failed writing data to socket\n");
        return -1;
    }

    while (!rc && totalReadBytes < xfer_size)
    {
        readBytes = read(socket, remote_data, xfer_size);
        if (readBytes > 0)
            totalReadBytes += readBytes;
        else
            rc = readBytes;
    }

    return rc;
}

/* Помещает в очередь запрос на прием */
int postReceiveRequest(struct RDMAResource* res)
{
    struct ibv_recv_wr receiveWR, * badWR = nullptr;
    memset(&receiveWR, 0, sizeof(receiveWR));

    struct ibv_sge receiveSGE;
    memset(&receiveSGE, 0, sizeof(receiveSGE));
    receiveSGE.addr = (uintptr_t)res->buffer;

}

int main(int argc, char* argv[]) {
    /* Заполняем структуру с конфигурацией параметрами из командной строки */
    struct config_t config {};
    memset(&config, 0, sizeof(config_t));
    if (fillOptions(&config, argc, argv))
        usage(argv[0]);

    /* Инициализируем RDMA устройство, контекст, очереди и регион памяти */
    struct RDMAResource res {};
    memset(&res, 0, sizeof(RDMAResource));
    res.deviceName = strdup(config.deviceName);
    res.devicePort = config.devicePort;
    createRDMAResource(&res);

    fprintf(stdout, "Local QP number: %d\n", res.queuePair->qp_num);
    fprintf(stdout, "Local QP Id: %d\n", res.portAttr.lid);

    /* Инициализируем сетевое соединение */
    if ((socket = InitSocket(config.serverAddress, config.listenPort) < 0))
        goto exit;

    /* Обмениваемся информацией с удаленной системой */
    if (getRemoteQPInfo(&res))
        goto exit;

    fprintf(stdout, "Remote QP number: %d\n", res.remoteQueueNum);
    fprintf(stdout, "Remote QP Id: %d\n", res.remoteId);

    /* Переводим QP в состояние INIT */
    if (modifyQPtoInit(&res))
        goto exit;

    /* For client (responder) post RR to be prepared for incoming messages */
    if (config.serverAddress)
    {

    }

    /* Modify QP to RTR state */
    if (modifyQPtoRTR(&res))
        goto exit;

exit:
    destroyRDMAResource(&res);

    return 0;
}