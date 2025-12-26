global _start

section .text

; Checks whether the last syscall executed correctly.
; If not, sets the value of the R9 register (return value) to 1.

%macro check_result 0
    cmp     rax, 0xfffffffffffff000
    jbe     %%skip_incorrect
    seta    r9b
    %%skip_incorrect:
%endmacro

; Syscall numbers for selected Linux system functions.

SYS_OPEN    equ 2
SYS_CLOSE   equ 3
SYS_FSTAT   equ 5
SYS_MMAP    equ 9
SYS_MUNMAP  equ 11
SYS_EXIT    equ 60

; Other constants.

O_RDWR      equ 2
S_IFMT      equ 0xf000
S_IFREG     equ 0x8000
PROT_READ   equ 1
PROT_WRITE  equ 2
MAP_SHARED  equ 1

; Reverses the file whose name is given as a program argument.
;
; If any of the following occurs:
; - no or multiple arguments are given
; - the given file is not a regular file or a symbolic link
; - any of the system function fails
; the program returns 1. Otherwise, 0 is returned.
;
; In case of an error, the file integrity is not guaranteed.

_start:

    xor     r9, r9                  ; set return value to 0

    cmp     qword [rsp], 2          ; check if exactly 1 argument is given
    setne   r9b                     ; if not, set return value to 1
    jne     exit                    ; and exit

; Opens the file.
; int open(const char *__file, int __oflag, ...)

    mov     rax, SYS_OPEN 
    mov     rdi, [rsp + 16]         ; filename (const char *__file)
    mov     rsi, O_RDWR             ; read/write permissions (int __oflag)
    xor     rdx, rdx                ; NULL

    syscall

    check_result                    ; check whether open() executed correctly
    ja      exit                    ; if not, exit
    
    mov     r8, rax                 ; file descriptor returned by open()

; Retrieves information about the file and stores it in a buffer.
; int fstat(int __fd, struct stat *__buf)

    mov     rax, SYS_FSTAT
    mov     rdi, r8                 ; file descriptor (int __fd)
    lea     rsi, [rsp - 128]        ; struct stat *__buf (stored in red zone)

    syscall
    
    check_result                    ; check whether fstat() executed correctly
    ja     close                    ; if not, close the file and exit
    
; Checks whether the file is a regular file.
; (symbolic links are already resolved by fstat())

    mov     rdi, [rsi + 24]         ; file type and mode (= statbuf.st_mode)
    and     rdi, S_IFMT             ; bit mask for the file type bit field

    cmp     rdi, S_IFREG            ; check whether the file is a regular file
    setne   r9b                     ; if not, set return value to 1
    jne     close                   ; then close the file and exit

; Checks whether the file is empty.

    mov     rsi, [rsi + 48]         ; file size (= statbuf.st_size)
    
    test    rsi, rsi                ; check whether the file is empty
    je      close                   ; if so, close the file and exit

; Maps the file into the virtual address space.
; void *mmap(void *__addr, size_t __len, int __prot, int __flags, int __fd, 
;            off_t __offset)
    
    mov     rax, SYS_MMAP           
    xor     rdi, rdi                ; let kernel choose (= NULL, void *__addr)
    mov     rdx, PROT_READ | PROT_WRITE ; read/write permissions (int __prot)
    mov     r10, MAP_SHARED         ; changes commited to the file (int __flags)

    syscall
    
    check_result                    ; check whether mmap() executed correctly
    ja      close                   ; if not, close the file and exit

    mov     rdi, rax                ; mapped memory address returned by mmap()

; Reverses the file by swapping mapped memory content.
; Swaps 8-byte blocks first and the remaining bytes are then swapped one by one.

    lea     r10, [rdi + 7]          ; end of the first 8-byte block
    lea     r11, [rdi + rsi - 8]    ; start of the last 8-byte block

    cmp     r10, r11                ; check if left pointer < right pointer
    jge     skip_swap_8             ; if not, skip the 8-byte swaps

loop_swap_8:

    movbe   rax, [r10 - 7]          ; reversed 8 bytes ending at R10
    movbe   rcx, [r11]              ; reversed 8 bytes starting at R11

    mov     [r10 - 7], rcx          ; move RCX into RAX's source (swap 1/2)
    mov     [r11], rax              ; move RAX into RCX's source (swap 2/2)

    lea     r10, [r10 + 8]          ; move left pointer to the next 8 bytes
    lea     r11, [r11 - 8]          ; move right pointer to the previous 8 bytes

    cmp     r10, r11                ; check if left pointer < right pointer
    jl      loop_swap_8             ; if so, repeat

skip_swap_8:

    lea     r10, [r10 - 7]          ; undo 8-byte movement, increment by 1 byte
    lea     r11, [r11 + 7]          ; undo 8-byte movement, decrement by 1 byte

    cmp     r10, r11                ; check if left pointer < right pointer
    jge     skip_swap               ; if not, skip the single byte swaps

loop_swap:

    mov     al, [r10]               ; load byte from R10
    xchg    al, [r11]               ; swap with byte from R11 (swap 1/2)
    mov     [r10], al               ; store byte from R11     (swap 2/2)

    inc     r10                     ; move left pointer to the next byte
    dec     r11                     ; move right pointer to the previous byte

    cmp     r10, r11                ; check if left pointer < right pointer
    jl      loop_swap               ; if so, repeat

skip_swap:

; Unmaps the memory region.
; int munmap(void *__addr, size_t __len)

    mov     rax, SYS_MUNMAP
    
    syscall
    check_result                    ; check whether munmap() executed correctly

; Closes the opened file.
; int close(int __fd)

close:

    mov     rax, SYS_CLOSE
    mov     rdi, r8                 ; file descriptor (int __fd)
    
    syscall
    check_result                    ; check whether close() executed correctly

; Ends the program.
; void exit(int __status)

exit:

    mov     rax, SYS_EXIT
    mov     rdi, r9                 ; return value (int __status)
    
    syscall
