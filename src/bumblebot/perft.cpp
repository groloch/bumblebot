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

    u64 nodes{0};
    for (int i = 0; i < moveList.size; ++i) {
        position.applyMove(moveList.moves[i]);
        nodes += perftDriver(position, depth - 1);
        position.undoMove(moveList.moves[i]);
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
                moveUci(move.from(), move.to(), move.isPromotion() ? move.promotionPieceType() : PieceType::None),
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

const std::vector<perft::PerftTest>& perft::testPositions(){
    static const std::vector<perft::PerftTest> tests{
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7, 3195901860ull},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 6, 8031647685ull},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 8, 3009794393ull},
        {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6, 706045033ull},
        {"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 6, 706045033ull},
        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ", 5, 89941194ull},
        {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 6, 6923051137ull},

        // chess960
        {"bqnb1rkr/pp3ppp/3ppn2/2p5/5P2/P2P4/NPP1P1PP/BQ1BNRKR w HFhf - 2 9", 6, 227689589ull},
        {"2nnrbkr/p1qppppp/8/1ppb4/6PP/3PP3/PPP2P2/BQNNRBKR w HEhe - 1 9", 6, 590751109ull},
        {"b1q1rrkb/pppppppp/3nn3/8/P7/1PPP4/4PPPP/BQNNRKRB w GE - 1 9 ", 6, 177654692ull},
        {"qbbnnrkr/2pp2pp/p7/1p2pp2/8/P3PP2/1PPP1KPP/QBBNNR1R w hf - 0 9", 6, 274103539ull},
        {"1nbbnrkr/p1p1ppp1/3p4/1p3P1p/3Pq2P/8/PPP1P1P1/QNBBNRKR w HFhf - 0 9", 6, 1250970898ull},
        {"qnbnr1kr/ppp1b1pp/4p3/3p1p2/8/2NPP3/PPP1BPPP/QNB1R1KR w HEhe - 1 9", 6, 775718317ull},
        {"q1bnrkr1/ppppp2p/2n2p2/4b1p1/2NP4/8/PPP1PPPP/QNB1RRKB w ge - 1 9", 6, 649209803ull},
        {"qbn1brkr/ppp1p1p1/2n4p/3p1p2/P7/6PP/QPPPPP2/1BNNBRKR w HFhf - 0 9", 6, 377184252ull},
    };
    return tests;
}

void run_perft_tests(){
    const std::vector<perft::PerftTest>& tests{perft::testPositions()};

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
