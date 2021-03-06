#ifndef MABO_BINARY_BINARY_HPP_INCLUDED
#define MABO_BINARY_BINARY_HPP_INCLUDED

#include <mabo/config.hpp>

#include <mabo/binary/bfd.hpp>

#ifdef MABO_WITH_RADARE2
#include <mabo/binary/radare2.hpp>
#endif

namespace mabo
{

#ifdef DOXYGEN
    struct binary;
    struct archive;
    struct object;
    struct section;
    struct symbol;

    struct binary : variant<archive, object>
    {
        binary(string_view file);
        string_view name() const;

        auto objects() const;
    };

    struct archive
    {
        archive() = delete;
        string_view name() const;

        auto objects() const;
    };

    struct object
    {
        object() = delete;
        string_view name() const;
        optional<mabo::archive> archive() const;

        auto sections() const;
        optional<mabo::section> section(string_view name) const;

        auto symbols() const;
        auto imports() const;
        auto libs() const;
        auto link_paths() const;
    };

    struct section
    {
        section() = delete;
        string_view name() const;

        template<class T>
        auto data() const;
    };

    struct symbol
    {
        symbol() = delete;
        string_view name() const;
        size_t addr() const;
        bool global() const;
        bool weak() const;

        // remove?
        mabo::object object() const;
    };

#else
    using namespace bfd;
#endif
}

#endif
