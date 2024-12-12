#include "currency_rate_provider.hpp"

#include <QFuture>
#include <QMutexLocker>
#include <QQmlEngine>
#include <QTimer>
#include <QtConcurrent>

#include "../core/price/currency_map.hpp"
#include "../core/price/currency_converter.hpp"

namespace {
const std::vector<laiin::PriceSource> SOURCES_TO_USE{
    laiin::PriceSource::CoinMarketCap,
    laiin::PriceSource::CoinGecko,
    laiin::PriceSource::CryptoWatch,
    laiin::PriceSource::CoinTelegraph,
    laiin::PriceSource::CryptoRank,
    laiin::PriceSource::CoinCodex,
    laiin::PriceSource::Fawazahmed0,
    // Exchanges
    laiin::PriceSource::Kraken,
};

QHash<QPair<int, int>, double> updateRates(const QList<QPair<int, int>> keys)
{
    QHash<QPair<int, int>, double> newRates;
    for (const auto &key : keys) {
        const auto currencyFrom = static_cast<laiin::Currency>(key.first);
        const auto currencyTo = static_cast<laiin::Currency>(key.second);

        for (const auto &source : SOURCES_TO_USE) {
            auto price_source = laiin::Converter::make_price_source(source);
            auto price_opt = price_source->price(currencyFrom, currencyTo);
            if (price_opt.has_value()) {
                newRates[key] = price_opt.value();
                break;
            }
        }
    }
    return newRates;
}
} // namespace

CurrencyExchangeRatesProvider::CurrencyExchangeRatesProvider()
{
    connect(&mUpdateFutureWatcher,
            &QFutureWatcher<QHash<QPair<int, int>, double>>::finished,
            this,
            [&]() {
                this->setRates(mUpdateFutureWatcher.result());
                QTimer::singleShot(3000, this, &CurrencyExchangeRatesProvider::startUpdate);
            });
    startUpdate();
}

CurrencyExchangeRatesProvider::~CurrencyExchangeRatesProvider()
{
    mUpdateFutureWatcher.waitForFinished();
}

QObject *CurrencyExchangeRatesProvider::qmlInstance(QQmlEngine * /*engine*/,
                                                    QJSEngine * /*scriptEngine*/)
{
    return new CurrencyExchangeRatesProvider();
}

int CurrencyExchangeRatesProvider::reevaluate() const
{
    // this dummy property is used for dynamic recalculation. Don't delete
    return 42;
}

double CurrencyExchangeRatesProvider::getPrice(const QString &from, const QString &to) const
{
    const auto fromIt = laiin::CurrencyMap.find(from.toUpper().toStdString());
    if (fromIt == laiin::CurrencyMap.cend()) {
        return 0.0;
    }
    const auto currencyFrom = static_cast<int>(std::get<0>(fromIt->second));

    const auto toIt = laiin::CurrencyMap.find(to.toUpper().toStdString());
    if (toIt == laiin::CurrencyMap.cend()) {
        return 0.0;
    }
    const auto currencyTo = static_cast<int>(std::get<0>(toIt->second));

    const auto key = qMakePair(currencyFrom, currencyTo);
    if (!mCurrentRates.contains(key)) {
        mCurrentRates[key] = 0.0;
    }

    return mCurrentRates.value(key);
}

void CurrencyExchangeRatesProvider::setRates(const QHash<QPair<int, int>, double> &newRates)
{
    QMutexLocker locker(&mRatesMutex);
    mCurrentRates = newRates;
    emit ratesUpdated();
}

void CurrencyExchangeRatesProvider::startUpdate()
{
    mUpdateFutureWatcher.setFuture(QtConcurrent::run(updateRates, mCurrentRates.keys()));
}
