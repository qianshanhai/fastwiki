
dir = lib fastwiki fastwiki-math fastwiki-httpd fastwiki-image \
	  fastwiki-zim fastwiki-stardict fastwiki-text 

all:
	@for i in $(dir); do cd $$i && make all && cd -; done

clean:
	@for i in $(dir); do cd $$i && make clean && cd -; done
