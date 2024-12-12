#ifndef COINGECKO_API_HPP_laiin
#define COINGECKO_API_HPP_laiin

#include "../price_api.hpp"

class CoinGeckoApi : public laiin::PriceApi
{
public:
    std::optional<double> price(laiin::Currency from, laiin::Currency to) const override;
};

#endif // COINGECKO_PRICE_SOURCE_HPP_laiin
