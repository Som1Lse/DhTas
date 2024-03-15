#include <sumhook.h>

#include <system_error>

#include <cstdlib>

// TODO: Support customisable API calls since the default ones might be hooked.

namespace smhk {

namespace {

unsigned char modrm_reg(unsigned char Byte){
    return (Byte >> 3)&7;
}

std::size_t modrm_size(const void* Start, std::size_t Size){
    auto Code = static_cast<const unsigned char*>(Start)+Size;
    auto Mod = Code[0] >> 6;
    if(Mod == 3){
        return Size+1;
    }else{
        auto Rm = Code[0]&7;
        if(Mod == 0 && Rm == 5){
            // TODO: Relative in 64-bit mode.
            return Size+5;
        }

        Size += (Mod == 0)?0:(Mod == 1)?1:4;

        if(Rm == 4){
            if(Mod == 0 && (Code[1]&7) == 5){
                return Size+6;
            }else{
                return Size+2;
            }
        }else{
            return Size+1;
        }
    }
}

std::uintptr_t ptr_diff(const void* Lhs, const void* Rhs){
    return reinterpret_cast<std::uintptr_t>(Lhs)-reinterpret_cast<std::uintptr_t>(Rhs);
}

std::size_t write_jmp(unsigned char* From, const void* To, unsigned char Byte = 0xE9u){
    From[0] = Byte;
    std::uint32_t Jmp = ptr_diff(To, From+JmpSize);
    std::memcpy(From+1, &Jmp, sizeof(Jmp));
    return JmpSize;
}

struct pfx_info {
    std::size_t Size;

    bool Data16, F2, F3;
};

pfx_info prefix_info(const void* Start){
    auto Code = static_cast<const unsigned char*>(Start);

    pfx_info r = {};
    for(auto& i = r.Size;; ++i){
        switch(Code[i]){
            case 0x66: r.Data16 = true; break;
            case 0xF2: r.F2 = true; break;
            case 0xF3: r.F3 = true; break;
            default: return r;
        }
    }
}

}

[[noreturn]] void invalid_instruction(const unsigned char* Code){
    char Message[128];
    std::snprintf(Message, sizeof(Message), "%p: Unknown instruction: "
        "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", Code,
        Code[0], Code[1], Code[2], Code[3], Code[4], Code[5], Code[6], Code[7], Code[8], Code[9],
        Code[10], Code[11], Code[12], Code[13], Code[14]);

    throw std::runtime_error(Message);
}

size_pair copy_instruction(unsigned char* To, const unsigned char* From){
    auto Info = instruction_info(From);

    assert(Info.Copy != 0);

    if(Info.Size == Info.Copy){
        if(Info.Relative == 0){
            std::memcpy(To, From, Info.Size);
        }else{
            assert(Info.Relative == 4);
            std::uint32_t Jmp;
            assert(Info.Size > sizeof(Jmp));

            std::memcpy(&Jmp, From+Info.Size-sizeof(Jmp), sizeof(Jmp));
            Jmp += ptr_diff(From, To);

            std::memcpy(To, From, Info.Size-sizeof(Jmp));
            std::memcpy(To+Info.Size-sizeof(Jmp), &Jmp, sizeof(Jmp));
        }
    }else{
        assert(Info.Relative == 1);

        // jcc rel8
        assert((From[Info.Prefix]&0xF0) == 0x70);

        assert(Info.Size == Info.Prefix+2);
        assert(Info.Copy == Info.Prefix+6);

        auto Jmp = static_cast<std::uint32_t>(static_cast<std::int8_t>(From[Info.Prefix+1]));
        Jmp += ptr_diff(From, To)+(Info.Size-Info.Copy);

        std::memcpy(To, From, Info.Prefix);

        // jcc rel32
        To[Info.Prefix]   = 0x0F;
        To[Info.Prefix+1] = 0x80|(From[Info.Prefix]&0x0F);

        std::memcpy(To+Info.Prefix+2, &Jmp, sizeof(Jmp));
    }

    return {Info.Size, Info.Copy};
}

std::size_t copy_code(unsigned char* To, const unsigned char* From, std::size_t Size){
    std::size_t i = 0;
    std::size_t j = 0;
    while(i < Size){
        auto Sizes = copy_instruction(To+j, From+i);
        i += Sizes.Size;
        j += Sizes.Copy;
    }

    assert(i == Size);

    return j;
}

instr_info instruction_info(const void* Start){
    auto Prefix = prefix_info(Start);
    auto Code = static_cast<const unsigned char*>(Start)+Prefix.Size;

    if((Code[0]&0xE0) == 0x40){
        // inc/dec/push/pop r32
        return {Prefix.Size, 1+Prefix.Size};
    }

    switch(Code[0]){
        case 0x99:   // cdq
        case 0xC3:   // ret
        case 0xCC: { // int3
            return {Prefix.Size, 1+Prefix.Size};
        }
        case 0xC2: { // ret imm16
            return {Prefix.Size, 3+Prefix.Size};
        }
        case 0x0C:   // or al, imm8
        case 0x24:   // and al, imm8
        case 0x3C:   // cmp al, imm8
        case 0x6A:   // push imm8
        case 0xA8:   // test al, imm8
        case 0xB0:   // mov al, imm8
        case 0xB1:   // mov cl, imm8
        case 0xB2:   // mov dl, imm8
        case 0xB3:   // mov bl, imm8
        case 0xB4:   // mov ah, imm8
        case 0xB5:   // mov ch, imm8
        case 0xB6:   // mov dh, imm8
        case 0xB7: { // mov bh, imm8
            return {Prefix.Size, 2+Prefix.Size};
        }
        case 0x0D:   // or eax, imm32
        case 0x25:   // and eax, imm32
        case 0x3D:   // cmp eax, imm32
        case 0x68:   // push imm32
        case 0xA9:   // test eax, imm32
        case 0xB8:   // mov eax, imm32
        case 0xB9:   // mov ecx, imm32
        case 0xBA:   // mov edx, imm32
        case 0xBB:   // mov ebx, imm32
        case 0xBC:   // mov esp, imm32
        case 0xBD:   // mov ebp, imm32
        case 0xBE:   // mov esi, imm32
        case 0xBF: { // mov edi, imm32
            if(Prefix.Data16){
                return {Prefix.Size, 3+Prefix.Size};
            }else{
                return {Prefix.Size, 5+Prefix.Size};
            }
        }
        case 0xA0:   // mov al, moffs8
        case 0xA1:   // mov eax, moffs32
        case 0xA2:   // mov moffs8, al
        case 0xA3: { // mov moffs32, eax
            return {Prefix.Size, 5+Prefix.Size};
        }
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74:
        case 0x75: case 0x76: case 0x77: case 0x78: case 0x79:
        case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E:
        case 0x7F: { // jcc rel8
            if(Prefix.Data16){
                return {Prefix.Size, 2+Prefix.Size, 1, 0};
            }else{
                return {Prefix.Size, 2+Prefix.Size, 1, 6+Prefix.Size};
            }
        }
        case 0xE8:   // call rel32
        case 0xE9: { // jmp rel32
            if(Prefix.Data16){
                return {Prefix.Size, 3+Prefix.Size, 2, 0};
            }else{
                return {Prefix.Size, 5+Prefix.Size, 4};
            }
        }
        case 0x80:   // add/or/adc/sbb/and/sub/xor/cmp r/m8, imm8
        case 0xC0: { // rol/ror/rcl/rcr/shl/shr r/m8, imm8
            return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+1};
        }
        case 0x83:   // add/or/adc/sbb/and/sub/xor/cmp r/m32, imm8
        case 0xC1: { // rol/ror/rcl/rcr/shl/shr r/m32, imm8
            return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+1};
        }
        case 0x81: { // add/or/adc/sbb/and/sub/xor/cmp r/m32, imm32
            if(Prefix.Data16){
                return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+2};
            }else{
                return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+4};
            }
        }
        case 0x00:   // add r/m8, r8
        case 0x01:   // add r/m32, r32
        case 0x02:   // add r8, r/m8
        case 0x03:   // add r32, r/m32
        case 0x08:   // or r/m8, r8
        case 0x09:   // or r/m32, r32
        case 0x0A:   // or r8, r/m8
        case 0x0B:   // or r32, r/m32
        case 0x20:   // and r/m8, r8
        case 0x21:   // and r/m32, r32
        case 0x22:   // and r8, r/m8
        case 0x23:   // and r32, r/m32
        case 0x28:   // sub r/m8, r8
        case 0x29:   // sub r/m32, r32
        case 0x2A:   // sub r8, r/m8
        case 0x2B:   // sub r32, r/m32
        case 0x30:   // xor r/m8, r/m8
        case 0x31:   // xor r32, r/m32
        case 0x32:   // xor r8, r/m8
        case 0x33:   // xor r32, r/m32
        case 0x38:   // cmp r/m8, r/m8
        case 0x39:   // cmp r32, r/m32
        case 0x3A:   // cmp r8, r/m8
        case 0x3B:   // cmp r32, r/m32
        case 0x84:   // test r8, r/m8
        case 0x85:   // test r32, r/m32
        case 0x88:   // mov r/m8, r8
        case 0x89:   // mov r/m32, r32
        case 0x8A:   // mov r8, r/m8
        case 0x8B:   // mov r32, r/m32
        case 0x8D:   // lea r32, m
        case 0xD0:   // rol/ror/rcl/rcr/shl/shr r/m8, 1
        case 0xD1:   // rol/ror/rcl/rcr/shl/shr r/m32, 1
        case 0xD8:   // fadd/fmul/fcom/fcomp/fsub/fsubr/fdiv/fdivr m32fp
        case 0xD9:   // fld/fst/fstp/fldenv/fldcw/fnstenv/fnstvw m32fp
        case 0xDA:   // fiadd/fimul/ficom/ficomp/fisub/fisubr/fidiv/fidivr m32int
        case 0xDB:   // fld/fst/fstp/fldenv/fldcw/fnstenv/fnstvw/fcmovcc m64fp
        case 0xDC:   // fadd/fmul/fcom/fcomp/fsub/fsubr/fdiv/fdivr m64fp
        case 0xDD:   // fld/fst/fstp/fldenv/fldcw/fnstenv/fnstvw/... m80fp
        case 0xDE:   // fiadd/fimul/ficom/ficomp/fisub/fisubr/fidiv/fidivr m16int
        case 0xDF:   // fild/fisttp/fist/fistp m16int | fbld/fbstp m80dec | fild/fistp m64int
        case 0xFE:   // inc/dec r/m8
        case 0xFF: { // inc/dec/call/jmp/push r/m32
            return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)};
        }
        case 0x69: { // imul r32, r/m32, imm32
            if(Prefix.Data16){
                return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+2};
            }else{
                return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+4};
            }
        }
        case 0x6B: { // imul r32, r/m32, imm8
            return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+1};
        }
        case 0xC6: {
            if(modrm_reg(Code[1]) == 0){ // mov r/m8, imm8
                return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+1};
            }else if(Code[1] == 0xF8){
                // This basically just exists so a test case passes.
                return {Prefix.Size, Prefix.Size+3};
            }
            break;
        }
        case 0xC7: {
            if(modrm_reg(Code[1]) == 0){ // mov r/m32, imm32
                if(Prefix.Data16){
                    return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+2};
                }else{
                    return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+4};
                }
            }else if(Code[1] == 0xF8){
                // This basically just exists so a test case passes.
                if(Prefix.Data16){
                    return {Prefix.Size, Prefix.Size+4, 2, 0};
                }else{
                    return {Prefix.Size, Prefix.Size+6, 4};
                }
            }
            break;
        }
        case 0xF6: {
            if(modrm_reg(Code[1]) < 2){ // test r/m8, imm8
                return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+1};
            }else{ // not/neg/mul/imul/div/idiv r/m8
                return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)};
            }
            break;
        }
        case 0xF7: {
            if(modrm_reg(Code[1]) < 2){ // test r/m32, imm32
                if(Prefix.Data16){
                    return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+2};
                }else{
                    return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)+4};
                }
            }else{ // not/neg/mul/imul/div/idiv r/m32
                return {Prefix.Size, modrm_size(Start, 1+Prefix.Size)};
            }
            break;
        }
        case 0x0F: {
            switch(Code[1]){
                case 0x80: case 0x81: case 0x82: case 0x83: case 0x84:
                case 0x85: case 0x86: case 0x87: case 0x88: case 0x89:
                case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E:
                case 0x8F: { // jcc rel32
                    if(Prefix.Data16){
                        return {Prefix.Size, 4+Prefix.Size, 2, 0};
                    }else{
                        return {Prefix.Size, 6+Prefix.Size, 4};
                    }
                }
                case 0x10:   // movups xmm1, xmm2/m128
                case 0x11:   // movups xmm2/m128, xmm1
                case 0x28:   // movaps xmm1, xmm2/m128
                case 0x29:   // movaps xmm2/m128, xmm1
                case 0x2A:   // cvtpi2ps xmm, mm/mm64
                case 0x2C:   // cvttps2pi mm, xmm/m64
                case 0x2D:   // cvtps2pi mm, xmm/m64
                case 0x2E:   // ucomiss xmm1, xmm2/m32
                case 0x2F:   // comiss xmm1, xmm2/m32
                case 0x57:   // xorps xmm1, xmm2/m128
                case 0x58:   // addps xmm1, xmm2/m128
                case 0x59:   // mulps xmm1, xmm2/m128
                case 0x6F:   // movq mm, mm/m64
                case 0x7F:   // movq mm/m64, mm
                case 0x90: case 0x91: case 0x92: case 0x93: case 0x94:
                case 0x95: case 0x96: case 0x97: case 0x98: case 0x99:
                case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E:
                case 0x9F:   // setcc r/m8
                case 0xAF:   // imul r32, r/m32
                case 0xB6:   // movzx r32, r/m8
                case 0xB7:   // movzx r32, r/m16
                case 0xBE:   // movsx r32, r/m8
                case 0xBF:   // movsx r32, r/m16
                case 0xEF: { // pxor mm, mm/m64
                    return {Prefix.Size, modrm_size(Start, 2+Prefix.Size)};
                }
            }
        }
    }

    invalid_instruction(Code);
}

size_pair code_size(const void* Start){
    std::size_t i = 0;
    std::size_t j = 0;
    while(i < JmpSize){
        auto Code = static_cast<const unsigned char*>(Start)+i;
        auto Info = instruction_info(Code);
        if(Info.Copy == 0){
            invalid_instruction(Code);
        }
        i += Info.Size;
        j += Info.Copy;
    }

    return {i, j};
}

std::size_t any_hook::make(void* Buffer_){
    assert(!IsSet);

    auto Code = reinterpret_cast<unsigned char*>(Function);

    auto CodeSizes = code_size(Function);
    CodeSize = CodeSizes.Size;

    auto Buffer = reinterpret_cast<unsigned char*>(Buffer_);

    std::size_t i = 0;

    OriginalCode = Buffer+i;

    std::memcpy(Buffer+i, Code, CodeSize);

    i += CodeSize;

    Trampoline = Buffer+i;

    auto Copied = copy_code(Buffer+i, Code, CodeSize);
    assert(Copied == CodeSizes.Copy);

    i += Copied;

    i += write_jmp(Buffer+i, Code+CodeSize);

    FlushInstructionCache(GetCurrentProcess(), Trampoline, CodeSize+Copied);

    return i;
}

namespace {

void set_hook(void* Code_, const void* Detour, std::size_t Size){
    auto Code = reinterpret_cast<unsigned char*>(Code_);

    DWORD OldProtect;
    if(!VirtualProtect(Code, Size, PAGE_EXECUTE_READWRITE, &OldProtect)){
        std::abort();
    }

    write_jmp(Code, Detour);
    std::memset(Code+5, 0xCC, Size-5); // Fill rest with int3.

    FlushInstructionCache(GetCurrentProcess(), Code, Size);

    if(!VirtualProtect(Code, Size, OldProtect, &OldProtect)){
        std::abort();
    }
}

void reset_hook(void* Code_, const void* OriginalCode, std::size_t Size){
    auto Code = reinterpret_cast<unsigned char*>(Code_);

    DWORD OldProtect;
    if(!VirtualProtect(Code, Size, PAGE_EXECUTE_READWRITE, &OldProtect)){
        std::abort();
    }

    std::memcpy(Code, OriginalCode, Size);

    FlushInstructionCache(GetCurrentProcess(), Code, Size);

    if(!VirtualProtect(Code, Size, OldProtect, &OldProtect)){
        std::abort();
    }
}

}

void any_hook::set(const void* Detour){
    set_hook(Function, Detour, CodeSize);
    IsSet = 1;
}

void any_hook::reset(){
    reset_hook(Function, OriginalCode, CodeSize);
    IsSet = 0;
}

std::size_t log_hook::make(void* Buffer_, const void* Detour, const void* Data){
    assert(!IsSet);

    auto Code = reinterpret_cast<unsigned char*>(Function);

    auto CodeSizes = code_size(Function);
    CodeSize = CodeSizes.Size;

    auto Buffer = reinterpret_cast<unsigned char*>(Buffer_);

    std::size_t i = 0;

    OriginalCode = Buffer+i;

    std::memcpy(Buffer+i, Code, CodeSize);

    i += CodeSize;

    auto Tbuf = Buffer+i;
    Trampoline = Tbuf;

    std::size_t j = 0;

    // pusha
    Tbuf[j++] = 0x60;

    // mov eax, esp
    Tbuf[j++] = 0x89;
    Tbuf[j++] = 0xE0;

    // push Data
    Tbuf[j++] = 0x68;
    std::memcpy(Tbuf+j, &Data, sizeof(Data));
    j += sizeof(Data);

    // push eax
    Tbuf[j++] = 0x50;

    // call Detour
    j += write_jmp(Tbuf+j, Detour, 0xE8u);

    // add esp, 0x8
    Tbuf[j++] = 0x83;
    Tbuf[j++] = 0xC4;
    Tbuf[j++] = 0x08;

    // popa
    Tbuf[j++] = 0x61;

    auto Copied = copy_code(Tbuf+j, Code, CodeSize);
    assert(Copied == CodeSizes.Copy);

    j += Copied;

    j += write_jmp(Tbuf+j, Code+CodeSize);

    assert(j == LogHookPayloadSize+Copied+JmpSize);

    FlushInstructionCache(GetCurrentProcess(), Trampoline, j);

    return i+j;
}

void log_hook::set(){
    set_hook(Function, Trampoline, CodeSize);
    IsSet = 1;
}

void log_hook::reset(){
    reset_hook(Function, OriginalCode, CodeSize);
    IsSet = 0;
}

}
