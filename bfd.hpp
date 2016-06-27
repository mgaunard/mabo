#ifndef MABO_BFD_HPP_INCLUDED
#define MABO_BFD_HPP_INCLUDED

#include <bfd.h>
#include <unordered_map>
#include <range/v3/view.hpp>

namespace mabo
{

struct bfd_initer
{
    bfd_initer()
    {
        bfd_init();
    }
};

struct bfd_handle
{
    bfd_handle(bfd* ptr) : abfd(ptr) {}

    bfd_handle(bfd_handle const&) = delete;
    bfd_handle& operator=(bfd_handle const&) = delete;


    bfd_handle(bfd_handle&& other) : abfd(other.abfd)
    {
        other.abfd = 0;
    }

    bfd_handle& operator=(bfd_handle&& other)
    {
        reset();
        abfd = other.abfd;
        other.abfd = 0;
        return *this;
    }

    void reset()
    {
        if(abfd)
            bfd_close(abfd);
        abfd = 0;
    }

    ~bfd_handle()
    {
        reset();
    }

    typedef bfd* (bfd_handle::*safe_bool)() const;
    operator safe_bool() const
    {
        return abfd ? &bfd_handle::get : 0;
    }

    bfd* get() const
    {
        return abfd;
    }

private:
    bfd* abfd;
};

struct context
{
    context()
    {
        static bfd_initer init;
    }

    void load_file(string_view str)
    {
        bfd_handle abfd = bfd_openr(str.to_string().c_str(), NULL);
        if(!abfd)
        {
            std::cerr << "failed to load binary" << std::endl;
        }

        binaries.push_back(load_file(abfd.get()));
    }

    auto undefined_symbols() const
    {
        return global_symbols_
             | ranges::view::remove_if([](auto& p) { return p.second; })
             | ranges::view::transform([](auto& p) -> std::string const& { return p.first; })
        ;
    }

private:
    object load_object(bfd* abfd)
    {
        bool dummy = true;
        return load_object(abfd, dummy);
    }

    object load_object(bfd* abfd, bool& used)
    {
        object o;
        o.name = abfd->filename;

        long storage_needed = bfd_get_symtab_upper_bound(abfd);

        if(storage_needed < 0)
        {
            std::cerr << "bfd_get_symtab_upper_bound failed" << std::endl;
            return o;
        }

        if(storage_needed == 0)
            return o;

        vector<asymbol*> symbol_table(storage_needed / sizeof(asymbol*));

        long number_of_symbols = bfd_canonicalize_symtab(abfd, &symbol_table[0]);
        if(number_of_symbols < 0)
        {
            std::cerr << "bfd_canonicalize_symtab failed" << std::endl;
            return o;
        }
        symbol_table.resize(number_of_symbols);

        std::unordered_map<string_view, bool> symbols;

        for(asymbol* sym : symbol_table)
        {
            std::cout << "symbol " << sym->name << " (section " << sym->section->name << "), flags " << sym->flags << std::endl;

            if(bfd_is_und_section(sym->section))
            {
                symbols[sym->name] = false;
            }
            else if(!(sym->flags & BSF_LOCAL))
            {
                auto it = global_symbols_.find(sym->name);
                if(it != global_symbols_.end())
                {
                    used = true;
                }
                symbols[sym->name] = true;
            }
        }

        if(used)
        {
            for(auto p : symbols)
            {
                auto it = global_symbols_.find(p.first.to_string());
                if(it != global_symbols_.end())
                {
                    if(p.second)
                    {
                        if(it->second)
                            std::cout << "duplicate symbol " << it->first << std::endl;

                        it->second = true;
                    }
                }
                else
                {
                    global_symbols_[p.first.to_string()] = p.second;
                }
            }
        }

        return o;
    }

    binary load_file(bfd* abfd)
    {
        if(bfd_check_format(abfd, bfd_archive))
        {
            std::cout << "archive" << std::endl;

            archive ar;
            ar.name = abfd->filename;

            bfd_handle prev = 0;
            for(;;)
            {
                bfd_handle archived = bfd_openr_next_archived_file(abfd, prev.get());
                if(!archived)
                    break;

                if(bfd_check_format(archived.get(), bfd_object))
                {
                    bool used = false;
                    object o = load_object(archived.get(), used);
                    ar.objects.emplace_back(std::move(o), used);
                }

                prev = std::move(archived);
            }

            return ar;
        }
        else if(bfd_check_format(abfd, bfd_object))
        {
            return load_object(abfd);
        }
        else
        {
            throw std::runtime_error("unsupported file type");
        }
    }

    vector<binary> binaries;
    std::unordered_map<string, bool> global_symbols_;
};

}

#endif
