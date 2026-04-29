# include "uci.h"

# include "types.h"
# include "perft.h"
# include "move.h"
# include "position.h"
# include "debugging.h"

# include <iostream>
# include <string>
# include <sstream>



void run_uci() {
    while(true){
        std::string line;
        std::getline(std::cin, line);

        std::stringstream ss{line};
        std::string command;
        ss >> command;

        if(command == "perft"){
            std::string fenPieces, fenTurn, fenCastles, fenEnPassant, fen50, fenPlies;
            int depth{0};
            if(ss >> depth >> fenPieces >> fenTurn >> fenCastles >> fenEnPassant >> fen50 >> fenPlies){
                std::string fen = fenPieces + " " + fenTurn + " " + fenCastles + " " + fenEnPassant + " " + fen50 + " " + fenPlies;
                run_perft(fen.c_str(), depth);
            }else{
                // std::string fen{"r3k2r/p1ppqpb1/bn2pnp1/3P4/1p2P1N1/2N2Q2/PPPBBPpP/1R2K2R b Kkq - 1 2"};
                // Position position{fen.c_str()};
                // Move move{
                //     squares::g2, squares::h1, false, false, true, true, true, PieceType::Bishop, PieceType::Rook
                // };
                // position.applyMove(move);
                // Position position2{"r3k2r/p1ppqpb1/bn2pnp1/3P4/1p2P1N1/2N2Q2/PPPBBP1P/1R2K2b w kq - 0 3"};

                // u64 nodes1{run_perft(position, 1).nodes};
                // u64 nodes2{run_perft(position2, 1).nodes};

                // std::cout << "Nodes1: " << nodes1 << std::endl;
                // std::cout << "Nodes2: " << nodes2 << std::endl;

                // comparePositions(position, position2);
                
                run_perft_tests();
            }
        } else if(command == "quit"){
            break;
        }
    }
}