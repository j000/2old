#ifndef BOOL_H
#define BOOL_H

#include <stdbool.h>
#ifndef bool
#define bool uint_fast8_t
enum {
	false,
	true
}
#endif

#endif /* BOOL_H */
