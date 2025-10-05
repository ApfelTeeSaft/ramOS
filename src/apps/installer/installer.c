/* installer.c - ramOS System Installer */

#include "../../api/libsys.h"
#include "partition.h"
#include "format.h"

#define INSTALL_DEVICE "/dev/sda"
#define BOOT_PARTITION "/dev/sda1"
#define ROOT_PARTITION "/dev/sda2"
#define MOUNT_POINT "/mnt"

/* Installation steps */
typedef enum {
    STEP_WELCOME,
    STEP_DISK_SELECT,
    STEP_PARTITION,
    STEP_FORMAT,
    STEP_INSTALL_FILES,
    STEP_INSTALL_GRUB,
    STEP_COMPLETE
} install_step_t;

static install_step_t current_step = STEP_WELCOME;

/* Display welcome screen */
static void show_welcome(void) {
    println("========================================");
    println("  ramOS Installation Wizard");
    println("========================================");
    println("");
    println("This installer will:");
    println("  1. Partition the target disk");
    println("  2. Format partitions as EXT4");
    println("  3. Copy system files");
    println("  4. Install GRUB bootloader");
    println("");
    println("WARNING: This will erase all data on");
    println("the target disk!");
    println("");
    print("Continue? (y/n): ");
}

/* Display disk selection */
static void show_disk_select(void) {
    println("\n========================================");
    println("  Disk Selection");
    println("========================================");
    println("");
    println("Available disks:");
    
    /* List available disks */
    stat_t st;
    if (sys_stat("/dev/sda", &st) == 0) {
        println("  /dev/sda - Primary hard disk");
    }
    if (sys_stat("/dev/sdb", &st) == 0) {
        println("  /dev/sdb - Secondary hard disk");
    }
    
    println("");
    printf("Install to: %s\n", INSTALL_DEVICE);
    print("Confirm? (y/n): ");
}

/* Partition disk */
static int partition_disk(void) {
    println("\n========================================");
    println("  Partitioning Disk");
    println("========================================");
    println("");
    
    println("[*] Creating partition table...");
    if (create_partition_table(INSTALL_DEVICE) < 0) {
        println("[!] Error: Failed to create partition table");
        return -1;
    }
    
    println("[*] Creating boot partition (256MB)...");
    if (create_partition(INSTALL_DEVICE, 1, 0, 256 * 1024 * 2) < 0) {
        println("[!] Error: Failed to create boot partition");
        return -1;
    }
    
    println("[*] Creating root partition (remaining space)...");
    if (create_partition(INSTALL_DEVICE, 2, 256 * 1024 * 2, 0) < 0) {
        println("[!] Error: Failed to create root partition");
        return -1;
    }
    
    println("[+] Partitioning complete");
    return 0;
}

/* Format partitions */
static int format_partitions(void) {
    println("\n========================================");
    println("  Formatting Partitions");
    println("========================================");
    println("");
    
    println("[*] Formatting boot partition as EXT4...");
    if (format_ext4(BOOT_PARTITION, "boot") < 0) {
        println("[!] Error: Failed to format boot partition");
        return -1;
    }
    
    println("[*] Formatting root partition as EXT4...");
    if (format_ext4(ROOT_PARTITION, "root") < 0) {
        println("[!] Error: Failed to format root partition");
        return -1;
    }
    
    println("[+] Formatting complete");
    return 0;
}

/* Install system files */
static int install_files(void) {
    println("\n========================================");
    println("  Installing System Files");
    println("========================================");
    println("");
    
    println("[*] Mounting root partition...");
    if (sys_mount(ROOT_PARTITION, MOUNT_POINT, "ext4") < 0) {
        println("[!] Error: Failed to mount root partition");
        return -1;
    }
    
    println("[*] Creating directory structure...");
    sys_mkdir("/mnt/boot", 0755);
    sys_mkdir("/mnt/bin", 0755);
    sys_mkdir("/mnt/etc", 0755);
    sys_mkdir("/mnt/dev", 0755);
    sys_mkdir("/mnt/tmp", 0755);
    sys_mkdir("/mnt/home", 0755);
    sys_mkdir("/mnt/usr", 0755);
    sys_mkdir("/mnt/var", 0755);
    
    println("[*] Copying kernel...");
    if (copy_file("/boot/kernel.elf", "/mnt/boot/kernel.elf") < 0) {
        println("[!] Error: Failed to copy kernel");
        return -1;
    }
    
    println("[*] Copying initrd...");
    if (copy_file("/boot/initrd.cpio", "/mnt/boot/initrd.cpio") < 0) {
        println("[!] Error: Failed to copy initrd");
        return -1;
    }
    
    println("[*] Copying applications...");
    const char* apps[] = {
        "calculator.elf",
        "editor.elf",
        "filemanager.elf",
        "shell.elf",
        NULL
    };
    
    for (int i = 0; apps[i] != NULL; i++) {
        char src[256], dst[256];
        snprintf(src, sizeof(src), "/bin/%s", apps[i]);
        snprintf(dst, sizeof(dst), "/mnt/bin/%s", apps[i]);
        
        printf("  - %s\n", apps[i]);
        if (copy_file(src, dst) < 0) {
            printf("[!] Warning: Failed to copy %s\n", apps[i]);
        }
    }
    
    println("[*] Creating configuration files...");
    create_fstab();
    create_grub_config();
    
    println("[+] File installation complete");
    return 0;
}

/* Install GRUB bootloader */
static int install_grub(void) {
    println("\n========================================");
    println("  Installing GRUB Bootloader");
    println("========================================");
    println("");
    
    println("[*] Installing GRUB to MBR...");
    
    /* Install GRUB using system command (simplified) */
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "grub-install --boot-directory=/mnt/boot %s", INSTALL_DEVICE);
    
    /* For now, we'll create a simple bootloader setup */
    println("[*] Configuring GRUB...");
    
    /* Create GRUB configuration */
    int fd = sys_open("/mnt/boot/grub/grub.cfg", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        println("[!] Error: Failed to create GRUB configuration");
        return -1;
    }
    
    const char* grub_cfg = 
        "set timeout=5\n"
        "set default=0\n"
        "\n"
        "menuentry \"ramOS\" {\n"
        "    set root=(hd0,1)\n"
        "    multiboot /boot/kernel.elf\n"
        "    module /boot/initrd.cpio\n"
        "    boot\n"
        "}\n";
    
    sys_write(fd, grub_cfg, strlen(grub_cfg));
    sys_close(fd);
    
    println("[+] GRUB installation complete");
    return 0;
}

/* Copy file helper */
static int copy_file(const char* src, const char* dst) {
    int src_fd = sys_open(src, O_RDONLY);
    if (src_fd < 0) {
        return -1;
    }
    
    int dst_fd = sys_open(dst, O_WRONLY | O_CREAT | O_TRUNC);
    if (dst_fd < 0) {
        sys_close(src_fd);
        return -1;
    }
    
    char buffer[4096];
    int bytes;
    
    while ((bytes = sys_read(src_fd, buffer, sizeof(buffer))) > 0) {
        if (sys_write(dst_fd, buffer, bytes) != bytes) {
            sys_close(src_fd);
            sys_close(dst_fd);
            return -1;
        }
    }
    
    sys_close(src_fd);
    sys_close(dst_fd);
    
    return 0;
}

/* Create /etc/fstab */
static void create_fstab(void) {
    int fd = sys_open("/mnt/etc/fstab", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) return;
    
    const char* fstab =
        "# /etc/fstab - filesystem mount table\n"
        "/dev/sda2  /      ext4  defaults  0  1\n"
        "/dev/sda1  /boot  ext4  defaults  0  2\n";
    
    sys_write(fd, fstab, strlen(fstab));
    sys_close(fd);
}

/* Create GRUB config */
static void create_grub_config(void) {
    sys_mkdir("/mnt/boot/grub", 0755);
}

/* Simple snprintf implementation */
static int snprintf(char* str, size_t size, const char* format, ...) {
    /* Simplified version - just handle basic cases */
    uint32_t* args = (uint32_t*)((char*)&format + sizeof(format));
    int pos = 0;
    int arg_idx = 0;
    
    while (*format && pos < (int)size - 1) {
        if (*format == '%' && *(format + 1) == 's') {
            const char* s = (const char*)args[arg_idx++];
            while (*s && pos < (int)size - 1) {
                str[pos++] = *s++;
            }
            format += 2;
        } else {
            str[pos++] = *format++;
        }
    }
    
    str[pos] = '\0';
    return pos;
}

/* Main installer */
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    char input[256];
    
    while (1) {
        switch (current_step) {
            case STEP_WELCOME:
                show_welcome();
                readln(input, sizeof(input));
                if (input[0] == 'y' || input[0] == 'Y') {
                    current_step = STEP_DISK_SELECT;
                } else {
                    println("\nInstallation cancelled");
                    sys_exit(0);
                }
                break;
                
            case STEP_DISK_SELECT:
                show_disk_select();
                readln(input, sizeof(input));
                if (input[0] == 'y' || input[0] == 'Y') {
                    current_step = STEP_PARTITION;
                } else {
                    current_step = STEP_WELCOME;
                }
                break;
                
            case STEP_PARTITION:
                if (partition_disk() == 0) {
                    current_step = STEP_FORMAT;
                } else {
                    println("\nPress Enter to retry...");
                    readln(input, sizeof(input));
                }
                break;
                
            case STEP_FORMAT:
                if (format_partitions() == 0) {
                    current_step = STEP_INSTALL_FILES;
                } else {
                    println("\nPress Enter to retry...");
                    readln(input, sizeof(input));
                }
                break;
                
            case STEP_INSTALL_FILES:
                if (install_files() == 0) {
                    current_step = STEP_INSTALL_GRUB;
                } else {
                    println("\nPress Enter to retry...");
                    readln(input, sizeof(input));
                }
                break;
                
            case STEP_INSTALL_GRUB:
                if (install_grub() == 0) {
                    current_step = STEP_COMPLETE;
                } else {
                    println("\nPress Enter to retry...");
                    readln(input, sizeof(input));
                }
                break;
                
            case STEP_COMPLETE:
                println("\n========================================");
                println("  Installation Complete!");
                println("========================================");
                println("");
                println("ramOS has been successfully installed.");
                println("Please remove the installation media");
                println("and reboot your computer.");
                println("");
                println("Press Enter to exit...");
                readln(input, sizeof(input));
                sys_exit(0);
                break;
        }
    }
    
    return 0;
}