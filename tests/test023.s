	.file "/home/feggie/MyData/Virtualki/hw2/tests/test023.lama"

	.stabs "/home/feggie/MyData/Virtualki/hw2/tests/test023.lama",100,0,0,.Ltext

	.globl	main

	.data

string_1:	.string	"abc"

string_0:	.string	"bbc"

string_2:	.string	"test023.lama"

_init:	.int 0

	.section custom_data,"aw",@progbits

filler:	.fill	2, 4, 1

	.stabs "x:S1",40,0,0,global_x

global_x:	.int	1

	.text

.Ltext:

	.stabs "data:t1=r1;0;4294967295;",128,0,0,0

# IMPORT ("Std") / 

# PUBLIC ("main") / 

# EXTERN ("Llowercase") / 

# EXTERN ("Luppercase") / 

# EXTERN ("LtagHash") / 

# EXTERN ("LflatCompare") / 

# EXTERN ("LcompareTags") / 

# EXTERN ("LkindOf") / 

# EXTERN ("Ltime") / 

# EXTERN ("Lrandom") / 

# EXTERN ("LdisableGC") / 

# EXTERN ("LenableGC") / 

# EXTERN ("Ls__Infix_37") / 

# EXTERN ("Ls__Infix_47") / 

# EXTERN ("Ls__Infix_42") / 

# EXTERN ("Ls__Infix_45") / 

# EXTERN ("Ls__Infix_43") / 

# EXTERN ("Ls__Infix_62") / 

# EXTERN ("Ls__Infix_6261") / 

# EXTERN ("Ls__Infix_60") / 

# EXTERN ("Ls__Infix_6061") / 

# EXTERN ("Ls__Infix_3361") / 

# EXTERN ("Ls__Infix_6161") / 

# EXTERN ("Ls__Infix_3838") / 

# EXTERN ("Ls__Infix_3333") / 

# EXTERN ("Ls__Infix_58") / 

# EXTERN ("Li__Infix_4343") / 

# EXTERN ("Lcompare") / 

# EXTERN ("Lwrite") / 

# EXTERN ("Lread") / 

# EXTERN ("Lfailure") / 

# EXTERN ("Lfexists") / 

# EXTERN ("Lfwrite") / 

# EXTERN ("Lfread") / 

# EXTERN ("Lfclose") / 

# EXTERN ("Lfopen") / 

# EXTERN ("Lfprintf") / 

# EXTERN ("Lprintf") / 

# EXTERN ("LmakeString") / 

# EXTERN ("Lsprintf") / 

# EXTERN ("LregexpMatch") / 

# EXTERN ("Lregexp") / 

# EXTERN ("Lsubstring") / 

# EXTERN ("LmatchSubString") / 

# EXTERN ("Lstringcat") / 

# EXTERN ("LreadLine") / 

# EXTERN ("Ltl") / 

# EXTERN ("Lhd") / 

# EXTERN ("Lsnd") / 

# EXTERN ("Lfst") / 

# EXTERN ("Lhash") / 

# EXTERN ("Lclone") / 

# EXTERN ("Llength") / 

# EXTERN ("Lstring") / 

# EXTERN ("LmakeArray") / 

# EXTERN ("LstringInt") / 

# EXTERN ("global_sysargs") / 

# EXTERN ("Lsystem") / 

# EXTERN ("LgetEnv") / 

# EXTERN ("Lassert") / 

# LABEL ("main") / 

main:

# BEGIN ("main", 2, 0, [], [], []) / 

	.type main, @function

	.cfi_startproc

	movl	_init,	%eax
	test	%eax,	%eax
	jz	_continue
	ret
_continue:

	movl	$1,	_init
	pushl	%ebp
	.cfi_def_cfa_offset	8

	.cfi_offset 5, -8

	movl	%esp,	%ebp
	.cfi_def_cfa_register	5

	subl	$Lmain_SIZE,	%esp
	movl	%esp,	%edi
	movl	$filler,	%esi
	movl	$LSmain_SIZE,	%ecx
	rep movsl	
	call	__gc_init
	pushl	12(%ebp)
	pushl	8(%ebp)
	call	set_args
	addl	$8,	%esp
# SLABEL ("L1") / 

L1:

# STRING ("bbc") / 

	movl	$string_0,	%ebx
	pushl	%ebx
	call	Bstring
	addl	$4,	%esp
	movl	%eax,	%ebx
# LINE (1) / 

	.stabn 68,0,1,.L0

.L0:

# ST (Global ("x")) / 

	movl	%ebx,	global_x
# DROP / 

# LINE (2) / 

	.stabn 68,0,2,.L1

.L1:

# LD (Global ("x")) / 

	movl	global_x,	%ebx
# CONST (0) / 

	movl	$1,	%ecx
# ELEM / 

	pushl	%ecx
	pushl	%ebx
	call	Belem
	addl	$8,	%esp
	movl	%eax,	%ebx
# CALL ("Lwrite", 1, false) / 

	pushl	%ebx
	call	Lwrite
	addl	$4,	%esp
	movl	%eax,	%ebx
# DROP / 

# LINE (3) / 

	.stabn 68,0,3,.L2

.L2:

# LD (Global ("x")) / 

	movl	global_x,	%ebx
# CONST (0) / 

	movl	$1,	%ecx
# ELEM / 

	pushl	%ecx
	pushl	%ebx
	call	Belem
	addl	$8,	%esp
	movl	%eax,	%ebx
# CALL ("Lread", 1, false) / 

	pushl	%ebx
	call	Lread
	addl	$4,	%esp
	movl	%eax,	%ebx
# DROP / 

# LINE (4) / 

	.stabn 68,0,4,.L3

.L3:

# LD (Global ("x")) / 

	movl	global_x,	%ebx
# CONST (0) / 

	movl	$1,	%ecx
# ELEM / 

	pushl	%ecx
	pushl	%ebx
	call	Belem
	addl	$8,	%esp
	movl	%eax,	%ebx
# CALL ("Lwrite", 1, false) / 

	pushl	%ebx
	call	Lwrite
	addl	$4,	%esp
	movl	%eax,	%ebx
# DROP / 

# LINE (6) / 

	.stabn 68,0,6,.L4

.L4:

# LD (Global ("x")) / 

	movl	global_x,	%ebx
# DUP / 

	movl	%ebx,	%ecx
# SLABEL ("L29") / 

L29:

# DUP / 

	movl	%ecx,	%esi
# TAG ("A", 1) / 

	movl	$55,	%edi
	movl	$3,	-4(%ebp)
	pushl	%ebx
	pushl	%ecx
	pushl	-4(%ebp)
	pushl	%edi
	pushl	%esi
	call	Btag
	addl	$12,	%esp
	popl	%ecx
	popl	%ebx
	movl	%eax,	%esi
# CJMP ("nz", "L25") / 

	sarl	%esi
	cmpl	$0,	%esi
	jnz	L25
# LABEL ("L26") / 

L26:

# DROP / 

# JMP ("L24") / 

	jmp	L24
# LABEL ("L25") / 

L25:

# DUP / 

	movl	%ecx,	%esi
# CONST (0) / 

	movl	$1,	%edi
# ELEM / 

	pushl	%ebx
	pushl	%ecx
	pushl	%edi
	pushl	%esi
	call	Belem
	addl	$8,	%esp
	popl	%ecx
	popl	%ebx
	movl	%eax,	%esi
# DUP / 

	movl	%esi,	%edi
# TAG ("B", 1) / 

	movl	$57,	-4(%ebp)
	movl	$3,	-8(%ebp)
	pushl	%ebx
	pushl	%ecx
	pushl	%esi
	pushl	-8(%ebp)
	pushl	-4(%ebp)
	pushl	%edi
	call	Btag
	addl	$12,	%esp
	popl	%esi
	popl	%ecx
	popl	%ebx
	movl	%eax,	%edi
# CJMP ("nz", "L27") / 

	sarl	%edi
	cmpl	$0,	%edi
	jnz	L27
# LABEL ("L28") / 

L28:

# DROP / 

# JMP ("L26") / 

	jmp	L26
# LABEL ("L27") / 

L27:

# DUP / 

	movl	%esi,	%edi
# CONST (0) / 

	movl	$1,	-4(%ebp)
# ELEM / 

	pushl	%ebx
	pushl	%ecx
	pushl	%esi
	pushl	-4(%ebp)
	pushl	%edi
	call	Belem
	addl	$8,	%esp
	popl	%esi
	popl	%ecx
	popl	%ebx
	movl	%eax,	%edi
# DROP / 

# DROP / 

# DROP / 

# DROP / 

# SLABEL ("L31") / 

L31:

# CONST (3) / 

	movl	$7,	%ebx
# SLABEL ("L32") / 

L32:

# JMP ("L21") / 

	jmp	L21
# SLABEL ("L30") / 

L30:

# SLABEL ("L34") / 

L34:

# LABEL ("L24") / 

L24:

# DUP / 

	movl	%ebx,	%ecx
# STRING ("abc") / 

	movl	$string_1,	%esi
	pushl	%ebx
	pushl	%ecx
	pushl	%esi
	call	Bstring
	addl	$4,	%esp
	popl	%ecx
	popl	%ebx
	movl	%eax,	%esi
# PATT (StrCmp) / 

	pushl	%ebx
	pushl	%esi
	pushl	%ecx
	call	Bstring_patt
	addl	$8,	%esp
	popl	%ebx
	movl	%eax,	%ecx
# CJMP ("z", "L33") / 

	sarl	%ecx
	cmpl	$0,	%ecx
	jz	L33
# DROP / 

# SLABEL ("L36") / 

L36:

# CONST (2) / 

	movl	$5,	%ebx
# SLABEL ("L37") / 

L37:

# JMP ("L21") / 

	jmp	L21
# SLABEL ("L35") / 

L35:

# SLABEL ("L40") / 

L40:

# LABEL ("L33") / 

L33:

# DUP / 

	movl	%ebx,	%ecx
# DUP / 

	movl	%ecx,	%esi
# ARRAY (3) / 

	movl	$7,	%edi
	pushl	%ebx
	pushl	%ecx
	pushl	%edi
	pushl	%esi
	call	Barray_patt
	addl	$8,	%esp
	popl	%ecx
	popl	%ebx
	movl	%eax,	%esi
# CJMP ("nz", "L38") / 

	sarl	%esi
	cmpl	$0,	%esi
	jnz	L38
# LABEL ("L39") / 

L39:

# DROP / 

# JMP ("L22") / 

	jmp	L22
# LABEL ("L38") / 

L38:

# DUP / 

	movl	%ecx,	%esi
# CONST (0) / 

	movl	$1,	%edi
# ELEM / 

	pushl	%ebx
	pushl	%ecx
	pushl	%edi
	pushl	%esi
	call	Belem
	addl	$8,	%esp
	popl	%ecx
	popl	%ebx
	movl	%eax,	%esi
# CONST (1) / 

	movl	$3,	%edi
# BINOP ("==") / 

	xorl	%eax,	%eax
	cmpl	%edi,	%esi
	sete	%al
	sall	%eax
	orl	$0x0001,	%eax
	movl	%eax,	%esi
# CJMP ("z", "L39") / 

	sarl	%esi
	cmpl	$0,	%esi
	jz	L39
# DUP / 

	movl	%ecx,	%esi
# CONST (1) / 

	movl	$3,	%edi
# ELEM / 

	pushl	%ebx
	pushl	%ecx
	pushl	%edi
	pushl	%esi
	call	Belem
	addl	$8,	%esp
	popl	%ecx
	popl	%ebx
	movl	%eax,	%esi
# CONST (2) / 

	movl	$5,	%edi
# BINOP ("==") / 

	xorl	%eax,	%eax
	cmpl	%edi,	%esi
	sete	%al
	sall	%eax
	orl	$0x0001,	%eax
	movl	%eax,	%esi
# CJMP ("z", "L39") / 

	sarl	%esi
	cmpl	$0,	%esi
	jz	L39
# DUP / 

	movl	%ecx,	%esi
# CONST (2) / 

	movl	$5,	%edi
# ELEM / 

	pushl	%ebx
	pushl	%ecx
	pushl	%edi
	pushl	%esi
	call	Belem
	addl	$8,	%esp
	popl	%ecx
	popl	%ebx
	movl	%eax,	%esi
# CONST (3) / 

	movl	$7,	%edi
# BINOP ("==") / 

	xorl	%eax,	%eax
	cmpl	%edi,	%esi
	sete	%al
	sall	%eax
	orl	$0x0001,	%eax
	movl	%eax,	%esi
# CJMP ("z", "L39") / 

	sarl	%esi
	cmpl	$0,	%esi
	jz	L39
# DROP / 

# DROP / 

# SLABEL ("L42") / 

L42:

# CONST (1) / 

	movl	$3,	%ebx
# SLABEL ("L43") / 

L43:

# SLABEL ("L41") / 

L41:

# JMP ("L21") / 

	jmp	L21
# LABEL ("L22") / 

L22:

# FAIL ((6, 5), true) / 

	pushl	$11
	pushl	$13
	pushl	$string_2
	pushl	%ebx
	call	Bmatch_failure
	addl	$16,	%esp
# JMP ("L21") / 

	jmp	L21
# LABEL ("L21") / 

L21:

# CALL ("Lwrite", 1, false) / 

	pushl	%ebx
	call	Lwrite
	addl	$4,	%esp
	movl	%eax,	%ebx
# SLABEL ("L2") / 

L2:

# END / 

	movl	%ebx,	%eax
Lmain_epilogue:

	movl	%ebp,	%esp
	popl	%ebp
	xorl	%eax,	%eax
	.cfi_restore	5

	.cfi_def_cfa	4, 4

	ret
	.cfi_endproc

	.set	Lmain_SIZE,	8

	.set	LSmain_SIZE,	2

	.size main, .-main

