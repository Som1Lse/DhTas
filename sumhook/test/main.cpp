#include <windows.h>

#include <cstdio>
#include <cassert>

#include <sumhook.h>

#include <Zydis/Zydis.h>

constinit decltype(VirtualAlloc)* VirtualAlloc_Orig = nullptr;
constinit decltype(VirtualFree)* VirtualFree_Orig = nullptr;

void* WINAPI VirtualAlloc_Hook(void* Address, SIZE_T Size, DWORD AllocationType, DWORD Protect){
    auto r = VirtualAlloc_Orig(Address, Size, AllocationType, Protect);

    std::printf("VirtualAlloc(0x%p, %zu, 0x%08X, 0x%08X): 0x%p\n",
        Address, Size, AllocationType, Protect, r);

    return r;
}

BOOL WINAPI VirtualFree_Hook(void* Address, SIZE_T Size, DWORD FreeType){
    auto r = VirtualFree_Orig(Address, Size, FreeType);

    std::printf("VirtualFree(0x%p, %zu, 0x%08X): %d\n",
        Address, Size, FreeType, r);

    return r;
}

void test1(std::initializer_list<unsigned char> Opcode, bool Print = false){
    ZydisDecoder Decoder;
    ZydisDecoderInit(&Decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32);

    unsigned char Code[ZYDIS_MAX_INSTRUCTION_LENGTH] = {};

    auto it = Code;
    for(auto& e:Opcode){
        *it++ = e;
    }

    ZydisFormatter Formatter;
    ZydisFormatterInit(&Formatter, ZYDIS_FORMATTER_STYLE_INTEL);

    for(;;){
        ZydisDecodedInstruction Instruction;
        ZydisDecodedOperand Operands[ZYDIS_MAX_OPERAND_COUNT];

        auto Check = [&Code, &Instruction](){
            assert(smhk::instruction_info(Code).Size == Instruction.length);
        };

        auto r = ZydisDecoderDecodeFull(
            &Decoder, Code, sizeof(Code), &Instruction, Operands);

        if(ZYAN_SUCCESS(r)){
            char Message[256];

            if(Print){
                ZydisFormatterFormatInstruction(
                    &Formatter, &Instruction, Operands, Instruction.operand_count_visible,
                    Message, sizeof(Message), reinterpret_cast<std::uintptr_t>(Code), nullptr);

                for(std::size_t i = 0; i < Instruction.length; ++i){
                    std::printf("%02X ", Code[i]);
                }

                std::printf(": %s\n", Message);
            }

            Check();

            if(Instruction.length > Opcode.size()+1){
                while(++it[1] != 0){
                    r = ZydisDecoderDecodeFull(
                        &Decoder, Code, sizeof(Code), &Instruction, Operands);

                    assert(ZYAN_SUCCESS(r));

                    Check();
                }
            }
        }else if(Print){
            for(std::size_t i = 0; i < Instruction.length; ++i){
                std::printf("%02X ", Code[i]);
            }

            std::printf("INVALID!\n");
        }

        if(++it[0] == 0){
            break;
        }
    }
}

template <unsigned char... Ns>
void test(){
    test1({Ns..., 0xC2}); // ret imm16

    test1({Ns..., 0x0C}); // or al, imm8
    test1({Ns..., 0x24}); // and al, imm8
    test1({Ns..., 0x3C}); // cmp al, imm8
    test1({Ns..., 0x6A}); // push imm8
    test1({Ns..., 0xA8}); // test al, imm8
    test1({Ns..., 0xB0}); // mov al, imm8
    test1({Ns..., 0xB1}); // mov cl, imm8
    test1({Ns..., 0xB2}); // mov dl, imm8
    test1({Ns..., 0xB3}); // mov bl, imm8
    test1({Ns..., 0xB4}); // mov ah, imm8
    test1({Ns..., 0xB5}); // mov ch, imm8
    test1({Ns..., 0xB6}); // mov dh, imm8
    test1({Ns..., 0xB7}); // mov bh, imm8

    test1({Ns..., 0x0D}); // or eax, imm32
    test1({Ns..., 0x25}); // and eax, imm32
    test1({Ns..., 0x3D}); // cmp eax, imm32
    test1({Ns..., 0x68}); // push imm32
    test1({Ns..., 0xA9}); // test eax, imm3
    test1({Ns..., 0xB8}); // mov eax, imm32
    test1({Ns..., 0xB9}); // mov ecx, imm32
    test1({Ns..., 0xBA}); // mov edx, imm32
    test1({Ns..., 0xBB}); // mov ebx, imm32
    test1({Ns..., 0xBC}); // mov esp, imm32
    test1({Ns..., 0xBD}); // mov ebp, imm32
    test1({Ns..., 0xBE}); // mov esi, imm32
    test1({Ns..., 0xBF}); // mov edi, imm32

    test1({Ns..., 0xA0}); // mov al, moffs8
    test1({Ns..., 0xA1}); // mov eax, moffs32
    test1({Ns..., 0xA2}); // mov moffs8, al
    test1({Ns..., 0xA3}); // mov moffs32, eax

    test1({Ns..., 0x70}); // jo rel8
    test1({Ns..., 0x71}); // jno rel8
    test1({Ns..., 0x72}); // jb rel8
    test1({Ns..., 0x73}); // jnb rel8
    test1({Ns..., 0x74}); // je rel8
    test1({Ns..., 0x75}); // jne rel8
    test1({Ns..., 0x76}); // ja rel8
    test1({Ns..., 0x77}); // jna rel8
    test1({Ns..., 0x78}); // js rel8
    test1({Ns..., 0x79}); // jns rel8
    test1({Ns..., 0x7A}); // jp rel8
    test1({Ns..., 0x7B}); // jnp rel8
    test1({Ns..., 0x7C}); // jl rel8
    test1({Ns..., 0x7D}); // jnl rel8
    test1({Ns..., 0x7E}); // jng rel8
    test1({Ns..., 0x7F}); // jg rel8

    test1({Ns..., 0xE8}); // call rel32
    test1({Ns..., 0xE9}); // jmp rel32

    test1({Ns..., 0x80}); // add/or/adc/sbb/and/sub/xor/cmp r/m8, imm8
    test1({Ns..., 0xC0}); // rol/ror/rcl/rcr r/m8, imm8
    test1({Ns..., 0x81}); // add/or/adc/sbb/and/sub/xor/cmp r/m32, imm32
    test1({Ns..., 0x83}); // add/or/adc/sbb/and/sub/xor/cmp r/m32, imm8
    test1({Ns..., 0xC1}); // rol/ror/rcl/rcr r/m32, imm8

    test1({Ns..., 0x00}); // add r/m8, r8
    test1({Ns..., 0x01}); // add r/m32, r32
    test1({Ns..., 0x02}); // add r8, r/m8
    test1({Ns..., 0x03}); // add r32, r/m32
    test1({Ns..., 0x08}); // or r/m8, r8
    test1({Ns..., 0x09}); // or r/m32, r32
    test1({Ns..., 0x0A}); // or r8, r/m8
    test1({Ns..., 0x0B}); // or r32, r/m32
    test1({Ns..., 0x20}); // and r/m8, r8
    test1({Ns..., 0x21}); // and r/m32, r32
    test1({Ns..., 0x22}); // and r8, r/m8
    test1({Ns..., 0x23}); // and r32, r/m32
    test1({Ns..., 0x28}); // sub r/m8, r8
    test1({Ns..., 0x29}); // sub r/m32, r32
    test1({Ns..., 0x2A}); // sub r8, r/m8
    test1({Ns..., 0x2B}); // sub r32, r/m32
    test1({Ns..., 0x30}); // xor r/m8, r/m8
    test1({Ns..., 0x31}); // xor r32, r/m32
    test1({Ns..., 0x32}); // xor r8, r/m8
    test1({Ns..., 0x33}); // xor r32, r/m32
    test1({Ns..., 0x38}); // cmp r/m8, r/m8
    test1({Ns..., 0x39}); // cmp r32, r/m32
    test1({Ns..., 0x3A}); // cmp r8, r/m8
    test1({Ns..., 0x3B}); // cmp r32, r/m32
    test1({Ns..., 0x88}); // mov r/m8, r8
    test1({Ns..., 0x89}); // mov r/m32, r32
    test1({Ns..., 0x8A}); // mov r8, r/m8
    test1({Ns..., 0x8B}); // mov r32, r/m32
    test1({Ns..., 0x8D}); // lea r32, m
    test1({Ns..., 0xD0}); // rol/ror/rcl/rcr/shl/shr r/m8, 1
    test1({Ns..., 0xD1}); // rol/ror/rcl/rcr/shl/shr r/m32, 1
    test1({Ns..., 0xD8}); // fadd/fmul/fcom/fcomp/fsub/fsubr/fdiv/fdivr m32fp
    test1({Ns..., 0xD9}); // fld/fst/fstp/fldenv/fldcw/fnstenv/fnstvw m32fp
    test1({Ns..., 0xDA}); // fiadd/fimul/ficom/ficomp/fisub/fisubr/fidiv/fidivr m32int
    test1({Ns..., 0xDB}); // fld/fst/fstp/fldenv/fldcw/fnstenv/fnstvw/fcmovcc m64fp
    test1({Ns..., 0xDC}); // fadd/fmul/fcom/fcomp/fsub/fsubr/fdiv/fdivr m64fp
    test1({Ns..., 0xDD}); // fld/fst/fstp/fldenv/fldcw/fnstenv/fnstvw/... m80fp
    test1({Ns..., 0xDE}); // fiadd/fimul/ficom/ficomp/fisub/fisubr/fidiv/fidivr m16int
    test1({Ns..., 0xDF}); // fild/fisttp/fist/fistp m16int | fbld/fbstp m80dec | fild/fistp m64int
    test1({Ns..., 0xFE}); // inc/dec r/m8
    test1({Ns..., 0xFF}); // inc/dec/call/jmp/push r/m32

    test1({Ns..., 0x69}); // imul r32, r/m32, imm32
    test1({Ns..., 0x6B}); // imul r32, r/m32, imm8

    test1({Ns..., 0xC6}); // mov r/m8, imm8
    test1({Ns..., 0xC7}); // mov r/m32, imm32
    test1({Ns..., 0xF6}); // test r/m8, imm8
    test1({Ns..., 0xF7}); // test r/m32, imm32

    test1({Ns..., 0x0F, 0x80}); // jo rel32
    test1({Ns..., 0x0F, 0x81}); // jno rel32
    test1({Ns..., 0x0F, 0x82}); // jb rel32
    test1({Ns..., 0x0F, 0x83}); // jnb rel32
    test1({Ns..., 0x0F, 0x84}); // je rel32
    test1({Ns..., 0x0F, 0x85}); // jne rel32
    test1({Ns..., 0x0F, 0x86}); // ja rel32
    test1({Ns..., 0x0F, 0x87}); // jna rel32
    test1({Ns..., 0x0F, 0x88}); // js rel32
    test1({Ns..., 0x0F, 0x89}); // jns rel32
    test1({Ns..., 0x0F, 0x8A}); // jp rel32
    test1({Ns..., 0x0F, 0x8B}); // jnp rel32
    test1({Ns..., 0x0F, 0x8C}); // jl rel32
    test1({Ns..., 0x0F, 0x8D}); // jnl rel32
    test1({Ns..., 0x0F, 0x8E}); // jng rel32
    test1({Ns..., 0x0F, 0x8F}); // jg rel32

    test1({Ns..., 0x0F, 0x10}); // movups xmm1, xmm2/m128
    test1({Ns..., 0x0F, 0x11}); // movups xmm2/m128, xmm1
    test1({Ns..., 0x0F, 0x28}); // movaps xmm1, xmm2/m128
    test1({Ns..., 0x0F, 0x29}); // movaps xmm2/m128, xmm1
    test1({Ns..., 0x0F, 0x2A}); // cvtpi2ps xmm, mm/mm64
    test1({Ns..., 0x0F, 0x2C}); // cvttps2pi mm, xmm/m64
    test1({Ns..., 0x0F, 0x2D}); // cvtps2pi mm, xmm/m64
    test1({Ns..., 0x0F, 0x2E}); // ucomiss xmm1, xmm2/m32
    test1({Ns..., 0x0F, 0x2F}); // comiss xmm1, xmm2/m32
    test1({Ns..., 0x0F, 0x57}); // xorps xmm1, xmm2/m128
    test1({Ns..., 0x0F, 0x58}); // addps xmm1, xmm2/m128
    test1({Ns..., 0x0F, 0x59}); // mulps xmm1, xmm2/m128
    test1({Ns..., 0x0F, 0x6F}); // movq mm, mm/m64
    test1({Ns..., 0x0F, 0x7F}); // movq mm/m64, mm
    test1({Ns..., 0x0F, 0x90}); // seto r/m8
    test1({Ns..., 0x0F, 0x91}); // setno r/m8
    test1({Ns..., 0x0F, 0x92}); // setb r/m8
    test1({Ns..., 0x0F, 0x93}); // setnb r/m8
    test1({Ns..., 0x0F, 0x94}); // sete r/m8
    test1({Ns..., 0x0F, 0x95}); // setne r/m8
    test1({Ns..., 0x0F, 0x96}); // seta r/m8
    test1({Ns..., 0x0F, 0x97}); // setna r/m8
    test1({Ns..., 0x0F, 0x98}); // sets r/m8
    test1({Ns..., 0x0F, 0x99}); // setns r/m8
    test1({Ns..., 0x0F, 0x9A}); // setp r/m8
    test1({Ns..., 0x0F, 0x9B}); // setnp r/m8
    test1({Ns..., 0x0F, 0x9C}); // setl r/m8
    test1({Ns..., 0x0F, 0x9D}); // setnl r/m8
    test1({Ns..., 0x0F, 0x9E}); // setng r/m8
    test1({Ns..., 0x0F, 0x9F}); // setg r/m8
    test1({Ns..., 0x0F, 0xAF}); // imul r32, r/m32
    test1({Ns..., 0x0F, 0xB6}); // movzx r32, r/m8
    test1({Ns..., 0x0F, 0xB7}); // movzx r32, r/m16
    test1({Ns..., 0x0F, 0xBE}); // movsx r32, r/m8
    test1({Ns..., 0x0F, 0xBF}); // movsx r32, r/m16
    test1({Ns..., 0x0F, 0xEF}); // pxor mm, mm/m64
}

void f_int(int i){
    std::printf("f_int(%d)\n", i);
}

void foo(){
    f_int(1);
}

constinit decltype(foo)* foo_Orig = nullptr;

void foo_hook(){
    std::printf("foo_hook()\n");
    foo_Orig();
}

#define PREPARE(m, f) f##_Orig2.prepare(GET_PROC_ADDRESS(m, f), f##_Hook)

int main(){
    try {
        test();
        test<0x66>();
        test<0xF2>();
        test<0xF3>();

        foo();

        std::printf("Start\n");

        auto Kernel32 = GetModuleHandleW(L"kernel32");

        std::printf("set_hook\n");

        {
            smhk::unique_buffer Buffer = nullptr;

            smhk::unique_hook<decltype(&VirtualAlloc)> VirtualAlloc_Orig2 = nullptr;
            smhk::unique_hook<decltype(&VirtualFree)> VirtualFree_Orig2 = nullptr;
            smhk::unique_hook<decltype(&foo)> foo_Orig2 = nullptr;

            Buffer = smhk::create_hooks({
                PREPARE(Kernel32, VirtualAlloc),
                PREPARE(Kernel32, VirtualFree),
                foo_Orig2.prepare(foo, foo_hook),
            });

            VirtualAlloc_Orig = VirtualAlloc_Orig2;
            VirtualFree_Orig = VirtualFree_Orig2;
            foo_Orig = foo_Orig2;

            std::printf("hook set\n");

            VirtualFree(VirtualAlloc(nullptr, 100, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE), 0, MEM_RELEASE);

            foo();

            std::printf("unhook\n");
        }

        std::printf("unhooked\n");

        VirtualFree(VirtualAlloc(nullptr, 100, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE), 0, MEM_RELEASE);
    }catch(std::exception& e){
        std::printf("%s\n", e.what());
    }

    return 0;
}
