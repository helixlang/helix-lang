///--- The Helix Project ------------------------------------------------------------------------///
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
///-------------------------------------------------------------------------------------- C++ ---///

#ifndef __CXIR_H__
#define __CXIR_H__

#include <clang/Format/Format.h>
#include <llvm/ADT/StringRef.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "generator/include/CX-IR/tokens.def"
#include "generator/include/config/Gen_config.def"
#include "neo-pprint/include/hxpprint.hh"
#include "parser/ast/include/AST.hh"
#include "parser/preprocessor/include/private/utils.hh"
#include "token/include/Token.hh"

const std::regex
    double_semi_regexp(R"(;\r?\n\s*?;)");  // Matches any whitespace around the semicolons
inline std::string get_neo_clang_format_config() {
    return R"(
Language:        Cpp
BasedOnStyle:  Google
AccessModifierOffset: -2
NamespaceIndentation: Inner
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: true
AlignConsecutiveDeclarations: true
AlignEscapedNewlines: Left
AlignOperands:   true
AlignTrailingComments: true
AllowAllParametersOfDeclarationOnNextLine: true
AllowShortBlocksOnASingleLine: true
AllowShortCaseLabelsOnASingleLine: false
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
AlwaysBreakAfterDefinitionReturnType: None
AlwaysBreakAfterReturnType: None
AlwaysBreakBeforeMultilineStrings: false
AlwaysBreakTemplateDeclarations: true
BinPackArguments: false
BinPackParameters: false
BraceWrapping:
    AfterClass:      false
    AfterControlStatement: false
    AfterEnum:       false
    AfterFunction:   false
    AfterNamespace:  false
    AfterObjCDeclaration: false
    AfterStruct:     false
    AfterUnion:      false
    AfterExternBlock: false
    BeforeCatch:     false
    BeforeElse:      false
    IndentBraces:    false
    SplitEmptyFunction: true
    SplitEmptyRecord: true
    SplitEmptyNamespace: true
AllowShortFunctionsOnASingleLine: All
BreakBeforeBinaryOperators: None
BreakBeforeBraces: Custom
BreakBeforeInheritanceComma: true
BreakInheritanceList: BeforeColon
BreakBeforeTernaryOperators: true
BreakConstructorInitializersBeforeComma: true
ColumnLimit:     100
CommentPragmas:  '^ IWYU pragma:'
CompactNamespaces: false
ConstructorInitializerAllOnOneLineOrOnePerLine: false
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
Cpp11BracedListStyle: true
DerivePointerAlignment: false
DisableFormat:   false
ExperimentalAutoDetectBinPacking: false
FixNamespaceComments: true
ForEachMacros:
  - foreach
  - Q_FOREACH
  - BOOST_FOREACH
IncludeCategories:
  - Regex:           '^<.*\.h>'
    Priority:        1
  - Regex:           '^<.*'
    Priority:        2
  - Regex:           '.*'
    Priority:        3
IndentCaseLabels: true
IndentWidth:     4
IndentWrappedFunctionNames: false
KeepEmptyLinesAtTheStartOfBlocks: true
MacroBlockBegin: ''
MacroBlockEnd:   ''
MaxEmptyLinesToKeep: 1
PenaltyBreakAssignment: 2
PenaltyBreakBeforeFirstCallParameter: 19
PenaltyBreakComment: 300
PenaltyBreakFirstLessLess: 120
PenaltyBreakString: 1000
PenaltyExcessCharacter: 1000000
PenaltyReturnTypeOnItsOwnLine: 60
PointerAlignment: Right
ReflowComments:  true
SortIncludes:    true
SpaceAfterCStyleCast: false
SpaceAfterTemplateKeyword: true
SpaceBeforeAssignmentOperators: true
SpaceBeforeParens: ControlStatements
SpaceInEmptyParentheses: false
SpacesBeforeTrailingComments: 2
SpacesInAngles:  false
SpacesInContainerLiterals: true
SpacesInCStyleCastParentheses: false
SpacesInParentheses: false
SpacesInSquareBrackets: false
Standard:        Latest
TabWidth:        4
UseTab:          Never
)";
}

GENERATE_CXIR_TOKENS_ENUM_AND_MAPPING;

__CXIR_CODEGEN_BEGIN {
    class CX_Token {
      private:
        u64         line{};
        u64         column{};
        u64         length{};
        cxir_tokens type{};
        std::string file_name;
        std::string value;

      public:
        CX_Token() = default;
        CX_Token(const token::Token &tok, cxir_tokens set_type)
            : line(tok.line_number())
            , column(tok.column_number())
            , length(tok.length())
            , type(set_type)
            , file_name(std::filesystem::path(tok.file_name()).generic_string())
            , value(std::string(tok.value())) {}

        explicit CX_Token(cxir_tokens type)
            : length(1)
            , type(type)
            , file_name("_H1HJA9ZLO_17.helix-compiler.cxir")
            , value(cxir_tokens_map.at(type).has_value()
                        ? std::string(cxir_tokens_map.at(type).value())
                        : " /* Unknown Token */ ") {}

        CX_Token(cxir_tokens type, std::string value)
            : length(value.length())
            , type(type)
            , file_name("_H1HJA9ZLO_17.helix-compiler.cxir")
            , value(std::move(value)) {}

        CX_Token(const CX_Token &)            = default;
        CX_Token(CX_Token &&)                 = delete;
        CX_Token &operator=(const CX_Token &) = default;
        CX_Token &operator=(CX_Token &&)      = delete;
        ~CX_Token()                           = default;

        [[nodiscard]] u64         get_line() const { return line; }
        [[nodiscard]] u64         get_column() const { return column; }
        [[nodiscard]] u64         get_length() const { return length; }
        [[nodiscard]] cxir_tokens get_type() const { return type; }
        [[nodiscard]] std::string get_file_name() const { return file_name; }
        [[nodiscard]] std::string get_value() const { return value; }
        [[nodiscard]] std::string to_CXIR() const {
            if (value[0] == '#') {
                return "\n" + value + " ";
            }

            if (value == ";") {
                return "\n" + value + " ";
            }

            if (line == 0) {
                return " " + value + "\n";
            }

            return "\n#line " + std::to_string(line) + " \"" + file_name + "\"\n" + value + "\n";
        }

        [[nodiscard]] std::string to_clean_CXIR() const {
            if (value[0] == '#') {
                return value + " ";
            }
            return value + "\n";
        }
    };

    class CXIR : public __AST_VISITOR::Visitor {
      private:
        std::vector<std::unique_ptr<CX_Token>> tokens;
        std::vector<generator::CXIR::CXIR>     imports;
        const bool                             forward_only = false;

      public:
        explicit CXIR(bool forward_only = false, std::vector<generator::CXIR::CXIR> imports = {})
            : imports(std::move(imports))
            , forward_only(forward_only) {}

        CXIR(const CXIR &)            = default;
        CXIR(CXIR &&)                 = default;
        CXIR &operator=(const CXIR &) = delete;
        CXIR &operator=(CXIR &&)      = delete;
        ~CXIR() override              = default;

        [[nodiscard]] std::optional<std::string> get_file_name() const {
            if (tokens.empty()) {
                return std::nullopt;
            }

            for (const auto &token : tokens) {
                if (token->get_line() != 0) {
                    return token->get_file_name();
                }
            }

            return std::nullopt;
        }

        template <const bool add_core = true>
        [[nodiscard]] std::string to_CXIR() const {
            std::string cxir;

            if constexpr (add_core) {
                cxir += get_core() + "\n" + get_imports<false>() + "\n";
            } else {
                cxir += get_imports<false>() + "\n";
            }

            u64 current_line_no = 0;

            // normalize tokens
            // if tokens has a line number, add a new line
            for (u64 i = 0; i < tokens.size(); ++i) {
                // normalize so there arent multiple #line
                if (tokens[i]->get_value()[0] == '#') {
                    if (!((i + 1) < tokens.size())) {
                        cxir += "\n" + tokens[i]->get_value() + "\n";
                        continue;
                    }

                    cxir += "\n" + tokens[i]->get_value() + " " + tokens[i + 1]->get_value() + "\n";
                    ++i;
                } else if (tokens[i]->get_value() == ";") {
                    cxir += " " + tokens[i]->get_value() + "\n";
                } else if (tokens[i]->get_line() == 0 || tokens[i]->get_line() == current_line_no) {
                    cxir += " " + tokens[i]->get_value() + " ";
                } else {
                    cxir += "\n#line " + std::to_string(tokens[i]->get_line()) + " \"" +
                            tokens[i]->get_file_name() + "\"\n";
                    cxir += " " + tokens[i]->get_value() + " ";
                    current_line_no = tokens[i]->get_line();
                }
            }

            // If cxir is empty, log and return early
            if (cxir.empty()) {
                print("CXIR is empty after processing tokens.");
                return cxir;
            }

            return cxir;
        }

        [[nodiscard]] std::string to_readable_CXIR() const {
            std::string cxir = get_imports<true>() + "\n";

            // Build the CXIR string from tokens
            for (const auto &token : tokens) {
                cxir += token->to_clean_CXIR();
            }

            // If cxir is empty, log and return early
            if (cxir.empty()) {
                print("CXIR is empty after processing tokens.");
                return cxir;
            }

            // Format the CXIR code
            return format_cxir(cxir);
        }

        [[nodiscard]] static std::string format_cxir(const std::string &cxir) {
            // Get the configuration as a string
            std::string config = get_neo_clang_format_config();

            // Use a basic clang format style to simplify testing
            clang::format::FormatStyle style =
                clang::format::getGoogleStyle(clang::format::FormatStyle::LanguageKind::LK_Cpp);

            // Parse the configuration string into the FormatStyle object
            auto error = clang::format::parseConfiguration(config, &style);
            if (error) {
                print("failed to parse configuration: ", error.message());
                throw std::runtime_error("Failed to parse configuration");
            }

            // Format the cxir code using the style
            llvm::StringRef              codeRef(cxir);
            clang::tooling::Range        range(0, cxir.size());
            clang::tooling::Replacements replacements =
                clang::format::reformat(style, codeRef, {range});

            // Apply the replacements to get the formatted code
            llvm::Expected<std::string> formattedCode =
                clang::tooling::applyAllReplacements(cxir, replacements);

            if (!formattedCode) {
                print("failed to format code: ", llvm::toString(formattedCode.takeError()).c_str());
                throw std::runtime_error("Error formatting code");
            }

            return std::regex_replace(*formattedCode, double_semi_regexp, ";");
        }

        static std::string get_core();

        template <const bool readable>
        std::string get_imports() const {
            std::string cxir;

            for (const auto &import : this->imports) {
                if constexpr (readable) {
                    cxir += import.to_readable_CXIR();
                } else {
                    cxir += import.to_CXIR<false>();
                }
            }

            return cxir;
        }

        void visit(const parser ::ast ::node ::LiteralExpr &node) override;
        void visit(const parser ::ast ::node ::BinaryExpr &node) override;
        void visit(const parser ::ast ::node ::UnaryExpr &node) override;
        void visit(const parser ::ast ::node ::IdentExpr &node) override;
        void visit(const parser ::ast ::node ::NamedArgumentExpr &node) override;
        void visit(const parser ::ast ::node ::ArgumentExpr &node) override;
        void visit(const parser ::ast ::node ::ArgumentListExpr &node) override;
        void visit(const parser ::ast ::node ::GenericInvokeExpr &node) override;
        void visit(const parser ::ast ::node ::GenericInvokePathExpr &node) override;
        void visit(const parser ::ast ::node ::ScopePathExpr &node) override;
        void visit(const parser ::ast ::node ::DotPathExpr &node) override;
        void visit(const parser ::ast ::node ::ArrayAccessExpr &node) override;
        void visit(const parser ::ast ::node ::PathExpr &node) override;
        void visit(const parser ::ast ::node ::FunctionCallExpr &node) override;
        void visit(const parser ::ast ::node ::ArrayLiteralExpr &node) override;
        void visit(const parser ::ast ::node ::TupleLiteralExpr &node) override;
        void visit(const parser ::ast ::node ::SetLiteralExpr &node) override;
        void visit(const parser ::ast ::node ::MapPairExpr &node) override;
        void visit(const parser ::ast ::node ::MapLiteralExpr &node) override;
        void visit(const parser ::ast ::node ::ObjInitExpr &node) override;
        void visit(const parser ::ast ::node ::LambdaExpr &node) override;
        void visit(const parser ::ast ::node ::TernaryExpr &node) override;
        void visit(const parser ::ast ::node ::ParenthesizedExpr &node) override;
        void visit(const parser ::ast ::node ::CastExpr &node) override;
        void visit(const parser ::ast ::node ::InstOfExpr &node) override;
        void visit(const parser ::ast ::node ::AsyncThreading &node) override;
        void visit(const parser ::ast ::node ::Type &node) override;
        void visit(const parser ::ast ::node ::NamedVarSpecifier &node, bool omit_t);
        void visit(const parser ::ast ::node ::NamedVarSpecifier &node) override {
            visit(node, false);
        }
        void visit(const parser ::ast ::node ::NamedVarSpecifierList &node, bool omit_t);
        void visit(const parser ::ast ::node ::NamedVarSpecifierList &node) override {
            visit(node, false);
        }
        void visit(const parser ::ast ::node ::ForPyStatementCore &node) override;
        void visit(const parser ::ast ::node ::ForCStatementCore &node) override;
        void visit(const parser ::ast ::node ::ForState &node) override;
        void visit(const parser ::ast ::node ::WhileState &node) override;
        void visit(const parser ::ast ::node ::ElseState &node) override;
        void visit(const parser ::ast ::node ::IfState &node) override;
        void visit(const parser ::ast ::node ::SwitchCaseState &node) override;
        void visit(const parser ::ast ::node ::SwitchState &node) override;
        void visit(const parser ::ast ::node ::YieldState &node) override;
        void visit(const parser ::ast ::node ::DeleteState &node) override;

        void visit(const parser ::ast ::node ::ImportState &node) override;
        void visit(const parser ::ast ::node ::ImportItems &node) override;
        void visit(const parser ::ast ::node ::SingleImport &node) override;
        void visit(const parser ::ast ::node ::SpecImport &node) override;
        void visit(const parser ::ast ::node ::MultiImportState &node) override;

        void visit(const parser ::ast ::node ::ReturnState &node) override;
        void visit(const parser ::ast ::node ::BreakState &node) override;
        void visit(const parser ::ast ::node ::BlockState &node) override;
        void visit(const parser ::ast ::node ::SuiteState &node) override;
        void visit(const parser ::ast ::node ::ContinueState &node) override;
        void visit(const parser ::ast ::node ::CatchState &node) override;
        void visit(const parser ::ast ::node ::FinallyState &node) override;
        void visit(const parser ::ast ::node ::TryState &node) override;
        void visit(const parser ::ast ::node ::PanicState &node) override;
        void visit(const parser ::ast ::node ::ExprState &node) override;
        void visit(const parser ::ast ::node ::RequiresParamDecl &node) override;
        void visit(const parser ::ast ::node ::RequiresParamList &node) override;
        void visit(const parser ::ast ::node ::EnumMemberDecl &node) override;
        void visit(const parser ::ast ::node ::UDTDeriveDecl &node) override;
        void visit(const parser ::ast ::node ::TypeBoundList &node) override;
        void visit(const parser ::ast ::node ::TypeBoundDecl &node) override;
        void visit(const parser ::ast ::node ::RequiresDecl &node) override;
        void visit(const parser ::ast ::node ::ModuleDecl &node) override;
        void visit(const parser ::ast ::node ::StructDecl &node) override;
        void visit(const parser ::ast ::node ::ConstDecl &node) override;
        void visit(const parser ::ast ::node ::ClassDecl &node) override;
        void visit(const parser ::ast ::node ::InterDecl &node) override;
        void visit(const parser ::ast ::node ::EnumDecl &node) override;
        void visit(const parser ::ast ::node ::TypeDecl &node) override;
        void visit(const parser ::ast ::node ::FuncDecl &node, bool no_return_t);
        void visit(const parser ::ast ::node ::VarDecl &node) override;
        void visit(const parser ::ast ::node ::FFIDecl &node) override;
        void visit(const parser ::ast ::node ::LetDecl &node) override { visit(node, false); };
        void visit(const parser ::ast ::node ::LetDecl &node, bool is_in_statement);
        void visit(const parser ::ast ::node ::OpDecl &node) override;
        /// REMOVED: void visit(const parser ::ast ::node ::OpDecl &node, bool remove_self) {};
        void visit(const parser ::ast ::node ::Program &node) override {
            visit(const_cast<parser ::ast ::node ::Program &>(node));
        }
        void visit(parser ::ast ::node ::Program &node);
        void visit(const parser ::ast ::node ::FuncDecl &node) override { visit(node, false); };
    };

    // inline CXIR get_node_json(const __AST_VISITOR::NodeT<> &node) {
    //     auto visitor = CXIR();

    //     if (node == nullptr) {

    //     }

    //     node->accept(visitor);
    //     return visitor.json;
    // }
}
#endif  // __CXIR_H__