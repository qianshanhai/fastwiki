
dir = lib fastwiki fastwiki-math fastwiki-httpd fastwiki-image \
	  fastwiki-zim fastwiki-stardict fastwiki-text  fastwiki-audio

all:
	@for i in $(dir); do make -C $$i all; done

clean:
	@for i in $(dir); do make -C $$i clean; done
