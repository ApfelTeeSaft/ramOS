ramOS initrd README
===================

This is the initial RAM disk for ramOS. All files here are loaded into
memory at boot time and made available through a simple read-only filesystem.

Files in this initrd:
- README.txt - This file
- motd.txt   - Message of the day

You can list these files with the 'ls' command and view them with 'cat'.

To add more files to the initrd:
1. Place files in the initrd_root/ directory
2. Run 'make initrd iso' to rebuild
3. The new files will be available after booting

ramOS is a minimal hobby operating system for learning purposes.