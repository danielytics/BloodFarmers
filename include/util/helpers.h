#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <functional>
#include <memory>

namespace helpers {

template <typename ContainerType>
void remove(ContainerType& container, std::size_t index)
{
    auto it = container.begin() + index;
    auto last = container.end() - 1;
    if (it != last) {
        // If not the last item, move the last into this element
        *it = std::move(*last);
    }
    // Remove the last item in the container
    container.pop_back();
}

template <typename ContainerType>
void move_back_and_replace(ContainerType& container, std::size_t index, typename ContainerType::reference&& data)
{
    container.push_back(std::move(container[index]));
    container[index] = std::move(data);
}

// If elements in 'in' have a deleted copy ctor, then _inserter may not work when compiling with EASTL, this is a workaround
template <typename InputContainer, typename OutputContainer>
void move_back (InputContainer& in, OutputContainer& out)
{
    for (auto& item : in) {
        out.push_back(std::move(item));
    }
}

template <typename ContainerType>
void pad_with (ContainerType& container, std::size_t size, typename ContainerType::value_type value)
{
    while (container.size() < size) {
        container.push_back(value);
    }
}

std::string readToString(const std::string& filename);

struct exit_scope_obj {
    template <typename Lambda>
    exit_scope_obj(Lambda& f) : func(f) {}
    template <typename Lambda>
    exit_scope_obj(Lambda&& f) : func(std::move(f)) {}
    ~exit_scope_obj() {func();}
private:
    std::function<void()> func;
};
#define CONCAT_IDENTIFIERS_(a,b) a ## b
#define ON_SCOPE_EXIT_(name,num) helpers::exit_scope_obj CONCAT_IDENTIFIERS_(name, num)
#define on_exit_scope ON_SCOPE_EXIT_(exit_scope_obj_, __LINE__)

template <typename T, typename Ctor, typename Dtor>
struct unique_maker_obj {
    explicit unique_maker_obj (Ctor ctor, Dtor dtor) : ctor(ctor), dtor(dtor) {}

    template <typename... Args>
    std::unique_ptr<T, Dtor> construct (Args... args) {
        return std::unique_ptr<T, Dtor>(ctor(args...), dtor);
    }
private:
    Ctor ctor;
    Dtor dtor;
};

template <typename T, typename Ctor>
unique_maker_obj<T, Ctor, void(*)(T*)> ptr (Ctor ctor) {
    return unique_maker_obj<T, Ctor, void(*)(T*)>(ctor, [](T* p){ delete p; });
}
template <typename T, typename Ctor, typename Dtor>
unique_maker_obj<T, Ctor, void(*)(T*)> ptr (Ctor ctor, Dtor dtor) {
    return unique_maker_obj<T, Ctor, void(*)(T*)>(ctor, dtor);
}
template <typename T>
std::unique_ptr<T> ptr (T* raw) {
    return std::unique_ptr<T>(raw);
}

template <typename T = char>
inline T* align(void* pointer, uintptr_t bytes_alignment) {
    intptr_t value = reinterpret_cast<intptr_t>(pointer);
    value += (-value) & (bytes_alignment - 1);
    return reinterpret_cast<T*>(value);
}

template <typename T>
inline T roundDown(T n, T m) {
    return n >= 0 ? (n / m) * m : ((n - m + 1) / m) * m;
}

}


#endif // HELPERS_H
