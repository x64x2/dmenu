#ifndef COINTELEGRAPH_API_HPP_laiin
#define COINTELEGRAPH_API_HPP_laiin

#include "../price_api.hpp"

class CoinTelegraphApi : public laiin::PriceApi
{
public:
    std::optional<double> price(laiin::Currency from, laiin::Currency to) const override;
};

#endif // COINTELEGRAPH_PRICE_SOURCE_HPP_laiin
