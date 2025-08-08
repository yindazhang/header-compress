#ifndef CONTROL_NODE_H
#define CONTROL_NODE_H

#include "ns3/node.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"

#include "ppp-header.h"
#include "command-header.h"

#include "nic-node.h"
#include "switch-node.h"

#include <unordered_map>
#include <vector>
#include <bitset>
#include <random>

namespace ns3
{

class Application;
class Packet;
class Address;
class Time;

class ControlNode : public Node
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    ControlNode();
    virtual ~ControlNode();

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
	
	void SetID(uint32_t id);
    uint32_t GetID();

    void SetLabelSize(uint32_t labelsize);

    void SetOutput(std::string output);

	void SetTopology(uint32_t K, 
    	uint32_t NUM_BLOCK,
		uint32_t RATIO,
  		std::vector<Ptr<NICNode>> nics,
		std::vector<Ptr<SwitchNode>> edges,
		std::vector<Ptr<SwitchNode>> aggs,
		std::vector<Ptr<SwitchNode>> cores);

  protected:
    uint64_t m_data = 0;
    uint64_t m_update = 0;
    uint64_t m_delete = 0;

    std::string m_output;
    FILE* fout;

    uint32_t m_labelSize = 16384;
    uint32_t m_nid;

	uint32_t m_K;
    uint32_t m_NUM_BLOCK;
	uint32_t m_RATIO;
    std::vector<Ptr<NICNode>> m_nics;
	std::vector<Ptr<SwitchNode>> m_edges;
	std::vector<Ptr<SwitchNode>> m_aggs;
	std::vector<Ptr<SwitchNode>> m_cores;

    std::unordered_map<Ptr<Node>, std::unordered_map<uint16_t, FlowV4Id>> m_label4;
    std::unordered_map<Ptr<Node>, std::unordered_map<uint16_t, FlowV6Id>> m_label6;

    std::unordered_map<Ptr<Node>, std::map<FlowV4Id, uint16_t>> m_flow4;
    std::unordered_map<Ptr<Node>, std::map<FlowV6Id, uint16_t>> m_flow6;

    std::set<FlowV4Id> m_delete4;
    std::set<FlowV6Id> m_delete6;

    std::map<FlowV4Id, uint64_t> m_v4count;
    std::map<FlowV6Id, uint64_t> m_v6count;

	bool ProcessNICData4(CommandHeader cmd);
    bool ProcessNICData6(CommandHeader cmd);
    uint16_t AllocateLabel(Ptr<Node> node);

    Ptr<Node> GetNode(uint16_t id);

    void GenNICUpdateCompress4(FlowV4Id id, uint16_t nodeId, uint16_t label);
    void GenNICUpdateCompress6(FlowV6Id id, uint16_t nodeId, uint16_t label);

    void GenNICUpdateDecompress4(FlowV4Id id, std::pair<uint16_t, uint16_t> mp);
    void GenNICUpdateDecompress6(FlowV6Id id, std::pair<uint16_t, uint16_t> mp);

    void GenNICDeleteCompress4(uint16_t nodeId, FlowV4Id id);
    void GenNICDeleteCompress6(uint16_t nodeId, FlowV6Id id);

    void SendCommand(CommandHeader& cmd);

    void EraseFlow4(const std::map<FlowV4Id, std::vector<Ptr<Node>>>& mp);
    void EraseFlow6(const std::map<FlowV6Id, std::vector<Ptr<Node>>>& mp);

    void GenSwitchUpdate(std::pair<uint16_t, uint16_t> mp, uint16_t newLabel, uint32_t devId);

    void ClearNode(Ptr<Node> node);
    void ClearFlow();
};

} // namespace ns3

#endif /* CONTROL_NODE_H */