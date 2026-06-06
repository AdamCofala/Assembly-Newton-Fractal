global newton_asm

section .rodata
half:       dd 0.5
one:        dd 1.0
two:        dd 2.0
three:      dd 3.0
zero:       dd 0.0
eps_conv:   dd 1.0e-6

; Roots of z^3 - 1 = 0
r2r:        dd -0.5
r2i:        dd 0.8660254
r3i:        dd -0.8660254   ; r3r is same as r2r (-0.5)

; Base brightness for roots
base_b0:    dd 1.5
base_b1:    dd 1.0
base_b2:    dd 0.8

section .text

; ABI x86-64 Linux:
; rdi = width
; rsi = height
; rdx = rgb_out
; xmm0 = centerX
; xmm1 = centerY
; xmm2 = zoom
; rcx = maxIter

newton_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 16              ; Input params space

    movss [rsp+0], xmm0      ; centerX
    movss [rsp+4], xmm1      ; centerY
    movss [rsp+8], xmm2      ; zoom

    xor r8, r8               ; y = 0

.y_loop:
    cmp r8, rsi
    jge .done

    xor r9, r9               ; x = 0

.x_loop:
    cmp r9, rdi
    jge .next_y

    ; Get camera params
    movss xmm0, [rsp+0]
    movss xmm1, [rsp+4]
    movss xmm2, [rsp+8]

    ; SCREEN TO COMPLEX PLANE


    ; re = centerX + (x - width * 0.5f) / zoom

    cvtsi2ss xmm3, r9       ; x to float
    cvtsi2ss xmm4, rdi      ; width to float
    mulss xmm4, [rel half]  ; width *0.5f
    subss xmm3, xmm4        ; x - width *0.5f
    divss xmm3, xmm2        ; (x - width * 0.5f) / zoom
    addss xmm3, xmm0        ; xmm3 = re


    ; im = centerY - (y - height * 0.5f) / zoom

    cvtsi2ss xmm5, r8       ; y to float
    cvtsi2ss xmm6, rsi      ; height to float
    mulss xmm6, [rel half]  ; height * 0.5f
    subss xmm5, xmm6        ; y - height * 0.5f
    divss xmm5, xmm2        ; (y - height * 0.5f) / zoom
    movaps xmm7, xmm1       ; xmm7 = centerY
    subss xmm7, xmm5        ; xmm7 = im

    movaps xmm14, xmm3      ; zr = re
    movaps xmm15, xmm7      ; zi = im

    xor r11, r11            ; iter = 0

.iter_loop:
    cmp r11, rcx            ; cmp iter >= maxIter
    jge .after_iter


    ; Re(z)^2
    movaps xmm3, xmm14      ; xmm3 = zr
    mulss xmm3, xmm3        ; xmm3 = zr*zr

    ; Im(z)^2
    movaps xmm4, xmm15      ; xmm4 = zi
    mulss xmm4, xmm4        ; xmm4 = zi

    ; Re(z^2) = Re(z)^2 - Im(z)^2
    movaps xmm5, xmm3
    subss xmm5, xmm4         ; xmm5 = zr2

    ; Im(z^2) = 2.0f * Re(z) * Im(z)
    movaps xmm6, xmm14
    mulss xmm6, xmm15
    mulss xmm6, [rel two]    ; xmm6 = zi2

    ; Re(z^3) = Re(z^2)*Re(z) - Im(z^2)*Im(z)
    movaps xmm7, xmm5
    mulss xmm7, xmm14
    movaps xmm8, xmm6
    mulss xmm8, xmm15
    subss xmm7, xmm8         ; xmm7 = zr3

    ; Im(z^3) = Re(z^2)*Im(z) + Im(z^2)*Re(z)
    movaps xmm8, xmm5
    mulss xmm8, xmm15
    movaps xmm9, xmm6
    mulss xmm9, xmm14
    addss xmm8, xmm9         ; xmm8 = zi3

    ; Re(F(z)) = Re(z^3) - 1.0f
    movaps xmm9, xmm7
    subss xmm9, [rel one]    ; xmm9 = fr

    ; Im(F(z)) = Im(z^3)
    movaps xmm10, xmm8       ; xmm10 = fi

    ; Re(F`(z)) = 3.0f * Re(z^2)
    movaps xmm11, xmm5
    mulss xmm11, [rel three] ; xmm11 = dr

    ; Im(F`(z)) = 3.0f * Im(z^2)
    movaps xmm12, xmm6
    mulss xmm12, [rel three] ; xmm12 = di

    ; denom = Re(F`(z))^2 + Im(F`(z))
    movaps xmm3, xmm11
    mulss xmm3, xmm3
    movaps xmm4, xmm12
    mulss xmm4, xmm4
    addss xmm3, xmm4         ; xmm3 = denom

    ; if (denom == 0.0f) break;
    ucomiss xmm3, [rel zero]
    je .after_iter

    ; Rational step = [Re(F(z))*Re(F`(z)) + Im(F(z))*Im(F`(z))] / denom
    movaps xmm4, xmm9
    mulss xmm4, xmm11
    movaps xmm5, xmm10
    mulss xmm5, xmm12
    addss xmm4, xmm5
    divss xmm4, xmm3         ; xmm4 = tr

    ; Imaginary step = (Im(F(z))*Re(F`(z)) - Re(F(z))*Im(F`(z))) / denom
    movaps xmm5, xmm10
    mulss xmm5, xmm11
    movaps xmm6, xmm9
    mulss xmm6, xmm12
    subss xmm5, xmm6
    divss xmm5, xmm3         ; xmm5 = ti

    ; Re(z_new) = Re(z) - tr
    subss xmm14, xmm4

    ; Re(z_new) = Re(z) - ti
    subss xmm15, xmm5

    ; if (|F(Z)| < 1e-6f) break;
    movaps xmm4, xmm9
    mulss xmm4, xmm4
    movaps xmm5, xmm10
    mulss xmm5, xmm5
    addss xmm4, xmm5         ; xmm4 = fr*fr + fi*fi
    ucomiss xmm4, [rel eps_conv]
    jb .after_iter

    inc r11
    jmp .iter_loop

.after_iter:

    ;  (Root Detection)

    ; d1 = (zr - 1.0)^2 + zi^2
    movaps xmm3, xmm14
    subss xmm3, [rel one]
    mulss xmm3, xmm3
    movaps xmm4, xmm15
    mulss xmm4, xmm4
    addss xmm3, xmm4         ; xmm3 = d1

    ; d2 = (zr - r2r)^2 + (zi - r2i)^2   [r2r = -0.5, r2i = 0.866...]
    movaps xmm4, xmm14
    subss xmm4, [rel r2r]
    mulss xmm4, xmm4
    movaps xmm5, xmm15
    subss xmm5, [rel r2i]
    mulss xmm5, xmm5
    addss xmm4, xmm5         ; xmm4 = d2

    ; d3 = (zr - r2r)^2 + (zi - r3i)^2   [r3r = -0.5, r3i = -0.866...]
    movaps xmm5, xmm14
    subss xmm5, [rel r2r]
    mulss xmm5, xmm5
    movaps xmm6, xmm15
    subss xmm6, [rel r3i]
    mulss xmm6, xmm6
    addss xmm5, xmm6         ; xmm5 = d3

    ; Choose base brightness
    movss xmm6, [rel base_b0] ; Default root = 0
    movaps xmm7, xmm3         ; minD = d1

    ucomiss xmm4, xmm7        ; if (d2 < minD)
    jae .check_d3
    movaps xmm7, xmm4         ; minD = d2
    movss xmm6, [rel base_b1] ; root = 1

.check_d3:
    ucomiss xmm5, xmm7        ; if (d3 < minD)
    jae .calc_brightness
    movss xmm6, [rel base_b2] ; root = 2

.calc_brightness:

    ; t = (float)iter / (float)maxIter
    cvtsi2ss xmm0, r11
    cvtsi2ss xmm1, rcx
    divss xmm0, xmm1          ; xmm0 = t

    ; gray = baseBrightness * (1.0f - t)
    movss xmm1, [rel one]
    subss xmm1, xmm0          ; 1.0f - t
    mulss xmm1, xmm6          ; xmm1 = gray

    ; Save to buffor
    ; idx = (y * width + x) * 3
    mov r10, r8
    imul r10, rdi
    add r10, r9
    imul r10, r10, 3

    mov rax, r10
    shl rax, 2                ; idx * 4 , Conversion to bytes


    movss [rdx + rax + 8], xmm1 ; Blue tint

    mulss xmm1, [rel half]  ; 0.5 * gray
    movss [rdx + rax + 4], xmm1
    movss [rdx + rax + 0], xmm1

    inc r9
    jmp .x_loop

.next_y:
    inc r8
    jmp .y_loop

.done:
    mov rsp, rbp
    pop rbp
    ret