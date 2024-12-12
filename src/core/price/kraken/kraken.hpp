#ifndef KRAKEN_API_HPP_laiin
#define KRAKEN_API_HPP_laiin

#include "../price_api.hpp"

class KrakenApi : public laiin::PriceApi
{
public:
    std::optional<double> price(laiin::Currency from, laiin::Currency to) const override;
};

#endif // KRAKEN_API_HPP_laiin
