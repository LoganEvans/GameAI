#include "ais/fourInARow.h"

#include <memory>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "proto/game.grpc.pb.h"

std::unique_ptr<game::FourInARow::Game>
newGame(game::FourInARowService::Stub *stub, int serverPlayer, int difficulty) {
  grpc::ClientContext context;
  auto newGameReq = game::FourInARow::NewGameReq();
  newGameReq.set_serverplayer(1);
  newGameReq.set_difficulty(3);
  auto game = std::make_unique<game::FourInARow::Game>();
  stub->NewGame(&context, newGameReq, game.get());
  return game;
}

std::unique_ptr<game::FourInARow::Empty>
makeMove(game::FourInARowService::Stub *stub, game::FourInARow::Game* game, int row, int col) {
  grpc::ClientContext context;
  auto makeMoveReq = game::FourInARow::MakeMoveReq();
  *makeMoveReq.mutable_game() = *game;
  auto* move = makeMoveReq.mutable_move();
  move->set_row(0);
  move->set_col(0);
  auto empty = std::make_unique<game::FourInARow::Empty>();
  stub->MakeMove(&context, makeMoveReq, empty.get());
  return empty;
}

std::unique_ptr<game::FourInARow::MoveList>
getMoves(game::FourInARowService::Stub *stub, game::FourInARow::Game* game) {
  grpc::ClientContext context;
  auto moveList = std::make_unique<game::FourInARow::MoveList>();
  stub->GetMoves(&context, *game, moveList.get());
  return moveList;
}

int main() {
  printf("here: %d\n", ais::fiar::foo());

  auto stub = game::FourInARowService::NewStub(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));

  auto game = newGame(stub.get(), 1, 3);
  makeMove(stub.get(), game.get(), 0, 0);

  printf("end: %s\n", getMoves(stub.get(), game.get()).);
  return 0;
}
