#ifndef _DOCA_UTILS_H_
#define _DOCA_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <doca_error.h>
#include <doca_types.h>

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))	/* Return the minimum value between X and Y */
#endif

#ifndef MAX
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))	/* Return the maximum value between X and Y */
#endif

/*
 * Prints DOCA SDK and runtime versions
 *
 * @param [in]: unused
 * @doca_config [in]: unused
 * @return: the function exit with EXIT_SUCCESS
 */
doca_error_t sdk_version_callback(void *param, void *doca_config);

/*
 * Read the entire content of a file into a buffer
 *
 * @path [in]: file path
 * @out_bytes [out]: file data buffer
 * @out_bytes_len [out]: file length
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t read_file(char const *path, char **out_bytes, size_t *out_bytes_len);

#endif  /* _DOCA_UTILS_H_ */