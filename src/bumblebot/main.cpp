#include <iostream>
#include <string>

#include "types.h"
#include "bitboard.h"
#include "position.h"
#include "move.h"
#include "debugging.h"
#include "perft.h"
#include "uci.h"
#include "bench.h"


int main(int argc, char* argv[]){
    init();

    if(argc > 1 && std::string(argv[1]) == "bench"){
        run_bench(argc, argv);
        return 0;
    }

    run_uci();

    return 0;
}
