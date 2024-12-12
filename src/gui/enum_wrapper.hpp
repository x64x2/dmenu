#pragma once

#ifndef ENUM_WRAPPER_HPP_laiin
#define ENUM_WRAPPER_HPP_laiin

#include <QObject>

namespace laiin {

class EnumWrapper : public QObject {
    Q_OBJECT
public:
    EnumWrapper(QObject* parent = nullptr);
    
    enum class WalletError {
        Ok = 0, 
        WrongPassword, 
        PasswordsDoNotMatch, 
        AlreadyExists,
        IsOpenedByAnotherProgram,
        DoesNotExist,
        BadNetworkType,
        IsNotOpened, // monero_wallet_obj is nullptr
        BadWalletType,
        InvalidMnemonic,
        InvalidAddress,
        InvalidViewKey,
        InvalidSpendKey,
    };    
    Q_ENUM(WalletError)

    enum class CartError {
        Ok,
        Missing,
        Full,
        ItemOutOfStock,
        ItemQuantitySurpassed,
        ItemQuantityNotSpecified,
        SellerAddOwnItem,
    };
    Q_ENUM(CartError)
    
    enum class LoginError {
        Ok = 0,
    
        WrongPassword = static_cast<int>(WalletError::WrongPassword),
        WalletIsOpenedByAnotherProgram = static_cast<int>(WalletError::IsOpenedByAnotherProgram),
        WalletDoesNotExist = static_cast<int>(WalletError::DoesNotExist),
        WalletBadNetworkType = static_cast<int>(WalletError::BadNetworkType),
        WalletIsNotOpened = static_cast<int>(WalletError::IsNotOpened),
        WalletBadWalletType = static_cast<int>(WalletError::BadWalletType),
        WalletInvalidMnemonic = static_cast<int>(WalletError::InvalidMnemonic),
        WalletInvalidAddress = static_cast<int>(WalletError::InvalidAddress),
        WalletInvalidViewKey = static_cast<int>(WalletError::InvalidViewKey),
        WalletInvalidSpendKey = static_cast<int>(WalletError::InvalidSpendKey),
        
        DaemonIsNotConnected = 100,
        
        UserNotFound = 200,
        UserIsNullPointer,
    };
    Q_ENUM(LoginError)
    
    enum class RegistrationError {
        Ok = 0,
        
        Password_NoUpperCaseLetter,
        Password_NoLowerCaseLetter,
        Password_NoDigit,
        Password_NoSpecialCharacter,
        Password_LengthTooShort, // or WrongLength
    
        Username_LengthTooShort, // MinimumLengthReached
        Username_LengthTooLong, // MaximumLengthReached
        Username_NoSpacesAllowed,
        Username_NoSymbolsAllowedWithExceptions,
        Username_MustBeginWithLetter,
        Username_MustEndWithAlphaNumericCharacter,
        Username_TakenOrUnavailable,
        Username_ReservedForInternalUse,
    };
    Q_ENUM(RegistrationError)

    enum class Sorting {
        // Listing sorting
        SortNone = 0,
        SortByCategory,
        SortByMostRecent,
        SortByLatest = SortByMostRecent,
        SortByDateNewest = SortByMostRecent,
        SortByOldest,
        SortByDateOldest = SortByOldest,
        SortByAlphabeticalOrder,
        SortByName = SortByAlphabeticalOrder,
        SortByPriceLowest,
        SortByPriceHighest,
        SortByAverageRating,
        SortByMostFavorited,
        SortByMostSales,
        // Inventory sorting
        SortByAvailability, // If item is in stock
        SortByQuantitySmallest,
        SortByQuantityBiggest,
        SortByProductCode,
        SortByCondition,

    };
    Q_ENUM(Sorting)
};
    
}

Q_DECLARE_METATYPE(laiin::EnumWrapper::WalletError)
Q_DECLARE_METATYPE(laiin::EnumWrapper::CartError)
Q_DECLARE_METATYPE(laiin::EnumWrapper::LoginError)
Q_DECLARE_METATYPE(laiin::EnumWrapper::RegistrationError)
Q_DECLARE_METATYPE(laiin::EnumWrapper::Sorting)
#endif
