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
                        .AddConstructor<Splitcache>()
						.AddAttribute("SizeOfNormal",
									  "Sets the cache size for the normal cache",
									  StringValue("d"),
									  // This might break
									  MakeUintegerAccessor(&Splitcache::m_normal_size),
									  MakeUintegerChecker<std::uint32_t> ()
									  )
						.AddAttribute("SizeOfSpecial",
									  "Sets the cache size for the special cache",
									  StringValue("100"),
									  // This might break
									  MakeUintegerAccessor(&Splitcache::m_special_size),
									  MakeUintegerChecker<std::uint32_t>()
									  )
						.AddAttribute("PolicyOfNormal",
									  "Sets the cache Policy for the normal cache",
									  StringValue("ns3::ndn::cs::Lru"),
									  // This might break
									  MakeStringAccessor(&Splitcache::m_normal_policy),
									  MakeStringChecker()
									  )
						.AddAttribute("PolicyOfSpecial",
									  "Sets the cache Policy for the special cache",
									  StringValue("ns3::ndn::cs::Lru"),
									  // This might break
									  MakeStringAccessor(&Splitcache::m_special_policy),
									  MakeStringChecker()
									  );

  return tid;
}
/*
 * @brief Creates an object factory which makes its two sub-caches
 * 
 */
Splitcache::Splitcache()
{
	// Create object factory
	ObjectFactory m_contentStoreFactory;

	// Set type and create 
	std::cout << "Debugline" << std::endl;
	std::cout << m_normal_policy << std::endl;
	std::cout << m_normal_size << std::endl;
	m_contentStoreFactory.SetTypeId(m_normal_policy);
	m_contentStoreFactory.Set("MaxSize", StringValue(std::to_string(m_normal_size)));
	m_normal = m_contentStoreFactory.Create<ContentStore>();
	
	m_contentStoreFactory.SetTypeId(m_special_policy);
	m_contentStoreFactory.Set("MaxSize", StringValue(std::to_string(m_special_size)));
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

	this->m_cacheMissesTrace(interest);
	// If normal data
	if (incr % 2 == 0)
	{
		return m_normal->Lookup(interest);
	}
	// If special data
	else 
	{
		return m_special->Lookup(interest);
	}
	incr++;
  return 0;
}

bool
Splitcache::Add(shared_ptr<const Data> data)
{
	// if special data


	// If normal data
	if (incr % 2 == 0)
	{
		return m_normal->Add(data);
	}
	// If special data
	else
	{
		return m_special->Add(data);
	}
	incr++;
  return false;
}

void
Splitcache::Print(std::ostream& os) const
{
}

uint32_t
Splitcache::GetSize() const
{
	m_normal->GetSize();
	return m_special->GetSize();
  return 0;
}

Ptr<cs::Entry>
Splitcache::Begin()
{
		m_normal->Begin();
		return m_special->Begin();
  return 0;
}

Ptr<cs::Entry>
Splitcache::End()
{
	m_normal->End();
	return m_special->End();
  return 0;
}

Ptr<cs::Entry> Splitcache::Next(Ptr<cs::Entry> a)
{	
	if (incr % 2 == 0)
	{
		return m_normal->Next(a);
	}
	else
	{
		return m_special->Next(a);
	}
	incr++;
  return 0;
}

} // namespace cs
} // namespace ndn
} // namespace ns3
