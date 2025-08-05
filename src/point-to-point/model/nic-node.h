#ifndef NIC_NODE_H
#define NIC_NODE_H

#include "ns3/node.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

#include "ppp-header.h"
#include "hctcp-header.h"
#include "command-header.h"
#include "rohc-compressor.h"
#include "rohc-decompressor.h"

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

    void AddControlRouteTo(uint16_t id, uint32_t devId);

    void SetECMPHash(uint32_t hashSeed);
    void SetSetting(uint32_t setting);
    void SetVxLAN(uint32_t vxlan);
    void SetThreshold(uint32_t threshold);
    void SetID(uint32_t id);
    uint32_t GetID();

    void SetNextNode(uint16_t devId, uint16_t nodeId);

    uint16_t GetNextDev(FlowV4Id id);
    uint16_t GetNextDev(FlowV6Id id);

    uint16_t GetNextNode(uint16_t devId);

    void SetOutput(std::string output);

    bool IngressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev);
    Ptr<Packet> EgressPipeline(Ptr<Packet> packet, uint16_t protocol, Ptr<NetDevice> dev);

    uint64_t GetUserCount();
    void SetUserCount(uint64_t userCount);

    uint64_t GetMplsCount();
    void SetMplsCount(uint64_t mplsCount);

    protected:

    std::string m_output;

    uint32_t m_nid;
    uint32_t m_setting;
    uint32_t m_vxlan;

    uint64_t m_userCount = 0;
    uint64_t m_mplsCount = 0;
    uint64_t m_ecnCount = 0;

    uint32_t m_userThd = 8388608;
    int32_t m_userSize = 0;
    int m_hashSeed = 0;

    uint32_t m_threshold = 100;
    uint64_t m_drops = 0;

    //uint32_t m_sampleSize = 65536;
    //std::vector<uint32_t> m_sample;
    //std::vector<uint32_t> m_sampleMpls;

    std::map<FlowV4Id, std::pair<uint32_t, uint64_t>> m_v4count;
    std::map<FlowV6Id, std::pair<uint32_t, uint64_t>> m_v6count;

    std::unordered_map<uint32_t, std::vector<uint32_t>> m_v4route;
    std::map<std::pair<uint64_t, uint64_t>, std::vector<uint32_t>> m_v6route;
    std::unordered_map<uint32_t, std::vector<uint32_t>> m_idroute;

    std::unordered_map<uint32_t, uint32_t> m_node;

    std::map<FlowV4Id, uint16_t> m_compress4;
    std::map<FlowV6Id, uint16_t> m_compress6;

    std::unordered_map<uint16_t, FlowV4Id> m_decompress4;
    std::unordered_map<uint16_t, FlowV6Id> m_decompress6;

    std::unordered_map<Ptr<NetDevice>, Ptr<RohcCompressor>> m_rohcCom;
    std::unordered_map<Ptr<NetDevice>, Ptr<RohcDecompressor>> m_rohcDecom;

    void EncapVxLAN(Ptr<Packet> packet);

    void GenData4(FlowV4Id id);
    void GenData6(FlowV6Id id);

    void UpdateCompress4(CommandHeader cmd);
    void UpdateDecompress4(CommandHeader cmd);

    void UpdateCompress6(CommandHeader cmd);
    void UpdateDecompress6(CommandHeader cmd);

    void DeleteCompress4(CommandHeader cmd);
    void DeleteCompress6(CommandHeader cmd);

    void CheckEcnCount();
};

} // namespace ns3

#endif /* NIC_NODE_H */
