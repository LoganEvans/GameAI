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
  //assert(boardIsLegal());
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

  if ((board_[0] | board_[1]) == 0x3f3f3f3f3f3f3fULL) {
    return Player::Draw;
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

bool Board::boardIsLegal() const {
  int movesOne = __builtin_popcountll(board_[0]);
  int movesTwo = __builtin_popcountll(board_[1]);

  if (movesOne != movesTwo && movesOne != movesTwo + 1) {
    //assert(false);
    return false;
  }

  uint64_t merged = board_[0] | board_[1];
  if (((merged + 0x1010101010101ULL) & merged) != 0) {
    //assert(false);
    return false;
  }

  if (merged & ~0x3f3f3f3f3f3f3fULL) {
    //assert(false);
    return false;
  }

  return true;
}

Board::Spot State::pickMove() const {
  auto legalMoves = board_.legalMoves();
  double totalProb = 0.0;
  std::array<double, 7> winningProbs({});

  double maxProb = 0.0;
  int bestCol = 0;
  for (int col = 0; col < Board::kCols; col++) {
    if (legalMoves.legalRowInCol[col] == Board::LegalMoves::kIllegal) {
      continue;
    }

    auto* child = children_[col].get();

    auto prob = child->winProbability(playerToMove_);
    printf("col[%d] prob: %lf\t", col, prob);
    if (child && prob > maxProb) {
      maxProb = prob;
      bestCol = col;
    }
  }
  printf("\n");
  printf("Selected move with win prob: %lf\n", maxProb);

  return Board::Spot{.row = legalMoves.legalRowInCol[bestCol], .col = bestCol};
}

std::unique_ptr<State> State::makeMoveAndUpdateState(Board::Spot spot) {
  printf("> makeMoveAndUpdateState({.row = %d, .col = %d})\n", spot.row,
         spot.col);
  auto state = std::move(children_[spot.col]);
  if (!state) {
    board_.move(spot, playerToMove_);
    state = std::make_unique<State>(board_, Board::other(playerToMove_));
  }

  state->parent_ = nullptr;

  return state;
}

double State::winProbability(Board::Player player) const {
  if (hasChildren()) {
    auto p = minMaxWinProb_.load(std::memory_order_relaxed);
    return (player == Board::Player::One) ? 1.0 - p : p;
  }

  uint64_t numTrials =
      heuristicNumTrials_.load(std::memory_order_relaxed);
  uint64_t sum = heuristicSum_.load(std::memory_order_relaxed);
  double n = (playerToMove_ == Board::Player::One) ? numTrials - sum : sum;
  return n / heuristicNumTrials_;
}

void State::setWinProbability(Board::Player winner, double probability) {
  minMaxWinProb_.store((winner == Board::Player::One) ? 1.0 - probability
                                                      : probability,
                       std::memory_order_relaxed);
}

void State::updateProbabilities() {
  double prob = 0.0;
  if (hasChildren()) {
    for (int col = 0; col < Board::kCols; col++) {
      if (children_[col]) {
        prob = std::max(prob, children_[col]->winProbability(playerToMove()));
      }
    }
    setWinProbability(playerToMove(), prob);
  } else {
    setWinProbability(playerToMove(), winProbability(playerToMove()));
  }

  auto* parent = parent_;
  while (parent) {
    auto minProb = 1.0;
    for (const auto& child: parent->children_) {
      if (!child) {
        continue;
      }
      minProb = std::min(minProb, child->winProbability(child->playerToMove()));
    }

    auto maxProb = 1.0 - minProb;
    if (maxProb == parent->winProbability(parent->playerToMove())) {
      break;
    }

    parent->setWinProbability(parent->playerToMove(), maxProb);
  }
}

void State::markSolved(Board::Player winningPlayer) {
  heuristicNumTrials_.store(kCertain, std::memory_order_release);
  heuristicSum_.store(winningPlayer == Board::Player::One ? 0 : kCertain,
                      std::memory_order_release);
}

void State::createChildren() {
  std::lock_guard lock(childrenMutex_);

  if (children_[0]) {
    return;
  }

  auto otherPlayer = Board::other(playerToMove_);

  for (int col = 0; col < Board::kCols; col++) {
    int row = legalMoves_.legalRowInCol[col];
    if (row == Board::LegalMoves::kIllegal) {
      continue;
    }

    Board b(board());
    b.move(Board::Spot{.row = row, .col = col}, playerToMove_);

    children_[col] =
        std::make_unique<State>(/*board=*/b, /*playerToMove=*/otherPlayer);
    children_[col]->parent_ = this;
  }

  hasChildren_ = true;
  updateProbabilities();
}

void State::monteCarloTrial() {
  thread_local std::random_device rd;
  thread_local std::mt19937 gen(rd());

  Board b(board_);
  Board::Player player(playerToMove_);
  Board::Player otherPlayer = Board::other(player);

  if (b.hasWinningMove(player)) {
    b.move(b.getWinningMove(player), player);
  }

  while (b.winner() == Board::Player::None) {
    auto legal = b.legalMoves();

    int potentialMoves = 0;
    for (int col = 0; col < Board::kCols; col++) {
      int row = legal.legalRowInCol[col];
      if (row == Board::LegalMoves::kIllegal) {
        continue;
      }

      Board lookAhead(b);
      lookAhead.move(Board::Spot{.row = row, .col = col}, player);
      if (lookAhead.hasWinningMove(otherPlayer)) {
        continue;
      }

      potentialMoves++;
    }

    if (potentialMoves == 0) {
      for (int col = 0; col < Board::kCols; col++) {
        int row = legal.legalRowInCol[col];
        if (row != Board::LegalMoves::kIllegal) {
          b.move(Board::Spot{.row = row, .col = col}, player);
          std::swap(player, otherPlayer);
          break;
        }
      }
      break;
    }

    std::uniform_int_distribution<> selectionDist(0, potentialMoves);
    int move = selectionDist(gen);

    for (int col = 0; col < Board::kCols; col++) {
      int row = legal.legalRowInCol[col];
      if (row == Board::LegalMoves::kIllegal) {
        continue;
      }

      if (move == 0) {
        b.move(Board::Spot{.row = row, .col = col}, player);
        std::swap(player, otherPlayer);
        break;
      }
      move--;
    }
  }

  Board::Player winner = b.winner();
  if (winner == Board::Player::Two ||
      (playerToMove_ == Board::Player::One && winner == Board::Player::Draw)) {
    heuristicSum_.fetch_add(1, std::memory_order_relaxed);
  }

  heuristicNumTrials_.fetch_add(1, std::memory_order_relaxed);
  updateProbabilities();
}

void AI::thinkHard() {
  printf("> thinkHard()\n");
  std::random_device rd;
  std::mt19937 gen(rd());

  auto deadline = Clock::now() + durationPerMove_;

  int iters = 0;
  while (Clock::now() < deadline) {
    iters++;
    State* state = state_.get();

    auto wp = state->winProbability(state->playerToMove());
    if (wp == 0.0 || wp == 1.0) {
      break;
    }

    Board b(state->board());
    Board::Player playerToMove = state->playerToMove();

    while (b.winner() == Board::Player::None) {
      //auto wp = state->winProbability(state->playerToMove());
      //if (wp == 0.0 || wp == 1.0) {
      //  break;
      //}

      if (!state->hasChildren()) {
        if (state->heuristicNumTrials() >= kMonteCarloThreshold) {
          state->createChildren();
        } else if (b.hasWinningMove(state->playerToMove())) {
          printf("seen\n");
          state->setWinProbability(state->playerToMove(), 1.0);
          state->updateProbabilities();
        } else {
          state->monteCarloTrial();
          break;
        }
      }

      std::array<double, 7> winningProbs;
      double totalProb = 0.0;
      for (int col = 0; col < Board::kCols; col++) {
        auto *child = state->getChild(col);
        if (child == nullptr) {
          winningProbs[col] = 0.0;
        } else {
          winningProbs[col] = child->winProbability(state->playerToMove());
          totalProb += winningProbs[col];
        }
      }

      std::uniform_real_distribution<> selectionDist(0, totalProb);
      double selector = selectionDist(gen);
      double cumulative = 0.0;
      for (int col = 0; col < Board::kCols - 1; col++) {
        cumulative += winningProbs[col];
        if (cumulative >= selector) {
          state = state->getChild(col);
          break;
        }
      }
    }
  }
  printf("< thinkHard() -- iters: %d... %d, winProb: %lf\n", iters,
         Clock::now() < deadline,
         state_->winProbability(state_->playerToMove()));
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
