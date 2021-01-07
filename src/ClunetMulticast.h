#ifndef ClunetMulticast_h
#define ClunetMulticast_h

#include "ClunetCommon.h"

#include <ESP8266WiFi.h>
#include "ESPAsyncUDP.h"

#include <functional>


typedef struct {
  unsigned char src;
  unsigned char dst;
  unsigned char command;
  unsigned char size;
  char data[CLUNET_PACKET_DATA_SIZE];
} clunet_packet;


const IPAddress CLUNET_MULTICAST_IP(234, 5, 6, 7);

#define CLUNET_MULTICAST_PORT 12345

typedef std::function<void(clunet_packet* packet)> ClunetMulticastPacketHandlerFunction;

class ClunetMulticast{
  private:
    unsigned char _id;
    String _name;
    
    AsyncUDP _udp;
    
    ClunetMulticastPacketHandlerFunction _onPacketSentFn;
    ClunetMulticastPacketHandlerFunction _onPacketReceivedFn;
    ClunetMulticastPacketHandlerFunction _onPacketSniffFn;
    
    size_t _send(unsigned char sender, unsigned char address, unsigned char command, char* data, unsigned char size,
    ClunetMulticastPacketHandlerFunction onPacketSentFn = NULL);
  public: 
    ClunetMulticast(unsigned char deviceId, String deviceName);
    
    bool connect();
    void close();
    
    void onPacketSent(ClunetMulticastPacketHandlerFunction fn);
    void onPacketReceived(ClunetMulticastPacketHandlerFunction fn);
    void onPacketSniff(ClunetMulticastPacketHandlerFunction fn);
    
    size_t send(unsigned char address, unsigned char command, char* data, unsigned char size);
    size_t send_broadcast(unsigned char command, char* data, unsigned char size);
    size_t send_fake(unsigned char sender, unsigned char address, unsigned char command, char* data, unsigned char size);
    
    bool connected();
};

#endif
