from concurrent import futures
from splinter import Browser
from typing import (List,)

import grpc
import logging
import proto.game_pb2 as game_pb
import proto.game_pb2_grpc as game_grpc
import time


class Connect4:
    urlTemplate = (
            "https://waywardtrends.com/bryan/FourInARow/Main.html"
            "?cpu=1&diff={difficulty}")

    def __init__(self, serverPlayer=1, difficulty=3):
        self.browser = Browser("chrome")
        self.serverPlayer = serverPlayer
        self.difficulty = difficulty
        url = Connect4.urlTemplate.format(
                cpuPlayer=self.serverPlayer,
                difficulty=self.difficulty)
        if serverPlayer == 0:
            url += "&humancolor=b"
        self.browser.visit(url)

    def updateMoves(self):
        self.moves = self.browser.find_by_id(
                "moveListContainer").find_by_id("moveList").value.split()[4:]

    def makeMove(self, column: int):
        self.browser.find_by_id("boardStuff"
                ).find_by_id("chessboard"
                ).find_by_id(f"top{column + 1}").click()

    def getMoves(self) -> List[str]:
        return self.browser.find_by_id(
                "moveListContainer").find_by_id("moveList").value.split()[4:]


class Connect4Service(game_grpc.Connect4ServiceServicer):
    games = {}
    nextGameId = 0

    def NewGame(self, request: game_pb.Connect4.NewGameReq, context):
        logging.info("NewGame(serverPlayer=%s, difficulty=%s)",
                     request.serverPlayer, request.difficulty)
        gameId = Connect4Service.nextGameId
        Connect4Service.nextGameId += 1

        Connect4Service.games[gameId] = Connect4(
                serverPlayer=request.serverPlayer,
                difficulty=request.difficulty)

        return game_pb.Connect4.Game(id=gameId)

    def GetMoves(self, request: game_pb.Connect4.Game, context):
        gameId = request.id
        logging.info("GetMoves(%s)", gameId)
        game = Connect4Service.games[gameId]
        moveList = game_pb.Connect4.MoveList()
        moveList.moves.extend([
                game_pb.Connect4.Move(row=int(m[1]) - 1,
                                        col=ord(m[0]) - ord('a'))
                for m in game.getMoves()])
        return moveList

    def MakeMove(self, request: game_pb.Connect4.MakeMoveReq, context):
        gameId = request.game.id
        move = request.move
        logging.info(
                "MakeMoves(%s, {row: %s, col: %s})",
                gameId, move.row, move.col)
        Connect4Service.games[gameId].makeMove(move.col)
        return game_pb.Connect4.Empty()


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    game_grpc.add_Connect4ServiceServicer_to_server(
        Connect4Service(), server)
    server.add_insecure_port('[::]:50051')
    server.start()
    logging.info("Server started")
    server.wait_for_termination()

if __name__ == '__main__':
    logging.basicConfig(
        format="%(levelname)s:%(asctime)s %(filename)s:%(lineno)d|%(message)s",
        level=logging.INFO)
    serve()

