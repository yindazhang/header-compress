#ifndef RSVP_OBJECT_H
#define RSVP_OBJECT_H

#include "ns3/buffer.h"
#include "ns3/object-factory.h"
#include "ns3/object.h"

namespace ns3
{

class RsvpObject : public Object {
public:
	RsvpObject();
	~RsvpObject() override;

	static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    virtual void Print(std::ostream& os) const = 0;
    virtual void Serialize(Buffer::Iterator start) const = 0;
    virtual uint32_t Deserialize(Buffer::Iterator start) = 0;
    virtual uint32_t GetSerializedSize() const = 0;

    virtual uint16_t GetKind() const = 0;

    static Ptr<RsvpObject> CreateObject(uint16_t kind);

	enum CClass
	{
		Session = 1,
        Hop = 3,
        Integrity = 4,
        TimeValue = 5,
        ErrorSpec = 6,
        Scope = 7,
        Style = 8,
		FlowSpec = 9,
		FilterSpec = 10,
        SenderTemplate = 11,
        SenderSpec = 12,
        AdSpec = 13,
        PolicyData = 14,
        ResvConfirm = 15,
        Label = 16,
		LabelRequest = 19,
		ExplicitRoute = 20,
		RecordRoute = 21,
		Hello = 22,
		SessionAttribute = 207,
        Compress = 0xf1
	};

    /* Class-Num = 11bbbbbb

        The node should ignore the object but forward it,
        unexamined and unmodified, in all messages resulting
        from this message.
    */

    enum Error
	{
        ConflictStyle = 5,
        UnknownStyle = 6,
        ConflictDestPort = 7,
        ConflictSendPort = 8,
		UnknownClass = 13,
        UnknownType = 14
	};

};


} // namespace ns3

#endif /* RSVP_OBJECT_H */