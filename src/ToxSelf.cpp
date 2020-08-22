#include "include/ToxSelf.hpp"
#include "gdkmm/pixbuf.h"
#include <algorithm>
#include <cstring>
#include <ctype.h>
#include <filesystem>
#include <fstream>
#include <glibmm/checksum.h>
#include <memory>
#include <sodium/utils.h>
#include <sstream>
#include <stdexcept>
#include <tox/tox.h>
#include <tox/toxencryptsave.h>

using Lupus::ToxAvatar;
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

std::string ToxSelf::name(void) const
{
    auto size{tox_self_get_name_size(tox)};
    auto name{std::make_unique<u8p>(new u8[size])};
    tox_self_get_name(tox, *name);
    return {reinterpret_cast<char const *>(*name), size};
}

void ToxSelf::name(std::string const &name)
{
    Tox_Err_Set_Info error;
    tox_self_set_name(tox, reinterpret_cast<u8 const *>(name.data()), name.size(), &error);
    switch (error) {
    case TOX_ERR_SET_INFO_TOO_LONG:
        throw std::runtime_error{"Cannot set name, length exceed maximum size."};
    case TOX_ERR_SET_INFO_NULL:
        throw std::runtime_error{"Cannot set name."};
    default:
        break;
    }
}

std::string ToxSelf::statusMessage(void) const
{
    auto size{tox_self_get_status_message_size(tox)};
    auto statusMessage{std::make_unique<u8p>(new u8[size])};
    tox_self_get_status_message(tox, *statusMessage);
    return {reinterpret_cast<char const *>(*statusMessage), size};
}

void ToxSelf::statusMessage(std::string const &statusMessage)
{
    Tox_Err_Set_Info error;
    tox_self_set_status_message(tox, reinterpret_cast<u8 const *>(statusMessage.data()),
                                statusMessage.size(), &error);
    switch (error) {
    case TOX_ERR_SET_INFO_TOO_LONG:
        throw std::runtime_error{"Cannot set status message, length exceed maximum size."};
    case TOX_ERR_SET_INFO_NULL:
        throw std::runtime_error{"Cannot set status message."};
    default:
        break;
    }
}

std::string ToxSelf::publicKey(void) const
{
    auto publicKey{std::make_unique<u8p>(new u8[publicKeySize])};
    tox_self_get_public_key(tox, *publicKey);

    auto hexSize{publicKeySize * 2 + 1};
    auto hex{std::make_unique<char *>(new char[hexSize])};

    sodium_bin2hex(*hex, hexSize, *publicKey, publicKeySize);

    std::string str{const_cast<char const *>(*hex), hexSize};
    std::transform(str.begin(), str.end(), str.begin(), toupper);
    return str;
}

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

ToxAvatar::ToxAvatar(ToxSelf *toxSelf)
    : toxSelf{toxSelf}, _filename{avatarsDir + toxSelf->publicKey() + ".png"}
{
    if (!std::filesystem::exists(_filename)) {
        createToxIdentIcon();
        return;
    }

    if (std::filesystem::file_size(_filename) > maxFileSize) {
        std::filesystem::remove(_filename);
        createToxIdentIcon();
    }

    createPixbuf();
    createHash();
}

std::string const &ToxAvatar::filename(void) const { return _filename; }

Glib::RefPtr<Gdk::Pixbuf> const &ToxAvatar::pixbuf(void) const { return _pixbuf; }

std::string const &ToxAvatar::hash(void) const { return _hash; }

void ToxAvatar::createToxIdentIcon(void)
{
    using CS = Glib::Checksum;
    auto hash{CS::compute_checksum(CS::CHECKSUM_SHA256, toxSelf->publicKey())};
}

void ToxAvatar::createHash(void)
{
    auto const data{_pixbuf->get_pixels()};
    auto dataSize{_pixbuf->get_byte_length()};
    auto hashBin{std::make_unique<u8p>(new u8[tox_hash_length()])};
    tox_hash(*hashBin, data, dataSize);

    auto hashHexSize{tox_hash_length() * 2 + 1};
    auto hashHex{std::make_unique<char *>(new char[hashHexSize])};
    sodium_bin2hex(*hashHex, hashHexSize, *hashBin, tox_hash_length());
    _hash = {*hashHex, hashHexSize};
}

void ToxAvatar::createPixbuf(void)
{
    _pixbuf = Gdk::Pixbuf::create_from_file(_filename, standardSize, standardSize);
}
