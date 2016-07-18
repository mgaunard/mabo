#ifndef MABO_LINKLINE_HPP_INCLUDED
#define MABO_LINKLINE_HPP_INCLUDED

#include <mabo/config.hpp>
#include <mabo/binary.hpp>

namespace mabo
{
    namespace detail
    {
        template<class Dependencies, class Visited>
        void linkline(string& buffer, Dependencies&& dependencies, Visited&& visited, binary const& dependee)
        {
            if(!visited.insert(dependee.name().to_string()).second)
                return;

            buffer += dependee.name().to_string() + " ";
            for(binary const& dependent : dependencies[dependee])
            {
                linkline(buffer, dependencies, visited, dependent);
            }
        }
    }

    template<class Dependencies>
    string linkline(Dependencies&& dependencies)
    {
        string buffer;
        std::unordered_set<string> visited;

        for(auto&& dependency : dependencies)
        {
            detail::linkline(buffer, dependencies, visited, dependency.first);
        }

        return buffer;
    }
}

#endif
