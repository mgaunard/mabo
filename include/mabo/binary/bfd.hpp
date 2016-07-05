#ifndef MABO_BINARY_BFD_HPP_INCLUDED
#define MABO_BINARY_BFD_HPP_INCLUDED

#include <mabo/config.hpp>
#include <mabo/utility.hpp>

#include <bfd.h>
#include <elf.h>

#include <range/v3/view.hpp>
#include <range/v3/algorithm.hpp>

#include <type_traits>
#include <cassert>
#include <stdexcept>

namespace mabo { namespace bfd
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

// intrusive shared_ptr with member-backed aliasing
template<class T, ::bfd* (T::*member) = (::bfd* (T::*))0>
struct bfd_handle
{
    bfd_handle() : value(0) {}

    explicit bfd_handle(T* ptr) : value(ptr)
    {
        ::bfd* p = bfd();
        if(p)
            p->usrdata = (void*)((uintptr_t)p->usrdata + 1);
    }

    bfd_handle(bfd_handle&& other) : value(other.value)
    {
        other.value = 0;
    }

    bfd_handle& operator=(bfd_handle&& other)
    {
        value = other.value;
        other.value = 0;
        return *this;
    }

    bfd_handle(bfd_handle const& other) : value(other.value)
    {
        ::bfd* p = bfd();
        if(p)
            p->usrdata = (void*)((uintptr_t)p->usrdata + 1);
    }

    bfd_handle& operator=(bfd_handle const& other)
    {
        reset(other.value);
        return *this;
    }

    T* get() const
    {
        return value;
    }

    T* operator->() const
    {
        return value;
    }

    long use_count() const
    {
        ::bfd* p = bfd();
        return p ? (uintptr_t)p->usrdata : 0;
    }

    void reset(T* ptr = 0)
    {
        ::bfd* old_p = bfd();
        value = ptr;
        ::bfd* p = bfd();
        if(p)
            p->usrdata = (void*)((uintptr_t)p->usrdata + 1);
        if(old_p)
            if(!(old_p->usrdata = (void*)((uintptr_t)old_p->usrdata - 1)))
                bfd_close(old_p);
    }

    ~bfd_handle()
    {
        ::bfd* p = bfd();
        if(p)
            if(!(p->usrdata = (void*)((uintptr_t)p->usrdata - 1)))
                bfd_close(p);
    }

    typedef T* (bfd_handle::*safe_bool_type)() const;
    operator safe_bool_type() const
    {
        return value ? &bfd_handle::get : 0;
    }

    ::bfd* bfd()
    {
        if(value)
                if(!member)
                    return (::bfd*)value;
                else
                    return value->*member;
        else
            return 0;
    }

private:
    T* value;
};

struct object;

struct symbol
{
    explicit symbol(::asymbol* sym) : sym(sym)
    {
        assert(sym);
    }

    bfd::object object() const;

    string_view name() const
    {
        return sym->name;
    }

private:
    bfd_handle<::asymbol, &asymbol::the_bfd> sym;
};

struct section
{
    explicit section(::asection* sec) : sec(sec)
    {
        assert(sec);
    }

    string_view name() const
    {
        return sec->name;
    }

    template<class T>
    auto data() const
    {
        static_assert(std::is_trivially_copyable<T>::value, "sections can only be reinterpreted as trivial types");

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

        return section_data_range(sec.get());
    }

private:
    bfd_handle<::asection, &asection::owner> sec;
};

struct archive;

struct object
{
    explicit object(::bfd* abfd) : abfd(abfd)
    {
        assert(abfd);

        // partitioning criteria
        auto is_import = [](asymbol* sym)
        {
            return bfd_is_und_section(sym->section);
        };
        auto is_global = [](asymbol* sym)
        {
            return sym->flags & BSF_GLOBAL;
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

    string_view name() const
    {
        return abfd->filename;
    }

    auto sections() const
    {
        struct section_range : ranges::view_facade<section_range>
        {
            section_range() = default;
            section_range(asection* sec) : sec(sec)
            {
            }

        private:
            friend ranges::range_access;

            bfd::section get() const
            {
                return bfd::section(sec);
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

    optional<bfd::section> section(string_view name) const
    {
        asection* sec = bfd_get_section_by_name(abfd.get(), name.to_string().c_str());
        if(sec)
            return bfd::section(sec);
        else
            return {};
    }

    optional<bfd::archive> archive() const;

    auto symbols() const
    {
        return ranges::view::concat(
            ranges::make_iterator_range(symbols_.begin() + symbols_part1, symbols_.begin() + symbols_part2) | ranges::view::transform([](asymbol* sym) { return symbol(sym); }),
            ranges::make_iterator_range(dyn_symbols_.begin() + dyn_symbols_part1, dyn_symbols_.begin() + dyn_symbols_part2) | ranges::view::transform([](asymbol* sym) { return symbol(sym); })
        );
    }

    auto imports() const
    {
        return ranges::view::concat(
            ranges::make_iterator_range(symbols_.begin(), symbols_.begin() + symbols_part1) | ranges::view::transform([](asymbol* sym) { return symbol(sym); }),
            ranges::make_iterator_range(dyn_symbols_.begin(), dyn_symbols_.begin() + dyn_symbols_part1) | ranges::view::transform([](asymbol* sym) { return symbol(sym); })
        );
    }

    auto libs() const
    {
        // only ELF and Mach-O support this
        // we only care about ELF for now
        vector<string> libs;

        auto traverse = [&](auto t)
        {
            if(!section(".dynamic") || !section(".dynstr"))
                return;

            typedef decltype(t) T;
            for(T entry : section(".dynamic")->data<T>())
            {
                if(entry.d_tag == DT_NEEDED)
                {
                    auto it = section(".dynstr")->data<char>().begin() + entry.d_un.d_val;
                    string s;
                    for(; *it; ++it)
                        s += *it;
                    libs.push_back(s);
                }
            }
        };

        if(abfd->arch_info->bits_per_word == 64)
            traverse(Elf64_Dyn());
        else
            traverse(Elf32_Dyn());

        return libs;

    }

private:
    bfd_handle<::bfd> abfd;
    vector<::asymbol*> symbols_;
    size_t symbols_part1;
    size_t symbols_part2;
    vector<::asymbol*> dyn_symbols_;
    size_t dyn_symbols_part1;
    size_t dyn_symbols_part2;
};

// collection of objects, but only load as needed
struct archive
{
    explicit archive(::bfd* abfd) : abfd(abfd)
    {
        assert(abfd);
    }

    string_view name() const
    {
        return abfd->filename;
    }

    auto objects() const
    {
        struct object_range : ranges::view_facade<object_range>
        {
            object_range() = default;
            object_range(::bfd* abfd) : abfd(abfd), archived(0)
            {
                next();
            }

        private:
            friend ranges::range_access;

            object get() const
            {
                return bfd::object(archived.get());
            }

            bool done() const
            {
                return !archived;
            }

            void next()
            {
                do
                {
                    archived.reset(bfd_openr_next_archived_file(abfd, archived.get()));
                }
                while(archived && !bfd_check_format(archived.get(), bfd_object));
            }

            ::bfd* abfd;
            bfd_handle<::bfd> archived;
        };

        return ranges::view::bounded(object_range(abfd.get()));
    }

private:
    bfd_handle<::bfd> abfd;
};

inline bfd::object symbol::object() const
{
    return bfd::object(bfd_asymbol_bfd(sym));
}

inline optional<bfd::archive> object::archive() const
{
    if(abfd->my_archive)
        return bfd::archive(abfd->my_archive);
    else
        return {};
}

struct binary : variant<object, archive>
{
    typedef variant<object, archive> variant_type;

    binary(string_view file) : variant_type(load_file(file))
    {
    }

    string_view name() const
    {
        return mabo::visit(
            [&](auto&& o) { return o.name(); },
            *this
        );
    }

    auto objects() const
    {
        struct object_range : ranges::view_facade<object_range>
        {
            object_range() = default;
            object_range(variant<object, archive> const* bin) : bin(bin)
            {
                if(mabo::holds_alternative<archive>(*bin))
                {
                    rng = mabo::get<archive>(*bin).objects();
                    first = rng.begin();
                    if(first == rng.end())
                        bin = 0;
                }
            }

        private:
            friend ranges::range_access;

            bfd::object get() const
            {
                if(mabo::holds_alternative<object>(*bin))
                {
                    return mabo::get<object>(*bin);
                }
                else
                {
                    return *first;
                }
            }

            bool done() const
            {
                return !bin;
            }

            void next()
            {
                if(holds_alternative<object>(*bin))
                {
                    bin = 0;
                }
                else
                {
                    ++first;
                    if(first == rng.end())
                    {
                        bin = 0;
                    }
                }
            }

            variant<object, archive> const* bin;
            decltype(std::declval<archive>().objects()) rng;
            decltype(rng.begin()) first;
        };

        return ranges::view::bounded(object_range(this));
    }

private:
    variant_type load_file(string_view str)
    {
        bfd_initer_once init;
        (void)init;

        ::bfd* abfd = bfd_openr(str.to_string().c_str(), NULL);
        if(!abfd)
            throw std::runtime_error("failed to load binary " + str.to_string());

        if(bfd_check_format(abfd, bfd_archive))
        {
            return archive(abfd);
        }
        else if(bfd_check_format(abfd, bfd_object))
        {
            return object(abfd);
        }

        throw std::runtime_error("unsupported file type");
    }
};

} }

#endif
