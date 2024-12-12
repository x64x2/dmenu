#ifndef FAWAZAHMED0_CURRENCY_API_HPP_laiin
#define FAWAZAHMED0_CURRENCY_API_HPP_laiin

#include "../price_api.hpp"

class Fawazahmed0CurrencyApi : public laiin::PriceApi
{
public:
    std::optional<double> price(laiin::Currency from, laiin::Currency to) const override;
};

#endif // FAWAZAHMED0_CURRENCY_API_HPP_laiin
