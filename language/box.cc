/// --- The Helix Project ---------------------------------------------------------------------- ///
///                                                                                              ///
///   Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0).    ///
///   You are allowed to use, modify, redistribute, and create derivative works, even for        ///
///   commercial purposes, provided that you give appropriate credit, and indicate if changes    ///
///   were made.                                                                                 ///
///                                                                                              ///
///   For more information on the license terms and requirements, please visit:                  ///
///     https://creativecommons.org/licenses/by/4.0/                                             ///
///                                                                                              ///
///   SPDX-License-Identifier: CC-BY-4.0                                                         ///
///   Copyright (c) 2024 The Helix Project (CC BY 4.0)                                           ///
///                                                                                              ///
/// ------------------------------------------------------------------------------------ C++ --- ///

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --------------------------------------- core/new.hlx --------------------------------------- //
/// custom placement new operator
///
/// constructs an object of type `T` in the specified memory region `ptr`.
/// the operator takes a pointer to allocated memory and constructs `T`
/// in-place using the provided arguments.
///
/// \param ptr pointer to the pre-allocated memory where `T` will be constructed
/// \return the pointer `ptr` as a pointer to `T`
void *operator new(size_t /*unused*/, void *ptr) noexcept { return ptr; }
// ---------------------------------------------------------------------------------------------- //

// --------------------------------------- exceptions.hlx --------------------------------------- //
namespace exceptions {
class _Error {  // NOLINT
    const char *msg;
    const char *loc;

  public:
    explicit _Error(const char *loc, const char *msg);
    ~_Error();

    [[nodiscard]] const char        *what() const;
    [[nodiscard]] const char        *where() const;
    [[nodiscard]] static const char *make_loc(const char *file, const char *func);
};

inline _Error Error(const char *msg);
}  // namespace exceptions
// ---------------------------------------------------------------------------------------------- //

// ---------------------------------- memory/heap/internal.hlx ---------------------------------- //
namespace memory::_internal {
struct Allocation {
    enum class Type {
        None,
        Stack,
        Heap,
    };

    void       *ptr     = nullptr;
    size_t      size    = 0;
    bool        active  = false;
    Type        alloc_t = Type::None;
    Allocation *next    = nullptr;
};

class MemoryTable {
  private:
    Allocation **table;
    size_t       table_size;

  public:
    explicit MemoryTable(size_t size);

    MemoryTable(const MemoryTable &other)                = delete;
    MemoryTable &operator=(const MemoryTable &other)     = delete;
    MemoryTable(MemoryTable &&other) noexcept            = delete;
    MemoryTable &operator=(MemoryTable &&other) noexcept = delete;

    ~MemoryTable();

    void        insert(void *ptr, size_t size, Allocation::Type alloc_t);
    void        remove(void *ptr, Allocation::Type alloc_t);
    Allocation *find(void *ptr, Allocation::Type alloc_t) const;

    [[nodiscard]] size_t hash(void *key) const;

    [[nodiscard]] size_t       size() const;
    [[nodiscard]] Allocation **get_table() const;
};

constexpr size_t MAX_CHAIN_LENGTH = 4;  // maximum chain length to maintain O(1) operations
MemoryTable      memory_table(128);     // NOLINT
}  // namespace memory::_internal
// ---------------------------------------------------------------------------------------------- //

// ------------------------------------ memory/heap/heap.hlx ------------------------------------ //
namespace memory {
/// forward declaration for `drop` and `forget` to be defined.
template <class T>
class Heap;

/// removes memory management tracking from a `Heap` object without freeing memory
///
/// this function "forgets" the managed object inside the provided `Heap` instance,
/// effectively disabling RAII cleanup. it preserves the object's data but unlinks
/// it from automatic memory management, allowing for manual control if needed.
///
/// \tparam T the type of the object managed by `Heap`
/// \param mem reference to the `Heap` object to be forgotten
/// \throws exceptions::Error if the `Heap` object is not found in the memory table
template <typename T>
void forget(memory::Heap<T> &mem);

/// drops the managed memory of a `Heap` object, forcibly deallocating its resource
///
/// `drop` directly invokes the destructor and cleanup mechanisms of the managed object
/// within the specified `Heap` instance. this is used to immediately release resources,
/// bypassing RAII in cases where the object should be cleaned up manually.
///
/// \tparam T the type of the object managed by `Heap`
/// \param mem reference to the `Heap` object to be deallocated
/// \throws exceptions::Error if the `Heap` object is not found in the memory table
template <typename T>
void drop(memory::Heap<T> &mem);

/// a wrapper for dynamically allocating and managing a single instance of a class `T` on the heap
///
/// the `Heap` class provides RAII-based memory management for a heap-allocated object of type `T`,
/// enabling both automatic and manual memory control. by default, it registers and tracks the
/// allocated object, ensuring proper cleanup when the `Heap` instance goes out of scope.
/// additionally, `Heap` supports advanced memory management through the `memory` API for scenarios
/// requiring manual control.
///
/// example usage:
/// \code
/// class Foo {
///     int x;
/// public:
///     explicit Foo(int x) : x(x) {}
/// };
///
/// memory::Heap<Foo> foo(123);  // constructs and allocates Foo on the heap with x = 123
/// \endcode
///
/// \tparam T the type of the object to be allocated on the heap
/// \note
/// - copy operations are disabled to prevent multiple ownership of the heap-allocated resource
/// - move operations are allowed, transferring ownership without duplicating the underlying object
/// - this class is compatible with `memory::drop` & `memory::forget` to allow bypassing the default
///   RAII behavior
///
/// \throws exceptions::Error thrown on memory allocation failure, dereferencing a null
/// pointer, or dereferencing a deleted object
template <class T>
class Heap : public T {
    T   *_ptr;              ///! pointer to the managed object on the heap
    bool is_active = true;  ///! indicates if the managed object is active or "forgotten"

    /// allocates memory on the heap for a single instance of `T` without initialization
    ///
    /// this function uses `malloc` to allocate uninitialized memory for `T`.
    /// if allocation fails, it throws an exception.
    ///
    /// \return pointer to the allocated memory
    /// \throws exceptions::Error if memory allocation fails
    static T *_alloc();

    /// allocates and constructs an instance of `T` with the provided arguments
    ///
    /// this function uses placement-new to construct an object of type `T` in the allocated memory,
    /// forwarding the provided arguments.
    ///
    /// \tparam Args variadic template for constructor arguments
    /// \param args arguments to forward to the constructor of `T`
    /// \return pointer to the newly constructed object
    /// \throws exceptions::Error if memory allocation fails
    template <typename... Args>
    static T *_alloc_new(Args &&...args);

    /// frees the managed memory, if applicable, based on the object's active status
    ///
    /// this function checks if the managed object is registered with `memory` and performs a
    /// cleanup. if `ignore_forgotten` is set, it cleans up regardless of the memory tracking
    /// status.
    ///
    /// \param ignore_forgotten if true, ignores the memory tracking state and attempts to free the
    /// memory
    void _delete(bool ignore_forgotten = false);

    friend void memory::drop(memory::Heap<T> &mem);  //! grants memory::drop access to is_active

  public:
    /// constructs an instance of `T` on the heap, forwarding the provided arguments
    ///
    /// the constructor allocates and constructs the managed object with RAII enabled.
    /// the allocated memory is registered with `memory`.
    ///
    /// \tparam Args variadic template for constructor arguments
    /// \param args arguments to forward to the constructor of `T`
    template <typename... Args>
    explicit Heap(Args &&...args);

    // deleted copy constructor and copy assignment operator to prevent multiple ownership
    Heap(const Heap &)            = delete;
    Heap &operator=(const Heap &) = delete;

    // move assignment operator
    Heap &operator=(Heap &&) = default;

    /// move constructor transfers ownership of the managed object
    ///
    /// \param other rvalue reference to another Heap object
    Heap(Heap &&other) noexcept;

    /// destructor that cleans up the managed memory unless it has been manually forgotten
    ~Heap();

    /// dereferences the managed pointer
    ///
    /// \return reference to the managed object
    /// \throws exceptions::Error if `_ptr` is null or the object is inactive
    T &operator*() const;

    /// provides a pointer to the managed object
    ///
    /// \return pointer to the managed object
    /// \throws exceptions::Error if `_ptr` is null or the object is inactive
    T *operator&() const;

    /// equality comparison operator with another Heap
    ///
    /// \param other another Heap instance
    /// \return true if both manage the same memory address
    bool operator==(const Heap &other) const;

    /// equality comparison with nullptr
    ///
    /// \param nullptr
    /// \return true if the managed pointer is null
    bool operator==(::nullptr_t) const;

    /// deletes the managed object, ignoring memory tracking state
    /// used for scenarios requiring direct deletion without regard to tracking.
    ///
    /// \param ptr pointer to the Heap object
    static void operator delete(void *ptr);
};
}  // namespace memory
// ---------------------------------------------------------------------------------------------- //

// ------------------------------------ memory/heap/utils.hlx ----------------------------------- //
namespace memory {
/// registers a pointer to a dynamically allocated object for memory management
///
/// the function tracks the specified pointer `ptr` in the internal memory table,
/// allowing the memory manager to automatically free or monitor the object.
///
/// \tparam T the type of the object pointed to by `ptr`
/// \param ptr pointer to the object to be registered
/// \param alloc_t type of allocation to maintain safe memory records
/// \throws exceptions::Error if registration fails (e.g., memory allocation failure)
template <typename T>
void register_t(T *ptr, _internal::Allocation::Type alloc_t);

/// frees a dynamically allocated object registered with the memory manager
///
/// this function releases the memory associated with `ptr` if it is active and managed
/// by the memory system. it checks if the object can be destructed using RAII principles
/// and deletes accordingly, otherwise it directly frees the memory.
///
/// \tparam T the type of the object pointed to by `ptr`
/// \param ptr pointer to the object to be freed
/// \param alloc_t type of allocation to maintain safe memory records
/// \throws exceptions::Error if `ptr` is not found in the memory table or has been forgotten
template <typename T>
void free_t(T *ptr, _internal::Allocation::Type alloc_t);

/// checks if a pointer to a dynamically allocated object is registered and active
///
/// determines if `ptr` is currently tracked by the memory manager and has not
/// been manually forgotten or deactivated. useful for verifying whether an object
/// is still managed before performing operations on it.
///
/// \tparam T the type of the object pointed to by `ptr`
/// \param ptr pointer to the object to be checked
/// \param alloc_t type of allocation to maintain safe memory records
/// \return true if `ptr` is active and managed; false otherwise
template <typename T>
bool is_alloc_t(T *ptr, _internal::Allocation::Type alloc_t);

/// removes a pointer from memory management without freeing its memory
///
/// this function "forgets" the specified pointer `ptr` in the memory system,
/// disabling automatic cleanup and deallocating any tracking metadata.
/// useful for transitioning from automatic to manual memory management.
///
/// \tparam T the type of the object pointed to by `ptr`
/// \param ptr pointer to the object to be forgotten
/// \param alloc_t type of allocation to maintain safe memory records
/// \throws exceptions::Error if `ptr` is not found in the memory table
template <typename T>
void forget_t(T *ptr, _internal::Allocation::Type alloc_t);
}  // namespace memory
// ---------------------------------------------------------------------------------------------- //

// ------------------------------------- refences/utils.hlx ------------------------------------- //
namespace std::utils {
/// struct to remove reference qualifiers from a type `T`
///
/// `remove` is a type trait that provides a nested typedef `t` which removes reference
/// qualifiers from the type `T`. it is commonly used in template metaprogramming
/// for handling types in a standardized form.
///
/// \tparam T the type from which to remove the reference qualifier
template <typename T>
struct remove;

/// specialization of `remove` for lvalue reference types
///
/// this specialization provides the typedef `t` which removes the lvalue reference
/// from the type `T &`. it is used to standardize type handling when passing
/// arguments in templates or forwarding references.
///
/// \tparam T the type from which to remove the lvalue reference
template <typename T>
struct remove<T &>;

/// specialization of `remove` for rvalue reference types
///
/// this specialization provides the typedef `t` which removes the rvalue reference
/// from the type `T &&`. it is commonly used to simplify type manipulation in
/// templated functions and metaprogramming constructs.
///
/// \tparam T the type from which to remove the rvalue reference
template <typename T>
struct remove<T &&>;
}  // namespace std::utils

namespace std {
/// forwards an lvalue or rvalue as either an lvalue or rvalue, preserving reference qualifiers
///
/// `forward` is used to forward an argument of type `T`, preserving its lvalue or
/// rvalue nature, which is especially useful in perfect forwarding scenarios in
/// templated functions. it utilizes the type trait `utils::remove` to strip
/// any reference qualifiers from `T`.
///
/// \tparam T the deduced type of the parameter to forward
/// \param t the value to be forwarded
/// \return `t` forwarded as either an lvalue or rvalue, depending on the reference qualifiers of
/// `T`
/// \throws noexcept this function is marked `noexcept` to ensure it does not throw
template <typename T>
T &&forward(typename utils::remove<T>::t &t) noexcept;
}  // namespace std
// ---------------------------------------------------------------------------------------------- //

/// ∧∧∧∧∧∧∧∧∧∧∧∧∧∧∧∧ end of declarations -------------- start of implementations vvvvvvvvvvvvvvvv //

// --------------------------------------- exceptions.hlx --------------------------------------- //
namespace exceptions {
const char *_Error::make_loc(const char *file, const char *func) {
    // only libc functions are used here
    char *loc = (char *)malloc(strlen(file) + strlen(func) + 3);  // NOLINT
    strcpy(loc, file);
    strcat(loc, "::");
    strcat(loc, func);
    return loc;
}

_Error::_Error(const char *loc, const char *msg)
    : msg(msg)
    , loc(loc) {}

const char *_Error::what() const { return msg; }
const char *_Error::where() const { return loc; }

_Error::~_Error() { free((void *)loc); }  // NOLINT

inline _Error Error(const char *msg) { return _Error(_Error::make_loc(__FILE__, __func__), msg); }
}  // namespace exceptions
// ---------------------------------------------------------------------------------------------- //

// ---------------------------------- memory/heap/internal.hlx ---------------------------------- //
namespace memory::_internal {
MemoryTable::MemoryTable(size_t size)
    : table_size(size) {
    table = (Allocation **)malloc(sizeof(Allocation *) * table_size);  // NOLINT
    memset(table, 0, sizeof(Allocation *) * table_size);
}

MemoryTable::~MemoryTable() {
    for (size_t i = 0; i < table_size; ++i) {
        Allocation *current = table[i];  // NOLINT

        while (current != nullptr) {
            Allocation *next = current->next;

            if (current->active) {
                free(current->ptr);  // NOLINT
            }

            free(current);  // NOLINT
            current = next;
        }
    }

    free(table);  // NOLINT
}

void MemoryTable::insert(void *ptr, size_t size, Allocation::Type alloc_t) {
    size_t      index        = hash(ptr);
    Allocation *current      = table[index];  // NOLINT
    size_t      chain_length = 0;

    while (current != nullptr) {
        if (current->ptr == ptr) {
            throw exceptions::Error("Duplicate pointer insertion attempted.");
        }

        current = current->next;
        chain_length++;
    }

    if (chain_length >= MAX_CHAIN_LENGTH) {
        throw exceptions::Error("Exceeded maximum chain length, hash table is full.");
    }

    Allocation *new_alloc = (Allocation *)malloc(sizeof(Allocation));  // NOLINT

    new_alloc->ptr     = ptr;
    new_alloc->size    = size;
    new_alloc->active  = true;
    new_alloc->alloc_t = alloc_t;

    new_alloc->next = table[index];  // NOLINT
    table[index]    = new_alloc;     // NOLINT
}

Allocation *MemoryTable::find(void *ptr, Allocation::Type alloc_t) const {
    size_t      index        = hash(ptr);
    Allocation *current      = table[index];  // NOLINT
    size_t      chain_length = 0;

    while (current != nullptr && chain_length < MAX_CHAIN_LENGTH) {
        if (current->ptr == ptr && current->alloc_t == alloc_t) {
            return current;
        }

        current = current->next;
        chain_length++;
    }
    return nullptr;
}

void MemoryTable::remove(void *ptr, Allocation::Type alloc_t) {
    size_t      index        = hash(ptr);
    Allocation *current      = table[index];  // NOLINT
    Allocation *previous     = nullptr;
    size_t      chain_length = 0;

    while (current != nullptr && chain_length < MAX_CHAIN_LENGTH) {
        if (current->ptr == ptr && current->alloc_t == alloc_t) {
            if (previous == nullptr) {
                table[index] = current->next;  // NOLINT
            } else {
                previous->next = current->next;
            }

            free(current->ptr);  // NOLINT
            free(current);       // NOLINT

            return;
        }

        previous = current;
        current  = current->next;

        chain_length++;
    }

    throw exceptions::Error("Pointer not found for removal.");
}

size_t MemoryTable::hash(void *key) const {
    size_t h = reinterpret_cast<size_t>(key);  // NOLINT

    h ^= (h >> 20) ^ (h >> 12);
    h = h ^ (h >> 7) ^ (h >> 4);

    return h % table_size;
}

size_t       MemoryTable::size() const { return table_size; }
Allocation **MemoryTable::get_table() const { return table; }
}  // namespace memory::_internal
// ---------------------------------------------------------------------------------------------- //

// ------------------------------------ memory/heap/heap.hlx ------------------------------------ //
namespace memory {
template <typename T>
void forget(Heap<T> &mem) {
    forget_t(&mem, _internal::Allocation::Type::Heap);
}

template <typename T>
void drop(Heap<T> &mem) {
    if constexpr (__is_destructible(T)) {
        delete static_cast<T *>(&mem);  // NOLINT
    } else {
        free(&mem);
    }

    mem.is_active = false;
}

template <class T>
T *Heap<T>::_alloc() {
    void *memory = malloc(sizeof(T));  // NOLINT

    if (memory == nullptr) {
        throw exceptions::Error("failed to allocate memory");
    }

    return static_cast<T *>(memory);
}

template <class T>
template <typename... Args>
T *Heap<T>::_alloc_new(Args &&...args) {
    T *ptr = _alloc();
    new (ptr) T(std::forward<Args>(args)...);

    return ptr;
}

template <class T>
void Heap<T>::_delete(bool ignore_forgotten) {
    if (_ptr != nullptr) {
        bool is_found = is_alloc_t(_ptr, _internal::Allocation::Type::Heap);

        if (ignore_forgotten) {
            if (!is_found) {
                drop<T>(*this);
                is_active = false;
            }
        }

        if (is_found) {
            free_t(_ptr, _internal::Allocation::Type::Heap);
            is_active = false;
        }
    }
}

template <class T>
template <typename... Args>
Heap<T>::Heap(Args &&...args)
    : _ptr(_alloc_new(std::forward<Args>(args)...)) {
    register_t(_ptr, _internal::Allocation::Type::Heap);
}

template <class T>
Heap<T>::Heap(Heap &&other) noexcept
    : _ptr(other._ptr) {
    other._ptr = nullptr;
}

template <class T>
Heap<T>::~Heap() {
    _delete(false);
}

template <class T>
T &Heap<T>::operator*() const {
    if (!_ptr) {
        throw exceptions::Error("dereferencing a nullptr");
    }

    if (!is_active) {
        throw exceptions::Error("dereferencing a deleted object");
    }
    return *_ptr;
}

template <class T>
T *Heap<T>::operator&() const {
    if (!_ptr) {
        throw exceptions::Error("associated heap memory is null");
    }

    if (!is_active) {
        throw exceptions::Error("dereferencing a deleted object");
    }
    return this->_ptr;
}

template <class T>
bool Heap<T>::operator==(const Heap &other) const {
    return _ptr == other._ptr;
}

template <class T>
bool Heap<T>::operator==(::nullptr_t) const {
    return _ptr == nullptr;
}

template <class T>
void Heap<T>::operator delete(void *ptr) {
    static_cast<Heap *>(ptr)->_delete(true);
}
}  // namespace memory
// ---------------------------------------------------------------------------------------------- //

// ------------------------------------ memory/heap/utils.hlx ----------------------------------- //
namespace memory {
template <typename T>
void register_t(T *ptr, _internal::Allocation::Type alloc_t) {
    void                  *void_ptr = static_cast<void *>(ptr);
    size_t                 index    = _internal::memory_table.hash(void_ptr);
    _internal::Allocation *current  = _internal::memory_table.get_table()[index];  // NOLINT

    while (current != nullptr) {
        if (current->ptr == void_ptr && current->alloc_t == alloc_t) {
            current->size   = sizeof(T);
            current->active = true;
            return;
        }

        current = current->next;
    }

    _internal::memory_table.insert(void_ptr, sizeof(T), alloc_t);
}

template <typename T>
void free_t(T *ptr, _internal::Allocation::Type alloc_t) {
    void                  *void_ptr = static_cast<void *>(ptr);
    size_t                 index    = _internal::memory_table.hash(void_ptr);
    _internal::Allocation *current  = _internal::memory_table.get_table()[index];  // NOLINT
    _internal::Allocation *previous = nullptr;

    while (current != nullptr) {
        if (current->ptr == void_ptr && current->alloc_t == alloc_t) {
            if (current->active) {
                if constexpr (__is_destructible(T)) {
                    delete static_cast<T *>(current->ptr);  // NOLINT
                } else {
                    free(current->ptr);  // NOLINT
                }

                if (previous == nullptr) {
                    _internal::memory_table.get_table()[index] = current->next;  // NOLINT
                } else {
                    previous->next = current->next;
                }

                free(current);  // NOLINT
            }

            return;
        }

        previous = current;
        current  = current->next;
    }
}

template <typename T>
bool is_alloc_t(T *ptr, _internal::Allocation::Type alloc_t) {
    void                  *void_ptr = static_cast<void *>(ptr);
    size_t                 index    = _internal::memory_table.hash(void_ptr);
    _internal::Allocation *current  = _internal::memory_table.get_table()[index];  // NOLINT

    while (current != nullptr) {
        if (current->ptr == void_ptr && current->alloc_t == alloc_t) {
            return current->active;
        }
        current = current->next;
    }

    return false;
}

template <typename T>
void forget_t(T *ptr, _internal::Allocation::Type alloc_t) {
    void                  *void_ptr = static_cast<void *>(ptr);
    size_t                 index    = _internal::memory_table.hash(void_ptr);
    _internal::Allocation *current  = _internal::memory_table.get_table()[index];  // NOLINT

    while (current != nullptr) {
        if (current->ptr == void_ptr && current->alloc_t == alloc_t) {
            current->active = false;
            return;
        }

        current = current->next;
    }
}
}  // namespace memory
// ---------------------------------------------------------------------------------------------- //

// ------------------------------------- refences/utils.hlx ------------------------------------- //
namespace std::utils {
template <typename T>
struct remove {
    using t = T;
};

template <typename T>
struct remove<T &> {
    using t = T;
};

template <typename T>
struct remove<T &&> {
    using t = T;
};
}  // namespace std::utils

namespace std {
template <typename T>
T &&forward(typename utils::remove<T>::t &t) noexcept {
    return static_cast<T &&>(t);
}
}  // namespace std
// ---------------------------------------------------------------------------------------------- //

/// ∧∧∧∧∧∧∧∧∧∧∧∧∧∧∧∧ end of implementations ---------------------- start of test vvvvvvvvvvvvvvvv //

class Foo {  // NOLINT
  public:
    Foo() { printf("hi there\n"); }  // NOLINT

    ~Foo() { printf("bye bye\n"); }  // NOLINT

    /// test notes: clang is probaly optimizing this function and making it inline/static
    /// and puts it directly in the main function which would explain why
    /// the function is still called even after the memory is freed
    void do_something() { printf("something\n"); }  // NOLINT
};

void *some_ptr = nullptr;  // NOLINT

void something_else() {
    // Ensure `some_ptr` points to a valid object and performs the expected action
    assert(some_ptr != nullptr);
    ((Foo *)some_ptr)->do_something();  // NOLINT
}

void raII_test() {
    memory::Heap<Foo> foo;
    assert(&foo != nullptr);  // Ensure foo is initialized

    (*foo).do_something();  // RAII behavior, should work as expected

    // Now release the RAII control
    memory::forget(foo);
    some_ptr = static_cast<void *>(&foo);  // Store raw pointer after RAII is disabled

    // Ensure that memory::forget has taken control away from RAII
    // Since memory::forget(foo) was called, foo should not call its destructor
}

void manual_management_test() {
    // Explicitly create and forget another instance of Foo to test memory leak scenarios
    memory::Heap<Foo> manual_foo;

    manual_foo.do_something();  // RAII behavior, should work as expected

    // Now `manual_foo` is unmanaged, we should delete it manually
    some_ptr = static_cast<void *>(&manual_foo);
}

void random_test() {
    // memory::Heap<int> int_heap(123);
    // assert(*int_heap == 123);

    // printf("int: %d\n", *int_heap);
    // memory::forget(int_heap);
    // memory::drop(int_heap);

    /// no leaks.

    // printf("int: %d\n", *int_heap); // raises an exception
}

int main() {
    // Call raII_test to ensure foo can be manually managed without RAII interfering
    raII_test();

    // Call something_else to verify that `some_ptr` is valid and points to a valid object
    something_else();

    // Clean up by manually freeing `some_ptr`, simulating manual management
    delete (Foo *)some_ptr;  // NOLINT
    some_ptr = nullptr;

    // Test another instance where manual management is required
    manual_management_test();

    // foo should be deleted by RAII, so we should not have any memory leaks
    ((Foo *)some_ptr)->do_something();  // NOLINT
    /// this should be a dangling pointer, but it still works
    /// this line is weird because it still works even after the memory is freed

    random_test();

    // Confirm all memory is correctly freed and no leaks are introduced
    printf("All tests passed, memory management handled as expected.\n");

    return 0;
}
