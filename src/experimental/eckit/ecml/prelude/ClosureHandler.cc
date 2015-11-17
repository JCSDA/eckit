/*
 * (C) Copyright 1996-2015 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "experimental/eckit/ecml/parser/Request.h"
#include "experimental/eckit/ecml/prelude/ClosureHandler.h"

using namespace std;

namespace eckit {

ClosureHandler::ClosureHandler(const string& name)
: SpecialFormHandler(name)
{}

Request ClosureHandler::handle(const Request request, ExecutionContext&)
{
    return Cell::clone(request);
}

} // namespace eckit
