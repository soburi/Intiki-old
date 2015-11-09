#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"

#include <vector>

using namespace std;
using namespace clang;
using namespace llvm;

class IntikiPPCallbacksHandler : public PPCallbacks {
public:
    IntikiPPCallbacksHandler(Preprocessor &pp) : PP(pp) {}

private:
    Preprocessor &PP;
};

class TranslateEntry
{
public:
    uint32_t begin;
    uint32_t end;
    string replacement;

    static bool Compare(const TranslateEntry& lhs, const TranslateEntry& rhs);
};

class IntikiVisitor: public DeclVisitor<IntikiVisitor, bool>
{
    CompilerInstance& CI;
    vector<TranslateEntry> translate;
    string mainFile;
    bool mainLoopDetected;
public:
    IntikiVisitor(CompilerInstance &CI);
    bool VisitFunctionDecl(FunctionDecl *aFunctionDecl);

    vector<TranslateEntry>& getTranslateInfo() { return translate; }
    string& getMainFileName() { return mainFile; }
    bool isMainLoopDetected() { return mainLoopDetected; }

};

class IntikiASTConsumer : public ASTConsumer
{
    CompilerInstance& CI;
    IntikiVisitor &visitor;
public:
    explicit IntikiASTConsumer(CompilerInstance &CI, IntikiVisitor& aVisitor);
    void HandleTranslationUnit(ASTContext &Context) override;
};

class IntikiUtil
{
public:
    static void CommentLineDirectiveAndWrite(std::ofstream& out, const char* data, const int size);
    static void UncommentLineDirectiveAndWrite(std::ofstream& out, const char* data, const int size);

    static void FilterAndBackup(const string srcfile, const string postfix, void (*fp)(std::ofstream&,const char*,int) );
    static void FilterAndBackup(const char* data, const int size, const string srcfile, const string postfix, void (*fp)(std::ofstream&,const char*,int) );

    static void CommentLineDirectiveAndBackup(const string srcfile, const string postfix) {
    	FilterAndBackup(srcfile, postfix, IntikiUtil::CommentLineDirectiveAndWrite );
    }
    static void UncommentLineDirectiveAndBackup(const char* data, const int size, const string srcfile, const string postfix) {
    	FilterAndBackup(data, size, srcfile, postfix, IntikiUtil::UncommentLineDirectiveAndWrite );
    }

};

class IntikiFrontendAction : public SyntaxOnlyAction
{
public:
    unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override;
    void EndSourceFileAction() override;
    unique_ptr<IntikiVisitor> visitor;
};

