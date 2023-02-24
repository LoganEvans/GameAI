from concurrent import futures
from splinter import Browser
from typing import (List,)

import grpc
import logging
import proto.game_pb2 as game_pb
import proto.game_pb2_grpc as game_grpc
import time


class FourInARow:
    urlTemplate = (
            "https://waywardtrends.com/bryan/FourInARow/Main.html"
            "?cpu={cpuPlayer}&diff={difficulty}")

    def __init__(self, serverPlayer=1, difficulty=3):
        self.browser = Browser("chrome")
        self.serverPlayer = serverPlayer
        self.difficulty = difficulty
        url = FourInARow.urlTemplate.format(
                cpuPlayer=self.serverPlayer,
                difficulty=self.difficulty)
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


class FourInARowService(game_grpc.FourInARowServiceServicer):
    games = {}
    nextGameId = 0

    def NewGame(self, request: game_pb.FourInARow.NewGameReq, context):
        gameId = FourInARowService.nextGameId
        FourInARowService.nextGameId += 1

        FourInARowService.games[gameId] = FourInARow(
                serverPlayer=request.serverPlayer,
                difficulty=request.difficulty)

        return game_pb.FourInARow.Game(id=gameId)

    def GetMoves(self, request: game_pb.FourInARow.Game, context):
        gameId = request.id
        game = FourInARowService.games[gameId]
        moveList = game_pb.FourInARow.MoveList()
        moveList.moves.extend([
                game_pb.FourInARow.Move(row=int(m[1]) - 1,
                                        col=ord(m[0]) - ord('a'))
                for m in game.getMoves()])
        return moveList

    def MakeMove(self, request: game_pb.FourInARow.MakeMoveReq, context):
        gameId = request.game.id
        move = request.move
        FourInARowService.games[gameId].makeMove(move.col)
        return game_pb.FourInARow.Empty()


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    game_grpc.add_FourInARowServiceServicer_to_server(
        FourInARowService(), server)
    server.add_insecure_port('[::]:50051')
    server.start()
    server.wait_for_termination()


if __name__ == '__main__':
    logging.basicConfig()
    serve()

