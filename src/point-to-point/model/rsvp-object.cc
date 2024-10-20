#include "rsvp-object.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include "rsvp-error-spec4.h"
#include "rsvp-error-spec6.h"
#include "rsvp-filter-spec4.h"
#include "rsvp-filter-spec6.h"
#include "rsvp-hop4.h"
#include "rsvp-hop6.h"
#include "rsvp-lsp4.h"
#include "rsvp-lsp6.h"
#include "rsvp-flow-spec.h"
#include "rsvp-sender-spec.h"
#include "rsvp-label-request.h"
#include "rsvp-label.h"
#include "rsvp-style.h"
#include "rsvp-time-value.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RsvpObject");

NS_OBJECT_ENSURE_REGISTERED(RsvpObject);

RsvpObject::RsvpObject()
{
}

RsvpObject::~RsvpObject()
{
}

TypeId
RsvpObject::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RsvpObject")
                            .SetParent<Object>()
                            .SetGroupName("PointToPoint");
    return tid;
}

TypeId
RsvpObject::GetInstanceTypeId() const
{
    return GetTypeId();
}

Ptr<RsvpObject> 
RsvpObject::CreateObject(uint16_t kind)
{
    static ObjectFactory objectFactory;
    static std::map<uint16_t, TypeId> toTid = {
        {((uint16_t)RsvpObject::Session << 8) | 7, RsvpLsp4::GetTypeId()},
        {((uint16_t)RsvpObject::Session << 8) | 8, RsvpLsp6::GetTypeId()},
        {((uint16_t)RsvpObject::Hop << 8) | 1, RsvpHop4::GetTypeId()},
        {((uint16_t)RsvpObject::Hop << 8) | 2, RsvpHop6::GetTypeId()},
        {((uint16_t)RsvpObject::TimeValue << 8) | 1, RsvpTimeValue::GetTypeId()},
        {((uint16_t)RsvpObject::ErrorSpec << 8) | 1, RsvpErrorSpec4::GetTypeId()},
        {((uint16_t)RsvpObject::ErrorSpec << 8) | 2, RsvpErrorSpec6::GetTypeId()},
        {((uint16_t)RsvpObject::Style << 8) | 1, RsvpStyle::GetTypeId()},
        {((uint16_t)RsvpObject::FlowSpec << 8) | 2, RsvpFlowSpec::GetTypeId()},
        {((uint16_t)RsvpObject::SenderSpec << 8) | 2, RsvpSenderSpec::GetTypeId()},
        {((uint16_t)RsvpObject::FilterSpec << 8) | 7, RsvpFilterSpec4::GetTypeId()},
        {((uint16_t)RsvpObject::FilterSpec << 8) | 8, RsvpFilterSpec6::GetTypeId()},
        {((uint16_t)RsvpObject::Label << 8) | 1, RsvpLabel::GetTypeId()},
        {((uint16_t)RsvpObject::LabelRequest << 8) | 1, RsvpLabelRequest::GetTypeId()},
        // {((uint16_t)RsvpObject::CompressLay3 << 8) | 4, RsvpCompressIpv4::GetTypeId()},
        // {((uint16_t)RsvpObject::CompressLay3 << 8) | 6, RsvpCompressIpv6::GetTypeId()},
        // {((uint16_t)RsvpObject::CompressLay4 << 8) | 6, RsvpCompressTcp::GetTypeId()}
        };
 
    if(toTid.find(kind) == toTid.end()){
        std::cout << "Cannot find " << kind << " in toTid map" << std::endl;
        return nullptr;
    }

    objectFactory.SetTypeId(toTid[kind]);
    return objectFactory.Create<RsvpObject>();
}

} // namespace ns3
