; 主引导程序在内存中的入口地址，CPU在执行完BIOS后会跳转到这里执行(jmp 0x7c00)
org 0x7c00

; 初始化ax，ss，ds，es寄存器为0，cs基存器中值为0
start:
    mov ax, cs
	mov ss, ax
	mov ds, ax
	mov es, ax
	
	mov si, msg      ; 将msg这个地址赋值给si寄存器
	
; 功能代码，实现在屏幕上打印 Hello, LIYAO_OS!
print:
    mov al, [si]     ; 将si寄存器中的内容（是msg地址值）所指向的内容赋值给al寄存器
	add si, 1        ; si寄存器中内容加1
	cmp al, 0x00     ; 比较[si] 是不是 等于0,打印输出内容的结尾标志符为 0
	je last          ; 若为0，则跳转到 last 地址处
	mov ah, 0x0e     ; 若还不为0, 则
	mov bx, 0x0f
	int 0x10         ; 屏幕输出的中断功能标识
	jmp print        ; 循环跳到print
	
last:
    hlt              ; 让cpu停止 
	jmp last         ; 死循环使cpu停止在这，屏幕上看到的就一直是Hello, LIYAO_OS!
	
msg:
    db 0x0a, 0x0a            ; 定义数据，换行符
	db "Hello, LIYAO_OS!"    ; 定义字符串数据 Hello, LIYAO_OS!
	db 0x0a, 0x0a            ; 定时数据 换行符
	times 510-($-$$) db 0x00 ; 主引导程序占512字节，这512
							 ; 字节除了上面汇编代码占的字节外，其他的全部填充为 0， 
	                         ; 最后标志位 0x55aa会占2个字节，所以这里用510减去上面汇编代码占用的字节
	db 0x55, 0xaa            ; 主引导程序的结尾标志位