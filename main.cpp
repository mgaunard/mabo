#include <mabo/binary.hpp>
#include <mabo/context.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
    mabo::context ctx;
    for(const char* arg : ranges::make_iterator_range(argv+1, argv+argc))
    {
        ctx.load_file(arg);

        for(mabo::binary const& bin : ctx.binaries())
        {
            for(mabo::object const& obj : bin.objects())
            {
                mabo::optional<mabo::archive> archive = obj.archive();

                if(archive)
                    std::cout << "object " << archive->name() << "(" << obj.name() << ")\n";
                else
                    std::cout << "object " << obj.name() << "\n";

                std::cout << "\nsymbols:\n";
                for(mabo::symbol symbol : obj.symbols())
                {
                    std::cout << symbol.name() << " " << symbol.addr() << "\n";
                }

                std::cout << "\nimports:\n";
                for(mabo::symbol symbol : obj.imports())
                {
                    std::cout << symbol.name() << "\n";
                }

                std::cout << "\nlibs:\n";
                for(mabo::string_view lib : obj.libs())
                {
                    std::cout << lib << "\n";
                }

                #if 0
                std::cout << "\nsections:\n";
                for(mabo::section section : obj.sections())
                {
                    std::cout << section.name() << "\n";
                    std::cout << section.data() << "\n";
                }
                #endif
            }
        }
    }

}