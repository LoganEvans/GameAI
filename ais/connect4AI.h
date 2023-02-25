#pragma once

#include <atomic>
#include <chrono>
#include <limits>

#include "proto/game.pb.h"

namespace ais::conn4 {

class Board {
public:
  static constexpr int kRows = 6;
  static constexpr int kCols = 7;
  static constexpr uint64_t kColMask = (1ULL << (kRows + 1)) - 1;

  struct Spot {
    int32_t row{0};
    int32_t col{0};
  };

  enum class Player {
    One = 0,
    Two = 1,
    None = 2,
  };

  struct LegalMoves {
    static constexpr int8_t kIllegal = kRows;
    std::array<int8_t, 7> legalRowInCol;
  };

  Board() {}
  Board(const Board &other) : board_(other.board_) {}

  Board &operator=(const Board &other) {
    board_ = other.board_;
    return *this;
  }

  // Board Index
  static inline int bIdx(Player player) {
    return static_cast<int>(player);
  }

  Player getPlayer(Spot spot) const;

  void move(Spot spot, Player player);

  Player winner() const;

  LegalMoves legalMoves() const;

  Player nextPlayer() const;

private:
  std::array<uint64_t, 2> board_{};
};

class State {
public:
  static constexpr uint64_t kCertain = std::numeric_limits<uint64_t>::max();

  Board::Spot pickMove() const;

private:
  Board board_;
  Board::Spot lastMove_;
  Board::Player playerToMove_;

  // Start at 1 win per player based on the sunrise problem
  std::atomic<uint64_t> heuristicSum_{1};
  std::atomic<uint64_t> heuristicNumTrials_{2};

  Board::LegalMoves legalMoves_;
  std::array<std::unique_ptr<State>, Board::kCols> children_;
};

class AI {
 public:
   AI(int aiPlayer, int usecPerMove)
       : aiPlayer_(static_cast<Board::Player>(aiPlayer)),
         serverPlayer_(static_cast<Board::Player>((aiPlayer + 1) % 2)),
         durationPerMove_(std::chrono::microseconds(usecPerMove)) {}

   bool gameIsOver() const;

   std::unique_ptr<game::Connect4::Move> waitForMove();

   void makeServerMove(const game::Connect4::Move& move);

 private:
  const Board::Player aiPlayer_;
  const Board::Player serverPlayer_;
  const std::chrono::high_resolution_clock::duration durationPerMove_;
  Board board_;
};

} // namespace ais::conn4
