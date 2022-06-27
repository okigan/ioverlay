

install-deps:
  sudo apt install -y gcc make perl
  sudo apt install -y libavformat-devsudo
  sudo apt install -y libavformat-dev libavcodec-dev libavformat-dev
  
  
  
build:
	gcc \
		-L/usr/lib/x86_64-linux-gnu/ \
		-I/usr/include/x86_64-linux-gnu/ ioverlay.c   \
		-lavcodec -lavformat -lavfilter  -lswresample -lswscale -lavutil   
