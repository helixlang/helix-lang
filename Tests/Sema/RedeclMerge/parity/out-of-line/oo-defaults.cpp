// oracle for oo-defaults.k
class C {
public:
  void f(int i = 3);
  void g(int i, int j = 99);
};

void C::f(int i = 3) { }        // expected-error: redefinition of default argument

void C::g(int i = 88, int j) { }
