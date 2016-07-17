#include <mabo/binary.hpp>

#include "test.hpp"
#include "chdir.hpp"

using namespace testing;

TEST(binary, Test1Object)
{
    mabo::binary bin("test1.cpp.o");
    EXPECT_THAT(bin.name(), Eq("test1.cpp.o"));
    EXPECT_THAT(mabo::holds_alternative<mabo::object>(bin), Eq(true));

    mabo::object& obj = mabo::get<mabo::object>(bin);

    EXPECT_THAT(
        bin.objects() | ranges::view::transform(&mabo::object::name),
        ElementsAre(obj.name())
    );

    EXPECT_THAT(
        obj.symbols() | ranges::view::transform(&mabo::symbol::name),
        ElementsAre("g1")
    );

    EXPECT_THAT(
        obj.imports() | ranges::view::transform(&mabo::symbol::name),
        ElementsAre("f1")
    );

    EXPECT_THAT(obj.libs(), ElementsAre());
}

TEST(binary, Test2Object)
{
    mabo::binary bin("test2.cpp.o");
    EXPECT_THAT(bin.name(), Eq("test2.cpp.o"));
    EXPECT_THAT(mabo::holds_alternative<mabo::object>(bin), Eq(true));

    mabo::object& obj = mabo::get<mabo::object>(bin);

    EXPECT_THAT(
        bin.objects() | ranges::view::transform(&mabo::object::name),
        ElementsAre(obj.name())
    );

    EXPECT_THAT(
        obj.symbols() | ranges::view::transform(&mabo::symbol::name),
        ElementsAre("g2")
    );

    EXPECT_THAT(
        obj.imports() | ranges::view::transform(&mabo::symbol::name),
        ElementsAre("f2")
    );

    EXPECT_THAT(obj.libs(), ElementsAre());
}

TEST(binary, LibTestsArchive)
{
    mabo::binary bin("libtests.a");
    EXPECT_THAT(bin.name(), Eq("libtests.a"));
    EXPECT_THAT(mabo::holds_alternative<mabo::archive>(bin), Eq(true));

    mabo::archive& archive = mabo::get<mabo::archive>(bin);

    EXPECT_THAT(
        bin.objects() | ranges::view::transform(&mabo::object::name),
        ContainerEq(archive.objects() | ranges::view::transform(&mabo::object::name))
    );

    EXPECT_THAT(
        archive.objects() | ranges::view::transform(&mabo::object::name),
        ElementsAre("test1.cpp.o", "test2.cpp.o")
    );

    mabo::object test1 = *archive.objects().begin();
    mabo::object test2 = *ranges::next(archive.objects().begin(), 1);

    // test1
    EXPECT_THAT(
        test1.symbols() | ranges::view::transform(&mabo::symbol::name),
        ElementsAre("g1")
    );

    EXPECT_THAT(
        test1.imports() | ranges::view::transform(&mabo::symbol::name),
        ElementsAre("f1")
    );

    EXPECT_THAT(test1.libs(), ElementsAre());

    // test2
    EXPECT_THAT(
        test2.symbols() | ranges::view::transform(&mabo::symbol::name),
        ElementsAre("g2")
    );

    EXPECT_THAT(
        test2.imports() | ranges::view::transform(&mabo::symbol::name),
        ElementsAre("f2")
    );

    EXPECT_THAT(test2.libs(), ElementsAre());
}

TEST(binary, LibTestsShared)
{
    mabo::binary bin("libtests_shared.so");
    EXPECT_THAT(bin.name(), Eq("libtests_shared.so"));
    EXPECT_THAT(mabo::holds_alternative<mabo::object>(bin), Eq(true));

    mabo::object& obj = mabo::get<mabo::object>(bin);

    EXPECT_THAT(
        bin.objects() | ranges::view::transform(&mabo::object::name),
        ElementsAre(obj.name())
    );

    EXPECT_THAT(
        obj.symbols() | ranges::view::transform(&mabo::symbol::name),
        AllOf(Contains("g1"), Contains("g2"))
    );

    EXPECT_THAT(
        obj.imports() | ranges::view::transform(&mabo::symbol::name),
        AllOf(Contains("f1"), Contains("f2"))
    );

    EXPECT_THAT(obj.libs(), Not(Contains("libtest1_shared.so")));
}

TEST(binary, TestExecutable)
{
    mabo::binary bin("test_exe");
    EXPECT_THAT(bin.name(), Eq("test_exe"));
    EXPECT_THAT(mabo::holds_alternative<mabo::object>(bin), Eq(true));

    mabo::object& obj = mabo::get<mabo::object>(bin);

    EXPECT_THAT(
        bin.objects() | ranges::view::transform(&mabo::object::name),
        ElementsAre(obj.name())
    );

    EXPECT_THAT(
        obj.symbols() | ranges::view::transform(&mabo::symbol::name),
        AllOf(Contains("main"), Contains("f1"), Contains("g1"))
    );

    EXPECT_THAT(
        obj.imports() | ranges::view::transform(&mabo::symbol::name),
        AllOf(Contains("__libc_start_main"), Not(Contains("g1")))
    );

    EXPECT_THAT(
        obj.libs(),
        AllOf(Contains("libc.so.6"), Not(Contains("libtest1_shared.so")))
    );

    std::string current_path = get_executable_path();
    std::string current_dir = ::dirname(&current_path[0]);

    EXPECT_THAT(
        obj.link_paths(),
        AllOf(Not(Contains(current_dir)), Not(Contains(current_dir + "/2")))
    );
}

TEST(binary, TestExecutableShared)
{
    mabo::binary bin("test_exe_shared");
    EXPECT_THAT(bin.name(), Eq("test_exe_shared"));
    EXPECT_THAT(mabo::holds_alternative<mabo::object>(bin), Eq(true));

    mabo::object& obj = mabo::get<mabo::object>(bin);

    EXPECT_THAT(
        bin.objects() | ranges::view::transform(&mabo::object::name),
        ElementsAre(obj.name())
    );

    EXPECT_THAT(
        obj.symbols() | ranges::view::transform(&mabo::symbol::name),
        AllOf(Contains("main"), Contains("f1"))
    );

    EXPECT_THAT(
        obj.imports() | ranges::view::transform(&mabo::symbol::name),
        AllOf(Contains("__libc_start_main"), Contains("g1"))
    );

    EXPECT_THAT(
        obj.libs(),
        AllOf(Contains("libtest1_shared.so"), Contains("libc.so.6"))
    );

    std::string current_path = get_executable_path();
    std::string current_dir = ::dirname(&current_path[0]);

    EXPECT_THAT(
        obj.link_paths(),
        AllOf(Contains(current_dir), Not(Contains(current_dir + "/2")))
    );
}

TEST(binary, TestExecutableShared2)
{
    mabo::binary bin("test_exe_shared2");
    EXPECT_THAT(bin.name(), Eq("test_exe_shared2"));
    EXPECT_THAT(mabo::holds_alternative<mabo::object>(bin), Eq(true));

    mabo::object& obj = mabo::get<mabo::object>(bin);

    EXPECT_THAT(
        bin.objects() | ranges::view::transform(&mabo::object::name),
        ElementsAre(obj.name())
    );

    EXPECT_THAT(
        obj.symbols() | ranges::view::transform(&mabo::symbol::name),
        AllOf(Contains("main"), Contains("f1"), Contains("f2"))
    );

    EXPECT_THAT(
        obj.imports() | ranges::view::transform(&mabo::symbol::name),
        AllOf(Contains("__libc_start_main"), Contains("g1"), Contains("g2"))
    );

    EXPECT_THAT(
        obj.libs(),
        AllOf(Contains("libtest1_shared.so"), Contains("libtest2_shared.so"), Contains("libc.so.6"))
    );

    std::string current_path = get_executable_path();
    std::string current_dir = ::dirname(&current_path[0]);

    EXPECT_THAT(
        obj.link_paths(),
        AllOf(Contains(current_dir), Contains(current_dir + "/2"))
    );
}
