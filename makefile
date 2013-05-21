all:
	scons -f root.sconstruct 

env:
	./ork.build/bin/ork.build.int_env.py

.PHONY: docs

docs: .
	rm -rf docs/html_doxygen
	doxygen docs/ork.doxygen

clean:
	scons -c -f root.sconstruct 
	rm -rf stage/include/ork

install:
	scons -f root.sconstruct install
