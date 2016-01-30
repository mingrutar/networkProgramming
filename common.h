//
// common.h
//
#ifndef __pa3_common_h__
#define __pa3_common_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef WIN32
#include <io.h>
#include <time.h>
#include <windows.h>
#include <map>
#else
#include <iostream>
#include <ostream>
#include <istream>
#include <map.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif

#ifndef INADDR_NONE
#define INADDR_NONE	0xffffffff
#endif

//#define _TRACE
#define BUFF_SIZE		512
#define MSG_SIZE		128
#define NAME_SIZE		50
#define REQ_SIZE		30
#define HOP_SIZE		16
#define MAX_NODES		10

// the 1st char in message
#define MSGTYPE_QUERYIP	1
#define MSGTYPE_REG		2

#define MSGTYPE_RDV		3			// router DV exchange
#define MSGTYPE_HMSG	4			// host msg
#define MSGTYPE_BGP		5			// BGP

#define MSGTYPE_CMSG	6			// coordinator msg
#define MSGTYPE_CQUIT	7			// coordinator quit
#define MSGTYPE_CCHNG	8			// coordinator change cost
#define MSGTYPE_CDN		9			// coordinator link down in AS
#define MSGTYPE_CASDN	10			// coordinator link down between AS
#define MSGTYPE_CASUP	11			// coordinator link down between AS
#define MSGTYPE_CPRNT	12			// coordinator link down between AS

#define MSGTYPE_CPRNT_DV 13			// dump DV 
#define MSGTYPE_CPRNT_NT 14			// dump nbr table

#define ROUTER_INTERVAL	50			// 5 seconds
#define BGP_INTERVAL	10			// 10 seconds

using namespace std ;

class hostInfo
{
public:
	hostInfo(unsigned long hostIP, struct in_addr ip, long port)
		: _hostIP(hostIP),_port(port) {
#ifdef WIN32
		_IPAddr.S_un.S_addr = ip.S_un.S_addr;
#else
		_IPAddr.s_addr = ip.s_addr;
#endif	
}

	void dump();
private:
	unsigned long 	_hostIP;
	struct in_addr	_IPAddr;
	long			_port;
	friend class commObject;
	friend class coordinator;
};

typedef map<unsigned long, char*>			netPrifixMap;
typedef map<unsigned long, hostInfo* >		IPmap;	// IPadd, UDP_IP
typedef map<unsigned long, unsigned long>	IFWMap;	// prefix, hop
typedef map<unsigned long, long>			pa3Map;	// IP, cost

class commObject
{
public:
	commObject() : _ptime(NULL) {;}
	commObject(int timeout);
	virtual ~commObject();

	virtual size_t readMsg(char* pBuff, size_t sz);
	
	void tellGod(const char* pbuf, size_t sz);
	void init(const char* coor, int coorPort);
	size_t sendMsg(unsigned long destIp, const char* pBuff, size_t sz);
protected:
	hostInfo* queryConnection(unsigned long hostIP);

protected:
	struct sockaddr_in	cli_sin;
	struct sockaddr_in	srv_sin;
	struct sockaddr_in	peer_sin;
	int					sd;

	unsigned long	_realIP;
	IPmap			_IP2IP;

	struct timeval*	_ptime;
	char			_buff[MSG_SIZE];
};

//
//
//
#define MAXAS			6		//  maxmum number of AS
#define NODENAME_SIZE	15
#define IPADD_SIZE		18
#define MAX_NBR			4		// maximum number of neighbours
//AS 0
#define AS0_CN	7		//number of subnets
#define AS0_CR	6		//number of routers
#define AS0_CH	7		//number of hosts
#define AS0_IA	5		//number of border routers 

#define AS1_CN	3
#define AS1_CR	3
#define AS1_CH	3
#define AS1_IA	2

#define AS2_CN	3
#define AS2_CR	3
#define AS2_CH	3
#define AS2_IA	2

#define AS3_CN	1
#define AS3_CR	1
#define AS3_CH	1
#define AS3_IA	1

#define AS4_CN	3
#define AS4_CR	3
#define AS4_CH	3
#define AS4_IA	3

#define AS5_CN	2
#define AS5_CR	2
#define AS5_CH	2
#define AS5_IA	2

typedef struct st_netPrefix			// subnet definition
{
	char	_name[NODENAME_SIZE];	// name
	char	_ips[IPADD_SIZE];		// subnet
} st_np;
typedef struct st_router
{
	int		_id;						// route id
	int		_type;						// 0 - normal; 1 - BGP
	char	_name[NODENAME_SIZE];		// router name
	char	_ips[IPADD_SIZE];			// ip for interface 1
	char	_ips1[IPADD_SIZE];			// ip for interface 2
	int		_nbrs[MAX_NBR];				// neighbour router id
	int		_cost[MAX_NBR];				// cost to the neighbor
} st_router;
typedef struct st_host
{
	char	_name[NODENAME_SIZE];		// host name 
	char	_ips[IPADD_SIZE];			// host ip address
	int		_router;					// router id
} st_host;
typedef struct st_inter_as				// inter AS relation	
{
	int		_router;					// router id
	char	_ASname[NODENAME_SIZE];		// neighbor AS id
	char	_ips[IPADD_SIZE];			// ip of router in neighbor AS
	int		_cost;						// cost
} st_inter_as;
typedef struct st_as					// AS definition
{
	char			_name[NODENAME_SIZE];  // AS name
	int				_mask;					// network mask length 	
	int				_cnps;					// number of networks
	st_netPrefix*	_nps;					// pointer to subnet structure
	int				_crouters;				// number of routers
	st_router*		_routers;				// pointer to routers
	int				_chosts;				// number of hosts
	st_host*		_hosts;					// pointers to hosts
} st_as;

extern void pa3Printf(const char *format, ...);
extern int errexit(const char *format, ...);
extern void start_s(void* pObj);			// for send

#endif //__pa3_common_h__