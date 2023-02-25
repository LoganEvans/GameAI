#pragma once

#include <chrono>

#include "proto/game.pb.h"

namespace ais {

class FourInARowAI {
 public:
   FourInARowAI(int aiPlayer, int usecPerMove)
       : aiPlayer_(aiPlayer),
         durationPerMove_(std::chrono::microseconds(usecPerMove)) {}

   bool gameIsOver() const;

   std::unique_ptr<game::FourInARow::Move> waitForMove();

   void makeServerMove(const game::FourInARow::Move& move);

 private:
  const int aiPlayer_;
  const std::chrono::high_resolution_clock::duration durationPerMove_;
};

} // namespace ais
