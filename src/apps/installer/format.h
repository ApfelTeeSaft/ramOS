/* format.h - Filesystem formatting functions */

#ifndef FORMAT_H
#define FORMAT_H

/* Format partition as EXT4 */
int format_ext4(const char* device, const char* label);

#endif /* FORMAT_H */