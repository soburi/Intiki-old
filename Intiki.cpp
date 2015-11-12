//#undef __STRICT_ANSI__

#include "Intiki.h"


#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

#include <dirent.h>
#include <ext/stdio_filebuf.h>

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace llvm;

#if 0
#define DPUT(x) { cerr << x << endl; }
#define DFUN(x) { cerr << __func__ << endl; }
#define DVAL(x) { cerr << #x << ": " << x << endl; }
#else
#define DPUT(x)
#define DFUN(x)
#define DVAL(x)
#endif

int AppErr = 0;

IntikiVisitor::IntikiVisitor(CompilerInstance &aCI)
    : CI(aCI), mainLoopDetected(false)
{
    DFUN();
    FileID mainid = CI.getSourceManager().getMainFileID();
    const FileEntry* file = CI.getSourceManager().getFileEntryForID(mainid);
    mainFile = file->getName();
}

bool IntikiVisitor::VisitFunctionDecl(FunctionDecl *aFunctionDecl)
{
    DFUN();
    SourceManager& SM = CI.getSourceManager();
    if( aFunctionDecl->getNameAsString().compare("loop") == 0 && aFunctionDecl->isThisDeclarationADefinition() )
    {
        const FileID mainid = SM.getMainFileID();
        const FileID id = SM.getFileID( aFunctionDecl->getSourceRange().getBegin() );

        if(mainid != id) {
            return false;
        }

        TranslateEntry prologue;
        prologue.begin = aFunctionDecl->getSourceRange().getBegin().getRawEncoding();
        prologue.end   = aFunctionDecl->getBody()->getSourceRange().getBegin().getRawEncoding();
        prologue.end   += 1;
        prologue.begin -= 2; //OFFSET
        prologue.end   -= 2;
        prologue.replacement = "ARDUINO_LOOP_BEGIN";
        

        TranslateEntry epilogue;
        epilogue.begin = aFunctionDecl->getSourceRange().getEnd().getRawEncoding();
        epilogue.end   = aFunctionDecl->getSourceRange().getEnd().getRawEncoding();
        epilogue.end   += 1;
        epilogue.begin -= 2; //OFFSET
        epilogue.end   -= 2;
        epilogue.replacement = "ARDUINO_LOOP_END";

        translate.push_back(prologue);
        translate.push_back(epilogue);

        mainLoopDetected = true;
    }

    return true;
}

IntikiASTConsumer::IntikiASTConsumer(CompilerInstance& aCI, IntikiVisitor& aVisitor)
    : CI(aCI) , visitor(aVisitor)
{
    DFUN();
    Preprocessor &PP = CI.getPreprocessor();
    PP.addPPCallbacks(llvm::make_unique<IntikiPPCallbacksHandler>(PP));
}

void IntikiASTConsumer::HandleTranslationUnit(ASTContext& Context)
{
    DFUN();
    DeclContext* decl = Context.getTranslationUnitDecl();
    for (DeclContext::decl_iterator i = decl->decls_begin(); i != decl->decls_end(); i++)
    {
        visitor.Visit(*i);
    }
}

#define shiftbuf8(x) {\
    buf[0] = buf[1]; \
    buf[1] = buf[2]; \
    buf[2] = buf[3]; \
    buf[3] = buf[4]; \
    buf[4] = buf[5]; \
    buf[5] = buf[6]; \
    buf[6] = buf[7]; \
    buf[7] = x; \
    }
void IntikiUtil::UncommentLineDirectiveAndWrite(ofstream& ostr, const char* data, int size)
{
    DFUN();
    char buf[8];
    for(int i=0;i<size;i++) {
        shiftbuf8(data[i]);
        if( (i >= 7) ) {
            if( memcmp(buf, "\n//#line", 8) == 0) {
                ostr.write(buf,1);
                shiftbuf8(data[++i]);
                if(i >= size) break;
                shiftbuf8(data[++i]);
                if(i >= size) break;
                shiftbuf8(data[++i]);
            }
            ostr.write(buf,1);
        }
    }

    shiftbuf8('x');
    ostr.write(buf, sizeof(buf)-1 );
}

#define shiftbuf6(x) {\
    buf[0] = buf[1]; \
    buf[1] = buf[2]; \
    buf[2] = buf[3]; \
    buf[3] = buf[4]; \
    buf[4] = buf[5]; \
    buf[5] = x; \
    }
void IntikiUtil::CommentLineDirectiveAndWrite(ofstream& ostr, const char* data, int size)
{
    DFUN();
    char buf[6];
    char slash[1] = {'/'};
    for(int i=0;i<size;i++) {
        shiftbuf6(data[i]);
        if( (i >= 5) ) {
            if( memcmp(buf, "\n#line", 6) == 0) {
                ostr.write(buf,1);
                shiftbuf6(data[++i]);
                ostr.write(slash, 1);
                ostr.write(slash, 1);
                if(i >= size) break;
            }
            ostr.write(buf, 1);
        }
    }

    shiftbuf6('x');
    ostr.write(buf, sizeof(buf)-1 );
}

void IntikiUtil::FilterAndBackup(const string srcfile, const string postfix, void (*filterfunc)(ofstream&,const char*,int) )
{
    DFUN();
    DPUT(1);
    ifstream original;

    DVAL(srcfile);
    original.open(srcfile.c_str(), ios::binary);

    original.seekg(0, original.end);
    int length = original.tellg();
    DVAL(length);
    char* buf = new char[length];
    original.seekg(0, original.beg);
    original.read(buf, length);

    original.close();
    FilterAndBackup(buf, length, srcfile, postfix, filterfunc);
    delete[] buf;
}

void IntikiUtil::FilterAndBackup(const char* buf, const int size, const string srcfile, const string postfix, void (*filterfunc)(ofstream&,const char*,int) )
{
    DFUN();
    DPUT(2);
    int rc = 0;
    string backup = srcfile + postfix;

    rc = remove( backup.c_str() );
    rc = rename(srcfile.c_str(), backup.c_str() );
    
    ofstream filtered;

    filtered.open(srcfile.c_str(), ios::binary);

    filterfunc(filtered, buf, size);

    filtered.close();
}

unique_ptr<ASTConsumer> IntikiFrontendAction::CreateASTConsumer(CompilerInstance& aCI, StringRef file)
{
    DFUN();
    aCI.getASTContext().getPrintingPolicy();
    visitor = llvm::make_unique<IntikiVisitor>( IntikiVisitor(aCI) );
    return llvm::make_unique<IntikiASTConsumer>(aCI, *visitor.get() );
}


void IntikiFrontendAction::EndSourceFileAction()
{
    DFUN();
    vector<TranslateEntry>& translatelist = visitor.get()->getTranslateInfo();
    
    string srcfile = visitor.get()->getMainFileName();
    ifstream original;

    original.open(srcfile, ios::binary);

    original.seekg(0, original.end);
    uint32_t length = original.tellg();
    DVAL(length);
    original.seekg(0, original.beg);
    char* buf = NULL;
    int size = 0;

    vector<char> buffer(0);

    for( vector<TranslateEntry>::iterator i = translatelist.begin(); i != translatelist.end(); i++) {
        string& replace = i->replacement;

        if( i->begin > length || i->end > length )
        {
            DPUT("out range");
            DVAL( i->begin );
            DVAL( i->end );
            AppErr = -1;
            return;
        }

        size = (i->begin) - original.tellg();

        buf = new char[size];
        original.read(buf, size);
        for(int i=0; i<size; i++)
        {
            buffer.push_back(buf[i]);
        }
        for(unsigned int i=0; i<replace.length(); i++)
        {
            buffer.push_back(replace[i]);
        }

        delete[] buf;

        original.seekg(i->end );
    }
    size = length - original.tellg();
    buf = new char[size];
    original.read(buf, size);
    for(int i=0; i<size; i++)
    {
        buffer.push_back(buf[i]);
    }
    delete[] buf;

    original.close();

    DVAL(buffer.size());

    IntikiUtil::UncommentLineDirectiveAndBackup(buffer.data(), buffer.size(), srcfile, string(".commented") );
}

static llvm::cl::OptionCategory IntikiOptionCategory("Options for intiki");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...");

unique_ptr< vector<string> > extract_gplusplus_I(string gplusplus) {
    DFUN();
    FILE    *fp;

    string env_lang("");
    if( getenv("LANG") != NULL)
    {
	    env_lang = string( getenv("LANG") );
	    DVAL(env_lang);
    }
    putenv("LANG=C");
    string cmdline = gplusplus + string(" -xc++ -E -v nul 2>&1 >nul");
    DVAL(cmdline);

    if ( (fp=popen(cmdline.c_str(),"r")) !=NULL) {
    }
    else {
        perror ("can not exec commad");
    }

    int posix_handle = fileno(fp);
    
    __gnu_cxx::stdio_filebuf<char> filebuf(posix_handle, ios::in);
    istream is(&filebuf);
    string line;

    unique_ptr< vector<string> > inclist(new  vector<string> );

    bool search_list = false;
    while( !is.eof() )
    {
        getline(is, line);
        DVAL(line);

        string include("#include");
        string end_list("End of search list.");

        if( line.compare(0, include.length(), include) == 0 )
        {
            search_list = true;
        }
        else if( line.compare(0, end_list.length(), end_list) == 0 )
        {
            search_list = false;
        }
        else if(search_list)
        {
            string trimmed = line.erase(0, 1);
            inclist->push_back(trimmed);
        }
        else
        {
        }
    }
 
    (void) pclose(fp);

    if( env_lang != string("") )
    {
	    putenv((string("LANG=")+env_lang).c_str() );
	    DVAL( getenv("LANG") );
    }
    else
    {
	    //unsetenv( "LANG" );
    }

    return inclist;
} 
    

unique_ptr< vector<string> > extract_args(int argc, const char** argv)
{
    DFUN();
    unique_ptr< vector<string> > list(new  vector<string> );
    vector<string>& toolargs = *list;

    string gplusplus;

    const char* prev = NULL;
	string source_file = string("{source_file}");
    for(int i=1; i<argc; i++)
    {
        if(i>0) prev = argv[i-1];
        char* opt = const_cast<char*>(argv[i]);
        int len = strlen(opt);
        DVAL(opt);

        if(len < 2)
        {
            continue; //ignore
        }
        else if(opt[0] == '"' && opt[len-1] == '"')
        {
            opt[len-1] = '\0';
            opt++;
        }
        if(opt[0] == '-') {
            switch(opt[1])
            {
            case 'I':
            case 'D':
                break;
            case 'a':
            case 'C':
            case 'd':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
            case 'O':
            case 'o':
            case 'l':
            case 'L':
            case 'm':
            case 'n':
            case 'M':
            case 'V':
            case 'W':
            case 'w':
                opt = NULL;
                break;
            case 's':
                if(len > 3 && opt[2] == 't' && opt[3] == 'd')
                {
                }
                else
                {
                    opt = NULL;
                }
                break;
            case '-':
                if( string("--source_file").compare(opt) == 0)
                {
                    opt = NULL;
                }
                break;
            }
            if(opt != NULL)
            {
                string arg = string("-extra-arg=") + string(opt);
                DVAL(arg);
                toolargs.push_back(arg);
                continue;
            }
        }
        else {
            if( string("-g++").compare(prev) == 0) {
                gplusplus = opt;
                unique_ptr< vector<string> > gppincs = extract_gplusplus_I(gplusplus);
                for(int i=(*gppincs).size()-1; i>=0; i--) {
                    toolargs.push_back( string("-extra-arg=-I") + (*gppincs)[i] );
                }
            }
            else if( string("--source_file").compare(prev) == 0) {
                source_file = opt;
                DVAL(source_file);
            }
            else if( string("-o").compare(prev) == 0) {
            }
            else if( string("{source_file}").compare(opt) == 0) {
                DVAL(source_file);
                toolargs.push_back(source_file);
            }
            else if( string("{includes}").compare(opt) == 0) {
                //TODO
            }
            else {
                toolargs.push_back(string(opt) );
            }

        }
    }

    toolargs.push_back( string("-extra-arg=-w") );
    toolargs.push_back( string("-extra-arg=-nostdlib") );
    toolargs.push_back( string("-extra-arg=-nostdinc") );
    toolargs.push_back( string("-extra-arg=-nostdinc++") );
    toolargs.push_back( string("--") );

    toolargs.insert(toolargs.begin(), string(argv[0] ) );

    return list;
}

int main(int argc, const char **argv)
{
    unique_ptr<vector<string>> ptargs = extract_args(argc, argv);

    vector<string>& toolargs = *ptargs;
    int targc = toolargs.size();
    int rc = 0;

    char** targv = new char*[toolargs.size()];
    for(unsigned int i=0; i<toolargs.size(); i++)
    {
        targv[i] = const_cast<char*>(toolargs[i].c_str() );
        DVAL(targv[i]);
    }

    CommonOptionsParser op(targc, const_cast<const char**>(targv), IntikiOptionCategory);

    if( op.getSourcePathList().size() != 1 )
    {
        errs() << "Source is not assingd correctly.\n";
        for(unsigned int i=0; i<op.getSourcePathList().size(); i++) 
        {
            DVAL(op.getSourcePathList()[i]);
        }
        return -2;
    }


    string srcfile = op.getSourcePathList()[0];

    IntikiUtil::CommentLineDirectiveAndBackup(srcfile, string(".orig") );

    ClangTool Tool(op.getCompilations(), op.getSourcePathList());
    unique_ptr<FrontendActionFactory> factory = newFrontendActionFactory<IntikiFrontendAction>();
    rc = Tool.run(factory.get());

    switch(AppErr) {
        case -1:
            errs() << "loop() is not found.\n";
            return AppErr;
    }
    return 0;
}
