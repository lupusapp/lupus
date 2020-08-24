#ifndef __LUPUS_TOXPP_AVATAR__
#define __LUPUS_TOXPP_AVATAR__

#include "Self.hpp"
#include <memory>

namespace Toxpp
{
class Avatar;
}

class Toxpp::Avatar final
{
public:
    Avatar(std::shared_ptr<Toxpp::Self> &toxppSelf) : toxppSelf{toxppSelf} {}

private:
    std::shared_ptr<Toxpp::Self> toxppSelf;
};

#endif
