#include "wownero.hpp"

#include "../../tools/string.hpp"
#include "../../settings.hpp" // language

#include <nlohmann/json.hpp>

namespace laiin {

WowneroWallet::WowneroWallet() : Wallet(WalletType::Wownero) {}

WowneroWallet::~WowneroWallet() {}



}
