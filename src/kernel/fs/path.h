/* path.h - Path resolution and manipulation utilities */

#ifndef PATH_H
#define PATH_H

/* Normalize path (resolve . and .., remove duplicate slashes) */
char* path_normalize(const char* path);

/* Get directory name from path */
char* path_dirname(const char* path);

/* Get file name from path */
char* path_basename(const char* path);

/* Join two paths */
char* path_join(const char* path1, const char* path2);

/* Check if path is absolute */
int path_is_absolute(const char* path);

/* Get path extension */
const char* path_extension(const char* path);

#endif /* PATH_H */