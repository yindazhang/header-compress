#ifndef RSVP_COMPRESS_H
#define RSVP_COMPRESS_H

#include "rsvp-object.h"

namespace ns3
{

class RsvpCompress: public RsvpObject
{

public:
    RsvpCompress();
    ~RsvpCompress() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    uint16_t GetKind() const override;
 
    // TODO
 
protected:

    //TODO
};

} // namespace ns3

#endif /* RSVP_COMPRESS_H */