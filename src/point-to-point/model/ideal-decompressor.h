#ifndef IDEAL_DECOMPRESSOR_H
#define IDEAL_DECOMPRESSOR_H

#include "ns3/object.h"
#include "ns3/packet.h"

namespace ns3
{

class IdealDecompressor : public Object
{
	public:
		static TypeId GetTypeId();

    	IdealDecompressor();
		~IdealDecompressor();

		uint16_t Process(Ptr<Packet> packet);
};

} // namespace ns3

#endif /* IDEAL_DECOMPRESSOR */