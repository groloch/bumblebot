#include <iostream>
#include <string>

#include "types.h"
#include "bitboard.h"
#include "position.h"
#include "move.h"
#include "debugging.h"
#include "perft.h"
#include "uci.h"


int main(int argc, char* argv[]){
    init_magics();

    run_uci();

    return 0;
}
