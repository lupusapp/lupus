#pragma once

#include <optional>
#include <tox/tox.h>

namespace Lupus
{
class ToxSelf;
}

class Lupus::ToxSelf final
{
public:
    ToxSelf(void);
    ToxSelf(std::string const &profileFilename,
            std::optional<std::string> profilePassword = std::nullopt);
    ~ToxSelf(void);

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

