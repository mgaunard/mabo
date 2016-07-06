#ifndef MABO_RADARE2_HPP_INCLUDED
#define MABO_RADARE2_HPP_INCLUDED

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextern-c-compat"
#endif
#include <r_bin.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <cassert>

namespace mabo { namespace radare2
{

struct symbol
{
    explicit symbol(RBinSymbol* sym) : sym(sym)
    {
        assert(sym);
    }

    explicit symbol(RBinImport* sym) : sym(sym)
    {
        assert(sym);
    }

    string_view name() const
    {
        return mabo::visit(
            [](auto* sym)
            {
                return sym->name;
            },
            sym
        );
    }

    size_t addr() const
    {
        return mabo::visit(
            overload(
                [](RBinSymbol* sym)
                {
                    return (size_t)sym->vaddr;
                },
                [](RBinImport*)
                {
                    return (size_t)0;
                }
            ),
            sym
        );
    }

    bool global() const
    {
        return mabo::visit(
            [](auto* sym)
            {
                return !strcmp(sym->bind, "GLOBAL");
            },
            sym
        );
    }

private:
    variant<RBinSymbol*, RBinImport*> sym;
};

template<class T>
struct rlist_range : ranges::view_facade<rlist_range<T>>
{
    rlist_range(RList* list = 0) : iter(list ? list->head : 0) {}

private:
    friend ranges::range_access;

    struct cursor
    {
        cursor(RListIter* iter = 0) : iter(iter) {}

    private:
        friend ranges::range_access;

        T* get() const
        {
            return (T*)iter->data;
        }

        bool equal(cursor const& other) const
        {
            return iter == other.iter;
        }

        void next()
        {
            iter = iter->n;
        }

        RListIter* iter;
    };

    cursor begin_cursor() const { return cursor(iter);  }
    cursor end_cursor()   const { return cursor(0); }

    RListIter* iter;
};

struct object;

struct archive
{
    string_view name() const
    {
        return "";
    }

    auto objects()
    {
        return ranges::view::empty<object>();
    }
};

auto object_symbols_filter = [](RBinSymbol* sym) { return !strncmp(sym->name, "imp.", 4); };

struct object
{
    object() : obj(0) {}

    explicit object(RBinObject* obj) : obj(obj)
    {
        assert(obj);
    }

    string_view name() const
    {
        return obj->info->file;
    }

    optional<radare2::archive> archive() const
    {
        return {};
    }

    auto symbols() const
    {
        return  rlist_range<RBinSymbol>(obj->symbols)
            |   ranges::view::remove_if(object_symbols_filter)
            |   ranges::view::transform([](RBinSymbol* sym) { return symbol(sym); })
            |   ranges::view::remove_if([](symbol const& sym) { return !sym.global(); })
        ;
    }

    auto imports() const
    {
        return  rlist_range<RBinImport>(obj->imports)
            |   ranges::view::transform([](RBinImport* sym) { return symbol(sym); })
        ;
    }

    auto libs() const
    {
        return  rlist_range<const char>(obj->libs)
            |   ranges::view::transform([](const char* str) { return string_view(str); })
        ;
    }

private:
    RBinObject* obj;
};

struct binary : variant<archive, object>
{
    typedef variant<archive, object> variant_type;

    binary(string_view filename) : variant_type(load_file(filename))
    {
    }

    string_view name() const
    {
        return bin->file;
    }

    auto objects() const
    {
        return ranges::make_iterator_range(bin, bin+1)
             | ranges::view::transform([](RBin& bin) { return object(r_bin_get_object(&bin)); });
    }

private:
    object load_file(string_view filename)
    {
        bin = r_bin_new();

        ut64 baddr = UT64_MAX;
        ut64 laddr = UT64_MAX;
        int xtr_idx = 0;
        int fd = -1;
        int rawstr = 0;

        if(!r_bin_load(bin, filename.to_string().c_str(), baddr, laddr, xtr_idx, fd, rawstr))
            throw std::runtime_error("failed to load binary " + filename.to_string());

        return object(r_bin_get_object(bin));
    }

    RBin* bin;
};

} }

#endif
