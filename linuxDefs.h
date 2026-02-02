#pragma once
#include <stdint.h>

#pragma pack(push, 1)

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Xword;

#define EI_NIDENT 16

struct Elf64_Ehdr {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half    e_type;
    Elf64_Half    e_machine;
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;
    Elf64_Half    e_phentsize;
    Elf64_Half    e_phnum;
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shstrndx;
};

struct Elf64_Phdr {
    Elf64_Word    p_type;
    Elf64_Word    p_flags;
    Elf64_Off     p_offset;
    Elf64_Addr    p_vaddr;
    Elf64_Addr    p_paddr;
    Elf64_Xword   p_filesz;
    Elf64_Xword   p_memsz;
    Elf64_Xword   p_align;
};

struct Elf64_Dyn {
    int64_t d_tag;
    union {
        uint64_t d_val;
        uint64_t d_ptr;
    } d_un;
};

struct Elf64_Sym {
    Elf64_Word    st_name;
    unsigned char st_info;
    unsigned char st_other;
    Elf64_Half    st_shndx;
    Elf64_Addr    st_value;
    Elf64_Xword   st_size;
};

#pragma pack(pop)

// ELF Constants
#define PT_DYNAMIC 2
#define DT_STRTAB  5
#define DT_SYMTAB  6
#define DT_HASH    4
#define DT_GNU_HASH 0x6ffffef5




///////////////////////////////////////////////////////////////////////
#pragma pack(push, 1)

#define MM_PGD_OFFSET          128
#define MM_START_CODE_OFFSET   328
#define MM_END_CODE_OFFSET     336
#define MM_START_STACK_OFFSET  376
#define MM_SAVED_AUXV_OFFSET   416
#define MM_SAVED_AUXV_COUNT    52
#define MM_OWNER_OFFSET        1192
#define MM_EXE_FILE_OFFSET     1208
#define MM_STRUCT_SIZE         1344

typedef struct _MM_STRUCT_RAW {

    /* 0x000 */
    unsigned char _pad0[MM_PGD_OFFSET];

    /* 0x080 */
    void* pgd;                       // CR3 root (most important)

    /* pad to start_code */
    unsigned char _pad1[
        MM_START_CODE_OFFSET -
            MM_PGD_OFFSET -
            sizeof(void*)
    ];

    /* 0x148 */
    unsigned long start_code;
    unsigned long end_code;
    unsigned long start_data;
    unsigned long end_data;
    unsigned long start_brk;
    unsigned long brk;
    unsigned long start_stack;

    /* pad to auxv */
    unsigned char _pad2[
        MM_SAVED_AUXV_OFFSET -
            MM_START_STACK_OFFSET -
            sizeof(unsigned long)
    ];

    /* 0x1A0 */
    unsigned long saved_auxv[MM_SAVED_AUXV_COUNT];

    /* pad to owner */
    unsigned char _pad3[
        MM_OWNER_OFFSET -
            MM_SAVED_AUXV_OFFSET -
            sizeof(unsigned long) * MM_SAVED_AUXV_COUNT
    ];

    /* 0x4A8 */
    void* owner;                     // task_struct *

    /* 0x4B8 */
    void* exe_file;                  // struct file *

} MM_STRUCT_RAW;

#pragma pack(pop)


#pragma pack(push, 1)

typedef struct _LIST_HEAD_RAW {
    void* next;
    void* prev;
} LIST_HEAD_RAW;
#define TASK_TASKS_OFFSET     1296   // from pahole

#define TASK_MM_OFFSET        1376
#define TASK_ACTIVE_MM_OFFSET 1384
#define TASK_PID_OFFSET       1496
#define TASK_TGID_OFFSET      1500
#define TASK_COMM_OFFSET      2032
#define TASK_COMM_LEN         16

typedef struct _TASK_STRUCT_RAW {

    /* 0x000 */
    unsigned char _pad0[TASK_TASKS_OFFSET];

    /* 0x510 */
    LIST_HEAD_RAW tasks;        // global task list linkage

    /* pad up to mm */
    unsigned char _pad1[
        TASK_MM_OFFSET -
            TASK_TASKS_OFFSET -
            sizeof(LIST_HEAD_RAW)
    ];

    /* 0x560 */
    struct _MM_STRUCT_RAW* mm;

    /* 0x568 */
    struct _MM_STRUCT_RAW* active_mm;

    /* pad up to pid */
    unsigned char _pad2[
        TASK_PID_OFFSET -
            TASK_ACTIVE_MM_OFFSET -
            sizeof(void*)
    ];

    /* 0x5D8 */
    int pid;

    /* 0x5DC */
    int tgid;

    /* pad up to comm */
    unsigned char _pad3[
        TASK_COMM_OFFSET -
            TASK_PID_OFFSET -
            sizeof(int) * 2
    ];

    /* 0x7F0 */
    char comm[TASK_COMM_LEN];

} TASK_STRUCT_RAW;

#pragma pack(pop)


#pragma pack(push, 1)

typedef struct _LINUX_BINPRM_RAW {

    /* 0x00 */
    void* vma;              // struct vm_area_struct *
    unsigned long  vma_pages;

    /* 0x10 */
    void* mm;               // struct mm_struct *
    unsigned long  p;
    unsigned long  argmin;

    /* 0x28 */
    unsigned int   have_execfd : 1;
    unsigned int   execfd_creds : 1;
    unsigned int   secureexec : 1;
    unsigned int   point_of_no_return : 1;
    unsigned int   _pad_bits : 28;

    /* 0x30 */
    void* executable;       // struct file *
    void* interpreter;      // struct file *

    /* 0x40 */
    void* file;              // struct file *
    void* cred;              // struct cred *

    /* 0x50 */
    int            unsafe;
    unsigned int   per_clear;
    int            argc;
    int            envc;

    /* 0x60 */
    const char* filename;          // exec path
    const char* interp;            // PT_INTERP path (linker64)
    const char* fdpath;

    /* 0x78 */
    unsigned int   interp_flags;
    int            execfd;

    /* 0x80 */
    unsigned long  loader;
    unsigned long  exec;
    struct {
        unsigned long rlim_cur;
        unsigned long rlim_max;
    } rlim_stack;

    /* 0xA0 */
    char           buf[256];          // first bytes of ELF

} LINUX_BINPRM_RAW;

#pragma pack(pop)

#pragma pack(push, 1)

typedef struct _PATH_RAW {
    void* mnt;       // struct vfsmount *
    void* dentry;    // struct dentry *
} PATH_RAW;

typedef struct _FILE_RAW {

    /* 0x00 */
    union {
        void* f_llist;        // struct llist_node *
        void* f_rcuhead;      // struct callback_head *
        unsigned int   f_iocb_flags;
    };

    /* 0x10 */
    unsigned int   f_lock;
    unsigned int   f_mode;
    long long      f_count;

    /* 0x20 */
    unsigned char  f_pos_lock[48];    // struct mutex

    /* 0x50 */
    long long      f_pos;
    unsigned int   f_flags;

    /* padding */
    unsigned int   _pad0;

    /* 0x60 */
    unsigned char  f_owner[32];       // struct fown_struct

    /* 0x80 */
    void* f_cred;             // struct cred *
    unsigned char  f_ra[32];           // struct file_ra_state

    /* 0xA8 */
    PATH_RAW       f_path;             // struct path

    /* 0xB8 */
    void* f_inode;             // struct inode *

    /* 0xC0 */
    void* f_op;                // struct file_operations *
    unsigned long long f_version;
    void* f_security;
    void* private_data;
    void* f_ep;
    void* f_mapping;

    /* 0xF0 */
    unsigned int   f_wb_err;
    unsigned int   f_sb_err;

    /* 0xF8 */
    unsigned long long android_kabi_reserved1;
    unsigned long long android_kabi_reserved2;

} FILE_RAW;

#pragma pack(pop)

#pragma pack(push, 1)

/* minimal qstr */
typedef struct _QSTR_RAW {
    unsigned int hash;  // hash
    unsigned int len;   // length
    const char* name;   // pointer to name
} QSTR_RAW;

/* minimal hlist */
typedef struct _HLIST_NODE_RAW {
    void* next;
    void** pprev;
} HLIST_NODE_RAW;

/* minimal hlist_bl_node (2 pointers) */
typedef struct _HLIST_BL_NODE_RAW {
    void* next;
    void** pprev;
} HLIST_BL_NODE_RAW;

/* minimal lockref */
typedef struct _LOCKREF_RAW {
    union {
        unsigned long long lock_count;
        struct {
            unsigned int lock;
            int count;
        };
    };
} LOCKREF_RAW;

typedef struct _DENTRY_RAW {

    /* 0x00 */
    unsigned int d_flags;
    unsigned int d_seq;

    /* 0x08 */
    HLIST_BL_NODE_RAW d_hash;

    /* 0x18 */
    struct _DENTRY_RAW* d_parent;

    /* 0x20 */
    QSTR_RAW d_name;

    /* 0x30 */
    void* d_inode;

    /* 0x38 */
    unsigned char d_iname[32];

    /* 0x58 */
    LOCKREF_RAW d_lockref;

    /* 0x60 */
    void* d_op;
    void* d_sb;

    /* 0x70 */
    unsigned long long d_time;
    void* d_fsdata;

    /* 0x80 */
    union {
        LIST_HEAD_RAW d_lru;
        void* d_wait;
    };

    /* 0x90 */
    LIST_HEAD_RAW d_child;

    /* 0xA0 */
    LIST_HEAD_RAW d_subdirs;

    /* 0xB0 */
    union {
        HLIST_NODE_RAW d_alias;
        HLIST_BL_NODE_RAW d_in_lookup_hash;
        void* d_rcu;
    } d_u;

    /* 0xC0 */
    unsigned long long android_kabi_reserved1;
    unsigned long long android_kabi_reserved2;

} DENTRY_RAW;

#pragma pack(pop)

#pragma pack(push, 1)

typedef struct _FILENAME_RAW {

    /* 0x00 */
    const char* name;      // kernel pointer to filename string

    /* 0x08 */
    const char* uptr;      // userspace pointer (may be NULL)

    /* 0x10 */
    int refcnt;            // atomic_t (4 bytes)

    /* 0x14 */
    unsigned int _pad0;    // hole (alignment padding)

    /* 0x18 */
    void* aname;           // struct audit_names *

    /* 0x20 */
    char iname[0];         // flexible array (inline short name)

} FILENAME_RAW;

#pragma pack(pop)

typedef struct {
    /** A bitmask of `ANDROID_DLEXT_` enum values. */
    uint64_t flags;
    /** Used by `ANDROID_DLEXT_RESERVED_ADDRESS` and `ANDROID_DLEXT_RESERVED_ADDRESS_HINT`. */
    void* reserved_addr;
    /** Used by `ANDROID_DLEXT_RESERVED_ADDRESS` and `ANDROID_DLEXT_RESERVED_ADDRESS_HINT`. */
    size_t  reserved_size;
    /** Used by `ANDROID_DLEXT_WRITE_RELRO` and `ANDROID_DLEXT_USE_RELRO`. */
    int     relro_fd;
    /** Used by `ANDROID_DLEXT_USE_LIBRARY_FD`. */
    int     library_fd;
    /** Used by `ANDROID_DLEXT_USE_LIBRARY_FD_OFFSET` */
    UINT64 library_fd_offset;
} android_dlextinfo;



struct callback_head {
    uint64_t next;
    uint64_t func;
};

struct mm_struct;
struct vma_lock;

struct rb_node {
    uint64_t rb_parent_color;
    uint64_t rb_right;
    uint64_t rb_left;
};

struct list_head {
    uint64_t next;
    uint64_t prev;
};
#pragma pack(push, 1)

/*
 * RAW vm_area_struct
 * Kernel: Android x86_64
 * Size: 200 bytes
 */

typedef struct _VM_AREA_STRUCT_RAW {

    /* 0x000 */
    unsigned long vm_start;        // union start
    unsigned long vm_end;

    /* 0x010 */
    void* vm_mm;           // struct mm_struct *

    /* 0x018 */
    unsigned long vm_page_prot;    // pgprot_t

    /* 0x020 */
    unsigned long vm_flags;        // vm_flags_t

    /* 0x028 */
    unsigned char detached;        // bool

    /* padding */
    unsigned char _pad0[3];

    /* 0x02C */
    int           vm_lock_seq;

    /* 0x030 */
    void* vm_lock;         // struct vma_lock *

    /* shared.rb (rb_node) */
    /* 0x038 */
    unsigned long rb_parent_color;
    unsigned long rb_right;
    unsigned long rb_left;

    /* 0x050 */
    unsigned long rb_subtree_last;

    /* 0x058 */
    void* anon_vma_chain_next;
    void* anon_vma_chain_prev;

    /* 0x068 */
    void* anon_vma;         // struct anon_vma *

    /* 0x070 */
    void* vm_ops;           // struct vm_operations_struct *

    /* 0x078 */
    unsigned long vm_pgoff;

    /* 0x080 */
    void* vm_file;          // struct file *

    /* 0x088 */
    void* vm_private_data;

    /* 0x090 */
    void* anon_name;        // struct anon_vma_name *

    /* 0x098 */
    long long     swap_readahead_info;

    /* 0x0A0 */
    unsigned long vm_userfaultfd_ctx;

    /* 0x0A8 */
    unsigned long android_kabi_reserved1;
    unsigned long android_kabi_reserved2;
    unsigned long android_kabi_reserved3;

    /* 0x0C0 */
    unsigned long android_kabi_reserved4;

} VM_AREA_STRUCT_RAW;

#pragma pack(pop)
#pragma pack(push, 1)

/*
 * RAW vm_fault
 * Kernel: Android x86_64
 * Size: 112 bytes
 */

typedef struct _VM_FAULT_RAW {

    /* --- anonymous union --- */

    /* 0x00 */
    void* vma;           // struct vm_area_struct *

    /* 0x08 */
    unsigned int  gfp_mask;      // gfp_t

    /* padding (hole) */
    unsigned int  _pad0;         // aligns pgoff

    /* 0x10 */
    unsigned long pgoff;         // page offset within mapping

    /* 0x18 */
    unsigned long address;       // faulting virtual address

    /* 0x20 */
    unsigned long real_address;  // arch-specific (often same as address)

    /* --- end union --- */

    /* 0x28 */
    unsigned int  flags;         // enum fault_flag

    /* padding (hole) */
    unsigned int  _pad1;

    /* 0x30 */
    void* pmd;            // pmd_t *

    /* 0x38 */
    void* pud;            // pud_t *

    /* 0x40 */
    unsigned long orig_pte_or_pmd; // union { pte_t orig_pte; pmd_t orig_pmd; }

    /* 0x48 */
    void* cow_page;       // struct page *

    /* 0x50 */
    void* page;           // struct page *

    /* 0x58 */
    void* pte;            // pte_t *

    /* 0x60 */
    void* ptl;            // spinlock_t *

    /* 0x68 */
    void* prealloc_pte;   // pgtable_t

} VM_FAULT_RAW;

#pragma pack(pop)
#pragma pack(push, 1)

/*
 * RAW struct page
 * Kernel: Android x86_64
 * Size: 64 bytes
 */

typedef struct _PAGE_RAW {

    /* 0x00 */
    unsigned long flags;          // page flags

    /* 0x08 */
    union {
        /* Page cache / anon / buddy / lru */
        struct {
            unsigned long lru_next;    // list_head.next
            unsigned long lru_prev;    // list_head.prev
            void* mapping;     // struct address_space *
            unsigned long index;       // index / share
            unsigned long _private;     // private data
        };

        /* Page pool */
        struct {
            unsigned long pp_magic;
            void* pp;           // struct page_pool *
            unsigned long _pp_mapping_pad;
            unsigned long dma_addr;
            unsigned long dma_addr_upper_or_frag;
        };

        /* Compound page */
        struct {
            unsigned long compound_head;
        };

        /* Device page */
        struct {
            void* pgmap;        // struct dev_pagemap *
            void* zone_device_data;
        };

        /* RCU / callback */
        struct {
            unsigned long cb_next;
            unsigned long cb_func;
        };
    };

    /* 0x30 */
    union {
        int             _mapcount;     // atomic_t
        unsigned int    page_type;
    };

    /* 0x34 */
    int               _refcount;      // atomic_t

    /* 0x38 */
    unsigned long     memcg_data;

} PAGE_RAW;

#pragma pack(pop)