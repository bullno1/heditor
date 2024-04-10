#ifndef HEDITOR_UTILS_H
#define HEDITOR_UTILS_H

#define HED_PRIVATE static inline
#define HED_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define HED_MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define HED_CONTAINER_OF(PTR, TYPE, MEMBER) \
    (TYPE*)((char*)(PTR) - offsetof(TYPE, MEMBER))

#endif
