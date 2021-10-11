
ORACLE_HOME=/u01/app/oracle/product/11.2.0/db_1
#ORACLE_HOME=/u01/app/oracle/product/11.2.0/db

CPPFLAGS = -fno-strict-aliasing -fno-exceptions -fno-rtti -fPIC
DEBUG = -g
#DEBUG = 
CXX = /usr/bin/g++  $(CPPFLAGS) $(DEBUG)


# Targets
all: pump
RM = /bin/rm
RMF = $(RM) -f

lgc_network.o: lgc_network.cpp lgc_network.h
	$(CXX) $< -c -o $@

lgc_param.o: lgc_param.cpp lgc_param.h
	$(CXX) $< -c -o $@

lgc_sendMedia.o: lgc_sendMedia.cpp lgc_sendMedia.h
	$(CXX) $< -c -o $@

lgc_fileList.o: lgc_fileList.cpp lgc_fileList.h
	$(CXX) $< -c -o $@

lgc_ckpt.o: lgc_ckpt.cpp lgc_ckpt.h
	$(CXX) $< -c -o $@

lgc_api.o: lgc_api.cpp lgc_api.h
	$(CXX) $< -c -o $@

pump: lgc_api.o lgc_network.o lgc_param.o lgc_sendMedia.o lgc_fileList.o lgc_ckpt.o lgc_main.cpp
	$(CXX) lgc_main.cpp lgc_api.o lgc_network.o lgc_param.o lgc_sendMedia.o lgc_fileList.o lgc_ckpt.o -o $@
clean: 
	$(RMF) *.o *.bak *~ *.intpro *.extpro 1 2 3
	$(RMF) main test testchuck testfile lgc_main pump
