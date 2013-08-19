#
#
#

SOURCES = n3lar.c
INCLUDEDIRS = -I/usr/include/opencv
LIBS = -lm -lopencv_highgui -lopencv_core -lopencv_imgproc
FLAGS = -O3 -Wno-unused-result
OUTPUT = n3lar

#
#
#

output:
	$(CC) $(SOURCES) $(LIBS) $(INCLUDEDIRS) $(FLAGS) -o $(OUTPUT)