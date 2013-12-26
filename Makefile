	user = $(shell whoami)
	ifeq ($(user),root)
	INSTALL_DIR = /usr/lib/lv2
	else 
	INSTALL_DIR = ~/.lv2
	endif

	NAME = sc_record
	BUNDLE = $(NAME).lv2
	VER = 0.1

	CXXFLAGS = -I. -Wall -ffast-math  `pkg-config --cflags sndfile`
	LDFLAGS = -I. -shared -Llibrary -lc -lm  -fPIC -DPIC `pkg-config --libs sndfile` 

	OBJECTS = $(NAME).cpp 

.PHONY : all clean install uninstall 

all : $(NAME)
	mkdir -p ./$(BUNDLE)
	cp ./*.ttl ./$(BUNDLE)
	mv ./*.so ./$(BUNDLE)

clean :
	rm -f $(NAME).so
	rm -rf ./$(BUNDLE)

install : all
	mkdir -p $(DESTDIR)$(INSTALL_DIR)/$(BUNDLE)
	install ./$(BUNDLE)/* $(DESTDIR)$(INSTALL_DIR)/$(BUNDLE)

uninstall :
	@rm -rf $(INSTALL_DIR)/$(BUNDLE)

$(NAME) : 
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(NAME).so
