#include "ClunetMulticast.h"

ClunetMulticast::ClunetMulticast(unsigned char deviceId, String deviceName){
  _id = deviceId;
  _name = deviceName;
}

bool ClunetMulticast::connect(){
  bool r = _udp.listenMulticast(CLUNET_MULTICAST_IP, CLUNET_MULTICAST_PORT);
  if (r){
    _udp.onPacket([this](AsyncUDPPacket packet) {
      if (packet.isMulticast()){
        if (packet.length() >= CLUNET_PACKET_OFFSET_DATA) {
        clunet_packet* m = (clunet_packet*)packet.data();
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
				  time_t t;
				  time(&t);
				  if (t){
					char week_day = (char)weekday(t);
					if (!--week_day){
						week_day = 7;
					}
					char time_info[7] = {(char)(year(t)-2000), (char)month(t), (char)day(t), (char)hour(t), (char)minute(t), (char)second(t), week_day};
					send(m->src, CLUNET_COMMAND_TIME_INFO, &time_info[0], sizeof(time_info));
				  }
				}
				break;
            default:
				if (_onPacketReceivedFn){
				  _onPacketReceivedFn(m);
				}
          }
        }
        if (_onPacketSniffFn){
          _onPacketSniffFn(m);
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