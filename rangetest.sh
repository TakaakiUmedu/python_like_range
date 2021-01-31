c++ --std=c++17 -O2 -o rangetest rangetest.cpp
c++ --std=c++17 -O2 -D TEST_FOR -S -o rangetest-for.s rangetest.cpp
c++ --std=c++17 -O2 -D TEST_RANGE -S -o rangetest-range.s rangetest.cpp
c++ --std=c++17 -O2 -D TEST_FOR -o rangetest-for rangetest.cpp
c++ --std=c++17 -O2 -D TEST_RANGE -o rangetest-range rangetest.cpp
diff rangetest-for.s rangetest-range.s
./rangetest.exe
./rangetest-for.exe 10
./rangetest-for.exe 20
./rangetest-range.exe 10
./rangetest-range.exe 20
