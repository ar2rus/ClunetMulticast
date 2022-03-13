#include "ClunetMulticast.h"

ClunetMulticast::ClunetMulticast(unsigned char deviceId, String deviceName)
: _responsesQueue(LinkedList<clunet_response*>([](clunet_response *m){ delete  m; })){
  _id = deviceId;
  _name = deviceName;
  
  _requestId = 0;
}

ClunetMulticast::~ClunetMulticast(){
   _responsesQueue.free();
}

#define TIME_INFO_PACKET_SIZE 7

char getTimeInfo(char* buf){
	 time_t t = time(nullptr);
	 if (t){
		struct tm timeinfo;
		localtime_r(&t, &timeinfo);
		
		if (timeinfo.tm_year > 100){
			buf[0] = (char)(timeinfo.tm_year + 1900 - 2000);
			buf[1] = (char)timeinfo.tm_mon + 1;
			buf[2] = (char)timeinfo.tm_mday;
			buf[3] = (char)timeinfo.tm_hour;
			buf[4] = (char)timeinfo.tm_min;
			buf[5] = (char)timeinfo.tm_sec;
			buf[6] = (char)timeinfo.tm_wday;
			if (!buf[6]++){
				buf[6] = 7;
			}
			return TIME_INFO_PACKET_SIZE;
		}
	 }
	 
	 return 0;
}

bool ClunetMulticast::connect(){
  bool r = _udp.listenMulticast(CLUNET_MULTICAST_IP, CLUNET_MULTICAST_PORT);
  if (r){
    _udp.onPacket([this](AsyncUDPPacket packet) {
      if (packet.isMulticast()){
        if (packet.length() >= CLUNET_PACKET_OFFSET_DATA) {
			clunet_packet* m = (clunet_packet*)packet.data();
			  			
			if (m->src != _id && m->dst == _id){
				_handleResponse(m);
			}
			  
			if (_onPacketSniffFn){
				_onPacketSniffFn(m);
			}

			if (m->src != _id && (m->dst == _id || m->dst == CLUNET_ADDRESS_BROADCAST)){
			  switch(m->command){
				case CLUNET_COMMAND_DISCOVERY:
					send(m->src, CLUNET_COMMAND_DISCOVERY_RESPONSE, (char*)_name.c_str(), _name.length());
					break;
				case CLUNET_COMMAND_PING:
					send(m->src, CLUNET_COMMAND_PING_REPLY, m->data, m->size);
					break;
				case CLUNET_COMMAND_REBOOT:
					ESP.restart();
					break;
				case CLUNET_COMMAND_TIME: {
					  char buf[TIME_INFO_PACKET_SIZE];
					  char size = getTimeInfo(buf);
					  if (size){
						  send(m->src, CLUNET_COMMAND_TIME_INFO, buf, size);
					  }
					}
					break;
				default:
					if (_onPacketReceivedFn){
					  _onPacketReceivedFn(m);
					}
			  }
			}
        }
      }
    });
    
    send_broadcast(CLUNET_COMMAND_BOOT_COMPLETED, 0, 0);
  }
  return r;
}

void ClunetMulticast::close(){
  _udp.close();
}

void ClunetMulticast::onPacketSent(ClunetMulticastPacketHandlerFunction fn){
  _onPacketSentFn = fn;
}

void ClunetMulticast::onPacketReceived(ClunetMulticastPacketHandlerFunction fn){
  _onPacketReceivedFn = fn;
}

void ClunetMulticast::onPacketSniff(ClunetMulticastPacketHandlerFunction fn){
  _onPacketSniffFn = fn;
}

void ClunetMulticast::onResponseReceived(ClunetMulticastResponseReceivedHandlerFunction fn){
  _onResponseReceivedHandlerFn = fn;
}

size_t ClunetMulticast::_send(unsigned char sender, unsigned char address, unsigned char command, char* data, unsigned char size,
  ClunetMulticastPacketHandlerFunction onPacketSentFn){
  size_t r = 0;
  if (_udp.connected()){
    AsyncUDPMessage* m = new AsyncUDPMessage(CLUNET_PACKET_OFFSET_DATA + size);
    m->write(sender);
    m->write(address);
    m->write(command);
    m->write(size);
    m->write((uint8_t*)data, size);
    r = _udp.send(*m);
    if (onPacketSentFn){
		onPacketSentFn((clunet_packet*)m);
    }
    delete m;
  }
  return r;
}

size_t ClunetMulticast::send(unsigned char address, unsigned char command, char* data, unsigned char size){
  return _send(_id, address, command, data, size, _onPacketSentFn);
}

size_t ClunetMulticast::send_fake(unsigned char sender, unsigned char address, unsigned char command, char* data, unsigned char size){
  return _send(sender, address, command, data, size);
}

size_t ClunetMulticast::send_broadcast(unsigned char command, char* data, unsigned char size){
  return send(CLUNET_ADDRESS_BROADCAST, command, data, size);
}

void ClunetMulticast::_handleResponse(clunet_packet* packet){
	if (_responseFilterFn){
		if (_responseFilterFn(packet)){
			clunet_response* response = (clunet_response*)malloc(packet->len() + sizeof(clunet_response));
			response->requestId = _requestId;
			packet->copy(&response->packet);
			_responsesQueue.add(response);
		}
	}
}

int ClunetMulticast::request(unsigned char address, unsigned char command, char* data, unsigned char size, 
				ClunetMulticastResponseFilterFunction responseFilterFn, long timeout){
  if (_responseFilterFn){
	  return 0;
  }  
  _responseFilterFn = responseFilterFn;
  
  ++_requestId;
  
  while(!_responsesQueue.isEmpty() && _requestId - _responsesQueue.front()->requestId >= REQUEST_HISTORY_LENGTH){
    _responsesQueue.remove(_responsesQueue.front());
  }
  
  send(address, command, data, size);
  _ticker.once_ms(timeout, [this](){
	  if (_onResponseReceivedHandlerFn){
		_onResponseReceivedHandlerFn(_requestId, &_responsesQueue);
	  }
	  _responseFilterFn = NULL;
  });
  return _requestId;
}