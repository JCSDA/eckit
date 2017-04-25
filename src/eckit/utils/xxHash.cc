/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#include <iostream>
#include <cstring>

#include "eckit/exception/Exceptions.h"

#include "eckit/utils/xxHash.h"

namespace eckit {

xxHash::xxHash() {
    ctx_ = XXH64_createState();
    XXH64_reset(ctx_, 0);
}

xxHash::xxHash(const char* s) {
    ctx_ = XXH64_createState();
    XXH64_reset(ctx_, 0);
    add( s, strlen(s) );
}

xxHash::xxHash(const std::string& s) {
    ctx_ = XXH64_createState();
    XXH64_reset(ctx_, 0);
    add( s.c_str(), s.size() );
}

xxHash::xxHash(const void* data, size_t len) {
    ctx_ = XXH64_createState();
    XXH64_reset(ctx_, 0);
    add( data, len );
}

xxHash::~xxHash() {}

void xxHash::add(const void* buffer, long length) {
    if (length > 0) {
        XXH64_update(ctx_, static_cast<const unsigned char*>(buffer), length);
        if (!digest_.empty())
            digest_ = digest_t(); // reset the digest
    }
}

static const char* hex = "0123456789abcdef";

xxHash::digest_t xxHash::digest() const {

    if (digest_.empty()) { // recompute the digest

        XXH64_hash_t hash =  XXH64_digest(ctx_);

        char buffer[2 * 8];

        for(int i = 2*8; i--; ) {
            buffer[i] = hex[hash & 15];
            hash >>= 4;
        }

        digest_ = std::string(buffer, buffer+2*8);
    }

    return digest_;
}

} // namespace eckit