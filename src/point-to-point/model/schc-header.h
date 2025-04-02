#ifndef SCHC_HEADER_H
#define SCHC_HEADER_H

#include <vector>

#include "ppp-header.h"

namespace ns3
{

class SchcHeader: public Header
{

public:
    SchcHeader();
    ~SchcHeader() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    enum SchcType
    {
        UpdateCompress4 = 0x01,
        UpdateCompress6 = 0x02,
    };

    uint8_t GetType();
    void SetType(uint8_t type);

    uint16_t GetLabel();
    void SetLabel(uint16_t label);

    FlowV4Id GetFlow4Id();
    void SetFlow4Id(FlowV4Id flowId);

    FlowV6Id GetFlow6Id();
    void SetFlow6Id(FlowV6Id flowId);
 
protected:
    uint8_t m_type;
    uint16_t m_label;
    FlowV4Id m_flow4Id; // 13 bytes
    FlowV6Id m_flow6Id; // 37 bytes
};

} // namespace ns3

#endif /* SCHC_HEADER_H */