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

#ifndef TRIE_NORMAL_H_
#define TRIE_NORMAL_H_

/// @cond include_hidden

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ptr.h"

#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <tuple>
#include <boost/foreach.hpp>
#include <boost/mpl/if.hpp>

namespace ns3 {
namespace ndn {
namespace ndnSIM {

/////////////////////////////////////////////////////
// Allow customization for payload
//
template<typename Payload, typename BasePayload = Payload>
struct pointer_payload_traits {
  typedef Payload payload_type;  // general type of the payload
  typedef Payload* storage_type; // how the payload is actually stored
  typedef Payload* insert_type;  // what parameter is inserted

  typedef Payload* return_type;             // what is returned on access
  typedef const Payload* const_return_type; // what is returned on const access

  typedef BasePayload*
    base_type; // base type of the entry (when implementation details need to be hidden)
  typedef const BasePayload*
    const_base_type; // const base type of the entry (when implementation details need to be hidden)

  static Payload* empty_payload;
};

template<typename Payload, typename BasePayload>
Payload* pointer_payload_traits<Payload, BasePayload>::empty_payload = 0;

template<typename Payload, typename BasePayload = Payload>
struct smart_pointer_payload_traits {
  typedef Payload payload_type;
  typedef ns3::Ptr<Payload> storage_type;
  typedef ns3::Ptr<Payload> insert_type;

  typedef ns3::Ptr<Payload> return_type;
  typedef ns3::Ptr<const Payload> const_return_type;

  typedef ns3::Ptr<BasePayload> base_type;
  typedef ns3::Ptr<const BasePayload> const_base_type;

  static ns3::Ptr<Payload> empty_payload;
};

template<typename Payload, typename BasePayload>
ns3::Ptr<Payload> smart_pointer_payload_traits<Payload, BasePayload>::empty_payload = 0;

template<typename Payload, typename BasePayload = Payload>
struct non_pointer_traits {
  typedef Payload payload_type;
  typedef Payload storage_type;
  typedef const Payload& insert_type; // nothing to insert

  typedef Payload& return_type;
  typedef const Payload& const_return_type;

  typedef BasePayload& base_type;
  typedef const BasePayload& const_base_type;

  static Payload empty_payload;
};

template<typename Payload, typename BasePayload>
Payload non_pointer_traits<Payload, BasePayload>::empty_payload = Payload();

////////////////////////////////////////////////////
// forward declarations
//
template<typename FullKey, typename PayloadTraits, typename PolicyHook>
class trie_normal;

template<typename FullKey, typename PayloadTraits, typename PolicyHook>
inline std::ostream&
operator<<(std::ostream& os, const trie_normal<FullKey, PayloadTraits, PolicyHook>& trie_normal_node);

template<typename FullKey, typename PayloadTraits, typename PolicyHook>
bool
operator==(const trie_normal<FullKey, PayloadTraits, PolicyHook>& a,
           const trie_normal<FullKey, PayloadTraits, PolicyHook>& b);

template<typename FullKey, typename PayloadTraits, typename PolicyHook>
std::size_t
hash_value(const trie_normal<FullKey, PayloadTraits, PolicyHook>& trie_normal_node);

///////////////////////////////////////////////////
// actual definition
//
template<class T, class NonConstT>
class trie_normal_iterator;

template<class T>
class trie_normal_point_iterator;

template<typename FullKey, typename PayloadTraits, typename PolicyHook>
class trie_normal {
public:
  typedef typename FullKey::value_type Key;

  typedef trie_normal* iterator;
  typedef const trie_normal* const_iterator;

  typedef trie_normal_iterator<trie, trie_normal> recursive_iterator;
  typedef trie_normal_iterator<const trie_normal, trie_normal> const_recursive_iterator;

  typedef trie_normal_point_iterator<trie> point_iterator;
  typedef trie_normal_point_iterator<const trie_normal> const_point_iterator;

  typedef PayloadTraits payload_traits;

  inline trie_normal(const Key& key, size_t bucketSize = 1, size_t bucketIncrement = 1)
    : key_(key)
    , initialBucketSize_(bucketSize)
    , bucketIncrement_(bucketIncrement)
    , bucketSize_(initialBucketSize_)
    , buckets_(new bucket_type[bucketSize_]) // cannot use normal pointer, because lifetime of
                                             // buckets should be larger than lifetime of the
                                             // container
    , children_(bucket_traits(buckets_.get(), bucketSize_))
    , payload_(PayloadTraits::empty_payload)
    , parent_(nullptr)
  {
  }

  inline ~trie()
  {
    payload_ = PayloadTraits::empty_payload; // necessary for smart pointers...
    children_.clear_and_dispose(trie_delete_disposer());
  }

  void
  clear()
  {
    children_.clear_and_dispose(trie_delete_disposer());
  }

  template<class Predicate>
  void
  clear_if(Predicate cond)
  {
    recursive_iterator trie_normalNode(this);
    recursive_iterator end(0);

    while (trieNode != end) {
      if (cond(*trieNode)) {
        trie_normalNode = recursive_iterator(trieNode->erase());
      }
      trie_normalNode++;
    }
  }

  // actual entry
  friend bool operator==<>(const trie_normal<FullKey, PayloadTraits, PolicyHook>& a,
                           const trie_normal<FullKey, PayloadTraits, PolicyHook>& b);

  friend std::size_t
  hash_value<>(const trie_normal<FullKey, PayloadTraits, PolicyHook>& trie_normal_node);

  inline std::pair<iterator, bool>
  insert(const FullKey& key, typename PayloadTraits::insert_type payload)
  {
    trie_normal* trie_normalNode = this;

    BOOST_FOREACH (const Key& subkey, key) {
      typename unordered_set::iterator item = trie_normalNode->children_.find(subkey);
      if (item == trie_normalNode->children_.end()) {
        trie_normal* newNode = new trie_normal(subkey, initialBucketSize_, bucketIncrement_);
        // std::cout << "new " << newNode << "\n";
        newNode->parent_ = trie_normalNode;

        if (trieNode->children_.size() >= trie_normalNode->bucketSize_) {
          trie_normalNode->bucketSize_ += trie_normalNode->bucketIncrement_;
          trie_normalNode->bucketIncrement_ *= 2; // increase bucketIncrement exponentially

          buckets_array newBuckets(new bucket_type[trieNode->bucketSize_]);
          trie_normalNode->children_.rehash(bucket_traits(newBuckets.get(), trie_normalNode->bucketSize_));
          trie_normalNode->buckets_.swap(newBuckets);
        }

        std::pair<typename unordered_set::iterator, bool> ret =
          trie_normalNode->children_.insert(*newNode);

        trie_normalNode = &(*ret.first);
      }
      else
        trie_normalNode = &(*item);
    }

    if (trieNode->payload_ == PayloadTraits::empty_payload) {
      trie_normalNode->payload_ = payload;
      return std::make_pair(trieNode, true);
    }
    else
      return std::make_pair(trieNode, false);
  }

  /**
   * @brief Removes payload (if it exists) and if there are no children, prunes parents trie_normal
   */
  inline iterator
  erase()
  {
    payload_ = PayloadTraits::empty_payload;
    return prune();
  }

  /**
   * @brief Do exactly as erase, but without erasing the payload
   */
  inline iterator
  prune()
  {
    if (payload_ == PayloadTraits::empty_payload && children_.size() == 0) {
      if (parent_ == 0)
        return this;

      trie_normal* parent = parent_;
      parent->children_
        .erase_and_dispose(*this,
                           trie_normal_delete_disposer()); // delete this; basically, committing a suicide

      return parent->prune();
    }
    return this;
  }

  /**
   * @brief Perform prune of the node, but without attempting to parent of the node
   */
  inline void
  prune_node()
  {
    if (payload_ == PayloadTraits::empty_payload && children_.size() == 0) {
      if (parent_ == 0)
        return;

      trie_normal* parent = parent_;
      parent->children_
        .erase_and_dispose(*this,
                           trie_normal_delete_disposer()); // delete this; basically, committing a suicide
    }
  }

  // inline std::tuple<const iterator, bool, const iterator>
  // find (const FullKey &key) const
  // {
  //   return const_cast<trie*> (this)->find (key);
  // }

  /**
   * @brief Perform the longest prefix match
   * @param key the key for which to perform the longest prefix match
   *
   * @return ->second is true if prefix in ->first is longer than key
   */
  inline std::tuple<iterator, bool, iterator>
  find(const FullKey& key)
  {
    trie_normal* trie_normalNode = this;
    iterator foundNode = (payload_ != PayloadTraits::empty_payload) ? this : 0;
    bool reachLast = true;

    BOOST_FOREACH (const Key& subkey, key) {
      typename unordered_set::iterator item = trie_normalNode->children_.find(subkey);
      if (item == trie_normalNode->children_.end()) {
        reachLast = false;
        break;
      }
      else {
        trie_normalNode = &(*item);

        if (trieNode->payload_ != PayloadTraits::empty_payload)
          foundNode = trie_normalNode;
      }
    }

    return std::make_tuple(foundNode, reachLast, trie_normalNode);
  }

  /**
   * @brief Perform the longest prefix match satisfying preficate
   * @param key the key for which to perform the longest prefix match
   *
   * @return ->second is true if prefix in ->first is longer than key
   */
  template<class Predicate>
  inline std::tuple<iterator, bool, iterator>
  find_if(const FullKey& key, Predicate pred)
  {
    trie_normal* trie_normalNode = this;
    iterator foundNode = (payload_ != PayloadTraits::empty_payload) ? this : 0;
    bool reachLast = true;

    BOOST_FOREACH (const Key& subkey, key) {
      typename unordered_set::iterator item = trie_normalNode->children_.find(subkey);
      if (item == trie_normalNode->children_.end()) {
        reachLast = false;
        break;
      }
      else {
        trie_normalNode = &(*item);

        if (trieNode->payload_ != PayloadTraits::empty_payload && pred(trieNode->payload_)) {
          foundNode = trie_normalNode;
        }
      }
    }

    return std::make_tuple(foundNode, reachLast, trie_normalNode);
  }

  /**
   * @brief Find next payload of the sub-trie
   * @returns end() or a valid iterator pointing to the trie_normal leaf (order is not defined, enumeration
   * )
   */
  inline iterator
  find()
  {
    if (payload_ != PayloadTraits::empty_payload)
      return this;

    typedef trie_normal<FullKey, PayloadTraits, PolicyHook> trie_normal;
    for (typename trie_normal::unordered_set::iterator subnode = children_.begin();
         subnode != children_.end(); subnode++)
    // BOOST_FOREACH (trie &subnode, children_)
    {
      iterator value = subnode->find();
      if (value != 0)
        return value;
    }

    return 0;
  }

  /**
   * @brief Find next payload of the sub-trie satisfying the predicate
   * @param pred predicate
   * @returns end() or a valid iterator pointing to the trie_normal leaf (order is not defined, enumeration
   * )
   */
  template<class Predicate>
  inline const iterator
  find_if(Predicate pred)
  {
    if (payload_ != PayloadTraits::empty_payload && pred(payload_))
      return this;

    typedef trie_normal<FullKey, PayloadTraits, PolicyHook> trie_normal;
    for (typename trie_normal::unordered_set::iterator subnode = children_.begin();
         subnode != children_.end(); subnode++)
    // BOOST_FOREACH (const trie_normal &subnode, children_)
    {
      iterator value = subnode->find_if(pred);
      if (value != 0)
        return value;
    }

    return 0;
  }

  /**
   * @brief Find next payload of the sub-trie satisfying the predicate
   * @param pred predicate
   *
   * This version check predicate only for the next level children
   *
   * @returns end() or a valid iterator pointing to the trie_normal leaf (order is not defined, enumeration
   *)
   */
  template<class Predicate>
  inline const iterator
  find_if_next_level(Predicate pred)
  {
    typedef trie_normal<FullKey, PayloadTraits, PolicyHook> trie_normal;
    for (typename trie_normal::unordered_set::iterator subnode = children_.begin();
         subnode != children_.end(); subnode++) {
      if (pred(subnode->key())) {
        return subnode->find();
      }
    }

    return 0;
  }

  iterator
  end()
  {
    return 0;
  }

  const_iterator
  end() const
  {
    return 0;
  }

  typename PayloadTraits::const_return_type
  payload() const
  {
    return payload_;
  }

  typename PayloadTraits::return_type
  payload()
  {
    return payload_;
  }

  void
  set_payload(typename PayloadTraits::insert_type payload)
  {
    payload_ = payload;
  }

  Key
  key() const
  {
    return key_;
  }

  inline void
  PrintStat(std::ostream& os) const;

private:
  // The disposer object function
  struct trie_normal_delete_disposer {
    void
    operator()(trie* delete_this)
    {
      delete delete_this;
    }
  };

  template<class D>
  struct array_disposer {
    void
    operator()(D* array)
    {
      delete[] array;
    }
  };

  friend std::ostream& operator<<<>(std::ostream& os, const trie_normal& trie_normal_node);

public:
  PolicyHook policy_hook_;

private:
  boost::intrusive::unordered_set_member_hook<> unordered_set_member_hook_;

  // necessary typedefs
  typedef trie_normal self_type;
  typedef boost::intrusive::member_hook<trie, boost::intrusive::unordered_set_member_hook<>,
                                        &trie::unordered_set_member_hook_> member_hook;

  typedef boost::intrusive::unordered_set<trie, member_hook> unordered_set;
  typedef typename unordered_set::bucket_type bucket_type;
  typedef typename unordered_set::bucket_traits bucket_traits;

  template<class T, class NonConstT>
  friend class trie_normal_iterator;

  template<class T>
  friend class trie_normal_point_iterator;

  ////////////////////////////////////////////////
  // Actual data
  ////////////////////////////////////////////////

  Key key_; ///< name component

  size_t initialBucketSize_;
  size_t bucketIncrement_;

  size_t bucketSize_;
  typedef boost::interprocess::unique_ptr<bucket_type, array_disposer<bucket_type>> buckets_array;
  buckets_array buckets_;
  unordered_set children_;

  typename PayloadTraits::storage_type payload_;
  trie_normal* parent_; // to make cleaning effective
};

template<typename FullKey, typename PayloadTraits, typename PolicyHook>
inline std::ostream&
operator<<(std::ostream& os, const trie_normal<FullKey, PayloadTraits, PolicyHook>& trie_normal_node)
{
  os << "# " << trie_normal_node.key_ << ((trie_node.payload_ != PayloadTraits::empty_payload) ? "*" : "")
     << std::endl;
  typedef trie_normal<FullKey, PayloadTraits, PolicyHook> trie_normal;

  for (typename trie_normal::unordered_set::const_iterator subnode = trie_normal_node.children_.begin();
       subnode != trie_normal_node.children_.end(); subnode++)
  // BOOST_FOREACH (const trie_normal &subnode, trie_normal_node.children_)
  {
    os << "\"" << &trie_node << "\""
       << " [label=\"" << trie_normal_node.key_
       << ((trie_node.payload_ != PayloadTraits::empty_payload) ? "*" : "") << "\"]\n";
    os << "\"" << &(*subnode) << "\""
       << " [label=\"" << subnode->key_
       << ((subnode->payload_ != PayloadTraits::empty_payload) ? "*" : "") << "\"]"
                                                                              "\n";

    os << "\"" << &trie_node << "\""
       << " -> "
       << "\"" << &(*subnode) << "\""
       << "\n";
    os << *subnode;
  }

  return os;
}

template<typename FullKey, typename PayloadTraits, typename PolicyHook>
inline void
trie<FullKey, PayloadTraits, PolicyHook>::PrintStat(std::ostream& os) const
{
  os << "# " << key_ << ((payload_ != PayloadTraits::empty_payload) ? "*" : "") << ": "
     << children_.size() << " children" << std::endl;
  for (size_t bucket = 0, maxbucket = children_.bucket_count(); bucket < maxbucket; bucket++) {
    os << " " << children_.bucket_size(bucket);
  }
  os << "\n";

  typedef trie_normal<FullKey, PayloadTraits, PolicyHook> trie_normal;
  for (typename trie_normal::unordered_set::const_iterator subnode = children_.begin();
       subnode != children_.end(); subnode++)
  // BOOST_FOREACH (const trie_normal &subnode, children_)
  {
    subnode->PrintStat(os);
  }
}

template<typename FullKey, typename PayloadTraits, typename PolicyHook>
inline bool
operator==(const trie_normal<FullKey, PayloadTraits, PolicyHook>& a,
           const trie_normal<FullKey, PayloadTraits, PolicyHook>& b)
{
  return a.key_ == b.key_;
}

template<typename FullKey, typename PayloadTraits, typename PolicyHook>
inline std::size_t
hash_value(const trie_normal<FullKey, PayloadTraits, PolicyHook>& trie_normal_node)
{
  return boost::hash_value(trie_node.key_);
}

template<class Trie, class NonConstTrie> // hack for boost < 1.47
class trie_normal_iterator {
public:
  trie_normal_iterator()
    : trie_normal_(0)
  {
  }
  trie_normal_iterator(typename Trie::iterator item)
    : trie_normal_(item)
  {
  }
  trie_normal_iterator(Trie& item)
    : trie_normal_(&item)
  {
  }

  Trie& operator*()
  {
    return *trie_;
  }
  const Trie& operator*() const
  {
    return *trie_;
  }
  Trie* operator->()
  {
    return trie_normal_;
  }
  const Trie* operator->() const
  {
    return trie_normal_;
  }
  bool
  operator==(trie_iterator<const Trie, NonConstTrie>& other) const
  {
    return (trie_ == other.trie_);
  }
  bool
  operator==(trie_iterator<Trie, NonConstTrie>& other)
  {
    return (trie_ == other.trie_);
  }
  bool
  operator!=(trie_iterator<const Trie, NonConstTrie>& other) const
  {
    return !(*this == other);
  }
  bool
  operator!=(trie_iterator<Trie, NonConstTrie>& other)
  {
    return !(*this == other);
  }

  trie_normal_iterator<Trie, NonConstTrie>&
  operator++(int)
  {
    if (trie_->children_.size() > 0)
      trie_normal_ = &(*trie_->children_.begin());
    else
      trie_normal_ = goUp();
    return *this;
  }

  trie_normal_iterator<Trie, NonConstTrie>&
  operator++()
  {
    (*this)++;
    return *this;
  }

private:
  typedef typename boost::mpl::if_<boost::is_same<Trie, NonConstTrie>,
                                   typename Trie::unordered_set::iterator,
                                   typename Trie::unordered_set::const_iterator>::type set_iterator;

  Trie*
  goUp()
  {
    if (trie_->parent_ != 0) {
      // typename Trie::unordered_set::iterator item =
      set_iterator item = const_cast<NonConstTrie*>(trie_)
                            ->parent_->children_.iterator_to(const_cast<NonConstTrie&>(*trie_));
      item++;
      if (item != trie_normal_->parent_->children_.end()) {
        return &(*item);
      }
      else {
        trie_normal_ = trie_normal_->parent_;
        return goUp();
      }
    }
    else
      return 0;
  }

private:
  Trie* trie_normal_;
};

template<class Trie>
class trie_normal_point_iterator {
private:
  typedef typename boost::mpl::if_<boost::is_same<Trie, const Trie>,
                                   typename Trie::unordered_set::const_iterator,
                                   typename Trie::unordered_set::iterator>::type set_iterator;

public:
  trie_normal_point_iterator()
    : trie_normal_(0)
  {
  }
  trie_normal_point_iterator(typename Trie::iterator item)
    : trie_normal_(item)
  {
  }
  trie_normal_point_iterator(Trie& item)
  {
    if (item.children_.size() != 0)
      trie_normal_ = &*item.children_.begin();
    else
      trie_normal_ = 0;
  }

  Trie& operator*()
  {
    return *trie_;
  }
  const Trie& operator*() const
  {
    return *trie_;
  }
  Trie* operator->()
  {
    return trie_normal_;
  }
  const Trie* operator->() const
  {
    return trie_normal_;
  }
  bool
  operator==(trie_point_iterator<const Trie>& other) const
  {
    return (trie_ == other.trie_);
  }
  bool
  operator==(trie_point_iterator<Trie>& other)
  {
    return (trie_ == other.trie_);
  }
  bool
  operator!=(trie_point_iterator<const Trie>& other) const
  {
    return !(*this == other);
  }
  bool
  operator!=(trie_point_iterator<Trie>& other)
  {
    return !(*this == other);
  }

  trie_normal_point_iterator<Trie>&
  operator++(int)
  {
    if (trie_->parent_ != 0) {
      set_iterator item = trie_normal_->parent_->children_.iterator_to(*trie_);
      item++;
      if (item == trie_normal_->parent_->children_.end())
        trie_normal_ = 0;
      else
        trie_normal_ = &*item;
    }
    else {
      trie_normal_ = 0;
    }
    return *this;
  }

  trie_normal_point_iterator<Trie>&
  operator++()
  {
    (*this)++;
    return *this;
  }

private:
  Trie* trie_normal_;
};

} // ndnSIM
} // ndn
} // ns3

/// @endcond

#endif // TRIE_NORMAL_H_
