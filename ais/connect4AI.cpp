#include "ais/connect4AI.h"

#include <random>
#include <thread>

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
  assert(boardIsLegal());
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
    return false;
  }

  uint64_t merged = board_[0] | board_[1];
  if (((merged + 0x1010101010101ULL) & merged) != 0) {
    return false;
  }

  if (merged & ~0x3f3f3f3f3f3f3fULL) {
    return false;
  }

  return true;
}

double State::WinProb::prob(Board::Player playerToMove) const {
  uint64_t heuristic = heuristic_.load(std::memory_order_relaxed);
  double p =
      static_cast<double>(heuristic >> 32) / static_cast<uint32_t>(heuristic);
  return (playerToMove == Board::Player::One) ? 1.0 - p : p;
}

void State::WinProb::recordTrial(Board::Player winner) {
  uint64_t increment = 1;
  if (winner == Board::Player::Two) {
    increment += 1ULL << 32;
  }
  uint64_t prev = heuristic_.fetch_add(increment, std::memory_order_relaxed);
  assert(static_cast<uint32_t>(prev) != kCertain);
}

void State::WinProb::markSolved(Board::Player winner) {
  uint64_t v = kCertain;
  if (winner == Board::Player::Two) {
    v |= kCertain << 32;
  }
  heuristic_.store(v, std::memory_order_relaxed);
}

uint32_t State::WinProb::numTrials() const {
  return static_cast<uint32_t>(heuristic_.load(std::memory_order_relaxed));
}

State::State(State* parent, Board board, Board::Player playerToMove)
    : parent_(parent), board_(board), playerToMove_(playerToMove),
      legalMoves_(board_.legalMoves()) {
  if (board_.hasWinningMove(playerToMove)) {
    markSolved(playerToMove);
  }
}

Board::Spot State::pickMove() const {
  if (board().hasWinningMove(playerToMove())) {
    printf("Picking wining move\n");
    return board().getWinningMove(playerToMove());
  }

  auto legalMoves = board_.legalMoves();

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
    state = std::make_unique<State>(/*parent=*/nullptr, board_,
                                    Board::other(playerToMove_));
  }

  state->parent_ = nullptr;

  return state;
}

double State::winProbability(Board::Player player) const {
  return winProb_.prob(player);
}

void State::updateProbabilities(Board::Player trialWinner) {
  if (trialWinner != Board::Player::One && trialWinner != Board::Player::Two) {
    trialWinner = Board::other(playerToMove_);
  }

  auto* state = this;
  while (state) {
    state->winProb_.recordTrial(trialWinner);
    state = state->parent_;
  }
}

void State::markSolved(Board::Player winningPlayer) {
  winProb_.markSolved(winningPlayer);
//  auto* state = parent_;
//  while (state) {
//    bool isSolved = true;
//    for (int col = 0; col < Board::kCols; col++) {
//      int row = legalMoves_.legalRowInCol[col];
//      if (row == Board::LegalMoves::kIllegal) {
//        continue;
//      }
//      if (
//    }
//
//    if (state->playerToMove() == winningPlayer) {
//      state->
//      state = state->parent_;
//    } else {
//      break;
//    }
//  }
}

void State::createChildren() {
  std::lock_guard lock(childrenMutex_);
  if (hasChildren_) {
    return;
  }
  assert(!board().hasWinningMove(playerToMove()));

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

    children_[col] = std::make_unique<State>(/*parent=*/this, /*board=*/b,
                                             /*playerToMove=*/otherPlayer);

    if (b.hasWinningMove(otherPlayer)) {
      children_[col]->markSolved(otherPlayer);
    }
  }

  hasChildren_ = true;
}

void State::monteCarloTrial() {
  thread_local std::random_device rd;
  thread_local std::mt19937 gen(rd());

  Board b(board_);
  Board::Player player(playerToMove_);
  Board::Player otherPlayer = Board::other(player);

  if (b.hasWinningMove(player)) {
    updateProbabilities(player); // XXX markSolved
    return;
  }

  std::vector<Board::Spot> potentialMoves;
  potentialMoves.reserve(Board::kCols);

  while (b.winner() == Board::Player::None) {
    if (b.hasWinningMove(player)) {
      b.move(b.getWinningMove(player), player);
      break;
    }

    auto legal = b.legalMoves();
    potentialMoves.clear();

    for (int col = 0; col < Board::kCols; col++) {
      int row = legal.legalRowInCol[col];
      if (row == Board::LegalMoves::kIllegal) {
        continue;
      }

      Board lookAhead(b);
      Board::Spot spot{.row = row, .col = col};
      lookAhead.move(spot, player);
      if (!lookAhead.hasWinningMove(otherPlayer)) {
        potentialMoves.push_back(spot);
      }
    }

    if (potentialMoves.empty()) {
      updateProbabilities(/*trialWinner=*/otherPlayer);
      return;
    }

    std::uniform_int_distribution<> selectionDist(0, potentialMoves.size() - 1);
    b.move(potentialMoves[selectionDist(gen)], player);
    std::swap(player, otherPlayer);
  }

  updateProbabilities(b.winner());
}

void AI::thinkHard() {
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
    while (b.winner() == Board::Player::None) {
      Board::Player playerToMove = state->playerToMove();

      wp = state->winProbability(playerToMove);
      if (wp == 0.0 || wp == 1.0) {
        break;
      }

      if (!state->hasChildren()) {
        uint32_t numTrials = state->winProb().numTrials();
        assert(numTrials != State::kCertain);
        if (numTrials >= kMonteCarloThreshold) {
          state->createChildren();
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
          winningProbs[col] = child->winProbability(playerToMove);
          totalProb += winningProbs[col];
        }
      }

      if (totalProb == 0.0) {
        for (int col = 0; col < Board::kCols; col++) {
          if (state->legalMoves().legalRowInCol[col] ==
              Board::LegalMoves::kIllegal) {
            continue;
          }
          state = state->getChild(col);
          b = state->board();
          break;
        }
      } else {
        std::uniform_real_distribution<> selectionDist(0, totalProb);
        double selector = selectionDist(gen);
        double cumulative = 0.0;
        for (int col = 0; col < Board::kCols; col++) {
          cumulative += winningProbs[col];
          if (cumulative >= selector) {
            state = state->getChild(col);
            b = state->board();
            break;
          }
        }
      }
    }
  }
}

bool AI::gameIsOver() const {
  return state_->board().winner() != Board::Player::None;
}

std::unique_ptr<game::Connect4::Move> AI::waitForMove() {
  std::vector<std::thread> threads;
  for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
    threads.push_back(std::thread([&]() { thinkHard(); }));
  }
  for (int i = 0; i < threads.size(); i++) {
    threads[i].join();
  }

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
