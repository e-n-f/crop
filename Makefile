PGMS := crop scale deface markface jpegsize bigscale average blank scale2048

all: $(PGMS)

clean:
	rm -f $(PGMS)

crop: crop.c
	cc -Wall -o crop -O -g crop.c -I/Users/enf/include -L/Users/enf/lib -ljpeg

overlay: overlay.c
	cc -Wall -o overlay -O -g overlay.c -I/Users/enf/include -L/Users/enf/lib -ljpeg

bcrop: bcrop.c
	cc -Wall -o bcrop -O -g bcrop.c -I/Users/enf/include -L/Users/enf/lib -ljpeg

blank: blank.c
	cc -Wall -o blank -O -g blank.c -I/Users/enf/include -L/Users/enf/lib -ljpeg

jpegsize: jpegsize.c
	cc -Wall -o jpegsize -O -g jpegsize.c -I/Users/enf/include -L/Users/enf/lib -ljpeg

scale: scale.c
	cc -Wall -o scale -O -g scale.c -I/Users/enf/include -L/Users/enf/lib -ljpeg

scale2048: scale2048.c
	cc -Wall -o scale2048 -O -g scale2048.c -I/Users/enf/include -L/Users/enf/lib -ljpeg -lpng -lz

bigscale: bigscale.c
	cc -Wall -o bigscale -O -g bigscale.c -I/Users/enf/include -L/Users/enf/lib -ljpeg


deface: deface.c
	cc -Wall -o deface -O -g deface.c -I/Users/enf/include -L/Users/enf/lib -ljpeg -lm

markface: markface.c
	cc -Wall -o markface -O -g markface.c -I/Users/enf/include -L/Users/enf/lib -ljpeg -lm

average-big: average-big.c
	cc -Wall -o average-big -O -g average-big.c -I/Users/enf/include -L/Users/enf/lib -ljpeg

average: average.c
	cc -Wall -o average -O -g average.c -I/Users/enf/include -L/Users/enf/lib -ljpeg

H = $(wildcard *.h) $(wildcard *.hpp)
C = $(wildcard *.c) $(wildcard *.cpp)

indent:
	clang-format -i -style="{BasedOnStyle: Google, IndentWidth: 8, UseTab: Always, AllowShortIfStatementsOnASingleLine: false, ColumnLimit: 0, ContinuationIndentWidth: 8, SpaceAfterCStyleCast: true, IndentCaseLabels: false, AllowShortBlocksOnASingleLine: false, AllowShortFunctionsOnASingleLine: false, SortIncludes: false}" $(C) $(H)
