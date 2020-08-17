#include "include/ToxSelf.hpp"
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <tox/toxencryptsave.h>

using Lupus::ToxSelf;

using u8 = std::uint8_t;
using u8p = u8 *;
using cu8p = u8 const *;

std::string readFile(std::string const &filename)
{
    std::ostringstream oss;
    std::ifstream ifs{filename};
    ifs.exceptions(std::ios_base::badbit);
    oss << ifs.rdbuf();
    return oss.str();
}

ToxSelf::ToxSelf(void) : tox{tox_new(nullptr, nullptr)}
{
    if (!tox) {
        throw std::runtime_error{"Cannot create tox instance."};
    }

    tox_self_set_name(tox, reinterpret_cast<cu8p>(default_name), strlen(default_name), nullptr);
    tox_self_set_status_message(tox, reinterpret_cast<cu8p>(default_status_message),
                                strlen(default_status_message), nullptr);
}

ToxSelf::ToxSelf(std::string const &profileFilename, std::optional<std::string> profilePassword)
    : profileFilename{profileFilename}, profilePassword{profilePassword}
{
    std::string profile;
    try {
        profile = readFile(profileFilename);
    } catch (std::runtime_error &e) {
        throw std::runtime_error{"Cannot read profile file."};
    }

    if (profilePassword) {
        auto plainSize{profile.size() - TOX_PASS_ENCRYPTION_EXTRA_LENGTH};
        auto plain{std::make_unique<u8p>(new u8[plainSize])};

        if (!tox_pass_decrypt(reinterpret_cast<cu8p>(profile.data()), profile.size(),
                              reinterpret_cast<cu8p>(profilePassword->data()),
                              profilePassword->size(), *plain, nullptr)) {
            throw std::runtime_error{"Cannot decrypt profile."};
        }

        profile = std::string{reinterpret_cast<char const *>(*plain), plainSize};
    }

    std::unique_ptr<Tox_Options, decltype(&tox_options_free)> options{tox_options_new(nullptr),
                                                                      &tox_options_free};
    if (!options) {
        throw std::runtime_error{"Cannot create Tox Options."};
    }
    tox_options_set_savedata_type(options.get(), TOX_SAVEDATA_TYPE_TOX_SAVE);
    tox_options_set_savedata_data(options.get(), reinterpret_cast<cu8p>(profile.data()),
                                  profile.size());

    tox = tox_new(options.get(), nullptr);
    if (!tox) {
        throw std::runtime_error{"Cannot create Tox instance."};
    }
}

ToxSelf::~ToxSelf(void) { tox_kill(tox); }

std::string ToxSelf::filename(void) const { return profileFilename; }

void ToxSelf::filename(std::string const &filename) { profileFilename = filename; }

std::optional<std::string> ToxSelf::password(void) const { return profilePassword; }

void ToxSelf::password(std::string const &password)
{
    if (password.empty()) {
        profilePassword = {};
        return;
    }

    profilePassword = password;
}

void ToxSelf::save(void)
{
    auto size{tox_get_savedata_size(tox)};
    auto data{std::make_unique<u8p>(new u8[size])};
    tox_get_savedata(tox, *data);

    if (profilePassword) {
        auto tmpSize{size + TOX_PASS_ENCRYPTION_EXTRA_LENGTH};
        auto tmpData{std::make_unique<u8p>(new u8[tmpSize])};

        if (!tox_pass_encrypt(*data, size, reinterpret_cast<u8p>(profilePassword->data()),
                              profilePassword->size(), *tmpData, nullptr)) {
            throw std::runtime_error{"Cannot encrypt profile."};
        }

        size = tmpSize;
        data.reset(tmpData.release());
    }

    try {
        std::ofstream ofs{profileFilename};
        ofs.exceptions(std::ios_base::badbit);
        ofs.write(reinterpret_cast<char *>(*data), size);
        ofs.close();
    } catch (std::runtime_error &e) {
        throw std::runtime_error{"Cannot write profile file."};
    }
}

bool ToxSelf::isEncrypted(std::string const &profileFilename)
{
    try {
        return tox_is_data_encrypted(reinterpret_cast<cu8p>(readFile(profileFilename).data()));
    } catch (std::ifstream::failure &e) {
        throw std::runtime_error{"Cannot read profile file"};
    }
}
