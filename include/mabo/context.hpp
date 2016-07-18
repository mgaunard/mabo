#ifndef MABO_CONTEXT_HPP_INCLUDED
#define MABO_CONTEXT_HPP_INCLUDED

#include <mabo/config.hpp>
#include <mabo/binary.hpp>

#include <range/v3/view.hpp>

#include <fstream>
#include <iostream>
#include <list>
#include <unordered_map>
#include <unordered_set>

namespace mabo
{

namespace detail
{

struct symbol_status
{
    symbol_status(string_view s)
    : name(s.to_string())
    , state(NONE)
    {
    }

    string name;
    mutable optional<mabo::binary> object;
    enum type
    {
        NONE    = 0,
        UNDEF   = 1,
        DEFINED = 2,
        WEAK    = 4
    };
    mutable int state;

    bool operator==(symbol_status const& other) const
    {
        return name == other.name;
    }

    bool operator<(symbol_status const& other) const
    {
        return name < other.name;
    }

    friend size_t hash_value(symbol_status const& self)
    {
        return std::hash<string>()(self.name);
    }
};

}

}

namespace std
{
    template<> struct hash<::mabo::detail::symbol_status> : mabo::detail::hash_value {};
}

namespace mabo
{

struct context
{
    void load_file(string_view str)
    {
        binaries_.emplace_back(str);
        loaded_.insert(str.to_string());
    }

    void load_dynamic()
    {
        for(binary const& bin : binaries_)
        {
            load_dynamic(bin);
        }
    }

    auto binaries() const
    {
        return ranges::view::const_(binaries_);
    }

    auto dependencies(bool whole_archive = false, bool object_granularity = false) const
    {
        using detail::symbol_status;

        std::unordered_set<symbol_status> symbols;
        std::unordered_map<binary, std::unordered_set<binary>> dependencies;
        std::unordered_set<object> not_referenced;

        for(mabo::binary const& bin : binaries())
        {
            for(mabo::object const& obj : bin.objects())
            {
                binary bin_obj = object_granularity ? binary(obj) : bin;

                if(!whole_archive && obj.archive())
                {
                    bool referenced = false;
                    for(mabo::symbol const& sym : obj.symbols())
                    {
                        auto it = symbols.find(sym.name());
                        if(it != symbols.end() && (it->state & symbol_status::UNDEF))
                        {
                            referenced = true;
                            break;
                        }
                    }
                    if(!referenced)
                    {
                        not_referenced.insert(obj);
                        continue;
                    }
                }

                for(mabo::symbol const& sym : obj.symbols())
                {
                    auto it = symbols.insert(sym.name()).first;

                    if(it->state & symbol_status::UNDEF)
                        dependencies[*it->object].emplace(bin_obj);

                    if(it->state == symbol_status::DEFINED)
                        std::cout << "multiple definitions of symbol " << sym.name()
                                  << " defined in " << bin_obj.name()
                                  << ", previous definition in " << it->object->name()
                                  << std::endl;

                    it->state &= ~symbol_status::UNDEF;
                    it->state |= symbol_status::DEFINED;
                    if(sym.weak())
                        it->state |= symbol_status::WEAK;
                    it->object = bin_obj;
                }

                for(mabo::symbol const& sym : obj.imports())
                {
                    auto it = symbols.insert(sym.name()).first;

                    if(it->state & symbol_status::DEFINED)
                    {
                        dependencies[bin_obj].emplace(*it->object);
                    }
                    else
                    {
                        it->state = symbol_status::UNDEF;
                        it->object = bin_obj;
                    }
                    if(sym.weak())
                        it->state |= symbol_status::WEAK;
                }
            }
        }

        for(auto&& sym : symbols)
        {
            if(sym.state == symbol_status::UNDEF)
                std::cout << "undefined symbol " << sym.name << " in object " << sym.object->name() << std::endl;
        }

        for(mabo::object const& obj : not_referenced)
        {
            std::cout << "unused object: " << obj.name() << std::endl;
        }

        return dependencies;
    }

private:
    void load_dynamic(binary const& bin)
    {
        for(object const& obj : bin.objects())
        {
            for(string_view lib : obj.libs())
            {
                bool found = false;
                for(string_view path : obj.link_paths())
                {
                    // TODO: check compatible arch
                    std::string file = path.to_string();
                    file += "/";
                    file.insert(file.end(), lib.begin(), lib.end());
                    std::ifstream ifs(file);
                    if(ifs)
                    {
                        if(loaded_.find(file) == loaded_.end())
                        {
                            load_file(file);
                            load_dynamic(binaries_.back());
                        }
                        found = true;
                        break;
                    }
                }

                if(!found)
                    std::cerr << "dynamic library " << lib
                              << " not found"
                              << std::endl;
            }
        }
    }

    std::list<binary> binaries_;
    std::unordered_set<string> loaded_;
};

}

#endif
