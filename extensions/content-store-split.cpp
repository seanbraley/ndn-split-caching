/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "content-store-split.hpp"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/string.h"

// Might not nee this
//#include "ns3/object-factory.h"
#include "ns3/core-module.h"

NS_LOG_COMPONENT_DEFINE("ndn.cs.Splitcache");

namespace ns3 {
namespace ndn {
namespace cs {

NS_OBJECT_ENSURE_REGISTERED(Splitcache);

TypeId
Splitcache::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::ndn::cs::Splitcache")
		.SetGroupName("Ndn")
		.SetParent<ContentStore>()
		.AddConstructor<Splitcache>()

		.AddAttribute("NormalPolicy",
			"URI for the Normal policy",
			StringValue(""),
			MakeStringAccessor(&Splitcache::SetNormalPolicy),
			MakeStringChecker()
		)
		.AddAttribute("SpecialPolicy",
			"URI for the Special policy",
			StringValue(""),
			MakeStringAccessor(&Splitcache::SetSpecialPolicy),
			MakeStringChecker()
		)
		.AddAttribute("TotalCacheSize",
			"Value for the total cache size",
			UintegerValue(100),
			MakeUintegerAccessor(&Splitcache::m_total_size),
			MakeUintegerChecker<uint32_t>()
		)
		.AddAttribute("Configure",
			"Configure the split.",
			StringValue(""),
			MakeStringAccessor(&Splitcache::Configure),
			MakeStringChecker()
		)
		.AddTraceSource("CacheNormalHits", "Trace called every time there is a normal cache hit",
			MakeTraceSourceAccessor(&Splitcache::m_cacheNormalHitsTrace))
		.AddTraceSource("CacheSpecialHits", "Trace called every time there is a special cache hit",
			MakeTraceSourceAccessor(&Splitcache::m_cacheSpecialHitsTrace))
		.AddTraceSource("CacheNormalMisses", "Trace called every time there is a normal cache miss",
			MakeTraceSourceAccessor(&Splitcache::m_cacheNormalMissesTrace))
		.AddTraceSource("CacheSpecialMisses", "Trace called every time there is a special cache miss",
			MakeTraceSourceAccessor(&Splitcache::m_cacheSpecialMissesTrace))
		;

  return tid;
}
// Things to do with Hesham tomorrow
// TODO: Set up traces
// TODO: Try and set up attributes
// TODO: How to access policy things such as Set/Get MaxSize from HERE
//			This would involve some access to the underlying object


/*
 * @brief Creates an object factory which makes its two sub-caches
 * 
 */
Splitcache::Splitcache()
{
	// Create object factory

	// Set type and create 
	//std::cout << "Debugline" << std::endl;
	//std::cout << m_normal_policy << std::endl;
	//std::cout << m_normal_size << std::endl;

	/*
	m_contentStoreFactory.SetTypeId((m_normal_policy));
	m_contentStoreFactory.Set("MaxSize", StringValue(m_normal_size));
	m_normal = m_contentStoreFactory.Create<ContentStore>();
	
	m_contentStoreFactory.SetTypeId((m_special_policy));
	m_contentStoreFactory.Set("MaxSize", StringValue(m_special_size));
	m_special = m_contentStoreFactory.Create<ContentStore>();
	*/
	/*
	
	NS_LOG_DEBUG("I got a parameter! " << m_string_parameter << std::endl);

	m_contentStoreFactory.SetTypeId("ns3::ndn::cs::Fifo");
	m_contentStoreFactory.Set("MaxSize", StringValue("15"));
	m_normal = m_contentStoreFactory.Create<ContentStore>();

	m_contentStoreFactory.SetTypeId("ns3::ndn::cs::Fifo");
	m_contentStoreFactory.Set("MaxSize", StringValue("15"));
	m_special = m_contentStoreFactory.Create<ContentStore>();
	*/


	incr = 0;
}

Splitcache::~Splitcache()
{
}

shared_ptr<Data>
Splitcache::Lookup(shared_ptr<const Interest> interest)
{
	// normal

	// If special data
	//NS_LOG_DEBUG("Interest In Data");
	//NS_LOG_DEBUG(interest->getName());
	shared_ptr<Data> myData = 0;
	if (interest->getName().toUri().find("special") < 10)
	{
		// Get data from lookup
		myData = m_special->Lookup(interest);
		if (myData)
		{
			this->m_cacheHitsTrace(interest, myData);
			//this->m_cacheSpecialHitsTrace(interest, myData);
		}
		else
		{
			this->m_cacheMissesTrace(interest);
			//this->m_cacheSpecialMissesTrace(interest);
		}
		return myData;
	}
	// If normal data
	else 
	{

		myData = m_normal->Lookup(interest);
		if (myData)
		{
			this->m_cacheHitsTrace(interest, myData);
			//this->m_cacheNormalHitsTrace(interest, myData);
		}
		else
		{
			this->m_cacheMissesTrace(interest);
			//this->m_cacheNormalMissesTrace(interest);
		}
		return myData;
	}
}

bool
Splitcache::Add(shared_ptr<const Data> data)
{
	NS_LOG_DEBUG("Data Add Request");
	//NS_LOG_DEBUG(data->getName());
	NS_LOG_DEBUG("State of Cache: Special: " << m_special->GetSize() << " Normal " << m_normal->GetSize());
	NS_LOG_DEBUG("State of Cache: Special (MAX): " << m_special->GetMaxSize() << " Normal (MAX) " << m_normal->GetMaxSize());
	if (data->getName().toUri().find("special") < 10)
	{
		NS_LOG_DEBUG("Data Type: Special");

		if (m_special->GetSize() == m_special->GetMaxSize() && m_special->GetMaxSize() < m_special_max_size)
		{
			// Shrink normal
			NS_LOG_DEBUG("Decreasing Normal Cache Size");
			m_normal->SetMaxSize(m_normal->GetMaxSize() - 1);
			NS_LOG_DEBUG("Increasing Special Cache Size");
			m_special->SetMaxSize(m_special->GetMaxSize() + 1);
		}
		return m_special->Add(data);

	}
	// If normal data
	else
	{
		// Follow typical replacement policy
		NS_LOG_DEBUG("Data Type: Normal");
		return m_normal->Add(data);
	}
  return false;
}

void
Splitcache::Print(std::ostream& os) const
{
}

// Our shiny new functions

void
Splitcache::SetStringAttr(std::string attr)
{
	NS_LOG_DEBUG("SetStringAttr was called " << attr << std::endl);
	m_string_parameter = attr;

}

void
Splitcache::SetNormalPolicy(std::string attr)
{
	NS_LOG_DEBUG("SetNormalPolicy was called " << attr << std::endl);
	m_normal_policy = attr;
}


void
Splitcache::SetSpecialPolicy(std::string attr)
{
	NS_LOG_DEBUG("SetSpecialPolicy was called " << attr << std::endl);
	m_special_policy = attr;
}

void
Splitcache::SetTotalCacheSize(std::string attr)
{

}

void
Splitcache::SetPercentageSpecial(std::string attr)
{

}

void
Splitcache::Configure(std::string attr)
{

	NS_LOG_DEBUG("Start was Called " << attr << std::endl);
		
	// Create object factory
	ObjectFactory m_contentStoreFactory;

	// Calculate cache sizes

	// Get value (say 40)
	m_special_size = std::stoi(attr);

	// Special is total*value/100 500*40/100 = 200
	m_special_max_size = m_total_size*m_special_size / 100;

	// Normal is whats left
	m_normal_size = m_total_size - 1;

	// Normal Content Store
	m_contentStoreFactory.SetTypeId(m_normal_policy);
	m_contentStoreFactory.Set("MaxSize", StringValue(std::to_string(m_normal_size)));
	m_normal = m_contentStoreFactory.Create<ContentStore>();

	// Special Content Store STARTS AT 1
	m_contentStoreFactory.SetTypeId(m_special_policy);
	m_contentStoreFactory.Set("MaxSize", StringValue("1"));
	m_special = m_contentStoreFactory.Create<ContentStore>();

	NS_LOG_DEBUG("Created normal cache with size: " << m_normal_size << " and special with: " << "1" << std::endl);
	NS_LOG_DEBUG("Calculated Max Size: " << std::to_string(m_special_max_size) << std::endl);

}

uint32_t
Splitcache::GetSize() const
{
	NS_LOG_DEBUG("GetSize");

	return m_normal->GetSize() + m_special->GetSize();
}

Ptr<cs::Entry>
Splitcache::Begin()
{
	NS_LOG_DEBUG("Begin");

		m_normal->Begin();
		return m_special->Begin();
  return 0;
}

Ptr<cs::Entry>
Splitcache::End()
{
	NS_LOG_DEBUG("END");

	m_normal->End();
	return m_special->End();
  return 0;
}

uint32_t
Splitcache::GetMaxSize() const
{
	return 0;
}

void
Splitcache::SetMaxSize(uint32_t maxsize)
{
}

Ptr<cs::Entry> Splitcache::Next(Ptr<cs::Entry> a)
{	
	NS_LOG_DEBUG("Next");

  return 0;
}

} // namespace cs
} // namespace ndn
} // namespace ns3
