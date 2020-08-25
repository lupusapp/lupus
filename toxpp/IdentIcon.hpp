#ifndef __LUPUS_TOXPP_IDENTICON__
#define __LUPUS_TOXPP_IDENTICON__

#include "Toxpp.hpp"
#include "digestpp/algorithm/sha2.hpp"
#include "lodepng/lodepng.h"
#include <tox/tox.h>

#define N_ELEMENTS(a) (sizeof(a) / sizeof(*a))
#define PNG_SIZE      250
#define ROWS          5 // rows / cols
#define COLORS        2
#define COLORED_COLS  3
#define COLOR_BYTES   6

struct RGB {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;

    RGB(void) = default;
    RGB(std::uint8_t v) : r{v}, g{v}, b{v} {};
    RGB(std::uint8_t r, std::uint8_t g, std::uint8_t b) : r{r}, g{g}, b{b} {};
};

class Toxpp::IdentIcon final
{
public:
    IdentIcon(void) = delete;
    IdentIcon(std::vector<std::uint8_t> const &publicKeyBin)
    {
        std::uint8_t hash[256 / 8]{0};
        digestpp::sha256{}
            .absorb(publicKeyBin.data(), publicKeyBin.size())
            .digest(hash, N_ELEMENTS(hash));

        for (int i = 0; i < COLORS; ++i) {
            std::uint8_t const *hashPart{reinterpret_cast<std::uint8_t const *>(
                hash + N_ELEMENTS(hash) - COLOR_BYTES * (i + 1))};

            std::uint64_t hueInt{hashPart[0]};
            for (int i = 1; i < COLOR_BYTES; ++i) {
                hueInt = hueInt << 8;
                hueInt += hashPart[i];
            }

            auto hue{
                static_cast<float>(hueInt) /
                static_cast<float>(((static_cast<std::uint64_t>(1)) << (8 * COLOR_BYTES)) - 1)};
            auto sat{0.5f};
            auto lig{static_cast<float>(i) / COLORS + 0.3f};

            colors[i] = HSLToRGB(hue, sat, lig);
        }

        for (int row = 0; row < ROWS; ++row) {
            for (int col = 0; col < COLORED_COLS; ++col) {
                cellColorIndex[row][col] = hash[row * COLORED_COLS + col] % COLORS;
            }
        }
    }

    void write(std::string const &filename)
    {
        std::vector<std::uint8_t> image(PNG_SIZE * PNG_SIZE * 4);

        for (int row = 0; row < ROWS; ++row) {
            for (int col = 0; col < ROWS; ++col) {
                auto color{colors[cellColorIndex[row][std::abs((col * 2 - (ROWS - 1)) / 2)]]};

                int ypadding{PNG_SIZE / ROWS};
                int yfrom{ypadding * row};
                int yto{yfrom + ypadding};
                for (; yfrom < yto; ++yfrom) {
                    int xpadding{PNG_SIZE / ROWS};
                    int xfrom{xpadding * col};
                    int xto{xfrom + xpadding};

                    for (; xfrom < xto; ++xfrom) {
                        image[4 * PNG_SIZE * yfrom + 4 * xfrom + 0] = color.r;
                        image[4 * PNG_SIZE * yfrom + 4 * xfrom + 1] = color.g;
                        image[4 * PNG_SIZE * yfrom + 4 * xfrom + 2] = color.b;
                        image[4 * PNG_SIZE * yfrom + 4 * xfrom + 3] = 255;
                    }
                }
            }
        }

        auto error{lodepng::encode(filename, image, PNG_SIZE, PNG_SIZE)};
        if (error) {
            throw std::runtime_error{std::string{"Cannot write png: "} + lodepng_error_text(error)};
        }
    }

private:
    RGB HSLToRGB(float h, float s, float l)
    {
        if (!s) {
            return {static_cast<std::uint8_t>(l * 255)};
        }

        float hue{h};
        float v2 = (l < 0.5) ? (l * (1 + s)) : ((l + s) - (l * s));
        float v1 = 2 * l - v2;

        return {static_cast<uint8_t>(hueToRGB(v1, v2, hue + (1.0f / 3.0f)) * 255),
                static_cast<uint8_t>(hueToRGB(v1, v2, hue) * 255),
                static_cast<uint8_t>(hueToRGB(v1, v2, hue - (1.0f / 3.0f)) * 255)};
    }

    float hueToRGB(float v1, float v2, float h)
    {
        if (h < 0) {
            ++h;
        }

        if (h > 1) {
            --h;
        }

        if ((6 * h) < 1) {
            return (v1 + (v2 - v1) * 6 * h);
        }

        if ((2 * h) < 1) {
            return v2;
        }

        if ((3 * h) < 2) {
            return (v1 + (v2 - v1) * ((2.0f / 3) - h) * 6);
        }

        return v1;
    };

private:
    short cellColorIndex[ROWS][COLORED_COLS];
    RGB colors[COLORS];
};

#endif
