from classes.Token import Processed_Line, Token, Token_List
from core.config import load_config

INDENT_CHAR = load_config().Formatter["indent_char"]
re = __import__(load_config().Transpiler["regex_module"])

from core.panic import panic

CLASS_EXTENSION = "::"
SEPARATOR_FOR_CLASSES = "+"

def _interface(ast_list: Token_List, current_scope, parent_scope, root_scope, modifiers=None) -> str:
    pass

def _struct(ast_list: Token_List, current_scope, parent_scope, root_scope, modifiers=None) -> str:
    pass

def _union(ast_list: Token_List, current_scope, parent_scope, root_scope, modifiers=None) -> str:
    pass

def _enum(ast_list: Token_List, current_scope, parent_scope, root_scope, modifiers=None) -> str:
    pass

def _abstract(ast_list: Token_List, current_scope, parent_scope, root_scope, modifiers=None) -> str:
    pass

def _class(ast_list: Token_List, current_scope, parent_scope, root_scope, modifiers=None) -> str:
    data_structure_types = (
        parent_scope.get_keyword("INTERFACE"),
        parent_scope.get_keyword("STRUCT"),
        parent_scope.get_keyword("UNION"),
        parent_scope.get_keyword("ENUM"),
        parent_scope.get_keyword("ABSTRACT")
    )
    
    if CLASS_EXTENSION in ast_list:
        class_name: str = ast_list.splice(0, len(ast_list.get_all_before(CLASS_EXTENSION))).splice(1).full_line().replace("<", "[").replace(">", "]")
    else:
        class_name: str = ast_list.splice(0, len(ast_list.get_all_before(":"))).splice(1).full_line().replace("<", "[").replace(">", "]")
        
    class_extends: list[Token]  = [i for i in ast_list.get_all_after(CLASS_EXTENSION) if i.token != SEPARATOR_FOR_CLASSES and i.token != ":" and i.token != "(" and i.token != ")"] if ast_list[2].token == CLASS_EXTENSION else []
    class_decorators: list[str] = []

    if any([i in ast_list for i in data_structure_types]):
        if ast_list[0] == parent_scope.get_keyword("INTERFACE"):
            class_extends.insert(0, Token(None, "ABC", None, None))
            parent_scope.classes["ABC"] = {
                "extends": [],
                "modifiers": [],
                "unsafe":  False,
                "static":  False,
                "private": False,
            } if "ABC" not in parent_scope.classes else parent_scope.classes["ABC"]
        elif ast_list[0] == parent_scope.get_keyword("STRUCT") and (modifiers and parent_scope.get_keyword("FINAL") in modifiers):
            class_decorators.append("@dataclass(frozen=True)")
        elif ast_list[0] == parent_scope.get_keyword("STRUCT"):
            class_decorators.append("@dataclass")
        elif ast_list[0] == parent_scope.get_keyword("ENUM"):
            class_extends.insert(0, Token(None, "Enum", None, None)) # add generic enum
            parent_scope.classes["Enum"] = {
                "extends": [],
                "modifiers": [],
                "unsafe":  False,
                "static":  False,
                "private": False,
            } if "Enum" not in parent_scope.classes else parent_scope.classes["Enum"]
        elif ast_list[0] == parent_scope.get_keyword("ABSTRACT"):
            class_extends.append(Token(None, "ABC", None, None))
            parent_scope.classes["ABC"] = {
                "extends": [],
                "modifiers": [],
                "unsafe":  False,
                "static":  False,
                "private": False,
            } if "ABC" not in parent_scope.classes else parent_scope.classes["ABC"]
    else:
        # if the class has a final modifier, then it cannot be extended
        if modifiers and (parent_scope.get_keyword("FINAL") in modifiers and class_extends):
            panic(SyntaxError(f"Class '{class_name}' cannot be extended because it is marked as final"), "::", file=ast_list.file, line_no=ast_list[1].line_number)
    
    [panic(SyntaxError(f"Unexpected token '{i.token}' in class extension"), i.token, file=ast_list.file, line_no=ast_list.find_line_number(i.token)) for i in class_extends if i.token in (CLASS_EXTENSION, SEPARATOR_FOR_CLASSES, "(", ")", ",")] if class_extends else None
    
    for i in class_extends:
        if i.token not in parent_scope.classes.keys():
            panic(NameError(f"Class '{i.token}' not found"), i.token, file=ast_list.file, line_no=ast_list.find_line_number(i.token))
        
    output = f"{INDENT_CHAR*ast_list.indent_level}class {class_name}"
    if class_extends:
        output += f"({', '.join([i.token for i in class_extends])}, metaclass=multimeta)"
    else:
        output += "(metaclass=multimeta)"
    output += ":"
    
    parent_scope.classes[class_name] = {
        "extends": class_extends,
        "modifiers": modifiers,
        "unsafe":  False,
        "static":  False,
        "private": False,
    }
    
    if modifiers:
        if root_scope.get_keyword("ASYNC") in modifiers:
            panic(SyntaxError(f"Classes cannot be async"), file=ast_list.file, line_no=ast_list.find_line_number(root_scope.get_keyword("ASYNC")))
        if root_scope.get_keyword("STATIC") in modifiers:
            class_decorators.append("@singleton")
            
        parent_scope.classes[class_name] = {
            "extends": class_extends,
            root_scope.get_keyword("STATIC"): True if root_scope.get_keyword("STATIC") in modifiers else False,
            root_scope.get_keyword("PRIVATE"): True if root_scope.get_keyword("PRIVATE") in modifiers else False,
            root_scope.get_keyword("UNSAFE"): True if root_scope.get_keyword("UNSAFE") in modifiers else False,
        }
    else:
        parent_scope.classes[class_name] = {
            "extends": class_extends,
            "unsafe": False,
            "static": False,
            "final":  False,
        }

    if class_decorators:
        output = "\n" + "\n".join([f"{INDENT_CHAR*ast_list.indent_level}{i}" for i in class_decorators]) + "\n" + output
    
    return Processed_Line(output, ast_list)