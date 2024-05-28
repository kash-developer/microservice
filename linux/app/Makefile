
CC				=	g++

SRC_DIR			=	.
OBJ_DIR			=	../build
#LIBSRC_DIR		=	../../android/app/src/main/cpp
LIBSRC_DIR		=	../src_lib

INCLUDE			=	-I. -I../include -I$(LIBSRC_DIR)

CFLAG			=	-g -c -Wall -fPIC -std=c++11 -DDEBUG $(INCLUDE)
#LFLAG           =   -L$(OBJ_DIR) -lmicroservice -lpthread #-lmagic
LFLAG           =   -pthread -lpthread #-lmagic

LIBS            =   $(OBJ_DIR)/libmicroservice.a

APPOBJS			=	home_device.o
APPOBJS2		=	$(patsubst %,$(OBJ_DIR)/%,$(APPOBJS))

HEADERFILE		=	Makefile 

all				:	create_dir make_lib home_device 

create_dir		:
					mkdir -p $(OBJ_DIR)
				
clean			:
					rm -rf $(OBJ_DIR)

make_lib		:
					cd $(LIBSRC_DIR);make

home_device		:	$(HEADERFILE) $(APPOBJS2) $(LIBS)
					g++ $(LFLAG) -o $(OBJ_DIR)/home_device $(APPOBJS2) ../build/libmicroservice.a


$(OBJ_DIR)/%.o :	%.cpp $(HEADERFILE)
					$(CC) $(CFLAG) $< -o $@



