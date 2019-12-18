#include "ClunetMulticast.h"

ClunetMulticast::ClunetMulticast(unsigned char deviceId, String deviceName){
	_id = deviceId;
	_name = deviceName;
}

bool ClunetMulticast::connect(){
	bool r = _udp.listenMulticast(CLUNET_MULTICAST_IP, CLUNET_MULTICAST_PORT);
	if (r){
	  broadcast_send(CLUNET_COMMAND_BOOT_COMPLETED, 0, 0);
	}
	return r;
}

void ClunetMulticast::close(){
	_udp.close();
}

void ClunetMulticast::onMessage(ClunetMulticastMessageHandlerFunction fn){
	_udp.onPacket([this, fn](AsyncUDPPacket packet) {
		if (packet.isMulticast()){
		  if (packet.length() >= CLUNET_OFFSET_DATA) {
			clunet_message* m = (clunet_message*)packet.data();
			if (m->src_address != _id && (m->dst_address == _id || m->dst_address == CLUNET_BROADCAST_ADDRESS)){
				switch(m->command){
				  case CLUNET_COMMAND_DISCOVERY:
					send(m->src_address, CLUNET_COMMAND_DISCOVERY_RESPONSE, (char*)_name.c_str(), _name.length());
					break;
				  case CLUNET_COMMAND_PING:
					send(m->src_address, CLUNET_COMMAND_PING_REPLY, m->data, m->size);
					break;
				  case CLUNET_COMMAND_REBOOT:
					ESP.restart();
					break;
				  default:
					if (fn){
					  fn(m);
					}
				}
			}
		  }
		}
	});
}

size_t ClunetMulticast::send(unsigned char address, unsigned char command, char* data, unsigned char size){
	size_t r = 0;
	if (_udp.connected()){
	  AsyncUDPMessage* m = new AsyncUDPMessage(CLUNET_OFFSET_DATA + size);
	  m->write(_id);
	  m->write(address);
	  m->write(command);
	  m->write(size);
	  m->write((uint8_t*)data, size);
	  r = _udp.send(*m);
	  delete m;
	}
	return r;
}

size_t ClunetMulticast::broadcast_send(unsigned char command, char* data, unsigned char size){
	return send(CLUNET_BROADCAST_ADDRESS, command, data, size);
}