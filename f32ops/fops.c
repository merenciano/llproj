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

static uint32_t f32add(uint32_t ia, uint32_t ib)
{
    if ((ia & ((uint32_t)(1 << 31) - 1)) < (ib & ((uint32_t)(1 << 31) - 1)))
    {
        uint32_t tmp = ia;
        ia = ib;
        ib = tmp;
    }

    uint32_t rs = S(ia);
    uint32_t re = E(ia);

    uint32_t exp_dif = re - E(ib);
    uint32_t ma = M(ia);
    uint32_t mb = M(ib);
    uint32_t rm = 0;

    ma |= (1 << 23);
    mb |= (1 << 23);
    mb >>= exp_dif;

    if (S(ia) == S(ib))
    {
        rm = ma + mb;
    }
    else
    {
        rm = ma - mb;
    }

    // Normalize for addition
    if (rm & (1 << 24))
    {
        rm = rm >> 1;
        re++;
    }

    // Normalize for subtraction
    while (!(rm & (1 << 23)))
    {
        rm = rm << 1;
        re--;
    }
    
    uint32_t result = rm & ((1 << 23) - 1);
    result |= re << 23;
    result |= rs << 31;

    return result;
}

static float wf32add(float a, float b)
{
    uint32_t ia = *(uint32_t*)&a;
    uint32_t ib = *(uint32_t*)&b;
    uint32_t ires = f32add(ia, ib);
    return *(float*)&ires;
}

static uint32_t f32sub(uint32_t ia, uint32_t ib)
{
    if (ib & (1<<31))
    {
        ib &= ((uint32_t)(1<<31)-1);
    }
    else
    {
        ib |= (1<<31);
    }
    return f32add(ia, ib);
}

static float wf32sub(float a, float b)
{
    uint32_t ia = *(uint32_t*)&a;
    uint32_t ib = *(uint32_t*)&b;
    uint32_t ires = f32sub(ia, ib);
    return *(float*)&ires;
}

static uint32_t f32mul(uint32_t ia, uint32_t ib)
{
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
    float a1 = 2.244f;
    float a2 = 2.54f;
    float a3 = 42.3034f;

    float b1 = 3.48f;
    float b2 = -22.2f;
    float b3 = 12.1f;

    printf("Result 1: %f - %f = %f\n", a1, b1, a1-b1);
    printf("My Result 1: %f - %f = %f\n", a1, b1, wf32sub(a1, b1));

    printf("Result 2: %f - %f = %f\n", a2, b2, a2-b2);
    printf("My Result 2: %f - %f = %f\n", a2, b2, wf32sub(a2, b2));

    printf("Result 3: %f - %f = %f\n", a3, b3, a3-b3);
    printf("My Result 3: %f - %f = %f\n", a3, b3, wf32sub(a3, b3));

    return 0;
}
