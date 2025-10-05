/* path.c - Path utilities implementation */

#include "path.h"
#include "../mm/heap.h"

/* String utilities */
static size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static char* strcpy(char* dest, const char* src) {
    char* ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

static char* strncpy(char* dest, const char* src, size_t n) {
    char* ret = dest;
    while (n && (*dest++ = *src++)) n--;
    while (n--) *dest++ = '\0';
    return ret;
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/* Normalize path */
char* path_normalize(const char* path) {
    if (!path) return NULL;
    
    size_t len = strlen(path);
    char* result = (char*)kmalloc(len + 2);
    
    if (!result) return NULL;
    
    /* Simple normalization - just copy for now */
    /* TODO: Handle . and .. components */
    /* TODO: Remove duplicate slashes */
    
    strcpy(result, path);
    
    /* Ensure path ends with / if it's just / */
    if (result[0] == '/' && result[1] == '\0') {
        return result;
    }
    
    /* Remove trailing slash if not root */
    len = strlen(result);
    if (len > 1 && result[len - 1] == '/') {
        result[len - 1] = '\0';
    }
    
    return result;
}

/* Get directory name */
char* path_dirname(const char* path) {
    if (!path) return NULL;
    
    size_t len = strlen(path);
    char* result = (char*)kmalloc(len + 1);
    
    if (!result) return NULL;
    
    strcpy(result, path);
    
    /* Find last slash */
    char* last_slash = NULL;
    for (char* p = result; *p; p++) {
        if (*p == '/') {
            last_slash = p;
        }
    }
    
    if (!last_slash) {
        /* No slash found - current directory */
        strcpy(result, ".");
    } else if (last_slash == result) {
        /* Slash at beginning - root */
        result[1] = '\0';
    } else {
        /* Slash somewhere in middle */
        *last_slash = '\0';
    }
    
    return result;
}

/* Get base name */
char* path_basename(const char* path) {
    if (!path) return NULL;
    
    const char* last_slash = path;
    for (const char* p = path; *p; p++) {
        if (*p == '/' && *(p + 1) != '\0') {
            last_slash = p + 1;
        }
    }
    
    size_t len = strlen(last_slash);
    char* result = (char*)kmalloc(len + 1);
    
    if (!result) return NULL;
    
    strcpy(result, last_slash);
    
    /* Remove trailing slash if present */
    len = strlen(result);
    if (len > 1 && result[len - 1] == '/') {
        result[len - 1] = '\0';
    }
    
    return result;
}

/* Join paths */
char* path_join(const char* path1, const char* path2) {
    if (!path1 || !path2) return NULL;
    
    size_t len1 = strlen(path1);
    size_t len2 = strlen(path2);
    char* result = (char*)kmalloc(len1 + len2 + 2);
    
    if (!result) return NULL;
    
    strcpy(result, path1);
    
    /* Add separator if needed */
    if (len1 > 0 && result[len1 - 1] != '/' && path2[0] != '/') {
        result[len1] = '/';
        result[len1 + 1] = '\0';
    }
    
    /* Skip leading slash in path2 if path1 ends with slash */
    const char* p2 = path2;
    if (len1 > 0 && result[len1 - 1] == '/' && path2[0] == '/') {
        p2++;
    }
    
    strcpy(result + strlen(result), p2);
    
    return result;
}

/* Check if path is absolute */
int path_is_absolute(const char* path) {
    if (!path) return 0;
    return path[0] == '/';
}

/* Get extension */
const char* path_extension(const char* path) {
    if (!path) return NULL;
    
    const char* last_dot = NULL;
    const char* last_slash = NULL;
    
    for (const char* p = path; *p; p++) {
        if (*p == '.') {
            last_dot = p;
        } else if (*p == '/') {
            last_slash = p;
            last_dot = NULL; /* Reset dot after slash */
        }
    }
    
    /* Return extension only if dot comes after last slash */
    if (last_dot && (!last_slash || last_dot > last_slash)) {
        return last_dot + 1;
    }
    
    return NULL;
}