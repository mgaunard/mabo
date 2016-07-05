#ifndef CHDIR_HPP_INCLUDED
#define CHDIR_HPP_INCLUDED

#include <boost/predef/os.h>
#include <string>

#if (BOOST_OS_WINDOWS)
#  include <stdlib.h>
#elif (BOOST_OS_SOLARIS)
#  include <stdlib.h>
#  include <limits.h>
#elif (BOOST_OS_LINUX)
#  include <unistd.h>
#  include <limits.h>
#elif (BOOST_OS_MACOS)
#  include <mach-o/dyld.h>
#elif (BOOST_OS_BSD_FREE)
#  include <sys/types.h>
#  include <sys/sysctl.h>
#endif

#include <libgen.h>

/*
 * Returns the full path to the currently running executable,
 * or an empty string in case of failure.
 */
std::string get_executable_path() {
#if (BOOST_OS_WINDOWS)
    char* exe_path;
    if(_get_pgmptr(&exe_path) != 0)
        exe_path = "";
#elif (BOOST_OS_SOLARIS)
    char exe_path[PATH_MAX];
    if(realpath(getexecname(), exe_path) == NULL)
        exe_path[0] = '\0';
#elif (BOOST_OS_LINUX)
    char exe_path[PATH_MAX];
    ssize_t len = ::readlink("/proc/self/exe", exe_path, sizeof(exe_path));
    if(len == -1 || len == sizeof(exe_path))
        len = 0;
    exe_path[len] = '\0';
#elif (BOOST_OS_MACOS)
    char exe_path[PATH_MAX];
    uint32_t len = sizeof(exe_path);
    if(_NSGetExecutablePath(exe_path, &len) != 0)
    {
        exe_path[0] = '\0'; // buffer too small (!)
    }
    else
    {
        // resolve symlinks, ., .. if possible
        char *canonicalPath = realpath(exe_path, NULL);
        if(canonicalPath != NULL)
        {
            strncpy(exe_path,canonicalPath,len);
            free(canonicalPath);
        }
    }
#elif (BOOST_OS_BSD_FREE)
    char exe_path[2048];
    int mib[4];  mib[0] = CTL_KERN;  mib[1] = KERN_PROC;  mib[2] = KERN_PROC_PATHNAME;  mib[3] = -1;
    size_t len = sizeof(exe_path);
    if(sysctl(mib, 4, exe_path, &len, NULL, 0) != 0)
        exe_path[0] = '\0';
#endif
    return std::string(exe_path);
}

struct change_current_dir
{
    change_current_dir()
    {
        std::string s = get_executable_path();
        ::chdir(dirname(&s[0]));
    }
} change_current_dir_sym;

#endif