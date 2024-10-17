#ifndef RSVP_HEADER_H
#define RSVP_HEADER_H

#include "ns3/header.h"

#include "rsvp-object.h"

namespace ns3
{

/**
	*   RSVP Common Header

                0             1              2             3
         +-------------+-------------+-------------+-------------+
         | Vers | Flags|  Msg Type   |       RSVP Checksum       |
         +-------------+-------------+-------------+-------------+
         |  Send_TTL   | (Reserved)  |        RSVP Length        |
         +-------------+-------------+-------------+-------------+
*/

class RsvpHeader : public Header
{
public:

    RsvpHeader();
    ~RsvpHeader() override;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    enum MsgType
    {
        Path = 0x01,
        Resv = 0x02,
        PathErr = 0x03,
        ResvErr = 0x04,
        PathTear = 0x05,
        ResvTear = 0x06,
        ResvConf = 0x07
    };

    uint8_t GetType();
    void SetType(uint8_t type);

    uint8_t GetTtl();
    void SetTtl(uint8_t ttl);

    uint16_t GetLength();

    std::map<uint16_t, Ptr<RsvpObject>>& GetObjects();
    void AppendObject(Ptr<RsvpObject> object);

private:
    uint8_t m_type;
    uint8_t m_ttl;
    uint16_t m_length;
    std::map<uint16_t, Ptr<RsvpObject>> m_objects; 
};

} // namespace ns3

#endif /* RSVP_HEADER_H */