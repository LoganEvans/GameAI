#include "ais/fourInARowAI.h"

#include <random>

namespace ais {

bool FourInARowAI::gameIsOver() const { return false; }

std::unique_ptr<game::FourInARow::Move> FourInARowAI::waitForMove() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, 6);

  auto move = std::make_unique<game::FourInARow::Move>();
  move->set_col(dist(gen));

  return move;
}

void FourInARowAI::makeServerMove(const game::FourInARow::Move &move) {
  
}

} // namespace ais
