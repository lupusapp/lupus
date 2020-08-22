#pragma once

#include "color/src/color/color.hpp"
#include "gdkmm/pixbuf.h"
#include "include/Lupus.hpp"
#include <bits/c++config.h>
#include <optional>
#include <tox/tox.h>

namespace Lupus
{
class ToxSelf;
class ToxAvatar;
} // namespace Lupus

class Lupus::ToxSelf final
{
public:
    ToxSelf(void);
    ToxSelf(std::string const &profileFilename,
            std::optional<std::string> profilePassword = std::nullopt);
    ~ToxSelf(void);

    std::string name(void) const;
    void name(std::string const &name);
    inline static auto const nameMaxSize{tox_max_name_length()};

    std::string statusMessage(void) const;
    void statusMessage(std::string const &statusMessage);
    inline static auto const statusMessageMaxSize{tox_max_status_message_length()};

    std::string publicKey(void) const;
    inline static auto const publicKeySize{tox_public_key_size()};

    std::string filename(void) const;
    void filename(std::string const &filename);

    std::optional<std::string> password(void) const;
    void password(std::string const &password);

    void save(void);

    static bool isEncrypted(std::string const &profileFilename);

private:
    Tox *tox;
    std::string profileFilename;
    std::optional<std::string> profilePassword;

    static constexpr auto default_name = "Lupus' user";
    static constexpr auto default_status_message = "https://github.com/LupusApp/Lupus";
};

class Lupus::ToxAvatar final
{
public:
    ToxAvatar(void) = delete;
    ToxAvatar(ToxSelf *toxSelf);

    std::string const &filename(void) const;
    Glib::RefPtr<Gdk::Pixbuf> const &pixbuf(void) const;
    std::string const &hash(void) const;

    void createToxIdentIcon(void);

    inline static auto const avatarsDir{toxConfigDir + "avatars/"};
    static auto const maxFileSize{65536};
    static auto const standardSize{48};
    static auto const friendSize{36};

private:
    void createHash(void);
    void createPixbuf(void);

private:
    ToxSelf *toxSelf;
    std::string _filename;
    Glib::RefPtr<Gdk::Pixbuf> _pixbuf;
    std::string _hash;
};

class ToxIdentIcon final
{
public:
    static constexpr auto colors = 2;
    static constexpr auto size = 5;
    static constexpr auto coloredCols = 3;
    static constexpr auto colorBytes = 6;
}
