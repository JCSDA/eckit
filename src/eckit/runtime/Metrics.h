/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @author Baudouin Raoult
/// @date   Nov 2020

#ifndef eckit_Metrics_h
#define eckit_Metrics_h


#include <ctime>

#include "eckit/log/Timer.h"
#include "eckit/memory/NonCopyable.h"
#include "eckit/value/Value.h"


namespace eckit {

class Stream;
class Metric;

//----------------------------------------------------------------------------------------------------------------------

class Metrics : eckit::NonCopyable {
public:  // methods
    Metrics(bool main);
    ~Metrics();

    void set(const std::string& name, const char* value);
    void set(const std::string& name, const std::string& value);

    void set(const std::string& name, int value);
    void set(const std::string& name, unsigned int value);

    void set(const std::string& name, long value);
    void set(const std::string& name, unsigned long value);

    void set(const std::string& name, long long value);
    void set(const std::string& name, unsigned long long value);

    void set(const std::string& name, double value);

    void set(const std::string& name, const std::vector<std::string>& value);
    void set(const std::string& name, const std::set<std::string>& value);

    void timestamp(const std::string& name, time_t value);

    Metrics& object(const std::string& name);

    void error(const std::exception&);

    void send(Stream&) const;
    void receive(Stream&);

    void json(JSON&) const;

    static Metrics& current();


private:  // members
    mutable bool printed_;
    eckit::Timer timer_;

    time_t created_;

    std::vector<Metric*> metrics_;

private:  // methods
    void print(std::ostream&) const;


    friend std::ostream& operator<<(std::ostream& s, const Metrics& m) {
        m.print(s);
        return s;
    }
};

//----------------------------------------------------------------------------------------------------------------------

}  // namespace eckit

#endif
