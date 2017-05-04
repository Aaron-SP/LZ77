# LZ77
LZ77 Compression Algorithm

This tool is an implementation of the LZ77 Compression Algorithm. GNU/C++ is required to build the project.

'make' - will build the project
'bin/lz77 compress data/model.obj' - will compress the obj file
'bin/lz77 decompress data/model.obj.comp' - will decompress the obj file
'diff -q data/model.obj data/model.obj.comp.decomp' - will test for a difference between the original file and the decompressed file
'make clean' - will clean the project
