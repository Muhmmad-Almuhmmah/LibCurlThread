INCLUDEPATH += $$PWD/

SOURCES +=  $$PWD/curlThread.cpp
HEADERS +=  $$PWD/curlThread.h

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

INCLUDEPATH += $$PWD/curl/ $$PWD/ $$PWD/curl/

HEADERS += \
        $$PWD/curl/curl.h \
        $$PWD/curl/curlver.h \
        $$PWD/curl/easy.h \
        $$PWD/curl/mprintf.h \
        $$PWD/curl/multi.h \
        $$PWD/curl/stdcheaders.h \
        $$PWD/curl/system.h \
        $$PWD/curl/typecheck-gcc.h \
        $$PWD/curl/urlapi.h


LIBS += -L$$PWD/curl/lib/ -lcurl
LIBS += -L$$PWD/curl/lib/ -llibcurl.dll
INCLUDEPATH += $$PWD/curl
DEPENDPATH += $$PWD/curl
