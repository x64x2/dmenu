#ifndef CRYPTOWATCH_API_HPP_laiin
#define CRYPTOWATCH_API_HPP_laiin

#include "../price_api.hpp"

class CryptoWatchApi : public laiin::PriceApi
{
public:
    std::optional<double> price(laiin::Currency from, laiin::Currency to) const override;
};

#endif // CRYPTOWATCH_PRICE_SOURCE_HPP_laiin
