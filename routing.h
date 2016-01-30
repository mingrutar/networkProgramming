//
// routing.h
//

#ifndef __pa3_routing_h__
#define __pa3_routing_h__

#include "common.h"

typedef struct IPHeader
{
	unsigned long	destIP;
	unsigned long	srcIP;
	unsigned long	length;
	unsigned long	routerIP[HOP_SIZE];
} IPHeader;

class base
{
public:
	base(const char* pName, unsigned long myIP);
	virtual ~base();
	virtual void run() = 0;

	virtual void init(const char* coor, int coorPort) = 0;
	unsigned long toPrefix(unsigned long ip);
	char* toIPString(unsigned long ip);

protected:
	unsigned long	_myIP;
	const char*		_name;
	char			_inbuf[BUFF_SIZE];
	char			_outbuf[BUFF_SIZE];
	commObject*		_pcomm;				// UDP
};
class host : public base
{
public:
	host(const char* pName, unsigned long myIP, unsigned long myRouter);
	virtual ~host() {;}

	virtual void init(const char* coor, int coorPort);
	virtual void run();

protected:
	void sendMsg(const char* pMsg, size_t msgSz);
	void recvMsg(const void* pMsg, size_t msgSz);

private:
	unsigned long	_router;
	IPHeader		_header;
};

class router : public base
{
public:
	router(const char* pName, unsigned long IP);
	virtual ~router() {;}

	virtual void init(const char* coor, int coorPort);
	virtual void run();
	void addExtraIP(unsigned long ip);
	void setNbr(unsigned long nbr, long cost);

protected:
	void forwrdMsg(const char* _inbuf, size_t sz);
	void updateDV(const char* pBuf, size_t sz);
	void changeCost(const char* pBuf, size_t sz);
	void downLink(const char* pBuf, size_t sz);
	void ASdownLink(const char* pBuf, size_t sz);
	void ASupink(const char* pBuf, size_t sz);
	void printFT(const char *p);
	
	void getNeighbours();
	void process_DV();
	void process_Msg();
	
	void propaganda_DV();
	bool updateTables(unsigned long rIP, bool bAS);

	void printDV();
	void printNT();

protected:
	unsigned long	_myIP2;
	pa3Map			_nbr_table;
	pa3Map			_dv_table;
	IFWMap			_fwd_table;

	size_t			_cprop;
};

/*
class BGP : public router
{
public:
	BGP(char* file);

	virtual void run();

protected:
	ASMap _ASPath;
};	*/

extern char				*pmyAS;
extern long				ASid;
extern netPrifixMap		netPrefixTable;

#endif //#define __pa3_routing_h__