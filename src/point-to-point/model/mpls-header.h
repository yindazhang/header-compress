#ifndef MPLS_HEADER_H
#define MPLS_HEADER_H

#include "ns3/header.h"

namespace ns3
{

/**
 * \ingroup mpls
 * \brief
 * Label stack entry
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ Label
 *  |                Label                  | Exp |S|       TTL     | Stack
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ Entry
 *
 *
 * The label stack is represented as a sequence of "label stack entries".
 * For more infomation see RFC 3032 (http://www.ietf.org/rfc/rfc3032.txt)
 */

class MplsHeader : public Header
{
  public:

    MplsHeader();
    ~MplsHeader() override;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    enum EcnType
    {
        ECN_NotECT = 0x00,
        ECN_ECT1 = 0x01,
        ECN_ECT0 = 0x02,
        ECN_CE = 0x03
    };

    uint32_t GetLabel();
    void SetLabel(uint32_t label);

    uint8_t GetExp();
    void SetExp(uint8_t exp);

    uint8_t GetTtl();
    void SetTtl(uint8_t ttl);

    uint8_t GetBos();
    void SetBos();
    void ClearBos();

  private:
    uint32_t m_value;
};

} // namespace ns3

#endif /* MPLS_HEADER_H */