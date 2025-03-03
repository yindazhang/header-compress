#ifndef COMMAND_HEADER_H
#define COMMAND_HEADER_H

#include <vector>

#include "ppp-header.h"

namespace ns3
{

class CommandHeader: public Header
{

public:
    CommandHeader();
    ~CommandHeader() override;
 
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
 
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    enum CmdType
    {
        SwitchUpdate = 0x01,
        NICData4 = 0x03,
        NICData6 = 0x04,
        NICUpdateCompress4 = 0x05,
        NICUpdateCompress6 = 0x06,
        NICUpdateDecompress4 = 0x07,
        NICUpdateDecompress6 = 0x08,
        NICDeleteCompress4 = 0x09,
        NICDeleteCompress6 = 0x0a,
    };

    uint16_t GetSourceId();
    void SetSourceId(uint16_t id);

    uint16_t GetDestinationId();
    void SetDestinationId(uint16_t id);

    uint8_t GetType();
    void SetType(uint8_t type);

    uint16_t GetLabel();
    void SetLabel(uint16_t label);

    uint16_t GetNewLabel();
    void SetNewLabel(uint16_t label);

    uint8_t GetPort();
    void SetPort(uint8_t port);

    FlowV4Id GetFlow4Id();
    void SetFlow4Id(FlowV4Id flowId);

    FlowV6Id GetFlow6Id();
    void SetFlow6Id(FlowV6Id flowId);
 
protected:
    uint16_t m_srcId;
    uint16_t m_dstId;
    uint8_t m_type;  // All needed (5 bytes)
    uint16_t m_label;
    uint16_t m_newLabel;
    uint8_t m_port;
    FlowV4Id m_flow4Id; // 13 bytes
    FlowV6Id m_flow6Id; // 37 bytes
};

} // namespace ns3

#endif /* COMMAND_HEADER_H */