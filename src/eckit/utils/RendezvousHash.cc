/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/utils/RendezvousHash.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/thread/AutoLock.h"
#include "eckit/types/Types.h"

namespace eckit {

//----------------------------------------------------------------------------------------------------------------------


RendezvousHash::RendezvousHash(const std::set<RendezvousHash::Node>& nodes, const RendezvousHash::hash_func_ptr hash) :
    nodes_(nodes),
    hash_(hash)
{
}

RendezvousHash::~RendezvousHash()
{
}

bool RendezvousHash::addNode(const RendezvousHash::Node& node)
{
    AutoLock<Mutex> lock(mutex_);

    std::pair<iterator, bool> r = nodes_.insert(node);

    return r.second;
}

bool RendezvousHash::removeNode(const RendezvousHash::Node& node)
{
    AutoLock<Mutex> lock(mutex_);

    return bool(nodes_.erase(node));
}

RendezvousHash::Node RendezvousHash::selectNode(const RendezvousHash::Key& key)
{
    AutoLock<Mutex> lock(mutex_);

    std::string skey = flatten(key);

    iterator highest = nodes_.end();
    std::string hashest;

    std::vector<std::string> vs;

    for(iterator itr = nodes_.begin(); itr != nodes_.end(); ++itr) {
        std::string toHash = skey + "+" + *itr;
        std::string h = hash_(toHash);

        eckit::Log::info() << "node=" << *itr << ", str=" << toHash << ", hash = " << h << std::endl;

        if(h > hashest) {
            hashest = h;
            highest = itr;
        }

        vs.push_back(h);
    }

    std::sort(vs.begin(), vs.end());

    eckit::Log::info() << vs << std::endl;

    ASSERT(highest != nodes_.end());

    eckit::Log::info() << "highest=" << *highest << ", hashest=" << hashest << std::endl;

    return *highest;
}

std::string RendezvousHash::flatten(const RendezvousHash::Key& key) const
{
    std::ostringstream flat;

    for(Key::const_iterator itr = key.begin(); itr != key.end(); ++itr) {
        flat << "/" << itr->first << ":" << itr->second;
    }

    return flat.str();
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace eckit
