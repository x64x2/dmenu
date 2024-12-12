#pragma once

#ifndef WOWNERO_HPP_laiin
#define WOWNERO_HPP_laiin

#include "../wallet.hpp"

namespace laiin {

class WowneroWallet : public Wallet/*, public wownero_wallet_listener*/ {
public:
    WowneroWallet();
    ~WowneroWallet();
};

}
#endif // WOWNERO_HPP_laiin
