CFLAGS = -O2 -W -Wall -Werror -Wshadow -Wunused -pedantic -std=c11
SOURCES = pascal.c err.c
OBJECTS = $(subst .c,.o,$(SOURCES))
DEPENDS = $(subst .c,.d,$(SOURCES))
MAIN = pascal

all: $(DEPENDS) $(MAIN)

$(DEPENDS): %.d: %.c
	$(CC) -MM $< > $@

-include $(DEPENDS)

$(MAIN): $(OBJECTS)

clean:
	$(RM) $(OBJECTS) $(MAIN) $(DEPENDS)

.PHONY: all clean
