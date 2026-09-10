// Corpus for traits_typedef.k, nested_inst.k, native_arg.k: a container
// whose size_type is a DEPENDENT member typedef reached through a traits
// class -- std::vector's size_type through allocator_traits, in miniature.
#pragma once

template <class A> struct traits { using size_type = typename A::st; };
struct alloc { using st = unsigned long; };
template <class T, class A = alloc> struct cont {
    using size_type = typename traits<A>::size_type;
    T* first;
    size_type n;
    size_type size() const;
    void push(const T&);
};
