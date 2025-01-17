/**
 * @file gosh.h
 * @author Nostalgia3
 * @brief The standard monolithic API for everything in GambleOS.
 * @version 0.1
 * @date 2024-12-28
 * 
 */
#ifndef GOSH_H
#define GOSH_H

#include "types.h"
#include <stdarg.h>

/*********************************** ENUMS  ***********************************/

enum FBPreference {
    RESOLUTION,
    COLORS
};

enum FB_PixelFormat {
    /* A 32-bit RGB (8 bits per color, 1 extra byte) */
    PixelFormat_RGBX_32,
    /* An 8-bit index to a palette (ex. linear 320x200) */
    PixelFormat_LINEAR_I8,
    /* A 4-bit index to a palette (ex. planar 640x480) */
    PixelFormat_PLANAR_I4,
    /* Technically not a pixel format; used for compatibility reasons */
    PixelFormat_TEXT_640x480
};

enum Key {
    K_None, K_Back, K_Tab, K_Enter, K_Return, K_CapsLock, K_Escape, K_Space,
    K_PageUp, K_PageDown, K_End, K_Home, K_Left, K_Up, K_Right, K_Down, K_Insert,
    K_Delete, K_D0, K_D1, K_D2, K_D3, K_D4, K_D5, K_D6, K_D7, K_D8, K_D9, K_A,
    K_B, K_C, K_D, K_E, K_F, K_G, K_H, K_I, K_J, K_K, K_L, K_M, K_N, K_O, K_P,
    K_Q, K_R, K_S, K_T, K_U, K_V, K_W, K_X, K_Y, K_Z, K_F1, K_F2, K_F3, K_F4,
    K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, K_LeftShift, K_RightShift,
    K_LeftCtrl, K_RightCtrl, K_LeftAlt, K_RightAlt, KeyLen
};

enum DeviceType {
    // An unknown device -- this should be used for devices that don't conform
    // to the rest of the generic device types
    DEV_UNKNOWN,
    // A keyboard that can collect FIFO
    DEV_KEYBOARD,
    // A mouse that records absolute and relative position data
    DEV_MOUSE,
    // A network device that can be controlled by writing and reading a buffer
    DEV_NETWORK,
    // An audio device with a buffer that can be written to
    DEV_SPEAKER,
    // An audio device that can be read to
    DEV_MICROPHONE,
    // A storage device that can be read and written to
    DEV_DRIVE,
    // A virtual terminal, with stdio/stdout FIFOs
    DEV_VIRT_TERM,
    // A framebuffer for writing color data
    DEV_FRAMEBUFFER,
    // A driver
    DEV_DRIVER
};

/************************************ MISC ************************************/

void putc_dbg(u8 c);

// Play a sound via the PC speaker (very loud)
void play_sound(u32 nFrequence);
// Stop playing a sound on the PC speaker
void nosound();

// Reset the CPU, basically restarting the machine for other devices
void reset_cpu();

/* Cause a kernel panic, halting all operations on the CPU */
void kpanic();

// The Kernel's unique ID
#define KERNEL_ID 0x8008

/********************************** KEYBOARD **********************************/

struct GlobalSC {
    unsigned char sc;
    int extended;
};

// Push a key to the global FIFO
void pushc(u8 c, int extended);

// Halt the process until a character from the keyboard is detected
struct GlobalSC scanc();

// Get the latest ascii key pressed, or 0 if there was no key
struct GlobalSC getc();


/*********************************** DEVICE ***********************************/

typedef struct _Device {
    /* TRUE is the device is occupied */
    bool is_active;
    /* The generic type of device */
    enum DeviceType type;
    /* The code given to the device */
    u32 code;
    /* The unique ID of the device, given by the kernel */
    u32 id;
    /* The owner ID of the device */
    u32 owner;

    /* The type of this should be inferred based on the device type */
    void* data;
} Device;


typedef struct _KeyboardDeviceData {
    u8 key_statuses[KeyLen/8];
    u8 fifo[32];
    u8 fifo_ind;
    int extended
} KeyboardDeviceData;

typedef struct _MouseDeviceData {
    u8 buttons_down;
    u16 x;
    u16 y;

    void (*update_handler)(Device *dev);
} MouseDeviceData;

typedef struct _FBDeviceData {
    /* The width of the framebuffer */
    u16 w;
    /* The height of the framebuffer */
    u16 h;
    /* Format of pixels (16-color or 256-color, in most cases) */
    enum FB_PixelFormat format;

    void *buffer;

    void (*buf_update)();
} FBDeviceData;

typedef struct _VTDeviceData {
    // Contains all in text
    u8 infifo[32];
    u32 infifo_ind;
    size_t infifo_len;
    
    // Contains all unflushed out text
    u8* text;
    size_t textlen;
    size_t textind;

    // called when the infifo is written to
    void (*write_handler)(Device *vt);
} VTDeviceData;

typedef struct _DriveDeviceData {
    u16 sector_size;
    /* The number of sectors */
    u32 sectors;

    /* Read a sector and write it to addr (sizeof(addr) = sector_size) */
    void (*read_sector)(Device *dev, u32 sector, u8 *addr);
    /* Write a sector, writing data (sizeof(data) = sector_size) */
    void (*write_sector)(Device *dev, u32 sector, u8* data);
} DriveDeviceData;

typedef FBDeviceData FBInfo;

typedef struct _Connection {
    u8 type; // This is zero for PCI and one for USB
    
} Connection;

// Initialize the global device table. Don't call this unless you know what
// you're doing.
void _init_gdevt();

// Get the global device table.
Device* get_gdevt();

// Get the total number of devices (used and unused) in the global device table.
u32 get_gdevt_len();

// Get the number of used devices in the global device table.
u32 get_used_devices();

// Add a device to the global device table, where the id is either
// a process id, or a driver id, then returning a pointer to the device
Device* k_add_dev(u32 owner, enum DeviceType dev, u32 code);

// Get a device by querying the owner and type
Device* k_get_device_by_owner(u32 owner, enum DeviceType type);

// Get a device by the ID and type
Device* k_get_device_by_id(u32 id, enum DeviceType type);

Device* k_get_device_by_type(enum DeviceType type);

/*********************************** DRIVER ***********************************/

typedef struct _Driver {
    // The irqs the driver listens to
    u8 r_active_ints[32];
    // The identifier of the driver (used for creating devices)
    size_t r_id;
    // The null-terminated string for the name of a driver
    u8* name;
    // Generic data
    void *data;
    
    // Called when the driver is created (the system starts, in most cases).
    // The device passed is the driver psuedo-device
    void (*DriverEntry)(Device *dev);
    // Called when a registered interrupt is called
    void (*DriverInt)(Device *dev, u8 int_id);
    // Called when a PCI/USB device is connected
    void (*DriverConnection)(Device *driver, Connection conn);
    // Called when the driver is destroyed (the system closes, in most cases)
    void (*DriverEnd)();
} Driver;

// Load a driver into the system. Returns the ID
u32 load_driver(Driver* driver);

// Unload a driver from the system
void unload_driver(u32 id);

/************************************ INTS ************************************/

// Process interrupts for connected devices that care about them
void k_handle_int(u8 int_id);

// Connect an interrupt to a driver
bool k_register_int(Driver *driver, u8 int_id);

/************************************ PCIE ************************************/

#define PCI_IO_SPACE        (1 << 0)    // R/W
#define PCI_MEM_SPACE       (1 << 1)    // R/W
#define PCI_BUS_MASTER      (1 << 2)    // R/W
#define PCI_SPEC_CYCLES     (1 << 3)    // RO
#define PCI_MEM_INVAL_EN    (1 << 4)    // RO
#define PCI_VGA_PAL_SNOOP   (1 << 5)    // RO
#define PCI_PARITY_ERR_RESP (1 << 6)    // R/W
#define PCI_SERR_ENABLE     (1 << 8)    // R/W
#define PCI_BTB_ENABLE      (1 << 9)    // RO
#define PCI_INT_DISABLE     (1 << 10)   // R/W

#define PCI_MASS_STORAGE_CONTROLLER 0x01
#define PCI_IDE_CONTROLLER          0x01

typedef struct _PCIHeader {
    u16 vendor;
    u16 device;
    u16 command; // I believe this is write-only
    u16 status;
    u8 rev_id;
    u8 prog_if;
    u8 subclass;
    u8 class_code;
    u8 cache_line_size;
    u8 latency_timer;
    u8 header_type;
    u8 BIST;
} PCIHeader;

typedef struct _GenPCIHeader {
    u32 bar0, bar1, bar2, bar3, bar4, bar5;
    u32 cis_pointer;
    u16 subsystem_id;
    u16 subsystem_vendor_id;
    u32 expansion_rom_addr;
    u8  capabilities_pointer;
    u8  max_latency;
    u8  min_grant;
    u8  interrupt_pin;
    u8  interrupt_line;
} GenPCIHeader;

// Read a config register, returning a value
PCIHeader pci_get_header(u8 bus, u8 slot);
GenPCIHeader pci_get_gen_header(u8 bus, u8 slot);

u16 pci_read_config(u8 bus, u8 slot, u8 func, u8 offset);

u32 pci_read_config32(u8 bus, u8 slot, u8 func, u8 offset);
void pci_write_config32(u8 bus, u8 slot, u8 func, u8 offset, u32 data);

u16 scan_pci(u16 class, u16 subclass);

void pci_set_comm(u16 command, u8 bus, u8 slot);
void pci_unset_comm(u16 command, u8 bus, u8 slot);

u16  pci_get_comm(u8 bus, u8 slot);

/******************************** FRAME BUFFER ********************************/

// Framebuffers are the lowest possible abstraction (directly in front of
// drivers) for video graphics. In most cases, programs should use the "desktop"
// library for rendering.

// Returns TRUE if a framebuffer is available, otherwise FALSE. This should be
// used as a check before calling other fb_* functions.
bool    fb_is_available();

// Get the best framebuffer based on preference, returning the gdevt index of
// the framebuffer.
Device* fb_get(enum FBPreference);

// Return the information of the framebuffer
FBInfo  fb_get_info(Device *dev);

// Draw a single pixel at (x, y) with the best available match for the color
// sent
void    fb_draw_pixel(Device* fb, u16 x, u16 y, u32 color);
void    fb_draw_rect(Device* fb, u16 x, u16 y, u16 w, u16 h, u32 color);
void    fb_draw_char(Device *fb, u16 x, u16 y, u8 c, u32 color);
void    fb_clear(Device* fb, u32 color);

/****************************** VIRTUAL TERMINAL ******************************/

Device *stdvt();

// Write a character to the virtual terminal's stdout, without updating the
// virtual terminal
void putcnoup(Device *vt, u8 key);
// Write a string to the virtual terminal's stdout, without updating the virtual
// terminal
void putsnoup(Device *vt, u8 *st);

// Send an update reqeuest to the virtual terminal
void update(Device *vt);

// Write a formatted string to the first virtual terminal without update the
// virtual terminal
__attribute__ ((format (printf, 1, 2))) void kprintfnoup(const char* format, ...);

// Write a formatted string to the first virtual terminal
__attribute__ ((format (printf, 1, 2))) void kprintf(const char* format, ...);

// Write a string to the virtual terminal's stdout
void    puts_vt(Device *vt, u8* str);
// Write a character to the virtual terminal's stdout
void    putc_vt(Device *vt, u8 key);
// Write a character to the virtual terminal's stdin
void    input_vt(Device *vt, u8 key);
// Read a character from the virtual terminal's stdin FIFO
u8      readc_vt(Device *vt);
// Flush the virtual terminal, clearing all stored text data
void    flush_vt(Device *vt);

/*********************************** DRIVES ***********************************/

// Read a sector from a drive drive, and write it to data
bool    read_sector(Device *drive, u32 sector, u8* data);
bool    write_sector(Device *drive, u32 sector, u8* data);

/***************************** VIRTUAL FILESYSTEM *****************************/

typedef i32 fd_t;
typedef i64 off_t;

// scan a drive for any filesystems, adding them to the vfs at path if there are
// valid partitions found. Returns zero if failed
bool    mount_drive(Device *drive, char* path);

// Open a file at a specified path, returning the file descriptor, or an error
// if it failed to open
fd_t    open(char* path);

// Close a file based on the file descriptor
void    close(fd_t fd);

// Read count bytes from fd at offset to buf
int     pread(fd_t fd, void *buf, size_t count, off_t offset);
// Write count bytes to fd at offset from buf
int     pwrite(fd_t fd, void *buf, size_t count, off_t offset);

/************************************ TIME ************************************/

typedef enum TaskType {
    // Task is called repeatedly
    TASK_TYPE_PERIODIC_INTERRUPT,
    // Task is called once after some amount of time
    TASK_TYPE_SINGLE_INTERRUPT
} TaskType_t;

// Waits some amount of milliseconds. This is blocking
void    wait(u32 ms);

// Create a task, with a type and millisecond, returning the ID of the task, or
// -1 if it failed to create a task
int     add_task(void (*handler), TaskType_t type, u32 ms);

// Free the task with the id passed
void    free_task(int id);

#endif