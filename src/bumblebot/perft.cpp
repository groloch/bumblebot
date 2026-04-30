#include "perft.h"

#include "debugging.h"
#include "move.h"

#include <chrono>


u64 perftDriver(Position& position, int depth) {
    MoveList moveList{};
    generateMoves<MoveType::Legal>(position, moveList);

    if (depth == 0) {
        return 1;
    }
    if (depth == 1) {
        return moveList.size;
    }

    PositionData currentData{position.positionData};
    Bitboard currentAttacks{position.opponentAttacks}, currentCheckers{position.checkers};

    u64 nodes{0};
    for (int i = 0; i < moveList.size; ++i) {
        position.applyMove(moveList.moves[i]);
        u64 newnodes = perftDriver(position, depth - 1);
        nodes += newnodes;
        position.undoMove(moveList.moves[i], currentData);
        position.opponentAttacks = currentAttacks;
        position.checkers = currentCheckers;
        position.updatePins();
    }

    return nodes;
}


perft::PerftResult run_perft(Position& position, int depth){
    perft::PerftResult result{{}, 0};

    if(depth > 0){
        MoveList moveList;
        generateMoves<MoveType::Legal>(position, moveList);
        for (int i = 0; i < moveList.size; ++i) {
            Position nextPos{position};
            Move move{moveList.moves[i]};
            nextPos.applyMove(moveList.moves[i]);
            u64 newnodes = perftDriver(nextPos, depth - 1);

            perft::NodeResult nodeResult{
                moveUci(move.from, move.to, move.isPromotion ? move.promotionPieceType : PieceType::None),
                newnodes
            };
            std::cout << nodeResult.moveUci << " " << nodeResult.nodes << std::endl;
            result.nodeResults.push_back(nodeResult);
            result.nodes += newnodes;
        }
    }
    return result;
}

perft::PerftResult run_perft(const char* fen, int depth){
    Position position{fen};
    perft::PerftResult result = run_perft(position, depth);
    std::cout << fen << " Perft depth " << depth << ": " << result.nodes << " nodes" << std::endl;
    return result;
}

void run_perft_tests(){
    std::array<perft::PerftTest, 38> tests{{
        // {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7, 3195901860ull},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324ull},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609ull},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281ull},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902ull},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400ull},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20ull},

        // {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 6, 8031647685ull},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690ull},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603ull},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862ull},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039ull},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48ull},

        // {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 8, 3009794393ull},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 7, 178633661ull},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6, 11030083ull},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624ull},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238ull},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 3, 2812ull},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2, 191ull},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1, 14ull},

        // {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6, 706045033ull},
        {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 5, 15833292ull},
        {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333ull},
        {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3, 9467ull},
        {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 2, 264ull},
        {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 1, 6ull},

        // {"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 6, 706045033ull},
        {"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 5, 15833292ull},
        {"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 4, 422333ull},
        {"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 3, 9467ull},
        {"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 2, 264ull},
        {"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 1, 6ull},

        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ", 5, 89941194ull},
        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ", 4, 2103487ull},
        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ", 3, 62379ull},
        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ", 2, 1486ull},
        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ", 1, 44ull},

        // {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 6, 6923051137ull},
        {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5, 164075551ull},
        {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594ull},
        {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890ull},
        {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 2, 2079ull},
        {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 1, 46ull}
    }};

    int passedTests{0};
    int failedTests{0};
    u64 totalNodes{0};

    auto totalStart = std::chrono::high_resolution_clock::now();

    for(int i = 0; i < tests.size(); ++i) {
        perft::PerftTest test{tests[i]};

        auto start = std::chrono::high_resolution_clock::now();
        perft::PerftResult testResult{run_perft(test.fen, test.depth)};
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> testElapsed = end - start;

        totalNodes += testResult.nodes;

        if(testResult.nodes != test.expectedNodes){
            ++failedTests;
            std::cerr << "Test " << i << " failed: expected " << test.expectedNodes << " nodes, got " << testResult.nodes << " (" << testElapsed.count() << " ms)" << std::endl;
        } else {
            ++passedTests;
            std::cout << "Test " << i << " passed (" << testElapsed.count() << " ms)" << std::endl;
        }
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> totalElapsed = totalEnd - totalStart;

    int totalSeconds = static_cast<int>(totalElapsed.count() / 1000);
    int totalMilliseconds = static_cast<int>(totalElapsed.count()) % 1000;

    std::cout << passedTests << " tests passed, " << failedTests << " tests failed. Total time ";
    std::cout << totalSeconds << ".";
    std::cout << totalMilliseconds << " s" << std::endl;
    std::cout << "Total nodes: " << totalNodes << " (";
    std::cout << (totalNodes / totalSeconds) << " nodes/s)" << std::endl;
}