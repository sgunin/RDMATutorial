#pragma once

#include <getopt.h>

#include "LibVerbsHelper.h"
#include "TCPClientServer.h"

struct config_t 
{
	const char* deviceName;			/* HCA kernel device name */
	int			devicePort;			/* HCA device port */
	const char* serverAddress;		/* Remote server address */
	int			listenPort;			/* Local or remote listen port */
};

struct qpInfo_t
{
	uint64_t	addr;	/* Buffer address */
	uint32_t	rkey;	/* Remote key */
	uint32_t	qpNum;	/* QP number */
	uint16_t	lid;	/* LID of IB port */
};

int socket;