
# Makefile for libeeval, (C) Ilyes Gouta, 2007-2008.

ARCH = linux-x86

BUILDDIRS = generated lib obj bin

BISON_SOURCE = eeval.yc
FLEX_SOURCE = eeval.lex

TOKENIZER_SOURCE = generated/tokenizer.c
TOKENIZER_HEADER = generated/tokenizer.h
FRONTEND_SOURCE = generated/frontend.c

EEVAL_SOURCE = eeval.c ast.c identifier.c
EEVAL_BACKEND_SOURCE = $(ARCH)/registers.c $(ARCH)/stack.c $(ARCH)/optimizer.c $(ARCH)/opcodes.c

EEVAL_GENERATED_OBJS = $(patsubst generated/%.c,obj/%.o, $(TOKENIZER_SOURCE) $(FRONTEND_SOURCE))
EEVAL_OBJS = $(patsubst %.c,obj/%.o, $(EEVAL_SOURCE))
EEVAL_BACKEND_OBJS = $(patsubst $(ARCH)/%.c,obj/%.o, $(EEVAL_BACKEND_SOURCE))

TEST_SOURCE = test.c
TEST_OBJS = $(patsubst %.c,obj/%.o, $(TEST_SOURCE))

all: builddir libeeval test

obj/%.o: generated/%.c
	gcc -c -g -O0 -DYYDEBUG=1 -I. -Igenerated $< -o $@

obj/%.o: %.c
	gcc -c -g -O0 -DYYDEBUG=1 -I. -Igenerated $< -o $@

obj/%.o: $(ARCH)/%.c
	gcc -c -g -O0 -DYYDEBUG=1 -I. -Igenerated $< -o $@

builddir:
	@mkdir -p $(BUILDDIRS)

$(TOKENIZER_SOURCE): $(FLEX_SOURCE)
	flex --noline --header-file=$(TOKENIZER_HEADER) -o $(TOKENIZER_SOURCE) $(FLEX_SOURCE)

$(FRONTEND_SOURCE): $(BISON_SOURCE)
	bison --no-lines -d -o $(FRONTEND_SOURCE) $(BISON_SOURCE)

libeeval: $(TOKENIZER_SOURCE) $(FRONTEND_SOURCE) $(EEVAL_OBJS) $(EEVAL_GENERATED_OBJS) $(EEVAL_BACKEND_OBJS)
	@ar cr lib/$@.a $(EEVAL_OBJS) $(EEVAL_GENERATED_OBJS) $(EEVAL_BACKEND_OBJS)

test: $(TEST_OBJS)
	gcc $(TEST_OBJS) -Llib -leeval -o bin/test

clean:
	@rm -rf $(BUILDDIRS)
