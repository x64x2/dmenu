#ifndef CRYPTORANK_API_HPP_laiin
#define CRYPTORANK_API_HPP_laiin

#include "../price_api.hpp"

class CryptoRankApi : public laiin::PriceApi
{
public:
    std::optional<double> price(laiin::Currency from, laiin::Currency to) const override;
};

#endif // CRYPTORANK_PRICE_SOURCE_HPP_laiin
