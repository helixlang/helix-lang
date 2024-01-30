from src.classes.cache_store import cache

from src.globals import BODY_REQUIRED_KEYWORDS
from src.classes.ast import AST, AST_LIST

@cache
def normalize_tokens(_lines: tuple[AST, ...], path: str) -> tuple[AST_LIST, ...]:
    """
    This function normalizes the tokens. This is a smaller part of the tokenizer,
    but it is in its own file because it is a very important part of the tokenizer.
    Some of the things this function does is:
        - Reads the indent level of each line
        - Normalizes the indent level of each line
        - Normalizes the formatting of the code (e.g. `if (a) {` -> `if (a):`)

    Args:
        lines (str): The code to normalize
                     - LINES MUST BE SEPARATED BY LINE_BREAK
                     This is because this allows for the function to be cached

    Returns:
        str: The normalized code separated by LINE_BREAK, and SUB_LINE_BREAK
    """

    # spile the ast list into a list of asts contngn all the same metadata but the changed line
    lines: list[AST] = []
    
    for ast_line in _lines:
        for token in ast_line.line:
            lines.append(AST(ast_line.original_line, token, ast_line.line_number, ast_line.indent_level))

    stack:            list[AST]       = []
    current_line:     list[AST]       = []
    final_lines:      list[list[AST]] = []
    firs_inst:        bool            = True
    in_for_loop:      bool            = False
    indent_level:     int             = 0
    previous_element: AST             = AST("", "", 0, 0)
    
    # lines is expected to countian ['token', ...] for every token in the file NOT individual line
    def process_line(index: int):
        index = index[0]
        nonlocal stack, indent_level, firs_inst, previous_element
        
        token = lines[index].line
        if token in BODY_REQUIRED_KEYWORDS.keys():
            # append the line to the stack
            for i in range(index, len(lines)):
                if lines[i].line == "{":
                    break
            stack.append(lines[i])
            indent_level += 1
            firs_inst = True
        elif token == "{":
            if not firs_inst:
                indent_level += 1
            if len(stack) == indent_level:
                lines[index].line = ":"
                lines.insert(index + 1, AST(lines[index].original_line, "<\\n>", lines[index].line_number, lines[index].indent_level + 1))
                firs_inst = False
            if lines[index + 2].line == "}":
                lines[index + 2].line = "}"
                lines.insert(index + 3, AST(lines[index].original_line, "<\\r>", lines[index].line_number, lines[index].indent_level + 1))
        elif token == "}":
            if len(stack) == indent_level:
                lines[index].line = "<\\n>"
                indent_level -= 1
                previous_element = stack.pop()
            else:
                indent_level -= 1
        if token == "<\\r>":
            lines[index - 5].line = "<\\r1>"
        if token == ";":
            lines[index].line = "<\\n>"
        if lines[index].line == "<\\n>":
            lines.insert(index + 1, AST(lines[index].original_line, f"<\\t:{indent_level}>", lines[index].line_number, lines[index].indent_level + 1))
        if token == "..." and not lines[index + 1].line == "<\\n>":
            lines.insert(index + 1, AST(lines[index].original_line, "<\\n>", lines[index].line_number, lines[index].indent_level + 1))
        if token == "<\\r>":
            for i in range(index, -1, -1):
                if lines[i].line == "<\\r1>":
                    break
                else:
                    lines[i].line = ""
        if token == ";":
            lines[index].line = "<\\n>"
        
    frozenset((process_line(_),) for _ in enumerate(lines))
    
    if indent_level > 0:
        from src.core.panic import panic
        if stack[-1].indent_level == 0:
            stack[-1] = previous_element
        panic(SyntaxError(f"<Hex(01.E20)>: Expected an indent: level of 0, but got {indent_level}"), "{", file=path, line_no=stack[-1].line_number)
    
    def process_for_loops(index: int) -> str:
        nonlocal in_for_loop, lines
        token = lines[index].line
        if token == "for":
            in_for_loop = True
        if in_for_loop:
            if token == "<\\n>":
                lines[index].line = ";"
                if lines[index + 1].line.startswith("<\\t:"):
                    lines[index + 1].line = ""
            elif token == ":" and index + 1 < len(lines):
                in_for_loop = False

    frozenset((process_for_loops(_) for _ in range(len(lines))))

    lines.insert(0, AST(lines[0].original_line, "<\\t:0>", lines[0].line_number, 0))

    indent_level = 0
    def process_indent_level(ast_token: AST):
        nonlocal indent_level, lines, current_line, final_lines
        token = ast_token.line
        if token.startswith("<\\t:"):
            indent_level = int(token[4:-1])
            return
        if token == "<\\n>":
            for i in current_line: i.indent_level = indent_level
            final_lines.append(current_line)
            current_line = []
        else:
            current_line.append(ast_token)
    
    frozenset((process_indent_level(_) for _ in [_ for _ in lines if _.line]))
    
    if current_line:
        for i in current_line: i.indent_level = indent_level
        final_lines.append(current_line)
        
    return tuple(AST_LIST(_, _[0].indent_level) for _ in final_lines if _)