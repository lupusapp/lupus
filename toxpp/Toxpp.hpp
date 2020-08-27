#ifndef __LUPUS_TOXPP_TOXPP__
#define __LUPUS_TOXPP_TOXPP__

#include <stdexcept>
#include <string>

std::string getToxConfigDir(void)
{
#ifdef __linux__
#include <cstdlib>

    if (char const *dir{std::getenv("XDG_CONFIG_HOME")}) {
        return std::string{dir} + "/tox";
    }

    if (char const *dir{std::getenv("HOME")}) {
        return std::string{dir} + "/.config/tox";
    }

    throw std::runtime_error{"Cannot get tox config folder."};
#else
#error "Need to be implemented for platform other than linux."
#endif
}

namespace Toxpp
{
class Self;
class Avatar;
class IdentIcon;
class Bootstrap;

inline static auto const toxConfigDir{getToxConfigDir()};
} // namespace Toxpp

#endif
