import argparse
import grpc
import proto.game_pb2 as game_pb
import proto.game_pb2_grpc as game_grpc


parser = argparse.ArgumentParser(description="Four in a Row test client")
parser.add_argument("verb")
parser.add_argument("--serverPlayer")
parser.add_argument("--difficulty")
parser.add_argument("--gameId")
parser.add_argument("--row")
parser.add_argument("--col")
args = parser.parse_args()

if __name__ == "__main__":
    channel = grpc.insecure_channel("localhost:50051")
    stub = game_grpc.FourInARowServiceStub(channel)

    if args.verb == "newGame":
        print(stub.NewGame(game_pb.FourInARow.NewGameReq(
                serverPlayer=int(args.serverPlayer),
                difficulty=int(args.difficulty))))
    elif args.verb == "move":
        stub.MakeMove(game_pb.FourInARow.MakeMoveReq(
                game=game_pb.FourInARow.Game(id=int(args.gameId)),
                move=game_pb.FourInARow.Move(
                    row=int(args.row), col=int(args.col))))
    elif args.verb == "getMoves":
        print(stub.GetMoves(game_pb.FourInARow.Game(id=int(args.gameId))))
    else:
        assert(False)

