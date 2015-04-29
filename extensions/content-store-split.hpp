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

#ifndef NDN_CONTENT_STORE_SPLITCACHE_H
#define NDN_CONTENT_STORE_SPLITCACHE_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ndnSIM/model/cs/ndn-content-store.hpp"
//#include "ns3/ndnSIM/model/cs/content-store-impl.hpp"

namespace ns3 {
namespace ndn {
namespace cs {

/**
 * @ingroup ndn-cs
 * @brief Implementation of ContentStore that has two underlying caches
 */
class Splitcache : public ContentStore {
public:

	std::string m_string_parameter;
	std::string m_normal_policy;
	std::string m_special_policy;

	int incr;

	std::uint32_t m_total_size;
	std::uint32_t m_normal_size;
	std::uint32_t m_special_size;
	std::uint32_t m_special_max_size;

	Ptr<ContentStore> m_normal;
	Ptr<ContentStore> m_special;

  /**
   * \brief Interface ID
   *
   * \return interface ID
   */
  static TypeId
  GetTypeId();

  /**
   * @brief Default constructor
   */
  Splitcache();

  /**
   * @brief Virtual destructor
   */
  virtual ~Splitcache();

  virtual shared_ptr<Data>
  Lookup(shared_ptr<const Interest> interest);

  virtual bool
  Add(shared_ptr<const Data> data);

  virtual void
  Print(std::ostream& os) const;

  // Our functions
  void SetStringAttr(std::string attr);
  void SetNormalPolicy(std::string attr);
  void SetSpecialPolicy(std::string attr);
  void SetTotalCacheSize(std::string attr);
  void SetPercentageSpecial(std::string attr);
  void Configure(std::string attr);

  virtual uint32_t
  GetSize() const;

  virtual Ptr<cs::Entry>
  Begin();

  virtual Ptr<cs::Entry>
  End();

  virtual uint32_t
  GetMaxSize() const;

  virtual void
  SetMaxSize(uint32_t maxSize);

  virtual Ptr<cs::Entry> Next(Ptr<cs::Entry>);

protected:
	TracedCallback<shared_ptr<const Interest>,
		shared_ptr<const Data >> m_cacheNormalHitsTrace; ///< @brief trace of Normal cache hits
	TracedCallback<shared_ptr<const Interest>,
		shared_ptr<const Data >> m_cacheSpecialHitsTrace; ///< @brief trace of Special cache hits

	TracedCallback<shared_ptr<const Interest>> m_cacheNormalMissesTrace; ///< @brief trace of cache misses
	TracedCallback<shared_ptr<const Interest>> m_cacheSpecialMissesTrace; ///< @brief trace of cache misses

};

} // namespace cs
} // namespace ndn
} // namespace ns3

#endif // NDN_CONTENT_STORE_SPLITCACHE_H
