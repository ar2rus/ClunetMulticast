#ifndef ClunetMulticast_h
#define ClunetMulticast_h

#include "ClunetCommon.h"

#include <ESP8266WiFi.h>
#include "ESPAsyncUDP.h"
#include "StringArray.h"

#include <time.h>

#include <Ticker.h>

#include <functional>


#define REQUEST_HISTORY_LENGTH 10


typedef struct clunet_packet clunet_packet;
typedef struct clunet_response clunet_response;

struct clunet_packet{ 
  unsigned char src;
  unsigned char dst;
  unsigned char command;
  unsigned char size;
  char data[];
  
  unsigned char len(){return sizeof(clunet_packet) + size;}
  
  clunet_packet* copy(void* buf){
	  memcpy(buf, &src, len());
	  return (clunet_packet*)buf;
  }
  
  clunet_packet* copy(){
	  return copy(new char[len()]);
  }
};

struct clunet_response{
  int requestId;
  clunet_packet packet[];
  
  unsigned char len(){return sizeof(clunet_response) + packet->len();}
  
  clunet_response* copy(void* buf){
	  memcpy(buf, &requestId, len());
	  return (clunet_response*)buf;
  }
  
  clunet_response* copy(){
	  return copy(new char[len()]);
  }
};

const IPAddress CLUNET_MULTICAST_IP(234, 5, 6, 7);

#define CLUNET_MULTICAST_PORT 12345

typedef std::function<void(clunet_packet* packet)> ClunetMulticastPacketHandlerFunction;
typedef std::function<bool(clunet_packet* packet)> ClunetMulticastResponseFilterFunction;
typedef std::function<void(int requestId, LinkedList<clunet_response*>* responses)> ClunetMulticastResponseReceivedHandlerFunction;

class ClunetMulticast{
  private:
    unsigned char _id;
    String _name;
    
	int _requestId;
	Ticker _ticker;
	
    AsyncUDP _udp;
	
    ClunetMulticastPacketHandlerFunction _onPacketSentFn;
    ClunetMulticastPacketHandlerFunction _onPacketReceivedFn;
    ClunetMulticastPacketHandlerFunction _onPacketSniffFn;
	
	ClunetMulticastResponseFilterFunction _responseFilterFn;
	ClunetMulticastResponseReceivedHandlerFunction _onResponseReceivedHandlerFn;
	
	LinkedList<clunet_response*> _responsesQueue;
    void _handleResponse(clunet_packet* packet);
	
	size_t _send(unsigned char sender, unsigned char address, unsigned char command, char* data, unsigned char size,
    ClunetMulticastPacketHandlerFunction onPacketSentFn = NULL);
	
  public: 
    ClunetMulticast(unsigned char deviceId, String deviceName);
	~ClunetMulticast();
    
    bool connect();
    void close();
    
    void onPacketSent(ClunetMulticastPacketHandlerFunction fn);
    void onPacketReceived(ClunetMulticastPacketHandlerFunction fn);
    void onPacketSniff(ClunetMulticastPacketHandlerFunction fn);
	
    void onResponseReceived(ClunetMulticastResponseReceivedHandlerFunction fn);
    
    size_t send(unsigned char address, unsigned char command, char* data, unsigned char size);
    size_t send_broadcast(unsigned char command, char* data, unsigned char size);
    size_t send_fake(unsigned char sender, unsigned char address, unsigned char command, char* data, unsigned char size);
	
	int request(unsigned char address, unsigned char command, char* data, unsigned char size, 
				ClunetMulticastResponseFilterFunction responseFilterFn = NULL, long timeout = 50);
    
    bool connected();
};

#endif
