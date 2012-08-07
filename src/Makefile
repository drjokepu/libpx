CC=clang
AR=ar
ARFLAGS=-rcs
WARNINGFLAGS=-Wall -Wunreachable-code -Wshorten-64-to-32 -Wunused-parameter -Wignored-qualifiers -Wsign-conversion
CFLAGS=-I. $(WARNINGFLAGS) -g -O0
#CFLAGS=-I. -Wall -O4
LDFLAGS=
#LDFLAGS=-O4
EXECUTABLE_LDFLAGS=-ledit -lcurses -Xlinker -dead_strip
SECURITY_OBJECTS=security_common_crypto.o
OBJECTS=connection.o connection_params.o error.o message.o parameter.o response.o result.o query.o security.o utility.o $(SECURITY_OBJECTS)
PXOBJECTS=px.o

NAME=libpx
STATICLIB=$(NAME).a
DYNAMICLIB=$(NAME).dylib
EXECUTABLE=px
DESTROOT=/usr/local

%.o : %.c
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(STATICLIB) $(DYNAMICLIB) $(EXECUTABLE)

$(STATICLIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(DYNAMICLIB): $(OBJECTS)
	$(CC) -shared $(LDFLAGS) $(OBJECTS) -o $@

$(EXECUTABLE): $(STATICLIB) $(PXOBJECTS)
	$(CC) $(LDFLAGS) $(EXECUTABLE_LDFLAGS) $(PXOBJECTS) $(STATICLIB) -o $@

clean:
	rm -rf $(OBJECTS) $(PXOBJECTS) $(STATICLIB) $(DYNAMICLIB) $(EXECUTABLE)

