#include "cointelegraph.hpp"

if defined(laiin_USE_QT)
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
else
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#endif

#include <map>

#include "../currency_map.hpp"

namespace {

const std::map<laiin::Currency, std::string> CURRENCY_TO_ID{
    {laiin::Currency::USD, "USD"},
    {laiin::Currency::AUD, "AUD"},
    {laiin::Currency::BRL, "BRL"},
    {laiin::Currency::CAD, "CAD"},
    {laiin::Currency::CHF, "CHF"},
    {laiin::Currency::CNY, "CNY"},
    {laiin::Currency::EUR, "EUR"},
    {laiin::Currency::GBP, "GPB"},
    {laiin::Currency::HKD, "HKD"},
    {laiin::Currency::INR, "INR"},
    {laiin::Currency::JPY, "JPY"},
    {laiin::Currency::KRW, "KRW"},
    {laiin::Currency::MXN, "MXN"},
    {laiin::Currency::NZD, "NZD"},
    {laiin::Currency::PHP, "PHP"},
    {laiin::Currency::RUB, "RUB"},
    {laiin::Currency::SEK, "SEK"},
    {laiin::Currency::SGD, "SGD"},
    {laiin::Currency::ZAR, "ZAR"},
    {laiin::Currency::NGN, "NGN"},
    {laiin::Currency::GHS, "GHS"},
    
    {laiin::Currency::XAG, "XAG"},
    {laiin::Currency::XAU, "XAU"},
};

const std::map<laiin::Currency, std::string> CRYPTO_TO_ID{
    {laiin::Currency::BTC, "BTC"},
    {laiin::Currency::ETH, "ETH"},
    {laiin::Currency::XMR, "XMR"},
};

bool is_crypto(laiin::Currency c)
{
    return CRYPTO_TO_ID.find(c) != CRYPTO_TO_ID.cend();
}

bool is_currency(laiin::Currency c)
{
    return CURRENCY_TO_ID.find(c) != CURRENCY_TO_ID.cend();
}

} // namespace

std::optional<double> CoinTelegraphApi::price(laiin::Currency from, laiin::Currency to) const
{
    if (is_currency(from) && is_currency(to)) {
        return std::nullopt;
    }

    if defined(laiin_USE_QT)
    const QString BASE_URL{QStringLiteral("https://ticker-api.cointelegraph.com/rates/?full=true")};
    QNetworkAccessManager manager;
    QEventLoop loop;
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);

    const QUrl url(BASE_URL);
    auto reply = manager.get(QNetworkRequest(url));
    loop.exec();
    QJsonParseError error;
    const auto json_doc = QJsonDocument::fromJson(reply->readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        return std::nullopt;
    }
    const auto root_obj = json_doc.object();
    const auto data_val = root_obj.value("data");
    if (!data_val.isObject()) {
        return std::nullopt;
    }
    const auto data_obj = data_val.toObject();

    if (is_crypto(from)) {
        const auto crypto_obj = data_obj.value(QString::fromStdString(CRYPTO_TO_ID.at(from))).toObject();

        if (is_currency(to)) {
            const auto currecy_obj = crypto_obj.value(QString::fromStdString(CURRENCY_TO_ID.at(to))).toObject();
            if (!currecy_obj.contains("price")) {
                return std::nullopt;
            }
            return currecy_obj.value("price").toDouble();
        }

        if (is_crypto(to)) {
            const auto usd_obj = crypto_obj.value("USD").toObject();
            if (!usd_obj.contains("price")) {
                return std::nullopt;
            }
            const auto from_price = usd_obj.value("price").toDouble();

            const auto crypto_obj = data_obj.value(QString::fromStdString(CRYPTO_TO_ID.at(to))).toObject();
            const auto currecy_obj = crypto_obj.value("USD").toObject();
            if (!currecy_obj.contains("price")) {
                return std::nullopt;
            }
            const auto to_price = currecy_obj.value("price").toDouble();
            return from_price / to_price;
        }

        return std::nullopt;
    }

    if (is_currency(from)) {
        const auto crypto_obj = data_obj.value(QString::fromStdString(CRYPTO_TO_ID.at(to))).toObject();
        const auto currecy_obj = crypto_obj.value(QString::fromStdString(CURRENCY_TO_ID.at(from))).toObject();
        if (!currecy_obj.contains("price")) {
            return std::nullopt;
        }
        return 1.0 / currecy_obj.value("price").toDouble();
    }
    #else
    #endif
    
    return std::nullopt;
}
