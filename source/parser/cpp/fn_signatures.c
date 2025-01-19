#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include <sstream>
#include <string>
#include <vector>

#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/CommandLine.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

static std::ostringstream Result;

class DeclarationVisitor : public RecursiveASTVisitor<DeclarationVisitor> {
  public:
    explicit DeclarationVisitor(ASTContext *Context)
        : Context(Context) {}

    static bool VisitFunctionDecl(FunctionDecl *D) {
        if (D->isThisDeclarationADefinition()) {
            Result << D->getReturnType().getAsString() << " " << D->getNameAsString() << "();\n";
        }
        return true;
    }

    static bool VisitVarDecl(VarDecl *D) {
        if (D->hasExternalStorage() || D->isFileVarDecl()) {
            Result << D->getType().getAsString() << " " << D->getNameAsString() << ";\n";
        }
        return true;
    }

    static bool VisitCXXRecordDecl(CXXRecordDecl *D) {
        if (D->isThisDeclarationADefinition()) {
            Result << "class " << D->getNameAsString() << ";\n";
        }
        return true;
    }

    static std::string getResult() { return Result.str(); }

  private:
    ASTContext *Context;
};

class DeclarationConsumer : public ASTConsumer {
  public:
    explicit DeclarationConsumer(ASTContext *Context)
        : Visitor(Context) {}

    void HandleTranslationUnit(ASTContext &Context) override {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

    std::string getResult() { return Visitor.getResult(); }

  private:
    DeclarationVisitor Visitor;
};

class DeclarationAction : public ASTFrontendAction {
  public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef File) override {
        return std::make_unique<DeclarationConsumer>(&CI.getASTContext());
    }

    bool BeginSourceFileAction(CompilerInstance &CI) override { return true; }
};

std::string parse_signatures(const std::string &filename) {
    std::vector<const char *> args = {"parse_signatures", filename.c_str()};
    int                       argc = args.size();
    const char              **argv = args.data();

    llvm::cl::OptionCategory Category("parse_signatures options");
    auto                     ExpectedParser = CommonOptionsParser::create(argc, argv, Category);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return "";
    }

    CommonOptionsParser &OptionsParser = *ExpectedParser;
    ClangTool            Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

    // Add include path
    std::vector<std::string> compilationArgs = {"-std=c++23", "-I/usr/local/lib/clang/18/include"};
    Tool.appendArgumentsAdjuster(
        getInsertArgumentAdjuster(compilationArgs, ArgumentInsertPosition::BEGIN));

    Tool.run(newFrontendActionFactory<DeclarationAction>().get());

    return Result.str();
}

#pragma clang diagnostic pop