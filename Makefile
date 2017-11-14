SRC ?= main.c

SRCDIR ?= .
OBJDIR ?= .objdir
DEPDIR ?= .depdir

##########
# less important stuff
SHELL := /bin/sh
MKDIR ?= mkdir
RMDIR ?= rmdir

COLOR = echo -en '\e[1;34m'
RESET = echo -en '\e[0m'

##########
# warnings

# enable a lot of warnings and then some more
WARNINGS := -Wall -Wextra
# shadowing variables, are you sure?
WARNINGS += -Wshadow
# sizeof(void)
WARNINGS += -Wpointer-arith
# unsafe pointer cast qualifiers: `const char*` is cast to `char*`
WARNINGS += -Wcast-qual
# most of the time you don't want this
WARNINGS += -Werror=implicit-function-declaration
# let's stick with C89
# WARNINGS += -Werror=declaration-after-statement
# why warning for comments inside comments
WARNINGS += -Wno-comment

# be more strict
ifeq ($(shell $(CC) -dumpversion),4.7)
	# welcome to 2012
	# -Wpedantic is available since gcc 4.8
	WARNINGS += -pedantic
else
	WARNINGS += -Wpedantic
endif

# functions should be declared
#WARNINGS += -Wmissing-declarations

# C-specific warnings
CWARNINGS := $(WARNINGS)
# warn about fn() instead of fn(void)
CWARNINGS += -Wstrict-prototypes
# this probably should be enabled only in bigger projects
#CWARNINGS += -Wmissing-prototypes

# old standards (ANSI C, ANSI C++)
CFLAGS ?= $(CWARNINGS) -std=c11 -O2
CXXFLAGS ?= $(WARNINGS) -std=c++14 -O2
# for future use if needed
DEPFLAGS ?=
LDFLAGS ?= -lm

##########

# add unicode support
CFLAGS += $(shell pkg-config --cflags-only-I icu-uc icu-io)
CXXFLAGS += $(shell pkg-config --cflags-only-I icu-uc icu-io)
LDFLAGS += $(shell pkg-config --libs-only-l --libs-only-L icu-uc icu-io)

EXE := $(basename $(firstword $(SRC)))

OBJ := $(addprefix $(OBJDIR)/, \
	$(addsuffix .o, $(SRC)) \
)

DEP := $(addprefix $(DEPDIR)/, \
	$(addsuffix .d, $(SRC)) \
)

# be silent unless VERBOSE
ifndef VERBOSE
.SILENT: ;
endif

# default target
.PHONY: all
all: $(EXE) ## build executable

.PHONY: run
run: $(EXE) ## run program
	@./$(EXE)

.PHONY: debug
debug: CFLAGS := $(filter-out -O2,$(CFLAGS)) -D_DEBUG -g
debug: CXXFLAGS := $(filter-out -O2,$(CXXFLAGS)) -D_DEBUG -g
debug: $(EXE) ## build with debug enabled

.PHONY: debugrun
debugrun: debug run ## run debug version

$(EXE): $(OBJ)
	$(COLOR)
	echo "Link $^ -> $@"
	$(RESET)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

$(OBJDIR)/%.c.o: $(DEPDIR)/%.c.d
	@$(COLOR)
	echo "Compile $(SRCDIR)/$*.c -> $(OBJDIR)/$*.c.o"
	@$(RESET)
	$(CC) $(CFLAGS) -c -o $@ $*.c

$(OBJDIR)/%.cpp.o: $(DEPDIR)/%.cpp.d
	@$(COLOR)
	echo "Compile $(SRCDIR)/$*.cpp -> $(OBJDIR)/$*.cpp.o"
	@$(RESET)
	$(CXX) $(CXXFLAGS) -c -o $@ $*.cpp

$(DEPDIR)/%.c.d: $(SRCDIR)/%.c
	@$(COLOR)
	echo "Dependencies $(SRCDIR)/$*.c -> $(DEPDIR)/$*.c.d"
	@$(RESET)
	$(CC) $(DEPFLAGS) -MM -MT '$$(OBJDIR)/$*.c.o' -MF $@ $<
	sed -i 's,^\([^:]\+.o\):,\1 $$(DEPDIR)/$*.c.d:,' $@

$(DEPDIR)/%.cpp.d: $(SRCDIR)/%.cpp
	@$(COLOR)
	echo "Dependencies $(SRCDIR)/$*.cpp -> $(DEPDIR)/$*.cpp.d"
	@$(RESET)
	$(CXX) $(DEPFLAGS) -MM -MT '$$(OBJDIR)/$*.cpp.o' -MF $@ $<
	sed -i 's,^\([^:]\+.o\):,\1 $$(DEPDIR)/$*.c.d:,' $@

# include dependencies
-include $(wildcard $(DEP))

# depend on directory
$(OBJ): | $(OBJDIR)/.keepme
$(DEP): | $(DEPDIR)/.keepme

# create directory
$(OBJDIR)/.keepme:
	-$(MKDIR) $(OBJDIR)
	touch $@

# create directory
$(DEPDIR)/.keepme:
	-$(MKDIR) $(DEPDIR)
	touch $@

# delete stuff
.PHONY: clean
clean: mostlyclean ## delete everything this Makefile created
	-$(RM) $(EXE)

.PHONY: mostlyclean
mostlyclean: ## delete everything created, leave executable
	@$(COLOR)
	echo "Cleaning"
	@$(RESET)
ifneq ($(wildcard $(OBJDIR)),)
	-$(RM) $(OBJ)
	-$(RM) $(OBJDIR)/.keepme
	-$(RMDIR) $(OBJDIR)
endif
ifneq ($(wildcard $(DEPDIR)),)
	-$(RM) $(DEP)
	-$(RM) $(DEPDIR)/.keepme
	-$(RMDIR) $(DEPDIR)
endif

.PHONY: forceclean
forceclean: ## force delete all created temporary folders
	@$(COLOR)
	echo "Force cleaning"
	@$(RESET)
ifneq ($(wildcard $(OBJDIR)),)
	-$(RM) -r $(OBJDIR)
endif
ifneq ($(wildcard $(DEPDIR)),)
	-$(RM) -r $(DEPDIR)
endif

.PHONY: help
help: ## show this help
	@awk -F':.*##' '/: [^#]*##/{ printf("%12s: %s\n", $$1, $$2)}' $(MAKEFILE_LIST)

