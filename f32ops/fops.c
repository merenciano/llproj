#include <stdio.h>
#include <stdint.h>

inline static uint32_t S(uint32_t a)
{
    return ((uint32_t)a >> 31) & 1;
}

inline static uint32_t E(uint32_t a)
{
    return ((uint32_t)a >> 23) & ((1 << 8) - 1);
}

inline static uint32_t M(uint32_t a)
{
    return (uint32_t)a & ((1 << 23) - 1);
}

static uint32_t f32mul(uint32_t a, uint32_t b)
{
    uint32_t ia = *(uint32_t*)&a;
    uint32_t ib = *(uint32_t*)&b;

    uint32_t rs = S(ia) ^ S(ib);
    uint32_t re = E(ia) + E(ib) - 127;

    uint32_t result = 0;

    uint64_t rm = (uint64_t)(M(ia) | (1 << 23)) * (uint64_t)(M(ib) | (1 << 23));
    if (rm & ((uint64_t)1 << 47))
    {
        printf("Adjustment\n");
        rm = rm >> 1;
        re++;
    }
    rm = rm >> 23;
    rm = rm & ((1<<23) - 1);

    result = (uint32_t)rm;
    result |= re << 23;
    result |= rs << 31;

    return result;
}

static float wf32mul(float a, float b)
{
    uint32_t ia = *(uint32_t*)&a;
    uint32_t ib = *(uint32_t*)&b;
    uint32_t ires = f32mul(ia, ib);
    return *(float*)&ires;
}

int main(int argc, char **argv)
{
    float a1 = -0.3f;
    float a2 = 99.999f;
    float a3 = 1.3034f;

    float b1 = 2.3f;
    float b2 = -99.999f;
    float b3 = 0.0f;

    printf("Result 1: %f * %f = %f\n", a1, b1, a1*b1);
    printf("My Result 1: %f * %f = %f\n", a1, b1, wf32mul(a1, b1));

    printf("Result 2: %f * %f = %f\n", a2, b2, a2*b2);
    printf("My Result 2: %f * %f = %f\n", a2, b2, wf32mul(a2, b2));

    printf("Result 3: %f * %f = %f\n", a3, b3, a3*b3);
    printf("My Result 3: %f * %f = %f\n", a3, b3, wf32mul(a3, b3));

    return 0;
}
