#ifndef MABO_BFD_HPP_INCLUDED
#define MABO_BFD_HPP_INCLUDED

#include <mabo/config.hpp>
#include <mabo/utility.hpp>
#include <bfd.h>
#include <elf.h>
#include <unordered_map>
#include <range/v3/view.hpp>
#include <range/v3/algorithm.hpp>

template<class T>
struct print
{
    unsigned : 80;
    typedef T type;
};

namespace mabo
{

struct bfd_initer
{
    bfd_initer()
    {
        bfd_init();
    }
};

struct bfd_initer_once
{
    bfd_initer_once()
    {
        static bfd_initer init;
        (void)init;
    }
};

struct bfd_deleter
{
    void operator()(bfd* abfd) const
    {
        if(abfd)
            bfd_close(abfd);
    }
};

struct empty_deleter
{
    template<class T>
    void operator()(T* ptr) const
    {
    }
};

struct object;

struct symbol
{
    symbol(asymbol* sym) : sym(sym) {}

    mabo::object object() const;

    string_view name() const
    {
        return sym->name;
    }

private:
    asymbol* sym;
};

struct section
{
    section(asection* sec) : sec(sec) {}

    string_view name() const
    {
        return sec->name;
    }

    template<class T>
    auto data()
    {
        #define BUFFER_SIZE size_t(32)
        struct section_data_range : ranges::view_facade<section_data_range>
        {
            section_data_range(asection* sec) : sec(sec)
            {
                sz = bfd_get_section_size(sec) / sizeof(T);
            }

        private:
            friend ranges::range_access;

            struct cursor
            {
                cursor() = default;
                cursor(asection* sec, size_t sz, size_t idx) : sec(sec), sz(sz), idx(idx)
                {
                    advance(0, true);
                }

            private:
                friend ranges::range_access;

                T const& get() const
                {
                    return buffer[idx % BUFFER_SIZE];
                }

                void next()
                {
                    advance(1);
                }

                void prev()
                {
                    advance(-1);
                }

                bool equal(cursor const& other) const
                {
                    return idx == other.idx;
                }

                void advance(ptrdiff_t n, bool init = false)
                {
                    size_t old_pos = idx / BUFFER_SIZE * BUFFER_SIZE;
                    idx += n;
                    size_t pos = idx / BUFFER_SIZE * BUFFER_SIZE;
                    size_t bufsz = std::min(BUFFER_SIZE, sz-pos);

                    if(!init && pos == old_pos)
                        return;

                    if(!bfd_get_section_contents(sec->owner, sec, buffer, pos*sizeof(T), bufsz*sizeof(T)))
                        throw std::runtime_error("bfd_get_section_contents failed");
                }

                ptrdiff_t distance_to(cursor const& other) const
                {
                    return other.idx - idx;
                }

                asection* sec;
                size_t sz;
                size_t idx;
                T buffer[BUFFER_SIZE];
            };

            cursor   begin_cursor() const { return cursor(sec, sz, 0);  }
            cursor   end_cursor()   const { return cursor(sec, sz, sz); }

            asection* sec;
            size_t sz;
        };
        #undef BUFFER_SIZE

        return section_data_range(sec);
    }

private:
    asection* sec;
};

struct archive;

struct object
{
    object(bfd* abfd) : abfd(abfd)
    {
        // partitioning criteria
        auto is_import = [](asymbol* sym)
        {
            return bfd_is_und_section(sym->section);
        };
        auto is_global = [](asymbol* sym)
        {
            return !(sym->flags & BSF_LOCAL);
        };

        // load normal symbols
        long storage_needed = bfd_get_symtab_upper_bound(abfd);

        if(storage_needed < 0)
            throw std::runtime_error("bfd_get_symtab_upper_bound failed");

        symbols_.resize(storage_needed / sizeof(asymbol*));

        long number_of_symbols = bfd_canonicalize_symtab(abfd, &symbols_[0]);
        if(number_of_symbols < 0)
            throw std::runtime_error("bfd_canonicalize_symtab failed");

        symbols_.resize(number_of_symbols);

        symbols_part1 = ranges::partition(symbols_, is_import) - symbols_.begin();
        symbols_part2 = ranges::partition(ranges::make_iterator_range(symbols_.begin() + symbols_part1, symbols_.end()), is_global).get_unsafe() - symbols_.begin();

        // load dynamic symbols
        storage_needed = bfd_get_dynamic_symtab_upper_bound(abfd);

        // this fails for non-shared objects
        if(storage_needed < 0)
        {
            dyn_symbols_part1 = 0;
            dyn_symbols_part2 = 0;
            return;
        }

        dyn_symbols_.resize(storage_needed / sizeof(asymbol*));

        number_of_symbols = bfd_canonicalize_dynamic_symtab(abfd, &dyn_symbols_[0]);
        if(number_of_symbols < 0)
            throw std::runtime_error("bfd_canonicalize_dynamic_symtab failed");

        dyn_symbols_.resize(number_of_symbols);

        dyn_symbols_part1 = ranges::partition(dyn_symbols_, is_import) - dyn_symbols_.begin();
        dyn_symbols_part2 = ranges::partition(ranges::make_iterator_range(dyn_symbols_.begin() + dyn_symbols_part1, dyn_symbols_.end()), is_global).get_unsafe() - dyn_symbols_.begin();

    }

    string_view name()
    {
        return abfd->filename;
    }

    auto sections()
    {
        struct section_range : ranges::view_facade<section_range>
        {
            section_range() = default;
            section_range(asection* sec) : sec(sec)
            {
            }

        private:
            friend ranges::range_access;

            mabo::section get() const
            {
                return mabo::section(sec);
            }

            bool done() const
            {
                return !sec;
            }

            void next()
            {
                sec = sec->next;
            }

            asection* sec;
        };

        return ranges::view::bounded(section_range(abfd->sections));
    }

    mabo::section section(string_view name)
    {
        return mabo::section(bfd_get_section_by_name(abfd, name.to_string().c_str()));
    }

    optional<mabo::archive> archive() const;

    auto symbols()
    {
        return ranges::view::concat(
            ranges::make_iterator_range(symbols_.begin() + symbols_part1, symbols_.begin() + symbols_part2) | ranges::view::transform([](asymbol* sym) { return symbol(sym); }),
            ranges::make_iterator_range(dyn_symbols_.begin() + dyn_symbols_part1, dyn_symbols_.begin() + dyn_symbols_part2) | ranges::view::transform([](asymbol* sym) { return symbol(sym); })
        );
    }

    auto imports()
    {
        return ranges::view::concat(
            ranges::make_iterator_range(symbols_.begin(), symbols_.begin() + symbols_part1) | ranges::view::transform([](asymbol* sym) { return symbol(sym); }),
            ranges::make_iterator_range(dyn_symbols_.begin(), dyn_symbols_.begin() + dyn_symbols_part1) | ranges::view::transform([](asymbol* sym) { return symbol(sym); })
        );
    }

    auto libs()
    {
        vector<string> libs;

        auto traverse = [&](auto t)
        {
            typedef decltype(t) T;
            for(T entry : section(".dynamic").data<T>())
            {
                if(entry.d_tag == DT_NEEDED)
                {
                    auto it = section(".dynstr").data<char>().begin() + entry.d_un.d_val;
                    string s;
                    for(; *it; ++it)
                        s += *it;
                    libs.push_back(s);
                }
            }
        };

        if(true)//abfd->arch_info.bits_per_byte == 64)
            traverse(Elf64_Dyn());
        else
            traverse(Elf32_Dyn());

        return libs;

    }

private:
    bfd* abfd;
    vector<asymbol*> symbols_;
    size_t symbols_part1;
    size_t symbols_part2;
    vector<asymbol*> dyn_symbols_;
    size_t dyn_symbols_part1;
    size_t dyn_symbols_part2;
};

// collection of objects, but only load as needed
struct archive
{
    archive(bfd* abfd) : abfd(abfd) {}

    string_view name()
    {
        return abfd->filename;
    }

    auto objects()
    {
        struct object_range : ranges::view_facade<object_range>
        {
            object_range() = default;
            object_range(bfd* abfd) : abfd(abfd), archived(0)
            {
                next();
            }

        private:
            friend ranges::range_access;

            object get() const
            {
                return mabo::object(archived);
            }

            bool done() const
            {
                return !archived;
            }

            void next()
            {
                do
                {
                    archived = bfd_openr_next_archived_file(abfd, archived);
                }
                while(archived && !bfd_check_format(archived, bfd_object));
            }

            bfd* abfd;
            bfd* archived;
        };

        return ranges::view::bounded(object_range(abfd));
    }

private:
    bfd* abfd;
};

inline mabo::object symbol::object() const
{
    return mabo::object(bfd_asymbol_bfd(sym));
}

inline optional<mabo::archive> object::archive() const
{
    if(abfd->my_archive)
        return mabo::archive(abfd->my_archive);
    else
        return {};
}

using binary = variant<object, archive>;

struct context
{
    context()
    {
        static bfd_initer init;
    }

    void load_file(string_view str)
    {
        bfd* abfd = bfd_openr(str.to_string().c_str(), NULL);
        if(!abfd)
            throw std::runtime_error("failed to load binary " + str.to_string());

        binaries.push_back(load_file(abfd));
    }

    auto objects()
    {
        vector<mabo::object> objects;

        for(mabo::binary& binary : binaries)
        {
            MABO_VARIANT_NS::visit(
                overload(
                    [&](mabo::object& object) { objects.push_back(object); },
                    [&](mabo::archive& archive) { for(mabo::object object : archive.objects()) objects.push_back(std::move(object)); }
                ),
                binary
            );
        }

        return objects;
    }

private:
    binary load_file(bfd* abfd)
    {
        if(bfd_check_format(abfd, bfd_archive))
        {
            return archive(abfd);
        }
        else if(bfd_check_format(abfd, bfd_object))
        {
            return object(abfd);
        }
        else
        {
            throw std::runtime_error("unsupported file type");
        }
    }
public:
    vector<binary> binaries;
};

}

#endif
