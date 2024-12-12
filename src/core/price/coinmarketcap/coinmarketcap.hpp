#ifndef COINMARKETCAP_API_HPP_laiin
#define COINMARKETCAP_API_HPP_laiin

#include "../price_api.hpp"

class CoinMarketCapApi : public laiin::PriceApi
{
public:
    std::optional<double> price(laiin::Currency from, laiin::Currency to) const override;
};

#endif // COINMARKETCAP_PRICE_SOURCE_HPP_laiin
