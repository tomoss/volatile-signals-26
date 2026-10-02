#ifndef CLAIM_STORE_HPP
#define CLAIM_STORE_HPP

#include <optional>

#include "utils/claim_code.hpp"

class ClaimStore {
public:
    virtual ~ClaimStore() = default;

    virtual bool loadDeviceClaimStatus() = 0;
    virtual bool saveDeviceClaimStatus(bool p_claimed) = 0;

    virtual std::optional<ClaimCode> loadClaimCode() = 0;
    virtual bool saveClaimCode(const ClaimCode& p_code) = 0;
};

#endif // CLAIM_STORE_HPP
