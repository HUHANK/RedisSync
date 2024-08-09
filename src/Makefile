.SUFFIXES:
.SUFFIXES: .h .c .cpp .o

TARGET = RedisSync
LIB_DIR = $(HOME)/lib
CURRENT_DIR = $(PWD)
SHARE_DIR = $(HOME)/share
ROCKETMQ_DIR = $(HOME)/third_party/rocketmq
ThirdParty_DIR = $(HOME)/third_party
OBJ_DIR = $(CURRENT_DIR)/$(TARGET).obj

TMP_DIR = tmp0123456789
INSTALL_PACKAGE = install_package.tar.gz
INSTALL_BIN = $(TARGET).bin

OPT_DBG_FLAGS = -g 
INCFILES = -I$(CURRENT_DIR) -I$(SHARE_DIR) -I$(ROCKETMQ_DIR)/include -I$(ThirdParty_DIR) \
			-I$(SHARE_DIR)/ylt/standalone -I$(SHARE_DIR)/ylt/thirdparty

DEFINES = -fPIC -fpermissive
CPPFLAGS = $(OPT_DBG_FLAGS) $(INCFILES) $(DEFINES)
LDFLAGS = -L$(LIB_DIR) -L$(ROCKETMQ_DIR) -lshare -lrocketmq -lpthread -lstdc++fs -luuid -l:libocci.so.21.1 -l:libclntsh.so.21.1
CC = g++ -m64 -std=c++17
MAKEDEPEND = makedepend

DIRS := $(shell find . -maxdepth 3 -type d)
SRCS_CPP := $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))

OBJS := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS_CPP))

dep_files := $(patsubst %, %.d, $(OBJS))

all: $(TARGET)

package:$(TARGET)
	@echo "make package start..."
	rm -rf $(TMP_DIR) $(INSTALL_PACKAGE)
	mkdir $(TMP_DIR)
	cp $(TARGET) ./$(TMP_DIR)/
	cp service.sh ./$(TMP_DIR)/$(TARGET).sh
	tar -zcf $(INSTALL_PACKAGE) -C $(TMP_DIR)/ .
	rm -rf $(TMP_DIR)
	cat install.sh $(INSTALL_PACKAGE) > $(INSTALL_BIN)
	rm -rf $(INSTALL_PACKAGE)
	@echo $(INSTALL_BIN) "generated, make package finished."

$(TARGET):$(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	@echo -e "\n\t-------------\033[1;32mCompile Success Done!\033[0m-------------------"
	@echo -e " Compile Done Time:" $(shell date +"%Y-%m-%d %H:%M:%S") "\n"

$(OBJ_DIR)/%.o: %.cpp
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(CC) $(CPPFLAGS) -c $< -o $@  -MMD -MF $@.d

-include $(dep_files)

clean:
	rm -rf $(TARGET)
	rm -rf $(OBJ_DIR)
	rm -rf $(INSTALL_BIN)