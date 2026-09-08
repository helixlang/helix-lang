fn outer() {
    fn helper() { }
    if true {
        fn helper() { }    // shadows the outer one, or collides?
    }
}