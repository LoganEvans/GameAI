#include "ais/connect4AI.h"

#include <random>

namespace ais::conn4 {

Board::Player Board::getPlayer(Spot spot) const {
  for (auto value : std::to_array(
           {Board::Player::One, Board::Player::Two})) {
    if (1 & ((board_[bIdx(value)] >> (8 * spot.col)) >> spot.row)) {
      return value;
    }
  }
  return Player::None;
}

void Board::move(Spot spot, Player value) {
  board_[bIdx(value)] |= (1ULL << spot.row) << (8 * spot.col);
}

Board::Player Board::winner() const {
  for (auto value : std::to_array(
           {Board::Player::One, Board::Player::Two})) {
    uint64_t b = board_[bIdx(value)];

    uint64_t columnWin = b & (b >> 1) & (b >> 2) & (b >> 3);
    if (columnWin != 0) {
      return value;
    }

    uint64_t rowWin = b & (b >> 8) & (b >> 16) & (b >> 24);
    if (rowWin != 0) {
      return value;
    }

    uint64_t forwardDiagWin = b & (b >> 9) & (b >> 18) & (b >> 27);
    if (forwardDiagWin != 0) {
      return value;
    }

    uint64_t backDiagWin = b & (b >> 7) & (b >> 14) & (b >> 21);
    if (backDiagWin != 0) {
      return value;
    }
  }

  return Player::None;
}

Board::LegalMoves Board::legalMoves() const {
  LegalMoves legal;

  uint64_t b = board_[0] | board_[1];

  for (int col = 0; col < kCols; col++) {
    int colVal = (b >> (8 * col)) & kColMask;
    legal.legalRowInCol[col] = __builtin_ctz(~colVal);
  }

  return legal;
}

Board::Player Board::nextPlayer() const {
  uint64_t b = board_[0] | board_[1];
  return static_cast<Board::Player>(__builtin_popcountll(b) % 2);
}

Board::Spot State::pickMove() const {
  auto legalMoves = board_.legalMoves();

  // Check for winning moves
  for (int col = 0; col < Board::kCols; col++) {
    auto* child = children_[col].get();
    if (child && child->heuristicNumTrials_ == kCertain &&
        (child->heuristicSum_ == kCertain) ==
            static_cast<bool>(playerToMove_)) {
      return Board::Spot{.row = legalMoves.legalRowInCol[col], .col = col};
    }
  }

  // Pick one of the moves probabalistically
}

bool AI::gameIsOver() const {
  return board_.winner() != Board::Player::None;
}

std::unique_ptr<game::Connect4::Move> AI::waitForMove() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> colDist(0, 6);

  auto move = std::make_unique<game::Connect4::Move>();

  auto legalMoves = board_.legalMoves();
  while (true) {
    int col = colDist(gen);
    if (legalMoves.legalRowInCol[col] != Board::LegalMoves::kIllegal) {
      move->set_col(col);
      move->set_row(legalMoves.legalRowInCol[col]);
      return move;
    }
  }
}

void AI::makeServerMove(const game::Connect4::Move &move) {
  board_.move(Board::Spot{.row = static_cast<int32_t>(move.row()),
                          .col = static_cast<int32_t>(move.col())},
              serverPlayer_);
}

} // namespace ais::conn4
