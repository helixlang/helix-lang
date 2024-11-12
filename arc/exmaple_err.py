"""
print:

error: expected a reference to an 'int' but got a copy instead
    -->  at tests/main.hlx:1:4
   1 | let result? = divide(x, y);
     :               ~~~~~~~^~~~~
     |
  fix: change the call to `divide(&x, y);`

    note: the function definition is:
       -->  at tests/main.hlx:4:4
      4 | fn divide(parm1: &int, const parm2: i32) -> Exception? {
      5 |     if parm2 == 0 {
      6 |         return DivideByZero;

error: aborting due to previous error

with color
"""

print()
print("\033[1m" + "\033[31m" + "error" + "\033[0m" + "\033[97m" + ": expected a an '*int' (pointer) but got a 'int' (copy) instead" + "\033[0m")
print("    --> " + "\033[97m" + " at " + "\033[32m" + "tests/main.hlx" + "\033[97m" + ":" + "\033[33m" + "1" + "\033[97m" + ":" + "\033[33m" + "4" + "\033[0m")
print("   1 | " + "\033[97m" + "let result: Exception? = divide(" + "\033[1m" "\033[31m" + "val1" + "\033[97m" + ", val2);" + "\033[0m")
print("     :                          " + "\033[0m" + "~~~~~~~" + "\033[31m" + "^^^^" +  "\033[0m" + "~~~~~~~" + "\033[0m")
print("     |")
print("\033[1m" + "\033[32m" + "  fix" + "\033[0m" + "\033[97m" + ": change the call to " + "\033[32m" + "`divide(&val1, val2);`" + "\033[0m")
print()
print("    " + "\033[1m" + "\033[36m" + "note" + "\033[0m" + "\033[97m" + ": the function definition is:" + "\033[0m")
print("       --> " + "\033[97m" + " at " + "\033[32m" + "tests/main.hlx" + "\033[97m" + ":" + "\033[33m" + "4" + "\033[97m" + ":" + "\033[33m" + "4" + "\033[0m")
print("      4 | " + "\033[97m" + "fn divide(parm1: *int, const parm2: i32) -> Exception? {" + "\033[0m")
print("      5 |     if parm2 == 0:" + "\033[0m")
print("      6 |         return DivideByZero;" + "\033[0m")
print()
print("\033[1m" + "\033[31m" + "error" + "\033[0m" + "\033[97m" + ": aborting due to previous error" + "\033[0m", end="\n")
