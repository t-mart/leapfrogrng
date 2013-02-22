#Top level Makefile
#Brings together the salient targets from the project and any other misc stuff.

.PHONY: test load unload tunnel

#establish tunnels from
#	localhost:5904 -> warp1:5904 (VNC)
#	localhost:60022 -> factor004:22 (SSH)
#and bring up a shell on factor-3210
tunnel:
	ssh -L5904:warp1.cc.gatech.edu:5904 -L60022:factor004.cc.gatech.edu:22 factor-3210

#for this to work, run this once at your shell:
#	export GTUSER=<tmartin9>
#must have tunneled already s.t. localhost:60022 points to factor004 (run 'make tunnel')
#also, unfortunately assumes your project is at ~/leapfrogrng (which prolly
#isn't too much to ask)
deploy:
	#scp -P 60022 $(shell git ls-files) $(GTUSER)@localhost:leapfrogrng
	rsync -avzR -e 'ssh -p 60022' $(shell git ls-files) $(GTUSER)@localhost:leapfrogrng

test:
	make -C test

load:
	make -C src load

unload:
	make -C src unload

syslog:
	cp rsyslog.conf /etc/rsyslog.conf && sudo /etc/init.d/rsyslog restart
