#ifndef MABO_RADARE2_HPP_INCLUDED
#define MABO_RADARE2_HPP_INCLUDED

#include <r_core.h>

namespace mabo
{

struct context
{
    context()
    {
        r_core_init(&core);
    }

    ~context()
    {
        r_core_fini(&core);
    }

    void load_file(string_view str)
    {
        ut64 baddr = UT64_MAX;
        ut64 laddr = UT64_MAX;
        int xtr_idx = 0;
        int fd = -1;
        int rawstr = 0;

        if(!r_bin_load(core.bin, str.to_string().c_str(), baddr, laddr, xtr_idx, fd, rawstr))
        {
            std::cerr << "failed to load binary" << std::endl;
        }

        RList *symbols = r_bin_get_symbols(core.bin);
        if(!symbols)
        {
            std::cerr << "failed to get symbols" << std::endl;
        }

        RListIter *iter;
        void *symbol_;
        r_list_foreach(symbols, iter, symbol_)
        {
            RBinSymbol *symbol = (RBinSymbol*)symbol_;
            std::cout << "symbol = " << symbol->name << std::endl;
        }
    }

private:
    RCore core;
    vector<binary> binaries;
};

}

#endif
