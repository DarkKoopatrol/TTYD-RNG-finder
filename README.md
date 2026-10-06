# TTYD-RNG-finder
RNG functions that find your RNG seed in TTYD. Cannot be run on it own, since this only has functions that are meant to be used in other functions.

Notes on compilation: support for __uint128_t and Variable Length Arrays is required, so MSVC is not supported. In GCC, compiling with -O3 gives the fastest execution times, with -O1 not far behind, but -Ofast is slow.
