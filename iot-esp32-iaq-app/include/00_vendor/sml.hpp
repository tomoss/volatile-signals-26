#ifndef VENDOR_SML_HPP
#define VENDOR_SML_HPP

// boost/sml.hpp's index-sequence helpers trip the strict -Wsign-conversion flag.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "boost/sml.hpp"
#pragma GCC diagnostic pop

#endif // VENDOR_SML_HPP
