#ifndef POINT_TO_POINT_QUEUE_H
#define POINT_TO_POINT_QUEUE_H

#include "ns3/drop-tail-queue.h"
#include "ns3/random-variable-stream.h"
#include "point-to-point-net-device.h"

#include <vector>

namespace ns3
{

class PointToPointQueue : public Queue<Packet>
{
public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * \brief PointToPointQueue Constructor
     */
    PointToPointQueue();

    ~PointToPointQueue() override;

    bool Enqueue(Ptr<Packet> packet) override;
    Ptr<Packet> Dequeue(bool pause);
    Ptr<Packet> Dequeue() override;
    Ptr<Packet> Remove() override;
    Ptr<const Packet> Peek() const override;

    bool IsEmpty() const override;
    uint32_t GetNBytes() const override;

    uint64_t GetEcnCount();

protected:
    std::vector<Ptr<DropTailQueue<Packet>>> m_queues;
    uint32_t m_ecnSize{0};
    uint32_t m_ecnThreshold;
    uint64_t m_ecnCount{0};
    Ptr<UniformRandomVariable> m_random;

    bool SetEcn();
};

} // namespace ns3

#endif /* POINT_TO_POINT_QUEUE_H */