cc: cc.cpp c.tab.cpp c.lex.cpp
	# g++ c.tab.cpp c.lex.cpp cc.cpp -lm -ll -lfl -o $@
	clang++ -g -O3 c.tab.cpp c.lex.cpp cc.cpp -lm -ll -lfl -Wno-unknown-warning-option `llvm-config --cxxflags --ldflags --system-libs --libs core` -o $@  

c.tab.cpp c.tab.hpp: c.y
	bison -o c.tab.cpp -d c.y

c.lex.cpp: c.l c.tab.hpp
	flex -o c.lex.cpp -l c.l

clean::
	rm -f c.tab.cpp c.tab.hpp c.lex.cpp cc c.output
