#ifndef SWITCH_NODE_H
#define SWITCH_NODE_H

#include "ns3/node.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

#include "ppp-header.h"
#include "command-header.h"
#include "schc-header.h"

#include <bitset>
#include <random>
#include <unordered_map>

namespace ns3
{

class Application;
class Packet;
class Address;
class Time;

class SwitchNode : public Node
{
    
    public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    SwitchNode();
    virtual ~SwitchNode();

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

    void AddControlRouteTo(uint16_t id, uint32_t devId);
    // void SetRouteId(uint16_t id, uint32_t devId);

    void SetECMPHash(uint32_t hashSeed);
    void SetSetting(uint32_t setting);
    
    void SetID(uint32_t id);
    uint32_t GetID();

    void SetOutput(std::string output);

    void SetNextNode(uint16_t devId, uint16_t nodeId);

    uint16_t GetNextDev(FlowV4Id id);
    uint16_t GetNextDev(FlowV6Id id);

    uint16_t GetNextNode(uint16_t devId);

    bool IngressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev);
    Ptr<Packet> EgressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev);

    protected:
    std::string m_output;

    uint32_t m_nid;
    uint32_t m_setting;

    uint32_t m_userThd = 2064000;
    int32_t m_userSize = 0;
    int m_hashSeed;

    uint64_t m_drops = 0;
    uint64_t m_ecnCount = 0;

    std::unordered_map<uint32_t, std::vector<uint32_t>> m_v4route;
    std::map<std::pair<uint64_t, uint64_t>, std::vector<uint32_t>> m_v6route;
    std::unordered_map<uint32_t, std::vector<uint32_t>> m_idroute;

    std::unordered_map<uint16_t, std::pair<uint16_t, uint16_t>> m_mplsroute;

    std::unordered_map<uint32_t, uint32_t> m_node;

    std::map<FlowV4Id, std::pair<uint16_t, uint64_t>> m_sccompress4;
    std::map<FlowV6Id, std::pair<uint16_t, uint64_t>> m_sccompress6;

    std::unordered_map<uint16_t, FlowV4Id> m_scdecompress4;
    std::unordered_map<uint16_t, FlowV6Id> m_scdecompress6;

    std::map<FlowV4Id, std::pair<uint64_t, uint16_t>> m_scdetime4;
    std::map<FlowV6Id, std::pair<uint64_t, uint16_t>> m_scdetime6;

    void GenScUpdate4(FlowV4Id id, Ptr<NetDevice> dev);
    void GenScUpdate6(FlowV6Id id, Ptr<NetDevice> dev);

    void UpdateSchc4(SchcHeader cmd);
    void UpdateSchc6(SchcHeader cmd);

    uint16_t AllocateLabel(bool isv6);

    void UpdateMplsRoute(CommandHeader cmd);

    void CheckEcnCount();
    
    /* Hash function */
    uint32_t rotateLeft(uint32_t x, unsigned char bits);
    
	uint32_t hash(FlowV4Id id, uint32_t seed = 0);
    uint32_t hash(FlowV6Id id, uint32_t seed = 0);

	const uint32_t Prime[5] = {2654435761U,246822519U,3266489917U,668265263U,374761393U};

	const uint32_t prime[16] = {
        181, 5197, 1151, 137, 5569, 7699, 2887, 8753, 
        9323, 8963, 6053, 8893, 9377, 6577, 733, 3527
	};
};

} // namespace ns3

#endif /* SWITCH_NODE_H */