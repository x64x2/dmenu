#pragma once

#ifndef PRICE_API_HPP_laiin
#define PRICE_API_HPP_laiin

#include "currency_enum.hpp" // laiin::Currency::

#include <optional>

namespace laiin {

enum class PriceSource {
    // Crypto Price Aggregators
    CoinMarketCap = 0,
    CoinGecko,
    CryptoWatch,
    CoinTelegraph,
    CryptoRank,
    CoinCodex,
    Fawazahmed0,
    // Exchanges
    Kraken,
};

class PriceApi
{
public:
    //PriceApi() = default;
    virtual ~PriceApi() = default;

    virtual std::optional<double> price(laiin::Currency from, laiin::Currency to) const = 0;
};

}
#endif // PRICE_API_HPP_laiin
