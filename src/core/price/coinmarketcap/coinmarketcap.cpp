#include "coinmarketcap.hpp"

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

std::optional<double> CoinMarketCapApi::price(laiin::Currency from, laiin::Currency to) const
{
    const std::map<laiin::Currency, std::string> CURRENCY_TO_ID{
        {laiin::Currency::USD, "2781"},
        {laiin::Currency::AUD, "2782"},
        {laiin::Currency::BRL, "2783"},
        {laiin::Currency::CAD, "2784"},
        {laiin::Currency::CHF, "2785"},
        {laiin::Currency::CNY, "2787"},
        {laiin::Currency::EUR, "2790"},
        {laiin::Currency::GBP, "2791"},
        {laiin::Currency::HKD, "2792"},
        {laiin::Currency::INR, "2796"},
        {laiin::Currency::JPY, "2797"},
        {laiin::Currency::KRW, "2798"},
        {laiin::Currency::MXN, "2799"},
        {laiin::Currency::NZD, "2802"},
        {laiin::Currency::PHP, "2803"},
        {laiin::Currency::RUB, "2806"},
        {laiin::Currency::SEK, "2807"},
        {laiin::Currency::SGD, "2808"},
        {laiin::Currency::ZAR, "2812"},
        {laiin::Currency::NGN, "2819"},
        {laiin::Currency::GHS, "3540"},
        //{laiin::Currency::, ""},
        //{laiin::Currency::XAG, ""},
        //{laiin::Currency::XAU, ""},
        {laiin::Currency::BTC, "1"},
        {laiin::Currency::LTC, "2"},
        {laiin::Currency::XMR, "328"},
        {laiin::Currency::ETH, "1027"},
        {laiin::Currency::WOW, "4978"},
    };

    auto it = CURRENCY_TO_ID.find(from);
    if (it == CURRENCY_TO_ID.cend()) {
        return std::nullopt;
    }
    const auto idFrom = it->second;

    it = CURRENCY_TO_ID.find(to);
    if (it == CURRENCY_TO_ID.cend()) {
        return std::nullopt;
    }
    const auto idTo = it->second;
    
    if defined(laiin_USE_QT)
    const QString BASE_URL{QStringLiteral("https://api.coinmarketcap.com/data-api/v3/"
                                      "cryptocurrency/quote/latest?id=%1&convertId=%2")};
    QNetworkAccessManager manager;
    QEventLoop loop;
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);

    const QUrl url(BASE_URL.arg(QString::fromStdString(idFrom), QString::fromStdString(idTo)));
    auto reply = manager.get(QNetworkRequest(url));
    loop.exec();
    QJsonParseError error;
    const auto json_doc = QJsonDocument::fromJson(reply->readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        return std::nullopt;
    }
    const auto root_obj = json_doc.object();
    const auto data_val = root_obj.value("data");
    if (!data_val.isArray()) {
        return std::nullopt;
    }
    const auto data_arr = data_val.toArray();
    if (data_arr.empty()) {
        return std::nullopt;
    }
    const auto item = data_arr.first().toObject();
    const auto quote = item.value("quotes").toArray().first().toObject();
    if (!quote.contains("price")) {
        return std::nullopt;
    }
    return quote.value("price").toDouble();
    #else
    #endif
    
    return std::nullopt;
}
