#include <type_traits>
#include <concepts>

template<typename T>
concept SomeInterface = requires(T t) {
    { t.foo() } -> std::same_as<int>;
};


template <class T, typename...Es>
class AClass {};

template <class T, typename ...Es>
    requires(std::is_class_v<Es> && ...)
class AClass<T, Es...> {
    
};

template <class T, typename ...Es>
    requires (std::is_class_v<T> && (std::is_class_v<Es> && ...))
class AClass<T, Es...> // without extending an interface
{

};

int main() {

}