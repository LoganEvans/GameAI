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
  static constexpr Spot kIllegalSpot{.row = -1, .col = -1};

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
  Board(std::string str); // For debug use
  Board(const Board &other) : board_(other.board_) {}

  std::string debugString() const;

  Board &operator=(const Board &other) {
    board_ = other.board_;
    return *this;
  }

  // Board Index
  static inline int bIdx(Player player) { return static_cast<int>(player); }

  static inline Player other(Player player) {
    return (player == Player::One) ? Player::Two : Player::One;
  }

  Player getPlayer(Spot spot) const;

  void move(Spot spot, Player player);

  Player winner() const;

  LegalMoves legalMoves() const;

  Spot getWinningMove(Player player) const;

  Player nextPlayer() const;

  bool boardIsLegal() const;

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
  static constexpr uint64_t kMonteCarloBootstrap = 100;
  static constexpr uint64_t kMonteCarloSplitState = 1000;

  class WinProb {
  public:
    static constexpr uint64_t kCertain = std::numeric_limits<uint32_t>::max();

    WinProb &operator=(const WinProb &other) {
      heuristic_.store(other.heuristic_.load(std::memory_order_acquire),
                       std::memory_order_release);
      return *this;
    }

    double prob(Board::Player playerToMove) const;
    void recordTrial(Board::Player winner);
    void markSolved(Board::Player winner);
    Board::Player solvedWinnerImpl(uint64_t heuristic) const;
    Board::Player solvedWinner() const;
    uint32_t numTrials() const;

  private:
    // Storing two uint32_t values together allows the value to be incremented
    // atomically without a mutex. The format is (#playerTwoWins << 32) |
    // #trials
    std::atomic<uint64_t> heuristic_{(1ULL << 32) | 2};
  };

  State() = delete;
  State(State *parent, Board board, Board::Player playerToMove);

  Board::Spot pickMove() const;
  std::unique_ptr<State> makeMoveAndUpdateState(Board::Spot spot);

  const Board &board() const { return board_; }

  Board::Player playerToMove() const { return playerToMove_; }

  bool hasChildren() const { return hasChildren_; }

  const WinProb &winProb() const { return winProb_; }

  const Board::LegalMoves &legalMoves() const { return legalMoves_; }

  void recordMonteCarloResult(Board::Player trialWinner);

  void updateProbabilities();

  void markSolvedState(Board::Player winningPlayer);

  void createChildren();

  State *getChild(int col) const { return children_[col].get(); }

  Board::Player monteCarloTrial() const;

  int height() const;

private:
  State *parent_{nullptr};
  Board board_;
  Board::Player playerToMove_;
  WinProb winProb_{};
  Board::LegalMoves legalMoves_;
  bool hasChildren_{false};
  std::mutex childrenMutex_;
  std::array<std::unique_ptr<State>, Board::kCols> children_;
};

class AI {
public:
  typedef std::chrono::high_resolution_clock Clock;

  AI(int aiPlayer, int usecPerMove)
      : aiPlayer_(static_cast<Board::Player>(aiPlayer)),
        serverPlayer_(static_cast<Board::Player>((aiPlayer + 1) % 2)),
        durationPerMove_(std::chrono::microseconds(usecPerMove)),
        state_(std::make_unique<State>(/*parent=*/nullptr, Board(),
                                       Board::Player::One)) {}

  static void thinkHard(State *root, AI::Clock::duration durationPerMove);

  bool gameIsOver() const;

  std::unique_ptr<game::Connect4::Move> waitForMove();

  void makeServerMove(const game::Connect4::Move &move);

private:
  const Board::Player aiPlayer_;
  const Board::Player serverPlayer_;
  const Clock::duration durationPerMove_;
  std::unique_ptr<State> state_;
};

} // namespace ais::conn4
