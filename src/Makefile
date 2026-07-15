clean:
	cd ./core && make clean
	cd ./devices && make clean
preall:
	cd ./core && make all
	cd ./devices && make all

all: preall
	@cd ../extra && python makeheader.py
