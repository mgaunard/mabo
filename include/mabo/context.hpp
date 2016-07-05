#ifndef MABO_CONTEXT_HPP_INCLUDED
#define MABO_CONTEXT_HPP_INCLUDED

#include <mabo/config.hpp>
#include <mabo/binary.hpp>

#include <range/v3/view.hpp>

namespace mabo
{

struct context
{
    void load_file(string_view str)
    {
        binaries_.emplace_back(str);
    }

    auto binaries()
    {
        return ranges::view::const_(binaries_);
    }

private:
    vector<binary> binaries_;
};

}

#endif
