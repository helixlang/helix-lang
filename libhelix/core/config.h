#ifndef HELIX_CONFIG_H
#define HELIX_CONFIG_H

#define _new(type, ...) new type(__VA_ARGS__)
#define _delete(ptr) delete ptr
#define _delete_array(ptr) delete[] ptr
#define BEGIN_LIBHELIX_NAMESPACE namesapce helix::std {
#define END_LIBHELIX_NAMESPACE }

namespace helix {
    namespace libcxx::std = ::std;
} // end namespace helix

#endif // HELIX_CONFIG_H
