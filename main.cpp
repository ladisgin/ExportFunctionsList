#include <filesystem>
#include <fstream>
#include <iostream>

#include <clang/AST/AST.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <clang/Tooling/Tooling.h>

using namespace clang;

const std::string tab = "  ";

ast_matchers::DeclarationMatcher functionMatcher =
    ast_matchers::functionDecl(
        ast_matchers::allOf(
            ast_matchers::isDefinition(), ast_matchers::isExpansionInMainFile(),
            ast_matchers::hasBody(ast_matchers::compoundStmt()),
            ast_matchers::unless(ast_matchers::cxxDestructorDecl()),
            ast_matchers::unless(ast_matchers::cxxConstructorDecl()),
            ast_matchers::unless(ast_matchers::functionTemplateDecl())))
        .bind("functions");

class FunctionPrinter : public ast_matchers::MatchFinder::MatchCallback {
public:
  virtual void run(const ast_matchers::MatchFinder::MatchResult &Result) {
    if (const clang::FunctionDecl *fd =
            Result.Nodes.getNodeAs<clang::FunctionDecl>("functions")) {
      auto &sm = fd->getASTContext().getSourceManager();
      auto beginLoc = FullSourceLoc(fd->getBeginLoc(), sm);
      auto endLoc = FullSourceLoc(fd->getEndLoc(), sm);
      if (isFirst) {
        isFirst = false;
      } else {
        *fs << ",\n";
      }

      auto path = std::filesystem::path(tooling::getAbsolutePath(
          sm.getFilename(FullSourceLoc(fd->getLocation(), sm))));

      while (std::filesystem::exists(path) && std::filesystem::is_symlink(path)) {
        path = std::filesystem::read_symlink(path);
      }
      
      *fs << tab << "\"" << fd->getQualifiedNameAsString() << "\": {\n"
          << tab << tab << "\"path\": " << path.lexically_normal() << ",\n"
          << tab << tab << "\"start\": \"" << beginLoc.getLineNumber()
          << "\",\n"
          << tab << tab << "\"start column\": \"" << beginLoc.getColumnNumber()
          << "\",\n"
          << tab << tab << "\"end\": \"" << endLoc.getLineNumber() << "\",\n"
          << tab << tab << "\"end column\": \"" << ""
          << endLoc.getColumnNumber() << "\"\n"
          << tab << "}";
    }
  }

  bool isFirst = true;
  std::ofstream *fs = nullptr;
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Waited at least one argument with compile_commands.json "
                 "directory path";
    return -1;
  }

  std::string errorMessage;
  static std::unique_ptr<tooling::CompilationDatabase> cc =
      tooling::JSONCompilationDatabase::loadFromDirectory(argv[1],
                                                          errorMessage);

  tooling::ClangTool Tool(*cc, cc->getAllFiles());

  FunctionPrinter Printer;
  std::ofstream fs;
  std::string resultPath = argc > 2 ? argv[2] : "result.json";
  fs.open(resultPath);
  Printer.fs = &fs;
  fs << "{\n";

  ast_matchers::MatchFinder Finder;
  Finder.addMatcher(functionMatcher, &Printer);
  Tool.run(tooling::newFrontendActionFactory(&Finder).get());

  fs << "\n}";
  fs.close();

  return 0;
}
