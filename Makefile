#just some factor connection helpers
#!!!!!!make sure to look at ssh_config_entry!!!!!!

#establish tunnels from
#	localhost:5904 -> warp1
#	localhost:60022 -> warp1
#and bring up a shell on factor-3210
tunnel:
	ssh -L5904:warp1.cc.gatech.edu:5904 -L60022:factor004.cc.gatech.edu:22 factor-3210

vnc:
	vncviewer localhost:4

.PHONY: test
test:
	make -C test
