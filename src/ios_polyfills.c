#include <stdint.h>

#ifdef __arm__

// Simple software division and modulo for ARMv7 since Xcode 15 dropped them from libclang_rt

__attribute__((weak)) uint32_t __udivsi3(uint32_t n, uint32_t d) {
    if (d == 0) return 0;
    uint32_t q = 0;
    uint32_t r = 0;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
            q |= (1U << i);
        }
    }
    return q;
}

__attribute__((weak)) uint32_t __umodsi3(uint32_t n, uint32_t d) {
    if (d == 0) return 0;
    uint32_t r = 0;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
        }
    }
    return r;
}

__attribute__((weak)) int32_t __modsi3(int32_t a, int32_t b) {
    int32_t rem = __umodsi3(a < 0 ? -a : a, b < 0 ? -b : b);
    return a < 0 ? -rem : rem;
}

__attribute__((weak)) uint64_t __udivdi3(uint64_t n, uint64_t d) {
    if (d == 0) return 0;
    uint64_t q = 0;
    uint64_t r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
            q |= (1ULL << i);
        }
    }
    return q;
}

__attribute__((weak)) uint64_t __umoddi3(uint64_t n, uint64_t d) {
    if (d == 0) return 0;
    uint64_t r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
        }
    }
    return r;
}

__attribute__((weak)) int64_t __moddi3(int64_t a, int64_t b) {
    int64_t rem = __umoddi3(a < 0 ? -a : a, b < 0 ? -b : b);
    return a < 0 ? -rem : rem;
}

__attribute__((weak)) int32_t __divsi3(int32_t a, int32_t b) {
    int sign = (a < 0) ^ (b < 0);
    uint32_t ua = a < 0 ? -a : a;
    uint32_t ub = b < 0 ? -b : b;
    uint32_t q = __udivsi3(ua, ub);
    return sign ? -q : q;
}

__attribute__((weak)) int64_t __divdi3(int64_t a, int64_t b) {
    int sign = (a < 0) ^ (b < 0);
    uint64_t ua = a < 0 ? -a : a;
    uint64_t ub = b < 0 ? -b : b;
    uint64_t q = __udivdi3(ua, ub);
    return sign ? -q : q;
}

#endif // __arm__

#ifdef __APPLE__
// _availability_version_check was added in iOS 13 for __builtin_available().
// Since we are targeting iOS 9/10, we should return 0 to indicate the features are NOT available.
__attribute__((weak)) uint32_t _availability_version_check(uint32_t count, void *versions) {
    return 0;
}
#endif

__attribute__((weak)) int64_t __fixdfdi(double a) {
    union { double d; uint64_t u; } v = { .d = a };
    int sign = v.u >> 63;
    int exp = ((v.u >> 52) & 0x7FF) - 1023;
    if (exp < 0) return 0;
    if (exp >= 63) return sign ? INT64_MIN : INT64_MAX;
    uint64_t frac = (v.u & ((1ULL << 52) - 1)) | (1ULL << 52);
    uint64_t res = (exp > 52) ? (frac << (exp - 52)) : (frac >> (52 - exp));
    return sign ? -(int64_t)res : (int64_t)res;
}

__attribute__((weak)) uint64_t __fixunsdfdi(double a) {
    union { double d; uint64_t u; } v = { .d = a };
    int sign = v.u >> 63;
    if (sign) return 0;
    int exp = ((v.u >> 52) & 0x7FF) - 1023;
    if (exp < 0) return 0;
    if (exp >= 64) return UINT64_MAX;
    uint64_t frac = (v.u & ((1ULL << 52) - 1)) | (1ULL << 52);
    return (exp > 52) ? (frac << (exp - 52)) : (frac >> (52 - exp));
}

__attribute__((weak)) double __floatundidf(uint64_t a) {
    uint32_t hi = a >> 32;
    uint32_t lo = a & 0xFFFFFFFF;
    return ((double)hi) * 4294967296.0 + (double)lo;
}

__attribute__((weak)) double __floatdidf(int64_t a) {
    if (a < 0) {
        if (a == INT64_MIN) return -9223372036854775808.0;
        return -__floatundidf((uint64_t)(-a));
    }
    return __floatundidf((uint64_t)a);
}

__attribute__((weak)) float __floatdisf(int64_t a) {
    return (float)__floatdidf(a);
}

// ObjC stubs — these are needed because SDL2 references UIPointerStyle (iOS 13.4+)
// which doesn't exist on iOS 9/10. We provide a null class so ObjC messaging nil is safe.
__attribute__((weak)) void objc_msgSend_stret() {}

__attribute__((weak)) void* OBJC_CLASS_$_UIPointerStyle;
__attribute__((weak)) void* OBJC_CLASS_$_UIPointerInteraction;
__attribute__((weak)) void* OBJC_CLASS_$_UIPointerRegion;
__attribute__((weak)) void* OBJC_CLASS_$_GCColor;
__attribute__((weak)) void* OBJC_CLASS_$_GCKeyboard;
__attribute__((weak)) void* OBJC_CLASS_$_GCMouse;

__attribute__((weak)) void* GCInputRightShoulder;
__attribute__((weak)) void* GCInputRightThumbstick;
__attribute__((weak)) void* GCInputRightThumbstickButton;
__attribute__((weak)) void* GCInputRightTrigger;
__attribute__((weak)) void* GCInputXboxPaddleFour;
__attribute__((weak)) void* GCInputXboxPaddleOne;
__attribute__((weak)) void* GCInputXboxPaddleThree;
__attribute__((weak)) void* GCInputXboxPaddleTwo;
__attribute__((weak)) void* GCKeyboardDidConnectNotification;
__attribute__((weak)) void* GCKeyboardDidDisconnectNotification;
__attribute__((weak)) void* GCMouseDidConnectNotification;
__attribute__((weak)) void* GCMouseDidDisconnectNotification;
