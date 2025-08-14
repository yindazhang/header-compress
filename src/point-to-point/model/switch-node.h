#ifndef SWITCH_NODE_H
#define SWITCH_NODE_H

#include "ns3/node.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

#include "ppp-header.h"
#include "hctcp-header.h"
#include "command-header.h"
#include "rohc-compressor.h"
#include "rohc-decompressor.h"

#include <bitset>
#include <random>
#include <unordered_map>
#include <unordered_set>

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
    void SetPFC(uint32_t pfc);
    
    void SetID(uint32_t id);
    uint32_t GetID();

    void SetOutput(std::string output);

    void SetNextNode(uint16_t devId, uint16_t nodeId);

    uint16_t GetNextDev(FlowV4Id id);
    uint16_t GetNextDev(FlowV6Id id);

    uint16_t GetNextNode(uint16_t devId);

    void MarkNicDevice(Ptr<NetDevice> device);

    bool IngressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev);
    Ptr<Packet> EgressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev);

    protected:
    std::string m_output;

    uint32_t m_nid;
    uint32_t m_setting;
    uint32_t m_pfc{0};

    uint32_t m_userThd = 2064000;
    uint32_t m_pfcThd = 250000;
    uint32_t m_pfcNicThd = 50000;
    uint32_t m_resumeThd = 200000;
    uint32_t m_resumeNicThd = 40000;
    int32_t m_userSize = 0;
    std::unordered_map<Ptr<NetDevice>, uint32_t> m_ingressSize;
    std::unordered_map<Ptr<NetDevice>, bool> m_pause;
    std::unordered_set<Ptr<NetDevice>> m_nicDevices;

    int m_hashSeed;

    uint64_t m_drops = 0;
    uint64_t m_ecnCount = 0;
    uint64_t m_pfcCount = 0;

    std::unordered_map<uint32_t, std::vector<uint32_t>> m_v4route;
    std::map<std::pair<uint64_t, uint64_t>, std::vector<uint32_t>> m_v6route;
    std::unordered_map<uint32_t, std::vector<uint32_t>> m_idroute;

    std::unordered_map<uint16_t, std::pair<uint16_t, uint16_t>> m_mplsroute;

    std::unordered_map<uint32_t, uint32_t> m_node;

    std::unordered_map<Ptr<NetDevice>, Ptr<RohcCompressor>> m_rohcCom;
    std::unordered_map<Ptr<NetDevice>, Ptr<RohcDecompressor>> m_rohcDecom;

    void SendPFC(Ptr<NetDevice> dev, bool pause);

    void UpdateMplsRoute(CommandHeader cmd);

    void CheckEcnCount();
};

} // namespace ns3

#endif /* SWITCH_NODE_H */