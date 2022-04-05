#ifndef UTILITIES_H__
#define UTILITIES_H__

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __GNUC__
#define INLINE                                                  __attribute__((always_inline))
#elif defined(_MSC_VER)
#define INLINE                                                  __forceinline
#else
#define INLINE                                                  inline
#endif

#define __FILENAME__                                            (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Not very C99 ish, but hey, nobody ain't perfect
#define __FTD_LOG(_file, _fmt, ...)                             fprintf(_file, "[%s:%d] [%s] " _fmt "%s\n", __FILENAME__, __LINE__, __func__, __VA_ARGS__)
#define inf(...)                                                __FTD_LOG(stdout, "[info] " __VA_ARGS__, "")
#define wrn(...)                                                __FTD_LOG(stdout, "[warning] " __VA_ARGS__, "")
#define err(...)                                                __FTD_LOG(stderr, "[error] " __VA_ARGS__, "")

#define ABS(X)                                                  ((X) < 0 ? (-X) : (X))
#define SWAP(X, Y, T)                                           do { T s = X; X = Y; Y = s; } while(0)
#define UNUSED(x)                                               ((void)(x))


#define DYNAMIC_ARRAY(NAME, T)                                  \
    typedef struct NAME {                                       \
        T *data;                                                \
        size_t used;                                            \
        size_t reserved;                                        \
    } NAME;                                                     \
    static inline void init_##NAME(NAME *v) {                   \
        v->data = NULL; v->used = v->reserved = 0;              \
    }                                                           \
    static inline void reserve_##NAME(NAME *v, size_t n) {      \
        v->reserved += n;                                       \
        v->data = (T *)realloc(v->data, v->reserved*sizeof(T)); \
    }                                                           \
    static inline void shrink_##NAME(NAME *v) {                 \
        v->reserved = v->used;                                  \
        v->data = (T *)realloc(v->data, v->reserved*sizeof(T)); \
    }                                                           \
    static inline void append_##NAME(NAME *v, T *t) {           \
        if(v->reserved <= v->used)                              \
            reserve_##NAME(v, 1);                               \
        v->data[v->used++] = *t;                                \
    }                                                           \
    static inline void free_##NAME(NAME *v) {                   \
        free(v->data);                                          \
        memset(v, 0, sizeof(T));                                \
    }

#endif
