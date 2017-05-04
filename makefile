default:
	g++ source/lz77.cpp -o bin/lz77 2> "gcc.txt"
clean:
	rm -f bin/lz77
	rm -f data/model.obj.comp
	rm -f data/model.obj.comp.decomp