#pragma once

#include <atomic>
#include <chrono>
#include <cstring>
#include <limits>
#include <mutex>

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
    Draw = 3,
  };

  struct LegalMoves {
    static constexpr int8_t kIllegal = kRows;
    std::array<int8_t, 7> legalRowInCol;
  };

  Board() {}
  Board(std::string str);  // For debug use
  Board(const Board &other) : board_(other.board_) {}

  std::string debugString() const;

  Board &operator=(const Board &other) {
    board_ = other.board_;
    return *this;
  }

  // Board Index
  static inline int bIdx(Player player) {
    return static_cast<int>(player);
  }

  static inline Player other(Player player) {
    return (player == Player::One) ? Player::Two : Player::One;
  }

  Player getPlayer(Spot spot) const;

  void move(Spot spot, Player player);

  Player winner() const;

  LegalMoves legalMoves() const;

  bool hasWinningMove(Player player) const;

  Spot getWinningMove(Player player) const;

  Player nextPlayer() const;

  std::array<uint64_t, 2> board_{};
private:
};

inline bool operator==(const Board::Spot &lhs, const Board::Spot &rhs) {
  return lhs.row == rhs.row and lhs.col == rhs.col;
}

inline bool operator!=(const Board::Spot &lhs, const Board::Spot &rhs) {
  return !(lhs == rhs);
}

class State {
public:
  static constexpr uint64_t kCertain = std::numeric_limits<uint64_t>::max();

  State() = delete;
  State(Board board, Board::Player playerToMove)
      : board_(board), playerToMove_(playerToMove) {}

  Board::Spot pickMove() const;
  std::unique_ptr<State> makeMoveAndUpdateState(Board::Spot spot);

  const Board& board() const { return board_; }

  Board::Player playerToMove() const { return playerToMove_; }

  bool hasChildren() const { return hasChildren_; }

  int heuristicNumTrials() const {
    return heuristicNumTrials_.load(std::memory_order_relaxed);
  }

  double winProbability(Board::Player player) const;

  void updateProbabilities();

  void markSolved(Board::Player winningPlayer);

  void createChildren();

  State *getChild(int col) const { return children_[col].get(); }

  void monteCarloTrial();

private:
  Board board_;
  Board::Player playerToMove_;

  // Start at 1 win per player based on the sunrise problem
  std::atomic<uint64_t> heuristicSum_{1};
  std::atomic<uint64_t> heuristicNumTrials_{2};

  Board::LegalMoves legalMoves_;
  State* parent_{nullptr};
  bool hasChildren_{false};
  std::mutex childrenMutex_;
  std::atomic<double> minMaxWinProb_{0.0};
  std::array<std::unique_ptr<State>, Board::kCols> children_;
};

class AI {
 public:
   typedef std::chrono::high_resolution_clock Clock;

   AI(int aiPlayer, int usecPerMove)
       : aiPlayer_(static_cast<Board::Player>(aiPlayer)),
         serverPlayer_(static_cast<Board::Player>((aiPlayer + 1) % 2)),
         durationPerMove_(std::chrono::microseconds(usecPerMove)),
         state_(std::make_unique<State>(Board(), Board::Player::One)) {}

   void thinkHard();

   bool gameIsOver() const;

   std::unique_ptr<game::Connect4::Move> waitForMove();

   void makeServerMove(const game::Connect4::Move& move);

 private:
  static constexpr uint64_t kMonteCarloThreshold = 1000;

  const Board::Player aiPlayer_;
  const Board::Player serverPlayer_;
  const Clock::duration durationPerMove_;
  std::unique_ptr<State> state_;
};

} // namespace ais::conn4
