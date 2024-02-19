from classes.Token import Token, Token_List, Processed_Line
from core.config import load_config
from core.token.tokenize_line import tokenize_line

INDENT_CHAR = load_config().Formatter["indent_char"]
re = __import__(load_config().Transpiler["regex_module"])

from core.panic import panic


def _unmarked(ast_list: Token_List, current_scope, parent_scope, root_scope) -> str:

    # example: name = "John" ->  name.set("John")
    # example: a, b = 5, 6 ->  a.set(5); b.set(6)
    # example: a . somethign = 12 ->  a.something.set(12)
    # example: a, b = (5, 6), (7, 8)  ->  a.set((5, 6)); b.set((7, 8))
    # example: a, b = (5, 6), somecall("yes", a=19) -> a.set((5, 6)); b.set(somecall("yes", a=19))
    
    expecting_name = False
    
    if "=" in ast_list:
        # so something like a = 5 would become a.set(5)
        # and something like a, b = 5, 6 would become a.set(5); b.set(6)
        if ast_list.count("=") > 1:
            panic(
                SyntaxError("Multiple assignments are not allowed in a single line\nYou can use un-packing instead and do `a, b = 1, 2` rather then `a = 1, b = 2`"),
                ast_list[ast_list.index("=")].token,
                file=ast_list.file,
                line_no=ast_list[0].line_number
            )
    
        temp = ast_list.get_all_before("=")
        variables: dict[str, str] = {} # {name: value}
        name = ""
        in_brackets = False
        bracket_count: int = 0
        
        for token in temp:
            if token == ":" and not in_brackets:
                panic(SyntaxError("You cannot use the `:` operator in a variable assignment"), token, file=ast_list.file, line_no=ast_list[0].line_number)
            if token == "," and not in_brackets:
                variables[name] = ""
                name = ""
                continue
            elif token == "::" and not in_brackets:
                panic(SyntaxError("You cannot use the `::` operator in a variable assignment"), token, file=ast_list.file, line_no=ast_list[0].line_number)
            elif token in ("(", "[", "{"):
                in_brackets = True
                bracket_count += 1
            elif token in (")", "]", "}"):
                bracket_count -= 1
                if bracket_count == 0:
                    in_brackets = False
            name += token.token + " "
        else:
            if name:
                variables[name] = ""
                name = ""
                
        temp = ast_list.get_all_after("=")
        value = Token_List([], ast_list.indent_level, ast_list.file)
        in_brackets = False
        bracket_count: int = 0
        
        index = 0
        
        for token in temp:
            if token == "," and not in_brackets:
                try:
                    variables[tuple(variables.keys())[index]] = value
                except IndexError:
                    panic(SyntaxError("Too many values to unpack for assignment"), "=", file=ast_list.file, line_no=ast_list[index].line_number)
                index += 1
                value = Token_List([], ast_list.indent_level, ast_list.file)
                continue
            elif token in ("(", "[", "{"):
                in_brackets = True
                bracket_count += 1
            elif token in (")", "]", "}"):
                bracket_count -= 1
                if bracket_count == 0:
                    in_brackets = False
            value.line.append(token)
        else:
            if value:
                try:
                    variables[tuple(variables.keys())[index]] = value
                except IndexError:
                    panic(SyntaxError("Too many values to unpack for assignment"), "=", file=ast_list.file, line_no=ast_list[-1].line_number)
                value = Token_List([], ast_list.indent_level, ast_list.file)
    
        
        print(variables)
            
        
    
    return Processed_Line("yessir", ast_list)