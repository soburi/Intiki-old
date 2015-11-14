LLVM_DIR    ?=D:/llvm
LLVM_INC     =$(LLVM_DIR)/include
LLVM_LIB     =$(LLVM_DIR)/lib

CFLAGS += -static -static-libgcc -static-libstdc++ \
	  -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcast-qual -Wno-missing-field-initializers \
	  -pedantic -Wno-long-long -Wno-maybe-uninitialized -Wno-comment \
	  -std=c++1y -fno-common -Woverloaded-virtual -fno-strict-aliasing -Os -DNDEBUG \
	  -I$(LLVM_INC) -I.

CXXFLAGS += -DCLANG_ENABLE_ARCMT -DCLANG_ENABLE_OBJC_REWRITER -DCLANG_ENABLE_STATIC_ANALYZER -DGTEST_HAS_RTTI=0 -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -fno-exceptions -fno-rtti 

CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangTooling.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangASTMatchers.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangFrontend.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangDriver.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangParse.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangSerialization.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangSema.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangEdit.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangAnalysis.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangAST.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangToolingCore.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangRewrite.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangLex.a
CLANG_STATIC_LIBS += $(LLVM_LIB)/libclangBasic.a

LLVM_STATIC_LIBS += $(LLVM_LIB)/libLLVMBitReader.a
LLVM_STATIC_LIBS += $(LLVM_LIB)/libLLVMCore.a
LLVM_STATIC_LIBS += $(LLVM_LIB)/libLLVMMC.a
LLVM_STATIC_LIBS += $(LLVM_LIB)/libLLVMMCParser.a
LLVM_STATIC_LIBS += $(LLVM_LIB)/libLLVMOption.a
LLVM_STATIC_LIBS += $(LLVM_LIB)/libLLVMSupport.a


SHARED_LIBS += -lpsapi -lshell32 -lole32 -lkernel32 -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -luuid -lcomdlg32 -ladvapi32 

LDFLAGS += $(CLANG_STATIC_LIBS) $(LLVM_STATIC_LIBS) $(SHARED_LIBS)

all: intiki

clean: intiki Intiki.o

intiki: Intiki.o
	$(CXX) $(CFLAGS) $< -o $@  -Wl,--major-image-version,0,--minor-image-version,0 $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CFLAGS) -o $@ -c $<

Intiki.o: Intiki.cpp Intiki.h


