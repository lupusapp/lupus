#ifndef __LUPUS_TOXPP_BOOTSTRAP__
#define __LUPUS_TOXPP_BOOTSTRAP__

#include "Toxpp.hpp"
#include <cstdint>
#include <initializer_list>
#include <sodium/utils.h>
#include <string>
#include <tox/tox.h>
#include <vector>

struct Node {
    std::string ip;
    std::uint16_t port;
    std::uint8_t keyHex[TOX_PUBLIC_KEY_SIZE * 2];
    std::uint8_t keyBin[TOX_PUBLIC_KEY_SIZE];
};

class Toxpp::Bootstrap final
{
public:
    class Node final
    {
    public:
        Node(void) = delete;
        Node(std::string const &ip, std::uint16_t port, std::string const &publicKeyHex)
            : _ip{ip}, _port{port}, _publicKeyBin(publicKeyBinSize)
        {
            this->publicKeyHex(publicKeyHex);
        }

        std::string ip(void) const { return _ip; }
        void ip(std::string const &ip) { _ip = ip; }

        std::uint16_t port(void) const { return _port; }
        void port(std::uint16_t port) { _port = port; }

        std::string publicKeyHex(void) const { return _publicKeyHex; }
        void publicKeyHex(std::string const &publicKeyHex)
        {
            if (publicKeyHex.size() != publicKeyHexSize) {
                throw std::runtime_error{"Cannot create Node: publicKeyHex must have a size of " +
                                         std::to_string(publicKeyHexSize)};
            }

            _publicKeyHex = publicKeyHex;

            sodium_hex2bin(_publicKeyBin.data(), _publicKeyBin.size(),
                           reinterpret_cast<char *>(_publicKeyHex.data()), _publicKeyHex.size(),
                           nullptr, nullptr, nullptr);
        }

        std::vector<std::uint8_t> publicKeyBin(void) const { return _publicKeyBin; }

    public:
        inline static constexpr auto const publicKeyHexSize{TOX_PUBLIC_KEY_SIZE * 2};
        inline static constexpr auto const publicKeyBinSize{TOX_PUBLIC_KEY_SIZE};

    private:
        std::string _ip;
        std::uint16_t _port;
        std::string _publicKeyHex;
        std::vector<std::uint8_t> _publicKeyBin;
    };

public:
    static Bootstrap defaultBootstrap(void)
    {
        return {{{"85.172.30.117"},
                 33445,
                 "8E7D0B859922EF569298B4D261A8CCB5FEA14FB91ED412A7603A585A25698832"},
                {{"95.31.18.227"},
                 33445,
                 "257744DBF57BE3E117FE05D145B5F806089428D4DCE4E3D0D50616AA16D9417E"},
                {{"94.45.70.19"},
                 33445,
                 "CE049A748EB31F0377F94427E8E3D219FC96509D4F9D16E181E956BC5B1C4564"},
                {{"46.229.52.198"},
                 33445,
                 "813C8F4187833EF0655B10F7752141A352248462A567529A38B6BBF73E979307"}};
    }

public:
    Bootstrap(void) = default;
    Bootstrap(std::initializer_list<Node> const &nodes)
    {
        for (auto &node : nodes) {
            add(node);
        }
    }
    void add(Node const &node) { nodes.push_back(node); }
    void run(Tox *const tox)
    {
        std::string message{"Cannot bootstrap "};

        for (auto const &node : nodes) {
            message += node.ip() + ":" + std::to_string(node.port()) + ":";

            Tox_Err_Bootstrap error;
            tox_bootstrap(tox, node.ip().data(), node.port(), node.publicKeyBin().data(), &error);

            switch (error) {
            case TOX_ERR_BOOTSTRAP_OK:
                break;
            case TOX_ERR_BOOTSTRAP_NULL:
                throw std::runtime_error{message + " UNKNOWN ERROR."};
            case TOX_ERR_BOOTSTRAP_BAD_HOST:
                throw std::runtime_error{message + " bad host."};
            case TOX_ERR_BOOTSTRAP_BAD_PORT:
                throw std::runtime_error{message + " bad port."};
            }
        }
    }

private:
    std::vector<Node> nodes;
};

#endif
