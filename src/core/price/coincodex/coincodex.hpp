#ifndef COINCODEX_API_HPP_laiin
#define COINCODEX_API_HPP_laiin

#include "../price_api.hpp"

class CoinCodexApi : public laiin::PriceApi
{
public:
    std::optional<double> price(laiin::Currency from, laiin::Currency to) const override;
};

#endif // COINCODEX_PRICE_SOURCE_HPP_laiin
