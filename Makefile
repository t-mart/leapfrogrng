#Top level Makefile
#Brings together the salient targets from the project and any other misc stuff.

.PHONY: test load unload tunnel

#establish tunnels from
#	localhost:5904 -> warp1:5904 (VNC)
#	localhost:60022 -> factor004:22 (SSH)
#and bring up a shell on factor-3210
tunnel:
	ssh -L5904:warp1.cc.gatech.edu:5904 -L60022:factor004.cc.gatech.edu:22 factor-3210

test:
	make -C test

load:
	make -C src load

unload:
	make -C src unload
