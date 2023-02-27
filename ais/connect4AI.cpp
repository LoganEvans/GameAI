#include "ais/connect4AI.h"

#include <random>

namespace ais::conn4 {

Board::Board(std::string str) {
  int row = 5;
  int col = 0;
  int charIdx = 0;
  do {
    char ch = str.at(charIdx++);
    if (ch == '\n') {
      ch = str.at(charIdx++);
      row--;
      col = 0;
    }

    if (ch == 'X') {
      move(Spot{.row=row, .col=col}, Player::One);
    } else if (ch == 'O') {
      move(Spot{.row=row, .col=col}, Player::Two);
    }
    col++;
  } while (row != 0 || col != kCols);
}

std::string Board::debugString() const {
  std::string b;
  for (int row = kRows - 1; row >= 0; row--) {
    for (int col = 0; col < kCols; col++) {
      auto player = getPlayer({.row = row, .col = col});
      if (player == Player::One) {
        b += " X";
      } else if (player == Player::Two) {
        b += " O";
      } else {
        b += " .";
      }

      if (col + 1 == kCols) {
        b.push_back('\n');
      }
    }
  }

  return b;
}

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

bool Board::hasWinningMove(Player player) const {
  // This uses addition carrying to place a piece at the top of the column and
  // then masks away illegal moves.
  uint64_t movesMask =
      ((board_[0] | board_[1]) + 0x1010101010101ULL) & 0x3f3f3f3f3f3f3fULL;
  uint64_t b = board_[bIdx(player)];

  uint64_t column = b & (b >> 1) & (b >> 2);
  if ((column & (movesMask >> 3)) != 0) {
    return true;
  }

  uint64_t row = (b >> 8) & (b >> 16) & (b >> 24);
  if ((row & movesMask) != 0) {
    return true;
  }
  row = b & (b >> 8) & (b >> 16);
  if ((row & (movesMask >> 24)) != 0) {
    return true;
  }

  uint64_t forwardDiag = (b >> 9) & (b >> 18) & (b >> 27);
  if ((forwardDiag & movesMask) != 0) {
    return true;
  }
  forwardDiag = b & (b >> 9) & (b >> 18);
  if ((forwardDiag & (movesMask >> 27)) != 0) {
    return true;
  }

  uint64_t backDiag = (b >> 7) & (b >> 14) & (b >> 21);
  if ((backDiag & movesMask) != 0) {
    return true;
  }
  backDiag = b & (b >> 7) & (b >> 14);
  if ((backDiag & (movesMask >> 21)) != 0) {
    return true;
  }

  return false;
}

Board::Spot Board::getWinningMove(Player player) const {
  auto legal = legalMoves();
  for (int col = 0; col < kCols; col++) {
    int row = legal.legalRowInCol[col];
    if (row == LegalMoves::kIllegal) {
      continue;
    }
    Board b(*this);
    Spot spot({.row = row, .col = col});
    b.move(spot, player);
    if (b.winner() == player) {
      return spot;
    }
  }

  return Spot{.row = -1, .col = -1};
}

Board::Player Board::nextPlayer() const {
  uint64_t b = board_[0] | board_[1];
  return static_cast<Board::Player>(__builtin_popcountll(b) % 2);
}

Board::Spot State::pickMove() const {
  auto legalMoves = board_.legalMoves();
  double totalProb = 0.0;
  std::array<double, 7> winningProbs({});

  for (int col = 0; col < Board::kCols; col++) {
    if (legalMoves.legalRowInCol[col] == Board::LegalMoves::kIllegal) {
      continue;
    }

    auto* child = children_[col].get();

    // Check for winning chains
    if (child && child->heuristicNumTrials_ == kCertain &&
        (child->heuristicSum_ == kCertain) ==
            static_cast<bool>(playerToMove_)) {
      return Board::Spot{.row = legalMoves.legalRowInCol[col], .col = col};
    }

    winningProbs[col] = winProbability(col);
    totalProb += winningProbs[col];
  }

  // If none of the moves lead to a win, pick a random one.
  if (totalProb == 0.0) {
    for (int col = 0; col < Board::kCols; col++) {
      if (legalMoves.legalRowInCol[col] != Board::LegalMoves::kIllegal) {
        winningProbs[col] = 1.0;
        totalProb += 1.0;
      }
    }
  }

  // Pick one of the moves probabalistically
  thread_local std::random_device rd;
  thread_local std::mt19937 gen(rd());
  thread_local std::uniform_real_distribution<> selectionDist(0.0, 1.0);

  double selector = selectionDist(gen) * totalProb;
  double cumulative = 0.0;
  for (int col = 0; col < Board::kCols - 1; col++) {
    cumulative += winningProbs[col];
    if (cumulative >= selector) {
      return Board::Spot{.row = legalMoves.legalRowInCol[col], .col = col};
    }
  }

  return Board::Spot{.row = legalMoves.legalRowInCol[Board::kCols - 1],
                     .col = Board::kCols - 1};
}

std::unique_ptr<State> State::makeMoveAndUpdateState(Board::Spot spot) {
  auto state = std::move(children_[spot.col]);
  if (!state) {
    state = std::make_unique<State>();
  }
  return state;
}

double State::winProbability(int col) const {
  auto *child = children_[col].get();
  if (child) {
    uint64_t numTrials =
        child->heuristicNumTrials_.load(std::memory_order_relaxed);
    uint64_t sum = child->heuristicSum_.load(std::memory_order_relaxed);
    double n = (playerToMove_ == Board::Player::One) ? numTrials - sum : sum;
    return n / child->heuristicNumTrials_;
  }
  return 0.5;
}

void State::createChild(int col) {
  std::lock_guard lg(childrenMutex_);

  if (children_[col]) {
    return;
  }

  children_[col] = std::make_unique<State>();
  children_[col]->parent_ = this;
}

void State::monteCarloTrial() {
  Board b(board_);

  while (b.winner() == Board::Player::None) {
    //auto winningMove = b.winningMove();



  }
}

void AI::thinkHard() {
  auto deadline = Clock::now() + durationPerMove_;

  while (Clock::now() < deadline) {
    
  }
}

bool AI::gameIsOver() const {
  return state_->board().winner() != Board::Player::None;
}

std::unique_ptr<game::Connect4::Move> AI::waitForMove() {
  thinkHard();

  auto spot = state_->pickMove();
  state_ = state_->makeMoveAndUpdateState(spot);

  auto move = std::make_unique<game::Connect4::Move>();
  move->set_col(spot.col);
  move->set_row(spot.row);
  return move;
}

void AI::makeServerMove(const game::Connect4::Move &move) {
  state_ = state_->makeMoveAndUpdateState(
      Board::Spot{.row = static_cast<int32_t>(move.row()),
                  .col = static_cast<int32_t>(move.col())});
}

} // namespace ais::conn4
