/*
 * Copyright (c) 2008 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PPP_HEADER_H
#define PPP_HEADER_H

#include "ns3/header.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

#include "rsvp-error-spec4.h"
#include "rsvp-error-spec6.h"
#include "rsvp-filter-spec4.h"
#include "rsvp-filter-spec6.h"
#include "rsvp-hop4.h"
#include "rsvp-hop6.h"
#include "rsvp-lsp4.h"
#include "rsvp-lsp6.h"
#include "rsvp-sender-spec.h"
#include "rsvp-flow-spec.h"
#include "rsvp-label-request.h"
#include "rsvp-label.h"
#include "rsvp-style.h"
#include "rsvp-time-value.h"

namespace ns3
{

std::pair<uint64_t, uint64_t> Ipv6ToPair(Ipv6Address address);
Ipv6Address PairToIpv6(std::pair<uint64_t, uint64_t> pair);

struct FlowV4Id
{
    uint32_t m_srcIP;
    uint32_t m_dstIP;
    uint16_t m_srcPort;
    uint16_t m_dstPort;
    uint8_t m_protocol;

    FlowV4Id();
    FlowV4Id(const FlowV4Id& flow);
};

bool operator == (const FlowV4Id& a, const FlowV4Id& b);
bool operator < (const FlowV4Id& a, const FlowV4Id& b);

struct FlowV6Id
{
    uint64_t m_srcIP[2];
    uint64_t m_dstIP[2];
    uint16_t m_srcPort;
    uint16_t m_dstPort;
    uint8_t m_protocol;

    FlowV6Id();
    FlowV6Id(const FlowV6Id& flow);
};

bool operator == (const FlowV6Id& a, const FlowV6Id& b);
bool operator < (const FlowV6Id& a, const FlowV6Id& b);

/**
 * \ingroup point-to-point
 * \brief Packet header for PPP
 *
 * This class can be used to add a header to PPP packet.  Currently we do not
 * implement any of the state machine in \RFC{1661}, we just encapsulate the
 * inbound packet send it on.  The goal here is not really to implement the
 * point-to-point protocol, but to encapsulate our packets in a known protocol
 * so packet sniffers can parse them.
 *
 * if PPP is transmitted over a serial link, it will typically be framed in
 * some way derivative of IBM SDLC (HDLC) with all that that entails.
 * Thankfully, we don't have to deal with all of that -- we can use our own
 * protocol for getting bits across the serial link which we call an ns3
 * Packet.  What we do have to worry about is being able to capture PPP frames
 * which are understandable by Wireshark.  All this means is that we need to
 * teach the PcapWriter about the appropriate data link type (DLT_PPP = 9),
 * and we need to add a PPP header to each packet.  Since we are not using
 * framed PPP, this just means prepending the sixteen bit PPP protocol number
 * to the packet.  The ns-3 way to do this is via a class that inherits from
 * class Header.
 */
class PppHeader : public Header
{
  public:
    /**
     * \brief Construct a PPP header.
     */
    PppHeader();

    /**
     * \brief Destroy a PPP header.
     */
    ~PppHeader() override;

    /**
     * \brief Get the TypeId
     *
     * \return The TypeId for this class
     */
    static TypeId GetTypeId();

    /**
     * \brief Get the TypeId of the instance
     *
     * \return The TypeId for this instance
     */
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

    /**
     * \brief Set the protocol type carried by this PPP packet
     *
     * The type numbers to be used are defined in \RFC{3818}
     *
     * \param protocol the protocol type being carried
     */
    void SetProtocol(uint16_t protocol);

    /**
     * \brief Get the protocol type carried by this PPP packet
     *
     * The type numbers to be used are defined in \RFC{3818}
     *
     * \return the protocol type being carried
     */
    uint16_t GetProtocol();

    void SetPadding(uint8_t padding);
    uint8_t GetPadding();

  private:
    /**
     * \brief The PPP protocol type of the payload packet
     */
    uint16_t m_protocol;
    uint16_t m_padding;
};

} // namespace ns3

#endif /* PPP_HEADER_H */
