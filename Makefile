

install-deps:
	sudo apt install -y gcc make perl
	sudo apt install -y libavformat-dev libavcodec-dev libavformat-dev libavfilter-dev
	sudo apt install -y build-essential gdb
	sudo apt install -y vlc


build:
	# gcc -g -fsanitize=address \
	gcc -g \
		-L/usr/lib/x86_64-linux-gnu/ \
		-I/usr/include/x86_64-linux-gnu/ ioverlay.c   \
		-lavformat -lavcodec -lavutil -lavfilter
		
