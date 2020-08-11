#ifndef WORKSHARE_TEST_H_
#define WORKSHARE_TEST_H_

#include <stdio.h>
#include <stdlib.h>
#define WS_ASSERT(a, b, t) do { if(a != b) { fprintf(stderr, "Error: returned "t", expected "t"(%s:%d)\n", a, b, __FILE__, __LINE__); abort();} } while(0)
#define WS_ASSERT_NOT_NULL(a) do { if(a == NULL) { fprintf(stderr, "Error: "#a" is NULL(%s:%d)\n", __FILE__, __LINE__); abort();} } while(0)

#define WS_UNUSED(a) (void)(a)
#define WS_SUCCESS() do {printf("SUCCESS !\n"); }while(0)
#endif