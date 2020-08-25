#ifndef __LUPUS_TOXPP_SELF__
#define __LUPUS_TOXPP_SELF__

#include "Avatar.hpp"
#include "Toxpp.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sodium/utils.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tox/tox.h>
#include <tox/toxencryptsave.h>
#include <vector>

#define DEFAULT_NAME           "Lupus' user"
#define DEFAULT_STATUS_MESSAGE "Lupus rocks!"

std::vector<std::uint8_t> readFile(std::string const &filename)
{
    std::ifstream ifs{filename, std::ios_base::binary};
    ifs.exceptions(std::ios_base::badbit);

    try {
        return {std::istreambuf_iterator<char>(ifs), {}};
    } catch (std::ifstream::failure &e) {
        throw std::runtime_error{std::string{"Cannot read file: "} + e.what()};
    }
}

void writeFile(std::string const &filename, std::vector<std::uint8_t> const &data)
{
    std::ofstream ofs{filename, std::ios_base::binary};
    ofs.exceptions(std::ios_base::badbit);

    try {
        std::copy(data.begin(), data.end(), std::ostreambuf_iterator<char>{ofs});
        ofs.close();
    } catch (std::exception &e) {
        throw std::runtime_error{std::string{"Cannot write file: "} + e.what()};
    }
}

class Toxpp::Self final
{
public:
    Self(void)
    {
        Tox_Err_New error;
        tox = tox_new(nullptr, &error);
        errNewSwitch(error);

        name(DEFAULT_NAME);
        statusMessage(DEFAULT_STATUS_MESSAGE);

        _avatar = {publicKeyBin(), publicKeyHex()};
    }

    Self(std::string const &filename, std::optional<std::string> const &password)
        : _filename{filename}, _password{password}
    {
        std::vector<std::uint8_t> profile{password ? decrypt(readFile(filename))
                                                   : readFile(filename)};
        Tox_Err_Options_New optionsError;
        std::unique_ptr<Tox_Options, decltype(&tox_options_free)> options{
            tox_options_new(&optionsError), &tox_options_free};
        errOptionsNewSwitch(optionsError);

        tox_options_set_savedata_type(options.get(), TOX_SAVEDATA_TYPE_TOX_SAVE);
        tox_options_set_savedata_data(options.get(), profile.data(), profile.size());

        Tox_Err_New error;
        tox = tox_new(options.get(), &error);
        errNewSwitch(error);

        _avatar = {publicKeyBin(), publicKeyHex()};
    }

    ~Self(void) { tox_kill(tox); }

    void filename(std::string const &filename) { _filename = filename; }
    auto filename(void) const { return _filename; }

    void password(std::optional<std::string> const &password)
    {
        if (password->empty()) {
            _password.reset();
        } else {
            _password = password;
        }
    }
    auto password(void) const { return _password; }

    Toxpp::Avatar const &avatar(void) const { return _avatar; }

    inline static auto const nameMaxSize{tox_max_name_length()};
    void name(std::string const &name)
    {
        TOX_ERR_SET_INFO error;
        tox_self_set_name(tox, reinterpret_cast<std::uint8_t const *>(name.data()), name.size(),
                          &error);
        errSetInfoSwitch(error, "Cannot set name:");
    }
    auto name(void) const
    {
        auto nameSize{tox_self_get_name_size(tox)};
        auto name{std::make_unique<std::uint8_t *>(new std::uint8_t[nameSize])};
        tox_self_get_name(tox, *name);
        return std::string{reinterpret_cast<char *>(*name), nameSize};
    }

    inline static auto const statusMessageMaxSize{tox_max_status_message_length()};
    void statusMessage(std::string const &statusMessage)
    {
        TOX_ERR_SET_INFO error;
        auto statusMessageUint8{reinterpret_cast<std::uint8_t const *>(statusMessage.data())};
        tox_self_set_status_message(tox, statusMessageUint8, statusMessage.size(), &error);
        errSetInfoSwitch(error, "Cannot set status message:");
    }
    std::string statusMessage(void) const
    {
        auto statusMessageSize{tox_self_get_status_message_size(tox)};
        auto statusMessage{std::make_unique<std::uint8_t *>(new std::uint8_t[statusMessageSize])};
        tox_self_get_status_message(tox, *statusMessage);
        return std::string{reinterpret_cast<char *>(*statusMessage), statusMessageSize};
    }

    // in UPPERCASE
    std::string publicKeyHex(void) const
    {
        auto bin{publicKeyBin()};
        auto hexSize{bin.size() * 2 + 1};
        auto hex{std::make_unique<char *>(new char[hexSize])};
        sodium_bin2hex(*hex, hexSize, bin.data(), bin.size());

        std::string hexString{*hex, hexSize - 1};
        std::transform(hexString.begin(), hexString.end(), hexString.begin(), toupper);
        return hexString;
    }

    std::vector<std::uint8_t> publicKeyBin(void) const
    {
        std::vector<std::uint8_t> bin(tox_public_key_size());
        tox_self_get_public_key(tox, bin.data());
        return bin;
    }

    static bool isEncrypted(std::string const &filename)
    {
        return tox_is_data_encrypted(readFile(filename).data());
    }

    void save(void)
    {
        std::vector<std::uint8_t> data(tox_get_savedata_size(tox));
        tox_get_savedata(tox, data.data());

        writeFile(_filename, _password ? encrypt(data) : data);
    }

private:
    void errSetInfoSwitch(Tox_Err_Set_Info &error, std::string const &prefix = "")
    {
        switch (error) {
        case TOX_ERR_SET_INFO_OK:
            break;
        case TOX_ERR_SET_INFO_NULL:
            throw std::runtime_error{prefix + " UNKNOWN ERROR."};
        case TOX_ERR_SET_INFO_TOO_LONG:
            throw std::runtime_error{prefix + " input is too long."};
        }
    }

    void errOptionsNewSwitch(Tox_Err_Options_New &error,
                             std::string const &prefix = "Cannot create Tox Options instance:")
    {
        switch (error) {
        case TOX_ERR_OPTIONS_NEW_OK:
            break;
        case TOX_ERR_OPTIONS_NEW_MALLOC:
            throw std::runtime_error{prefix + " unable to allocate enough memory."};
        }
    }

    void errNewSwitch(Tox_Err_New &error, std::string const &prefix = "Cannot create Tox instance:")
    {
        switch (error) {
        case TOX_ERR_NEW_OK:
            break;
        case TOX_ERR_NEW_NULL:
            throw std::runtime_error{prefix + " UNKNOWN ERROR."};
        case TOX_ERR_NEW_MALLOC:
            throw std::runtime_error{prefix + " unable to allocate enough memory."};
        case TOX_ERR_NEW_PORT_ALLOC:
            throw std::runtime_error{prefix + " unable to bind to a port."};
        case TOX_ERR_NEW_PROXY_BAD_TYPE:
            throw std::runtime_error{prefix + " invalid proxy type."};
        case TOX_ERR_NEW_PROXY_BAD_HOST:
            throw std::runtime_error{prefix + " invalid proxy host."};
        case TOX_ERR_NEW_PROXY_BAD_PORT:
            throw std::runtime_error{prefix + " invalid proxy port."};
        case TOX_ERR_NEW_PROXY_NOT_FOUND:
            throw std::runtime_error{prefix + " proxy address could not be resolved."};
        case TOX_ERR_NEW_LOAD_ENCRYPTED:
            throw std::runtime_error{prefix + " profile is encrypted."};
        case TOX_ERR_NEW_LOAD_BAD_FORMAT:
            throw std::runtime_error{prefix + " profile data is invalid."};
        }
    }

    void errEncryptionSwitch(Tox_Err_Encryption &error,
                             std::string const &prefix = "Cannot encrypt profile:")
    {
        switch (error) {
        case TOX_ERR_ENCRYPTION_OK:
            break;
        case TOX_ERR_ENCRYPTION_NULL:
        case TOX_ERR_ENCRYPTION_FAILED:
            throw std::runtime_error{prefix + " UNKNOWN ERROR."};
        case TOX_ERR_ENCRYPTION_KEY_DERIVATION_FAILED:
            throw std::runtime_error{prefix + " crypto failed."};
        }
    }

    void errDecryptionSwitch(Tox_Err_Decryption &error,
                             std::string const &prefix = "Cannot decrypt profile:")
    {
        switch (error) {
        case TOX_ERR_DECRYPTION_OK:
            break;
        case TOX_ERR_DECRYPTION_NULL:
        case TOX_ERR_DECRYPTION_FAILED:
            throw std::runtime_error{prefix + " UNKNOWN ERROR."};
        case TOX_ERR_DECRYPTION_INVALID_LENGTH:
            throw std::runtime_error{prefix + " invalid length."};
        case TOX_ERR_DECRYPTION_BAD_FORMAT:
            throw std::runtime_error{prefix + " bad format."};
        case TOX_ERR_DECRYPTION_KEY_DERIVATION_FAILED:
            throw std::runtime_error{prefix + " crypto failed."};
        }
    }

    inline static auto const encryptionExtraLength{tox_pass_encryption_extra_length()};
    std::vector<std::uint8_t> decrypt(std::vector<std::uint8_t> const &profile)
    {
        std::vector<std::uint8_t> plain(profile.size() - encryptionExtraLength);

        Tox_Err_Decryption error;
        if (!tox_pass_decrypt(profile.data(), profile.size(),
                              reinterpret_cast<std::uint8_t const *>(_password->data()),
                              _password->size(), plain.data(), &error)) {
            errDecryptionSwitch(error);
        }

        return plain;
    }

    std::vector<std::uint8_t> encrypt(std::vector<std::uint8_t> const &profile)
    {
        std::vector<std::uint8_t> encrypted(profile.size() + encryptionExtraLength);

        Tox_Err_Encryption error;
        if (!tox_pass_encrypt(profile.data(), profile.size(),
                              reinterpret_cast<std::uint8_t const *>(_password->data()),
                              _password->size(), encrypted.data(), &error)) {
            errEncryptionSwitch(error);
        }

        return encrypted;
    }

private:
    Tox *tox;
    std::string _filename;
    std::optional<std::string> _password;

    Toxpp::Avatar _avatar;
};

#endif
