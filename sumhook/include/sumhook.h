#ifndef SUMHOOK_H_INCLUDED
    #define SUMHOOK_H_INCLUDED 1

#include <memory>
#include <utility>
#include <type_traits>

#include <cassert>

#include <windows.h>

static_assert(sizeof(void*) == 4);

namespace smhk {

constexpr std::size_t JmpSize = 5;
constexpr std::size_t LogHookPayloadSize = 18;

template <typename T>
struct is_function_pointer:std::false_type {};

template <typename T>
struct is_function_pointer<T*>:std::is_function<T> {};

template <typename T>
concept function_pointer = is_function_pointer<T>::value;

template <typename T>
concept member_function = std::is_member_function_pointer<T>::value;

template <typename T>
concept hookable = function_pointer<T> || member_function<T>;

struct any_hook;

struct hook_prep {
    any_hook* Hook;
    const void* Detour;
};

inline void* fun_cast(function_pointer auto p){
    return reinterpret_cast<void*>(p);
}

template <function_pointer T>
inline T fun_cast(const void* p){
    return reinterpret_cast<T>(p);
}

inline void* fun_cast(member_function auto p){
    void* r[sizeof(p)/sizeof(void*)];
    static_assert(sizeof(r) == sizeof(p));
    std::memcpy(r, &p, sizeof(r));

    return r[0];
}

template <member_function T>
inline T fun_cast(const void* p){
    const void* a[sizeof(T)/sizeof(void*)] = {};
    static_assert(sizeof(a) == sizeof(T));

    a[0] = p;

    T r;
    std::memcpy(&r, a, sizeof(r));
    return r;
}

struct any_hook {
    any_hook()=default;

    explicit any_hook(void* Function)
        :Function(Function), CodeSize(0), OriginalCode(nullptr), Trampoline(nullptr) {}

    template <hookable T>
    explicit any_hook(T Function):any_hook(fun_cast(Function)) {}

    std::size_t make(void* Buffer);

    void set(const void* Detour);
    void reset();

    template <hookable T>
    void set(T Detour){
        set(fun_cast(Detour));
    }

    template <member_function T>
    void set(T Detour){
        set(from_mfn(Detour));
    }

    explicit operator bool() const {
        return Trampoline != nullptr;
    }

    hook_prep prepare(void* Func, const void* Detour){
        Function = Func;
        return {this, Detour};
    }

    template <hookable T>
    hook_prep prepare(T Func, T Detour){
        return prepare(fun_cast(Func), fun_cast(Detour));
    }

    void* Function;

    std::size_t CodeSize:(CHAR_BIT*sizeof(std::size_t))-1;
    std::size_t IsSet:1;

    const void* OriginalCode;
    const void* Trampoline;
};

template <hookable T>
struct hook:any_hook {
    hook()=default;

    explicit hook(T Function):any_hook(Function) {}

    T set(T Detour){
        this->set(fun_cast(Detour));
    }

    hook_prep prepare(T Func, T Detour){
        return any_hook::prepare(fun_cast(Func), fun_cast(Detour));
    }

    operator T() const {
        return fun_cast<T>(Trampoline);
    }
};

template <hookable T>
struct unique_hook:hook<T> {
    using base_type = hook<T>;

    constexpr unique_hook(std::nullptr_t = nullptr):base_type() {}

    explicit unique_hook(T Function):base_type(Function) {}

    unique_hook(unique_hook&& rhs) noexcept :hook(rhs) {
        static_cast<hook&>(rhs) = {};
    }

    unique_hook& operator=(unique_hook&& rhs) noexcept {
        if(this->IsSet){
            this->reset();
        }

        static_cast<hook&>(*this) = static_cast<hook&>(rhs);
        static_cast<hook&>(rhs) = {};

        return *this;
    }

    ~unique_hook(){
        if(this->IsSet){
            this->reset();
        }
    }
};

struct log_stack {
    std::uint32_t Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    void* Ret;
};

template <typename T>
using log_detour = void(*)(log_stack* Stack, T* Data);

struct log_hook;

struct log_hook_prep {
    log_hook* Hook;
    const void* Detour;
    const void* Data;
};

struct log_hook {
    log_hook()=default;

    explicit log_hook(void* Function)
        :Function(Function), CodeSize(0), OriginalCode(nullptr), Trampoline(nullptr) {}

    template <hookable T>
    explicit log_hook(T* Function):log_hook(reinterpret_cast<void*>(Function)) {}

    std::size_t make(void* Buffer, const void* Detour, const void* Data);

    template <typename T>
    std::size_t make(void* Buffer, log_detour<T> Detour, T* Data){
        return make(Buffer, reinterpret_cast<const void*>(Detour), Data);
    }

    void set();
    void reset();

    explicit operator bool() const {
        return Trampoline != nullptr;
    }

    log_hook_prep prepare(void* Func, const void* Detour, const void* Data){
        Function = Func;
        return {this, Detour, Data};
    }

    template <typename U>
    log_hook_prep prepare(void* Func, log_detour<U> Detour, U* Data){
        return prepare(Func, reinterpret_cast<const void*>(Detour), Data);
    }

    template <hookable T, typename U>
    log_hook_prep prepare(T* Func, log_detour<U> Detour, U* Data){
        return prepare(reinterpret_cast<void*>(Func), reinterpret_cast<const void*>(Detour), Data);
    }

    void* Function;

    std::size_t CodeSize:(CHAR_BIT*sizeof(std::size_t))-1;
    std::size_t IsSet:1;

    const void* OriginalCode;
    const void* Trampoline;
};

struct unique_log_hook:log_hook {
    using base_type = log_hook;

    constexpr unique_log_hook(std::nullptr_t = nullptr):base_type() {}

    template <hookable T>
    explicit unique_log_hook(T* Function): base_type(Function) {}

    unique_log_hook(unique_log_hook&& rhs) noexcept :log_hook(rhs) {
        static_cast<log_hook&>(rhs) = {};
    }

    unique_log_hook& operator=(unique_log_hook&& rhs) noexcept {
        if(this->IsSet){
            this->reset();
        }

        static_cast<log_hook&>(*this) = static_cast<log_hook&>(rhs);
        static_cast<log_hook&>(rhs) = {};

        return *this;
    }

    ~unique_log_hook(){
        if(this->IsSet){
            this->reset();
        }
    }
};

struct instr_info {
    instr_info()=default;

    instr_info(std::size_t Prefix, std::size_t Size, std::size_t Relative = 0)
        : instr_info(Prefix, Size, Relative, Size) {}

    instr_info(std::size_t Prefix, std::size_t Size, std::size_t Relative, std::size_t Copy)
        : Prefix(static_cast<std::uint8_t>(Prefix))
        , Size(static_cast<std::uint8_t>(Size))
        , Relative(static_cast<std::uint8_t>(Relative))
        , Copy(static_cast<std::uint8_t>(Copy)) {}

    std::uint8_t Prefix, Size, Relative, Copy;
};

instr_info instruction_info(const void* Code);

struct size_pair {
    std::size_t Size, Copy;
};

size_pair code_size(const void* Code);

size_pair copy_instruction(unsigned char* To, const unsigned char* From);
std::size_t copy_code(unsigned char* To, const unsigned char* From, std::size_t Size);

struct alloc_wrapper {
    static unsigned char* alloc(std::size_t Size){
        void* r;

        if(Size == 0){
            r = VirtualAlloc(nullptr, 1, MEM_COMMIT|MEM_RESERVE, PAGE_NOACCESS);
        }else{
            r = VirtualAlloc(nullptr, Size, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        }

        if(!r){
            throw std::bad_alloc();
        }

        return static_cast<unsigned char*>(r);
    }

    void operator()(void* p) const {
        VirtualFree(p, 0, MEM_RELEASE);
    }
};

using unique_buffer = std::unique_ptr<unsigned char[], alloc_wrapper>;

inline unique_buffer create_hooks(const hook_prep Hooks[], std::size_t Count){
    std::size_t Size = 0;
    for(std::size_t i = 0; i < Count; ++i){
        auto Sizes = code_size(Hooks[i].Hook->Function);
        Size += Sizes.Size+Sizes.Copy+JmpSize;
    }

    unique_buffer r(alloc_wrapper::alloc(Size));
    auto it = static_cast<unsigned char*>(r.get());

    for(std::size_t i = 0; i < Count; ++i){
        auto [Hook, Detour] = Hooks[i];
        it += Hook->make(it);
        Hook->set(Detour);
    }

    assert(it == r.get()+Size);

    return r;
}

inline unique_buffer create_hooks(const log_hook_prep Hooks[], std::size_t Count){
    std::size_t Size = 0;
    for(std::size_t i = 0; i < Count; ++i){
        auto Sizes = code_size(Hooks[i].Hook->Function);
        Size += Sizes.Size+Sizes.Copy+LogHookPayloadSize+JmpSize;
    }

    unique_buffer r(alloc_wrapper::alloc(Size));
    auto it = static_cast<unsigned char*>(r.get());

    for(std::size_t i = 0; i < Count; ++i){
        auto [Hook, Detour, Data] = Hooks[i];
        it += Hook->make(it, Detour, Data);
        Hook->set();
    }

    assert(it == r.get()+Size);

    return r;
}

template <typename T, std::size_t N>
inline unique_buffer create_hooks(const T (&Hooks)[N]){
    return create_hooks(Hooks, N);
}

template <typename T>
inline unique_buffer create_hooks(const T& t){
    return create_hooks(t.data(), t.size());
}

}

#endif // SUMHOOK_H_INCLUDED
