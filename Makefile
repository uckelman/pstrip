
INCDIR := include
SRCDIR := src
OBJDIR := bin
DEPDIR := bin
BINDIR := bin

CXX := g++
CPPFLAGS := -c -O -g -pg -W -Wall -Wextra -pedantic -pipe -MMD -MP
#CPPFLAGS := -c -O3 -W -Wall -Wextra -pedantic -pipe -MMD -MP
INCLUDES := -I$(INCDIR)
LDLIBS := -lstdc++ -lpff

SOURCES := main.cpp json_writer.cpp
OBJECTS := $(SOURCES:.cpp=.o)
DEPS    := $(OBJECTS:.o=.d)

SOURCES := $(SOURCES:%=$(SRCDIR)/%)
OBJECTS := $(OBJECTS:%=$(OBJDIR)/%)
DEPS    := $(DEPS:%=$(DEPDIR)/%)
BINARY  := $(BINDIR)/pstrip

all: $(BINARY)

debug: CPPFLAGS += -g -pg -fprofile-arcs -ftest-coverage
debug: CPPFLAGS := $(filter-out -O3,$(CPPFLAGS))
debug: LDFLAGS += -pg -fprofile-arcs
debug: all

-include $(DEPS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(INCLUDES) -c -o $@ $<

$(BINARY): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	$(RM) $(BINARY) $(OBJECTS) $(DEPS)

.PHONY: all clean debug
