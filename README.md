# TTYD-RNG-finder
RNG functions that find your RNG seed in TTYD. One finds your RNG seed by searching through every RNG seed, and the other only searches through a limited range. This cannot be run on its own, since this only has functions that are meant to be used in other projects.

In order to find your RNG seed, you have to jump and input the sound effect. Input 0 for "Hoo", 1 for "Waa", 2 for "Hu", 3 for "Hah", 4 for "Wahoo", 5 if you aren't sure, or 9 to restart the program. If you input something invalid, everything on that line will be rejected.

Notes on compilation: support for __uint128_t and Variable Length Arrays is required, so MSVC is not supported. In GCC, compiling with -O3 gives the fastest execution times, with -O1 not far behind, but -Ofast is slow.
