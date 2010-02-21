all: compile

compile: src
	@cd src; make

clean: src
	@cd src; make clean
