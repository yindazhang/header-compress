#ifndef NIC_NODE_H
#define NIC_NODE_H

#include "ns3/node.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

#include "ppp-header.h"
#include "rsvp-header.h"

#include <unordered_map>
#include <bitset>
#include <random>

namespace ns3
{

class Application;
class Packet;
class Address;
class Time;

class NICNode : public Node
{
    
    public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    NICNode();
    virtual ~NICNode();

    /**
     * \brief Associate a NetDevice to this node.
     *
     * \param device NetDevice to associate to this node.
     * \returns the index of the NetDevice into the Node's list of
     *          NetDevice.
     */
    uint32_t AddDevice(Ptr<NetDevice> device) override;

    /**
     * \brief Receive a packet from a device.
     * \param device the device
     * \param packet the packet
     * \param protocol the protocol
     * \param from the sender
     * \returns true if the packet has been delivered to a protocol handler.
     */
    bool ReceiveFromDevice(Ptr<NetDevice> device,
                                Ptr<const Packet> packet,
                                uint16_t protocol,
                                const Address& from);

    void AddHostRouteTo(Ipv4Address dest, uint32_t devId);
    void AddHostRouteTo(Ipv6Address dest, uint32_t devId);

    void SetECMPHash(uint32_t hashSeed);
    void SetSetting(uint32_t setting);
    void SetThreshold(uint32_t threshold);
    void SetDynamic(uint32_t dynamic);
    void SetID(uint32_t id);
    uint32_t GetID();

    bool IngressPipeline(Ptr<Packet> packet, uint32_t priority, uint16_t protocol, Ptr<NetDevice> dev);
    Ptr<Packet> EgressPipeline(Ptr<Packet> packet, uint32_t priority, uint16_t protocol, Ptr<NetDevice> dev);

    protected:

    std::mt19937 m_rand;

    uint32_t m_nid;
    uint32_t m_setting;

    uint32_t m_userThd = 2064000;
    int32_t m_userSize = 0;
    int m_hashSeed = 0;

    struct PathState{
        uint32_t label;
        uint32_t timeout;
        uint64_t time;
    };

    std::map<FlowV4Id, PathState> m_pathState4;
    std::map<FlowV6Id, PathState> m_pathState6;

    struct MplsCompress{
        uint32_t label;
    };
    std::map<FlowV4Id, MplsCompress> m_compress4;
    std::map<FlowV6Id, MplsCompress> m_compress6;

    struct MplsDecompress{
        uint16_t protocol;
        FlowV4Id v4Id;
        FlowV6Id v6Id;
    };
    uint32_t m_labelSize = 16 * 1024;
    std::unordered_map<uint32_t, MplsDecompress> m_decompress;

    uint32_t m_dynamic = 0;
    uint32_t m_threshold = 1000;
    uint32_t m_timeout = 10;
    uint64_t m_drops = 0;

    std::map<FlowV4Id, uint32_t> m_v4count;
    std::map<FlowV6Id, uint32_t> m_v6count;

    std::unordered_map<uint32_t, std::vector<uint32_t>> m_v4route;
    std::map<std::pair<uint64_t, uint64_t>, std::vector<uint32_t>> m_v6route;

    // Ingress
    bool IngressPipelineMPLS(Ptr<Packet> packet, Ptr<NetDevice> dev);
    void IngressPipelineRSVPResv(uint16_t protocol, RsvpHeader rsvpHeader, FlowV4Id v4Id, FlowV6Id v6Id);
    void IngressPipelineRSVPErr(uint16_t protocol, RsvpHeader rsvpHeader, FlowV4Id v4Id, FlowV6Id v6Id);

    void ClearRsvp4(FlowV4Id id, bool timeout);
    void ClearRsvp6(FlowV6Id id, bool timeout);

    void IngressPipelineRSVPTear4(FlowV4Id id, RsvpHeader pathHeader);
    void IngressPipelineRSVPTear6(FlowV6Id id, RsvpHeader pathHeader);

    void CreateRsvpTear4(FlowV4Id id, bool timeout);
    void CreateRsvpTear6(FlowV6Id id, bool timeout);

    void CreateRsvpPath4(FlowV4Id id);
    void CreateRsvpPath6(FlowV6Id id);

    bool CreateRsvpResv4(FlowV4Id id, RsvpHeader pathHeader);
    bool CreateRsvpResv6(FlowV6Id id, RsvpHeader pathHeader);

    uint32_t GetLabel();

    /* Hash function */
	uint32_t rotateLeft(uint32_t x, unsigned char bits);

	uint32_t inhash(const uint8_t* data, uint64_t length, uint32_t seed = 0);

	template<typename T>
	uint32_t hash(const T& data, uint32_t seed = 0);

	const uint32_t Prime[5] = {2654435761U,246822519U,3266489917U,668265263U,374761393U};

	const uint32_t prime[16] = {
        181, 5197, 1151, 137, 5569, 7699, 2887, 8753, 
        9323, 8963, 6053, 8893, 9377, 6577, 733, 3527
	};
};

} // namespace ns3

#endif /* NIC_NODE_H */