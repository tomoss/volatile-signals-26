#ifndef CLAIM_CODE_HPP
#define CLAIM_CODE_HPP

#include <array>

// 6-digit device registration code, plus null terminator.
using ClaimCode = std::array<char, 7>;

#endif // CLAIM_CODE_HPP
