#include "ais/connect4AI.h"

#include <array>
#include <random>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace ais::conn4 {

TEST(Board, ctor) {
  Board b;

  for (int row = 0; row < 6; row++) {
    for (int col = 0; col < 7; col++) {
      EXPECT_EQ(b.getPlayer(Board::Spot{.row = row, .col = col}),
                Board::Player::None);
    }
  }
}

TEST(Board, move) {
  for (auto player :
       std::to_array({Board::Player::One, Board::Player::Two})) {
    Board b;
    for (int row = 0; row < 6; row++) {
      for (int col = 0; col < 7; col++) {
        b.move(Board::Spot{.row = row, .col = col}, player);
        EXPECT_EQ(b.getPlayer(Board::Spot{.row = row, .col = col}), player);
      }
    }
  }
}

TEST(Board, winnerColumn) {
  for (auto player :
       std::to_array({Board::Player::One, Board::Player::Two})) {
    for (int rowStart = 0; rowStart < 2; rowStart++) {
      for (int col = 0; col < 7; col++) {
        Board b;
        b.move(Board::Spot{.row = rowStart, .col = col}, player);
        b.move(Board::Spot{.row = rowStart + 1, .col = col}, player);
        b.move(Board::Spot{.row = rowStart + 2, .col = col}, player);
        EXPECT_EQ(b.winner(), Board::Player::None);
        b.move(Board::Spot{.row = rowStart + 3, .col = col}, player);
        EXPECT_EQ(b.winner(), player);
      }
    }
  }
}

TEST(Board, winnerRow) {
  for (auto player :
       std::to_array({Board::Player::One, Board::Player::Two})) {
    for (int row = 0; row < 6; row++) {
      for (int colStart = 0; colStart < 3; colStart++) {
        Board b;
        b.move(Board::Spot{.row = row, .col = colStart}, player);
        b.move(Board::Spot{.row = row, .col = colStart + 1}, player);
        b.move(Board::Spot{.row = row, .col = colStart + 2}, player);
        EXPECT_EQ(b.winner(), Board::Player::None);
        b.move(Board::Spot{.row = row, .col = colStart + 3}, player);
        EXPECT_EQ(b.winner(), player);
      }
    }
  }
}

TEST(Board, winnerForwardDiag) {
  for (auto player :
       std::to_array({Board::Player::One, Board::Player::Two})) {
    for (int rowStart = 0; rowStart < 2; rowStart++) {
      for (int colStart = 0; colStart < 3; colStart++) {
        Board b;
        b.move(Board::Spot{.row = rowStart, .col = colStart}, player);
        b.move(Board::Spot{.row = rowStart + 1, .col = colStart + 1}, player);
        b.move(Board::Spot{.row = rowStart + 2, .col = colStart + 2}, player);
        EXPECT_EQ(b.winner(), Board::Player::None);
        b.move(Board::Spot{.row = rowStart + 3, .col = colStart + 3}, player);
        EXPECT_EQ(b.winner(), player);
      }
    }
  }
}

TEST(Board, winnerBackwardDiag) {
  for (auto player :
       std::to_array({Board::Player::One, Board::Player::Two})) {
    for (int rowStart = 0; rowStart < 2; rowStart++) {
      for (int colStart = 3; colStart < 7; colStart++) {
        Board b;
        b.move(Board::Spot{.row = rowStart, .col = colStart}, player);
        b.move(Board::Spot{.row = rowStart + 1, .col = colStart - 1}, player);
        b.move(Board::Spot{.row = rowStart + 2, .col = colStart - 2}, player);
        EXPECT_EQ(b.winner(), Board::Player::None);
        b.move(Board::Spot{.row = rowStart + 3, .col = colStart - 3}, player);
        EXPECT_EQ(b.winner(), player);
      }
    }
  }
}

TEST(Board, legalMoves) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, 1);

  for (int topRow = 0; topRow <= Board::kRows; topRow++) {
    Board b;
    for (int i = 0; i < topRow; i++) {
      for (int col = 0; col < Board::kCols; col++) {
        b.move(Board::Spot{.row = i, .col = col},
               static_cast<Board::Player>(dist(gen)));
      }
    }

    auto legalMoves = b.legalMoves();
    for (auto row : legalMoves.legalRowInCol) {
      EXPECT_EQ(row, topRow);
    }

    if (topRow == Board::kRows) {
      for (auto row : legalMoves.legalRowInCol) {
        EXPECT_EQ(row, Board::LegalMoves::kIllegal);
      }
    }
  }
}

TEST(Board, nextPlayer) {
  Board b;
  EXPECT_EQ(b.nextPlayer(), Board::Player::One);

  b.move(Board::Spot{.row=0, .col=3}, Board::Player::One);
  EXPECT_EQ(b.nextPlayer(), Board::Player::Two);

  b.move(Board::Spot{.row=1, .col=3}, Board::Player::Two);
  EXPECT_EQ(b.nextPlayer(), Board::Player::One);

  b.move(Board::Spot{.row=0, .col=2}, Board::Player::One);
  EXPECT_EQ(b.nextPlayer(), Board::Player::Two);
}

} // namespace ais::conn4
