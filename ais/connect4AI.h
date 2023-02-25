#pragma once

#include <chrono>

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

class AI {
 public:
   AI(int aiPlayer, int usecPerMove)
       : aiPlayer_(aiPlayer),
         durationPerMove_(std::chrono::microseconds(usecPerMove)) {}

   bool gameIsOver() const;

   std::unique_ptr<game::Connect4::Move> waitForMove();

   void makeServerMove(const game::Connect4::Move& move);

 private:
  const int aiPlayer_;
  const std::chrono::high_resolution_clock::duration durationPerMove_;
  Board board_;
};

} // namespace ais::conn4
