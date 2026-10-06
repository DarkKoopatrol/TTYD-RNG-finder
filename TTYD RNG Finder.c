#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>


#ifdef __has_attribute
    #if __has_attribute(noinline)
        #define NO_INLINE __attribute__((noinline))
    #endif
#endif
#ifndef NO_INLINE
    #define NO_INLINE
#endif


struct DynamicUint8Array
{
    uint8_t *restrict data;
    uint8_t entries;
    uint8_t capacity;
};

struct DynamicUint32Array
{
    uint32_t *restrict data;
    uint32_t entries;
    uint32_t capacity;
};

const uint32_t MULTI = 0x41C64E6D;
const uint32_t ADDI = 0x3039;


#define Mul128_u32(lowbits, d) (((__uint128_t)(lowbits) * (d)) >> 64)
#define ComputeM_u32(mod) (UINT64_C(0xFFFFFFFFFFFFFFFF) / (mod) + 1)
#define FastMod(div, mod, m) (uint32_t)Mul128_u32((m) * (div), mod)

#define IncrementRngState(rng) ((rng) * MULTI + ADDI)
#define RngModConst(rng, mod) FastMod((((rng) >> 16) & 0x7FFF), mod, ComputeM_u32(mod))
#define IncrementAndModConst(rng, mod) RngModConst(rng = IncrementRngState(rng), mod)


static inline uint64_t BinaryExpo(uint64_t base, uint32_t power)
{
    uint64_t result = 1;
    for (;;)
    {
        if (power & 1)
        {
            result *= base;
        }
        base *= base; // Yes putting this here and not at the end is faster
        power /= 2;
        if (!power)
        {
            return result;
        }
    }
}


static inline uint32_t IndexToRng(const uint32_t initial, const uint32_t index) // RNG(n, k) = (k * a^n) + (b * (a^n - 1)/(a - 1)), k = initial seed
{
    uint64_t exponent = BinaryExpo(MULTI, index);
    uint32_t verticalShift = initial * exponent;                         // Remove if RNG(0) being 0 is fine
    uint32_t geometricSeries = ADDI * ((exponent - 1) / 4 * 2756156051); // 2756156051 = ModInverse((multi - 1) / 4)
    return verticalShift + geometricSeries;
}


void GetUserInput(struct DynamicUint8Array *restrict sequenceArr, uint8_t minSize)
{
    printf("Input: ");

    uint8_t savedSize = sequenceArr->entries;
    int ch = getchar();
    do // Wanted to end only when input buffer is empty, but with speed optimizations this is no longer necessary
    {
        if (ch == '\n') // Save line
        {
            printf("Input: ");
            savedSize = sequenceArr->entries;
        }
        else if (('0' <= ch) && (ch <= '4'))
        {
            if (sequenceArr->entries >= sequenceArr->capacity)
            {
                sequenceArr->capacity = sequenceArr->entries + 1;
                sequenceArr->data = realloc(sequenceArr->data, sizeof(*sequenceArr->data) * (sequenceArr->capacity));
                if (sequenceArr->data == NULL)
                {
                    printf("Memory reallocation failed\n");
                    exit(EXIT_FAILURE);
                }
            }
            sequenceArr->data[sequenceArr->entries++] = ch - '0';
        }
        else if (ch == '5')
        {
            if (sequenceArr->entries >= sequenceArr->capacity)
            {
                sequenceArr->capacity = sequenceArr->entries + 1;
                sequenceArr->data = realloc(sequenceArr->data, sizeof(*sequenceArr->data) * (sequenceArr->capacity));
                if (sequenceArr->data == NULL)
                {
                    printf("Memory reallocation failed\n");
                    exit(EXIT_FAILURE);
                }
            }
            sequenceArr->data[sequenceArr->entries++] = 5;
            minSize++;
        }
        else if (ch == '9')
        {
            printf("Restarting RNG calculation.\n");
            while (getchar() != '\n');
            sequenceArr->data[0] = 9;
            return;
        }
        else // Invalid input, reject and clear line
        {
            printf("Invalid input. Line rejected.\nInput: ");
            while (getchar() != '\n');
            sequenceArr->entries = savedSize;
        }
    } while (((ch = getchar()) != '\n') || (sequenceArr->entries < minSize));
}


NO_INLINE uint32_t GetRngNoRange()
{
    #define minMatches 6                         // Arbitrary value, can be whatever you want. This exists to restrict memory usage, which has the side effect of affecting performance. More entries means more checks, but also means less memory writes, which are slow.
    const uint32_t matchArrInitialSize = 454463; // Max amount of matches, find values in https://docs.google.com/spreadsheets/d/1VY3PEyxFODt71zfcJx0ocyf96q6iSnkKL4pgPLFqM6Q.

    struct DynamicUint8Array sequenceArr = {.data = malloc(sizeof(*sequenceArr.data) * minMatches), .capacity = minMatches, .entries = 0};
    struct DynamicUint32Array matchArr = {.data = malloc(sizeof(*matchArr.data) * matchArrInitialSize), .entries = 0}; // Localizing matchArr, particularly entries, was a consideration, but was ultimately deemed not worth it for complicated reasons
    
    GetUserInput(&sequenceArr, minMatches);
    if (sequenceArr.data[0] == 9)
    {
        free(sequenceArr.data);
        free(matchArr.data);
        return GetRngNoRange();
    }

    size_t i = 0;
    for (; sequenceArr.data[i] == 5; i++); // Removes blanks from start of sequence

    bool blank = false; size_t size = 0;
    for (size_t j = i + 1; j < sequenceArr.entries; j++)
    {
        if (sequenceArr.data[j] != 5)
        {
            size++;
        }
        else
        {
            blank = true;
        }
    }
    if (blank && (size < minMatches)) // Not <= because we skip over the first value
    {
        GetUserInput(&sequenceArr, 1); // If there's a blank present, make the minimum size minMatches + 1 to avoid potentially indexing past the bounds of the array
    }

    sequenceArr.entries -= i;
    uint64_t packedBounds[sequenceArr.entries]; // Faster than struct
    for (size_t j = 0; j < sequenceArr.entries; j++)
    {
        if (sequenceArr.data[i] == 5)
        {
            packedBounds[j] = 9999;
        }
        else
        {
            const uint32_t lowerBound = sequenceArr.data[i] * 2000;
            const uint32_t upperBound = lowerBound + 1999;
            packedBounds[j] = ((uint64_t)lowerBound << 32 | upperBound);
        }
        i++;
    }

    for (uint32_t rng = (packedBounds[0] >> 32) << 16; rng < 0x80000000; rng += 8000 << 16)
    {
        uint32_t target = rng + (2000 << 16);
        if (target > 0x80000000)
        {
            target = 0x80000000;
        }
        for (; rng < target; rng++)
        {
            #if minMatches <= 1
                uint32_t rng2 = rng;
            #else
                uint32_t rng2 = IncrementRngState(rng);
                const uint32_t result = RngModConst(rng2, 10000);
                if ((result > (uint32_t)packedBounds[1]) || ((packedBounds[1] >> 32) > result)) // Bitwise OR instead of logical OR helps with branch misprediction, which is faster for 1 and 3, but is overall not worth it since logical OR can break out early which is faster for 0, 2, and 4
                {
                    goto exit;
                }
            #endif

            #if minMatches <= 2
                for (size_t i = minMatches; i < sequenceArr.entries; i++)
                {
                    const uint32_t result = IncrementAndModConst(rng2, 10000);
                    if ((result > (uint32_t)packedBounds[i]) || ((packedBounds[i] >> 32) > result))
                    {
                        goto exit;
                    }
                }
            #else
                size_t i = 2;
                do
                {
                    const uint32_t result = IncrementAndModConst(rng2, 10000);
                    if ((result > (uint32_t)packedBounds[i]) || ((packedBounds[i] >> 32) > result))
                    {
                        goto exit;
                    }
                } while (++i < sequenceArr.entries);
            #endif

            matchArr.data[matchArr.entries++] = rng2;
            exit:;
        }
    }

    while (matchArr.entries > 1)
    {
        printf("%u matches\n", matchArr.entries);
        matchArr.capacity = matchArr.entries;
        matchArr.data = realloc(matchArr.data, sizeof(*matchArr.data) * matchArr.capacity);
        matchArr.entries = 0;

        sequenceArr.entries = 0;
        GetUserInput(&sequenceArr, 1);
        if (sequenceArr.data[0] == 9)
        {
            free(sequenceArr.data);
            free(matchArr.data);
            return GetRngNoRange();
        }

        uint64_t packedBounds[sequenceArr.entries];
        for (size_t i = 0; i < sequenceArr.entries; i++)
        {
            if (sequenceArr.data[i] == 5)
            {
                packedBounds[i] = 9999;
            }
            else
            {
                const uint32_t lowerBound = sequenceArr.data[i] * 2000;
                const uint32_t upperBound = lowerBound + 1999;
                packedBounds[i] = ((uint64_t)lowerBound << 32 | upperBound);
            }
        }

        for (size_t i = 0; i < matchArr.capacity; i++)
        {
            uint32_t rng = IncrementRngState(matchArr.data[i]);
            const uint32_t result = RngModConst(rng, 10000);
            if ((result > (uint32_t)packedBounds[0]) || ((packedBounds[0] >> 32) > result))
            {
                goto exit2;
            }

            for (size_t j = 1; j < sequenceArr.entries; j++)
            {
                const uint32_t result = IncrementAndModConst(rng, 10000);
                if ((result > (uint32_t)packedBounds[j]) || ((packedBounds[j] >> 32) > result))
                {
                    goto exit2;
                }
            }
            matchArr.data[matchArr.entries++] = rng;
            exit2:;
        }
    }

    free(sequenceArr.data);
    if (matchArr.entries == 0)
    {
        printf("Invalid sequence. Re-enter your sequence.\n");
        free(matchArr.data);
        return GetRngNoRange();
    }

    const uint32_t returnVal = matchArr.data[0];
    free(matchArr.data);
    return returnVal;
}


#undef minMatches // Only necessary if both GetRng functions are in the same file


NO_INLINE uint32_t GetRngWithinRange(const uint32_t initial, const uint32_t advances)
{
    #define minMatches 1
    const uint32_t matchArrInitialSize = advances; // With a limited range, it's hard to reduce this number accurately

    uint32_t targetRNG = IndexToRng(initial, advances);
    struct DynamicUint8Array sequenceArr = {.data = malloc(sizeof(*sequenceArr.data) * minMatches), .capacity = minMatches, .entries = 0};
    struct DynamicUint32Array matchArr = {.data = malloc(sizeof(*matchArr.data) * matchArrInitialSize), .entries = 0};
    
    GetUserInput(&sequenceArr, minMatches);
    if (sequenceArr.data[0] == 9)
    {
        free(sequenceArr.data);
        free(matchArr.data);
        return GetRngWithinRange(initial, advances);
    }

    size_t i = 0;
    for (; sequenceArr.data[i] == 5; i++); // Removes blanks from start of sequence

    sequenceArr.entries -= i;
    uint64_t packedBounds[sequenceArr.entries]; // Faster than struct
    for (size_t j = 0; j < sequenceArr.entries; j++)
    {
        if (sequenceArr.data[i] == 5)
        {
            packedBounds[j] = 9999;
        }
        else
        {
            const uint32_t lowerBound = sequenceArr.data[i] * 2000;
            const uint32_t upperBound = lowerBound + 1999;
            packedBounds[j] = ((uint64_t)lowerBound << 32 | upperBound);
        }
        i++;
    }

    for (uint32_t rng = initial; rng != targetRNG;)
    {
        uint32_t result = IncrementAndModConst(rng, 10000); // Using rng without copying is faster. RNG needs to be advanced on the first loop so I can't advance it in the for loop header
        if ((result > (uint32_t)packedBounds[0]) || ((packedBounds[0] >> 32) > result))
        {
            goto exit;
        }

        #if minMatches <= 1
            uint32_t rng2 = rng;
        #else
            uint32_t rng2 = IncrementRngState(rng);
            result = RngModConst(rng2, 10000);
            if ((result > (uint32_t)packedBounds[1]) || ((packedBounds[1] >> 32) > result))
            {
                goto exit;
            }
        #endif

        #if minMatches <= 2
            for (size_t i = minMatches; i < sequenceArr.entries; i++)
            {
                const uint32_t result = IncrementAndModConst(rng2, 10000);
                if ((result > (uint32_t)packedBounds[i]) || ((packedBounds[i] >> 32) > result))
                {
                    goto exit; // It's faster to goto to the end of the for loop than to do the loop logic right here
                }
            }
        #else
            size_t i = 2;
            do
            {
                const uint32_t result = IncrementAndModConst(rng2, 10000);
                if ((result > (uint32_t)packedBounds[i]) || ((packedBounds[i] >> 32) > result))
                {
                    goto exit;
                }
            } while (++i < sequenceArr.entries);
        #endif

        matchArr.data[matchArr.entries++] = rng2;
        exit:;
    }

    while (matchArr.entries > 1)
    {
        printf("%u matches\n", matchArr.entries);
        matchArr.capacity = matchArr.entries;
        matchArr.data = realloc(matchArr.data, sizeof(*matchArr.data) * matchArr.capacity);
        matchArr.entries = 0;

        sequenceArr.entries = 0;
        GetUserInput(&sequenceArr, 1);
        if (sequenceArr.data[0] == 9)
        {
            free(sequenceArr.data);
            free(matchArr.data);
            return GetRngWithinRange(initial, advances);
        }

        uint64_t packedBounds[sequenceArr.entries];
        for (size_t i = 0; i < sequenceArr.entries; i++)
        {
            if (sequenceArr.data[i] == 5)
            {
                packedBounds[i] = 9999;
            }
            else
            {
                const uint32_t lowerBound = sequenceArr.data[i] * 2000;
                const uint32_t upperBound = lowerBound + 1999;
                packedBounds[i] = ((uint64_t)lowerBound << 32 | upperBound);
            }
        }

        for (size_t i = 0; i < matchArr.capacity; i++)
        {
            uint32_t rng = IncrementRngState(matchArr.data[i]);
            const uint32_t result = RngModConst(rng, 10000);
            if ((result > (uint32_t)packedBounds[0]) || ((packedBounds[0] >> 32) > result))
            {
                goto exit2;
            }

            for (size_t j = 1; j < sequenceArr.entries; j++)
            {
                const uint32_t result = IncrementAndModConst(rng, 10000);
                if ((result > (uint32_t)packedBounds[j]) || ((packedBounds[j] >> 32) > result))
                {
                    goto exit2;
                }
            }
            matchArr.data[matchArr.entries++] = rng;
            exit2:;
        }
    }

    free(sequenceArr.data);
    if (matchArr.entries == 0)
    {
        printf("Invalid sequence. Re-enter your sequence.\n");
        free(matchArr.data);
        return GetRngWithinRange(initial, advances);
    }

    const uint32_t returnVal = matchArr.data[0];
    free(matchArr.data);
    return returnVal;
}