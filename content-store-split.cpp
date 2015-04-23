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
						.AddConstructor<Splitcache>();
						/*
						.AddAttribute("SNormal",
									  "Sets the cache size for the normal cache",
									  StringValue("100"),
									  // This might break
									  MakeStringAccessor(&Splitcache::m_normal_size),
									  MakeStringChecker()
									  )
						.AddAttribute("SSpecial",
									  "Sets the cache size for the special cache",
									  StringValue("100"),
									  // This might break
									  MakeStringAccessor(&Splitcache::m_special_size),
									  MakeStringChecker()
									  )
						.AddAttribute("PNormal",
									  "Sets the cache Policy for the normal cache",
									  StringValue("ns3::ndn::cs::Lru"),
									  // This might break
									  MakeStringAccessor(&Splitcache::m_normal_policy),
									  MakeStringChecker()
									  )
						.AddAttribute("PSpecial",
									  "Sets the cache Policy for the special cache",
									  StringValue("ns3::ndn::cs::Lru"),
									  // This might break
									  MakeStringAccessor(&Splitcache::m_special_policy),
									  MakeStringChecker()
									  */

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
	ObjectFactory m_contentStoreFactory;

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
	
	m_contentStoreFactory.SetTypeId("ns3::ndn::cs::Lru");
	m_contentStoreFactory.Set("MaxSize", StringValue("15"));
	m_normal = m_contentStoreFactory.Create<ContentStore>();

	m_contentStoreFactory.SetTypeId("ns3::ndn::cs::Lru");
	m_contentStoreFactory.Set("MaxSize", StringValue("15"));
	m_special = m_contentStoreFactory.Create<ContentStore>();



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
	if( interest->getName().toUri().find("special") < 10 )
	{
		//NS_LOG_DEBUG("SPECIAL DATA");

		return m_special->Lookup(interest);
		//this->m_cacheMissesTrace(interest);
	}
	// If normal data
	else 
	{
		//NS_LOG_DEBUG("NORMAL DATA");
		return m_normal->Lookup(interest);
		//this->m_cacheHitsTrace(interest);
	}
}

bool
Splitcache::Add(shared_ptr<const Data> data)
{
	// if special data

	// If special data
	NS_LOG_DEBUG("Data Add Request");
	NS_LOG_DEBUG(data->getName());

	NS_LOG_DEBUG("=== MAX CACHE SIZE " << m_special->GetMaxSize());
	NS_LOG_DEBUG("=== MAX CACHE SIZE " << m_normal->GetMaxSize());

	if (m_special->GetSize() >= 10)
	{
		NS_LOG_DEBUG("============ MAKING INTO LOOP");
		m_special->SetMaxSize(6);
	}

	NS_LOG_DEBUG("=== MAX CACHE SIZE SPECIAL" << m_special->GetMaxSize());
	NS_LOG_DEBUG("=== MAX CACHE CURRENT MEMBERS " << m_special->GetSize());


	/*
	m_normal->SetMaxSize(200);
	NS_LOG_DEBUG("=== MAX CACHE SIZE " << m_normal->GetMaxSize());
	m_normal->SetMaxSize(150);
	NS_LOG_DEBUG("=== MAX CACHE SIZE " << m_normal->GetMaxSize());
	*/

	if (data->getName().toUri().find("special") < 10)
	{

		// Can we add? Is the special cache full? do we expand?

		// This gives the number of entries in the store
		NS_LOG_DEBUG("Special: " << m_special->GetSize() << " Normal " << m_normal->GetSize());

		//NS_LOG_DEBUG(m_special->MaxSize());

		return m_special->Add(data);
	}
	// If normal data
	else
	{
		NS_LOG_DEBUG("Data for Normal Cache");
		NS_LOG_DEBUG("Special: " << m_special->GetSize() << " Normal " << m_normal->GetSize());
		return m_normal->Add(data);
	}
  return false;
}

void
Splitcache::Print(std::ostream& os) const
{
}

// Our shiny new functions





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
