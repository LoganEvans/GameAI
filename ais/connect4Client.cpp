#include "ais/connect4AI.h"

#include <unistd.h>

#include <memory>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "proto/game.grpc.pb.h"

std::unique_ptr<game::Connect4::Game>
newGame(game::Connect4Service::Stub *stub, int serverPlayer, int difficulty) {
  grpc::ClientContext context;
  auto newGameReq = game::Connect4::NewGameReq();
  newGameReq.set_serverplayer(1);
  newGameReq.set_difficulty(3);
  auto game = std::make_unique<game::Connect4::Game>();
  stub->NewGame(&context, newGameReq, game.get());
  return game;
}

std::unique_ptr<game::Connect4::Empty>
makeMove(game::Connect4Service::Stub *stub, game::Connect4::Game *game,
         int row, int col) {
  grpc::ClientContext context;
  auto makeMoveReq = game::Connect4::MakeMoveReq();
  *makeMoveReq.mutable_game() = *game;
  auto* move = makeMoveReq.mutable_move();
  move->set_row(row);
  move->set_col(col);
  auto empty = std::make_unique<game::Connect4::Empty>();
  stub->MakeMove(&context, makeMoveReq, empty.get());
  return empty;
}

std::unique_ptr<game::Connect4::MoveList>
getMoves(game::Connect4Service::Stub *stub, game::Connect4::Game* game) {
  grpc::ClientContext context;
  auto moveList = std::make_unique<game::Connect4::MoveList>();
  stub->GetMoves(&context, *game, moveList.get());
  return moveList;
}

std::unique_ptr<game::Connect4::MoveList>
waitForServerMove(game::Connect4Service::Stub *stub,
                  game::Connect4::Game *game, int serverPlayer) {
  while (true) {
    auto moveList = getMoves(stub, game);
    if (moveList->moves().size() % 2 != serverPlayer) {
      return moveList;
    }
    usleep(100000);
  }
}

int main() {
  auto stub = game::Connect4Service::NewStub(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));

  const int aiPlayer = 0;
  const int serverPlayer = 1;

  auto game =
      newGame(stub.get(), /*serverPlayer=*/serverPlayer, /*difficulty=*/3);
  auto ai = ais::conn4::AI(/*aiPlayer=*/aiPlayer, /*usecPerMove=*/10000000);

  int moveNum = 0;
  while (!ai.gameIsOver()) {
    if (moveNum % 2 == serverPlayer) {
      auto moveList = waitForServerMove(stub.get(), game.get(),
                                        /*serverPlayer=*/serverPlayer);
      ai.makeServerMove(moveList->moves()[moveList->moves().size() - 1]);
    } else {
      auto move = ai.waitForMove();
      makeMove(stub.get(), game.get(), move->row(), move->col());
    }
    moveNum++;
  }

  return 0;
}
