global nsqrt

section .bss

buffer: resq 8000               ; buffer for T

section .text

nsqrt:

    mov     r10, rdi            ; copy value of RDI (*Q) into R10
                                ; as it will be modified by 'stosq'

    xor     rax, rax            ; set RAX to 0 (value to initialize Q with)
    mov     rcx, rdx            ; number of bits to set (n)
    shr     rcx, 6              ; number of 8-byte blocks to set (n / 64)

    rep stosq                   ; initialize [RDI] (Q) with zeros

    mov     rdi, r10            ; restore original value of RDI (*Q)

    xor     r9, r9              ; set R9 (current iteration, j - 1) to 0
    lea     r8, [rel buffer]    ; load buffer (T) address

; Single iteration of the algorithm.
; Calculates {RDX - R9 - 1}-th ((n - j)-th) bit of the result.

nsqrt_step:

    mov     r10, rdi            ; copy value of RDI (*Q) into R10
                                ; as it needs to be modified for 'stosq'

    xor     rax, rax            ; set RAX to 0 (value to initialize T with)
    mov     rdi, r8             ; 'stosq' requires destination [R8] to be [RDI]
    mov     rcx, rdx            ; half the number of bits to set (n)
    shr     rcx, 5              ; number of 8-byte blocks to set (2n / 64)

    rep stosq                   ; initialize [RDI] (T) with zeros

    mov     rdi, r10            ; restore original value of RDI (*Q)

    cmp     r9, 0               ; Q is empty (first iteration)
    je      no_copy             ; don't copy, don't shift

; Shifts Q (copied into T) {RDX - R9} (n - j + 1) bits to the right.
; Splits the shift between whole 8-byte blocks (div 64) and bits (mod 64).

    mov     rax, rdx            ; number of bits (n)
    sub     rax, r9             ; number of bits to shift (n - j + 1)
    mov     r10, rax            ; --------------- // ---------------
    shr     r10, 6              ; number of full 8-byte blocks to shift
    and     rax, 63             ; number of remaining bits to shift

    mov     r11, rsi            ; copy value of RSI (*R) into R11
                                ; as it needs to be modified for 'movsq'

    lea     r10, [r8 + r10*8]   ; [R8] (T) block to start copying to
    xchg    rdi, r10            ; swap RDI (*Q) and R10

    mov     rsi, r10            ; 'movsq' requires source [R10] to be [RSI]
    mov     rcx, rdx            ; number of bits to copy (n)
    shr     rcx, 6              ; number of 8-byte blocks to copy (n / 64)

    rep movsq                   ; copy [RSI] (Q) into [RDI] (segment of T)

    mov     rsi, r11            ; restore original value of RSI (*R)
    xchg    rdi, r10            ; restore original value of RDI (*Q)

    cmp     rax, 0              ; check if number of bits to shift == 0
    je      no_copy             ; if so, skip the "in-block" shifting

    mov     rcx, rdx            ; half the number of bits (n)
    shr     rcx, 5              ; number of 8-byte blocks (2n / 64)
    lea     r10, [r8 + rcx*8 - 8] ; last 8-byte block of [R8] (T)
    mov     rcx, rax            ; number of remaining bits to shift

    cmp     r10, r8             ; check if index == first
    je      shift_first_only    ; if so, no previous block to take LSBs from

; Shifts Q (copied into T) RCX ((n - j + 1) % 64) bits to the right.
; Least significant bits of each block are taken from the previous block.

loop_shift:
    mov     r11, [r10 - 8]      ; load previous 8 bytes of [R8] (T) into R11
    shld    [r10], r11, cl      ; shift bits, take LSBs from the previous block
    sub     r10, 8              ; move the pointer to the previous 8 bytes

    cmp     r8, r10             ; check if pointer == first
    jne     loop_shift          ; if not, repeat

; Shifts the first 8-byte block of Q RCX bits to the right.
; As there is no previous block, the LSBs are zeroed.

shift_first_only:

    shl     qword [r10], cl     ; shift bits

; Performs [R8] (T) + 4^{n - j} addition.
; 'no_copy' acts as a label to skip the shift process.

no_copy:

    lea     rcx, [rdx - 1]      ; {number of bits - 1} (n - 1)
    sub     rcx, r9             ; exponent of 4 (n - j)
    shl     rcx, 1              ; exponent of 2, index of bit to set (2n - 2j)
    bts     qword [r8], rcx     ; set RCX-th ((2n - 2j)-th) bit of [R8] (T)

; Performs in-place [RSI] - [R8] (R - T) subtraction.

    xor     rcx, rcx            ; set RCX (8-byte block pointer) to 0
    shr     rdx, 2              ; number of bytes to subtract (2n / 8)
    xor     r10b, r10b          ; set R10B to 0 (will store carry flag)

loop_subtract:
    
    mov     rax, [rsi + rcx]    ; load 8 bytes of [RSI] (R) into RAX

    bt      r10, 0              ; restore CF value
    sbb     rax, [r8 + rcx]     ; subtract {8 bytes of [R8] (T) + CF} from RAX
    setc    r10b                ; store CF value

    mov     [r8 + rcx], rax     ; store the result in [R8] (T)
    lea     rcx, [rcx + 8]      ; move the pointer to the next 8 bytes

    cmp     rcx, rdx            ; check if pointer == end
    jne     loop_subtract       ; if not, repeat

    shl     rdx, 2              ; restore original value of RDX (n)
    
    bt      r10, 0              ; restore CF value
    jc      no_change           ; negative result (R < T), do nothing

; First case: R >= T.
; Sets (n - j)-th bit of [RDI] (Q) and copies [R8] (R - T) into [RSI] (R).

change:

    lea     rcx, [rdx - 1]      ; {number of bits - 1} (n - 1)
    sub     rcx, r9             ; index of bit to set (n - j)
    bts     qword [rdi], rcx    ; set RCX-th ((n - j)-th) bit of [RDI] (Q)

    mov     r10, rdi            ; copy value of RDI (*Q) into R10
                                ; as it needs to be modified for 'movsq'
                                
    mov     r11, rsi            ; copy value of RSI (*R) into R11
                                ; as it needs to be modified for 'movsq'

    mov     rdi, rsi            ; 'movsq' requires destination [RSI] to be [RDI]
    mov     rsi, r8             ; 'movsq' requires source [R8] to be [RSI]
    mov     rcx, rdx            ; half the number of bits to copy (n)
    shr     rcx, 5              ; number of 8-byte blocks to copy (2n / 64)

    rep movsq                   ; copy [RSI] (T) into [RDI] (R)

    mov     rdi, r10            ; restore original value of RDI (*Q)
    mov     rsi, r11            ; restore original value of RSI (*R)

; Second case: R < T.
; Acts only as a label to skip the first case.

no_change:

    inc     r9                  ; move to the next iteration (j := j + 1)
    cmp     r9, rdx             ; check if iteration == last
    jne     nsqrt_step          ; if not, repeat

    ret                         ; return
