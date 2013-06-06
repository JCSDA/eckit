/*
 * (C) Copyright 1996-2013 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <streambuf>
#include <cassert>

#include "eckit/log/CallbackChannel.h"

//-----------------------------------------------------------------------------

namespace eckit {

//-----------------------------------------------------------------------------

class CallbackBuffer: public std::streambuf {
public:

    CallbackBuffer( CallbackChannel::callback_t c = 0, void* ctxt = 0, std::size_t size = 1024 ) : std::streambuf(),
      call_(c),
      ctxt_(ctxt),
      buffer_( size + 1 ) // + 1 so we can always write the '\0'
    {
        assert( size );
        char *base = &buffer_.front();
        setp(base, base + buffer_.size() - 1 ); // don't consider the space for '\0'
        
        if(c) register_callback(c,ctxt);
    }
    
    ~CallbackBuffer()
    {
        sync();
    }

    void register_callback(CallbackChannel::callback_t c, void* ctxt)
    {
        call_  = c;
        ctxt_  = ctxt;
    }

private:

    CallbackChannel::callback_t call_;
    void* ctxt_;
    std::vector<char> buffer_;

protected:
    
    bool dumpBuffer()
    {
        if( call_ && pbase() != pptr() ) // forward the output to the callback function
        {
            // insert '\0' at pptr() -- e can do it because we reserved an extra character

            *pptr() = '\0';
            pbump(1);

            // callback -- context can be null

            (*call_)( ctxt_, pbase() );
        }

        // rollback -- even if no forward was made

        std::ptrdiff_t n = pptr() - pbase();
        pbump(-n);
        return true;
    }

private:

    int_type overflow(int_type ch)
    {
        /* AutoLock<Mutex> lock(mutex); */
        if (ch == traits_type::eof() ) { return sync(); }
        dumpBuffer();
        sputc(ch);
        return traits_type::to_int_type(ch);
    }

    int_type sync()
    {
        /* AutoLock<Mutex> lock(mutex); */
        return dumpBuffer() ? 0 : -1;
    }

};

//-----------------------------------------------------------------------------

CallbackChannel::CallbackChannel() : Channel( new CallbackBuffer() ) 
{
}

CallbackChannel::CallbackChannel(callback_t c, void *ctxt)
 : Channel( new CallbackBuffer(c,ctxt) ) 
{
}

CallbackChannel::~CallbackChannel()
{
}

void CallbackChannel::register_callback(callback_t c, void *ctxt)
{
    static_cast<CallbackBuffer*>(rdbuf())->register_callback(c,ctxt);
}

//-----------------------------------------------------------------------------

} // namespace eckit