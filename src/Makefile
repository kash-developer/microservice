
CC				=	g++
AR				=	ar

SRC_DIR			=	.
#OBJ_DIR			=	../../../../../linux/build
OBJ_DIR			=	../linux/build

#INCLUDE			=	-I. -I../../../../../linux/include
INCLUDE			=	-I. -I../linux/include

CFLAG			=	-g -c -Wall -fPIC -std=c++11 -DDEBUG $(INCLUDE)
LFLAG           =   -lpthread #-lmagic

LIBOBJS			=	trace.o serial_device.o http_message.o http_request.o\
					http_response.o http_server.o home_device_lib.o jsoncpp.o\
					tools.o controller_device.o light_device.o doorlock_device.o\
					vantilator_device.o gasvalve_device.o curtain_device.o\
					boiler_device.o temperature_controller_device.o\
					breaker_device.o prevent_crime_ext_device.o system_aircon_device.o\
					powergate_device.o remote_inspector_device.o forwarder_lib.o\
					httpu_server.o httpu_message.o phone_device.o

LIBOBJS2		=	$(patsubst %,$(OBJ_DIR)/%,$(LIBOBJS))

HEADERFILE		=	Makefile *.h 

all				:	create_dir libmicroservice

create_dir		:
					mkdir -p $(OBJ_DIR)

clean			:
					rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/*.a

libmicroservice :	$(HEADERFILE) $(LIBOBJS2)
					$(AR) -r $(OBJ_DIR)/libmicroservice.a $(LIBOBJS2)


$(OBJ_DIR)/%.o :	%.cpp $(HEADERFILE)
					$(CC) $(CFLAG) $< -o $@



