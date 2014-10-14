/*
 * (C) Copyright 1996-2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <sstream>

#include "eckit/xpr/Expression.h"
#include "eckit/xpr/Value.h"
#include "eckit/xpr/Scope.h"
#include "eckit/xpr/Undef.h"
#include "eckit/xpr/Param.h"
#include "eckit/xpr/ParamDef.h"


namespace eckit {
namespace xpr {

//--------------------------------------------------------------------------------------------

Expression::Expression()
{
}

Expression::Expression(const args_t &args) :
    args_(args)
{
}

Expression::Expression(args_t &args, Swap )
{
    std::swap(args_, args);
}

Expression::~Expression()
{
}

ExpPtr Expression::eval(bool optimize) const
{
    Scope ctx("Expression::eval()");
    return ExpPtr( optimise(optimize)->evaluate(ctx));
}

ExpPtr Expression::eval(Scope& ctx) const
{
    // FIXME: We assume that the code is already optimised. Needs reviewing
    return /*optimise()->*/evaluate(ctx);
}

ExpPtr Expression::resolve(Scope & ctx) const
{
    return self();
}

ExpPtr Expression::optimise(size_t) const
{
    return self();
}

ExpPtr Expression::optimise(bool doit, size_t depth) const
{
    if(doit)
    {
#ifdef _DEBUG
        for(size_t k = 0; k < depth; ++k) std::cout << "   ";
        std::cout << "Optimise : " << *this << std::endl;
#endif
        // First check is the arguments can be option

        bool changed = false;
        args_t result;
        result.reserve(args_.size());
        for(size_t i = 0; i < args_.size(); ++i) {
            ExpPtr o = args_[i]->optimise(true, depth + 1);
            result.push_back(o);
            changed = changed || (o.get() != args_[i].get());
        }

        if(changed) {
#ifdef _DEBUG
            for(size_t k = 0; k < depth; ++k) std::cout << "   ";
            std::cout << "Arguments have changed" << std::endl;
#endif
            ExpPtr o = cloneWith(result);
            return o->optimise(true, depth + 1);
        }

        ExpPtr p = self();
#ifdef _DEBUG
        for(size_t k = 0; k < depth; ++k) std::cout << "   ";
        std::cout << "Optimise : " << *this << std::endl;
#endif
        for(;;)
        {
            ExpPtr o = p->optimise(depth + 1);
#ifdef _DEBUG
            for(size_t k = 0; k < depth; ++k) std::cout << "   ";
#endif
            if( o == p ) {
#ifdef _DEBUG
                for(size_t k = 0; k < depth; ++k) std::cout << "   ";
                std::cout << "-" << std::endl;
#endif
                return o;
            }
            else {
#ifdef _DEBUG
                for(size_t k = 0; k < depth; ++k) std::cout << "   ";
                std::cout << " ==> " << *o << std::endl;
#endif
                p = o;
            }
        }
    }
    return self();
}

bool Expression::optimiseArgs(args_t & result) const
{
    result.reserve(args_.size());
    bool changed = false;
    for(size_t i = 0; i < args_.size(); ++i) {
        ExpPtr o = args_[i]->optimise(true);
        result.push_back(o);
        changed = changed || (o.get() != args_[i].get());
    }
    return changed;
}

ExpPtr Expression::eval(ExpPtr e , bool optimize) const
{
    Scope ctx("Expression::eval( ExpPtr e )", e);
    return ExpPtr( optimise(optimize)->evaluate(ctx));
}

ExpPtr Expression::eval(ExpPtr a, ExpPtr b , bool optimize) const
{
    Scope ctx("Expression::eval(  ExpPtr a, ExpPtr b )", a, b);
    return ExpPtr( optimise(optimize)->evaluate(ctx));
}

ExpPtr Expression::args( size_t i) const
{
    ASSERT( i < args_.size() );
    ASSERT( args_[i] );
    return  args_[i];
}

ExpPtr Expression::args(size_t i, Scope& ctx , bool eval) const
{
    ExpPtr e = args(i)->resolve(ctx);
    if(eval) {
        return e->eval(ctx);
    }
    else {
        return e;
    }
}

std::string Expression::str() const
{
    std::ostringstream os;
    print(os);
    return os.str();
}


void Expression::printArgs(std::ostream& out) const
{
    size_t count = arity();
    for(size_t i = 0; i < count; ++i) {
        if(i) out << ", ";
        out << *args_[i];
    }
}

std::ostream& operator<<( std::ostream& os, const Expression& v)
{
    if(format(os) == xpr::CodeFormat)
    {
        v.asCode(os);
    }
    else {
        v.print(os);
    }
    return os;
}

//--------------------------------------------------------------------------------------------

} // namespace xpr
} // namespace eckit