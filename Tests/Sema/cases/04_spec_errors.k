// EXPECT: error "redefinition of specialization 'Pair'" (two definitions for <i32>).
// EXPECT: error "explicit specialization of 'Late' after it was already instantiated".
// EXPECT: error "'Orphan' is declared as a specialization, but no primary template".
// EXPECT: fwd spec + def of Pair<bool> links silently (no error for that pair).
class <T> Pair { var a: T; var b: T }

class <> Pair<bool>;
class <> Pair<bool> { var packed: u8 }

class <> Pair<i32> { var x: i32 }
class <> Pair<i32> { var y: i32 }

class <T> Late { var v: T }
var early: Late<u8>
class <> Late<u8> { var w: u8 }

class <> Orphan<i32> { var z: i32 }