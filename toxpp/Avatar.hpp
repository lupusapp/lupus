#ifndef __LUPUS_TOXPP_AVATAR__
#define __LUPUS_TOXPP_AVATAR__

#include "Toxpp.hpp"
#include "lodepng/lodepng.h"
#include "toxpp/IdentIcon.hpp"
#include <filesystem>

class Toxpp::Avatar final
{
public:
    Avatar(void) = default;
    Avatar &operator=(Avatar const &) = default;
    Avatar(std::vector<std::uint8_t> const &publicKeyBin, std::string const &publicKeyHex)
        : _filename{Toxpp::toxConfigDir + "/avatars/" + publicKeyHex + ".png"}
    {
        if (auto avatarsPath{std::filesystem::path{_filename}.remove_filename()};
            !std::filesystem::exists(avatarsPath)) {
            std::filesystem::create_directories(avatarsPath);
        }

        if (!std::filesystem::exists(_filename)) {
            try {
                Toxpp::IdentIcon{publicKeyBin}.write(_filename);
            } catch (std::exception &e) {
                std::throw_with_nested(std::runtime_error{"Cannot save default IdentIcon."});
            }
        }

        auto error{lodepng::decode(_avatar, _size.first, _size.second, _filename)};
        if (error) {
            throw std::runtime_error{std::string{"Cannot load avatar: "} +
                                     lodepng_error_text(error)};
        }
    }

    std::string filename(void) const { return _filename; }
    std::vector<std::uint8_t> avatar(void) const { return _avatar; }
    std::pair<unsigned, unsigned> size(void) const { return _size; }

private:
    std::string _filename;
    std::vector<std::uint8_t> _avatar;
    std::pair<unsigned, unsigned> _size;
};

#endif
