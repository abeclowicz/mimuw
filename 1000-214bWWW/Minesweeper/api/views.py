import json
import random
import threading

from django.contrib import auth
from django.contrib.auth.models import User
from django.core.exceptions import BadRequest
from django.db.models import F
from django.http import HttpResponse
from drf_spectacular.types import OpenApiTypes
from drf_spectacular.utils import extend_schema, OpenApiParameter
from google.auth.transport import requests as google_requests
from google.oauth2 import id_token
from rest_framework import viewsets
from rest_framework.decorators import action
from rest_framework.exceptions import MethodNotAllowed
from rest_framework.permissions import IsAdminUser, AllowAny
from rest_framework.response import Response
from rest_framework.status import HTTP_201_CREATED, HTTP_204_NO_CONTENT

from .permissions import IsGameOwner
from .serializers import *


class GameViewSet(viewsets.ModelViewSet):
    serializer_class = GameSerializer
    queryset = Game.objects.prefetch_related("tile_set").all()
    http_method_names = ["get", "post", "delete"]

    def get_permissions(self):
        if self.action == "destroy":
            return [IsAdminUser()]
        elif self.action in ["retrieve", "flag", "reveal"]:
            return [IsGameOwner()]
        return [AllowAny()]

    def create(self, request, *args, **kwargs):
        serializer = GameCreateSerializer(data=request.data)
        serializer.is_valid(raise_exception=True)

        config = {
            Game.Difficulty.EASY: {"rows": 8, "cols": 8, "mines": 10},
            Game.Difficulty.MEDIUM: {"rows": 14, "cols": 20, "mines": 40},
            Game.Difficulty.HARD: {"rows": 20, "cols": 32, "mines": 99},
            Game.Difficulty.EXPERT: {"rows": 25, "cols": 36, "mines": 140},
        }

        difficulty = serializer.validated_data["difficulty"]
        cfg = config.get(difficulty, config[Game.Difficulty.MEDIUM])

        game = Game.objects.create(
            player=request.user if request.user.is_authenticated else None,
            difficulty=difficulty,
            rows=cfg["rows"],
            columns=cfg["cols"],
        )

        if not request.user.is_authenticated:
            anonymous_games = request.session.get("anonymous_games", [])
            anonymous_games.append(game.id)
            request.session["anonymous_games"] = anonymous_games
            request.session.modified = True

        tiles = [
            Tile(game=game, row=row, column=col)
            for row in range(game.rows)
            for col in range(game.columns)
        ]

        for i in random.sample(range(len(tiles)), cfg["mines"]):
            tiles[i].is_mine = True

        tile_dict = {(tile.row, tile.column): tile for tile in tiles}

        for tile in tile_dict.values():
            if tile.is_mine:
                continue

            for dr in [-1, 0, 1]:
                for dc in [-1, 0, 1]:
                    neighbour = tile_dict.get((tile.row + dr, tile.column + dc))
                    if neighbour and neighbour.is_mine:
                        tile.adjacent_mines += 1

        empty_indices = [
            i
            for i in range(len(tiles))
            if (not tiles[i].is_mine) and tiles[i].adjacent_mines == 0
        ]

        if not empty_indices:
            empty_indices = [i for i in range(len(tiles)) if not tiles[i].is_mine]

        tiles[random.choice(empty_indices)].is_crossed = True

        Tile.objects.bulk_create(tiles)

        return Response(GameSerializer(game).data, status=HTTP_201_CREATED)

    @extend_schema(request=CoordinateSerializer, responses={HTTP_204_NO_CONTENT: None})
    @action(detail=True, methods=["post"])
    def flag(self, request, pk=None):
        game = self.get_object()

        if game.status != Game.Status.ACTIVE:
            raise BadRequest({"error": "Game has already finished"})

        tile = game.tile_set.filter(
            row=request.data.get("row"), column=request.data.get("column")
        ).first()

        if not tile:
            raise BadRequest({"error": "Invalid tile coordinates"})

        if tile.is_revealed:
            raise BadRequest({"error": "Cannot flag a revealed tile"})

        tile.is_flagged = not tile.is_flagged

        tile.save()
        game.save()

        return Response(status=HTTP_204_NO_CONTENT)

    @extend_schema(
        request=CoordinateSerializer(many=True), responses=RevealResponseSerializer
    )
    @action(detail=True, methods=["post"])
    def reveal(self, request, pk=None):
        game = self.get_object()

        if game.status != Game.Status.ACTIVE:
            raise BadRequest({"error": "Game has already finished"})

        serializer = CoordinateSerializer(data=request.data, many=True)
        serializer.is_valid(raise_exception=True)

        tiles = list(game.tile_set.all())
        tile_dict = {(tile.row, tile.column): tile for tile in tiles}

        dfs_stack = []

        for coordinates in serializer.validated_data:
            tile = tile_dict.get((coordinates["row"], coordinates["column"]))
            if tile and (not tile.is_revealed) and (not tile.is_flagged):
                dfs_stack.append(tile)

        if not dfs_stack:
            raise BadRequest({"error": "Invalid tile(s) coordinates"})

        if any(tile.is_mine for tile in dfs_stack):
            game.status = Game.Status.LOST
            game.save()

            return Response(
                RevealResponseSerializer(
                    {"status": game.status, "tiles": game.tile_set.all()}
                ).data
            )

        tiles_to_update = []

        while dfs_stack:
            tile = dfs_stack.pop()

            tile.is_revealed = True
            tile.is_crossed = False
            tiles_to_update.append(tile)

            if tile.adjacent_mines == 0:
                for dr in [-1, 0, 1]:
                    for dc in [-1, 0, 1]:
                        neighbor = tile_dict.get((tile.row + dr, tile.column + dc))
                        if (
                            neighbor
                            and (not neighbor.is_revealed)
                            and (not neighbor.is_flagged)
                        ):
                            dfs_stack.append(neighbor)

        if tiles_to_update:
            Tile.objects.bulk_update(tiles_to_update, ["is_revealed", "is_crossed"])

            mines = sum(1 for tile in tiles if tile.is_mine)
            unrevealed = sum(1 for tile in tiles if not tile.is_revealed)

            if mines == unrevealed:
                game.status = Game.Status.WON
                game.save()

                broadcast_leaderboard_top5(game.difficulty)
            else:
                game.save()

        return Response(
            RevealResponseSerializer(
                {"status": game.status, "tiles": tiles_to_update}
            ).data
        )

    @extend_schema(
        parameters=[
            OpenApiParameter(
                name="difficulty",
                type=OpenApiTypes.INT,
                location="query",
                required=False,
                enum=Game.Difficulty.values,
            ),
            OpenApiParameter(
                name="limit", type=OpenApiTypes.INT, location="query", required=False
            ),
        ],
        responses=ScoreSerializer(many=True),
    )
    @action(detail=False, methods=["get"])
    def leaderboard(self, request):
        serializer = QuerySerializer(data=request.GET)
        serializer.is_valid(raise_exception=True)

        difficulty = serializer.validated_data.get("difficulty")
        limit = serializer.validated_data["limit"]

        queryset = Game.objects.filter(status=Game.Status.WON)

        if difficulty:
            queryset = queryset.filter(difficulty=difficulty)

        queryset = queryset.annotate(
            duration=F("updated_at") - F("created_at")
        ).order_by("duration")[:limit]

        return Response(ScoreSerializer(queryset, many=True).data)


""" ---------------------------------------------------------------------------------------------------------------- """

GOOGLE_CLIENT_ID = (
    "XXXXXXXXXXXX-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX.apps.googleusercontent.com"
)


def google_login(request):
    if request.method == "POST":
        data = json.loads(request.body)

        try:
            idinfo = id_token.verify_oauth2_token(
                id_token=data.get("credential"),
                request=google_requests.Request(),
                audience=GOOGLE_CLIENT_ID,
                clock_skew_in_seconds=10,
            )

            username = idinfo["sub"]

            if idinfo.get("email"):
                username = idinfo["email"]
            if idinfo.get("given_name") and idinfo.get("family_name"):
                username = idinfo["given_name"] + " " + idinfo["family_name"]

            user, created = User.objects.get_or_create(
                username=username,
                defaults={
                    "email": idinfo.get("email", ""),
                    "first_name": idinfo.get("given_name", ""),
                    "last_name": idinfo.get("family_name", ""),
                },
            )

            auth.login(request, user)
            return HttpResponse(status=HTTP_204_NO_CONTENT)

        except ValueError:
            raise BadRequest({"error": "Invalid credentials"})

    raise MethodNotAllowed(request.method)


def logout(request):
    if request.method == "POST":
        auth.logout(request)
        return HttpResponse(status=HTTP_204_NO_CONTENT)

    raise MethodNotAllowed(request.method)


""" ---------------------------------------------------------------------------------------------------------------- """


class SSEBroadcaster:
    def __init__(self):
        self.listeners = set()
        self.lock = threading.Lock()

    def add_listener(self, queue):
        with self.lock:
            self.listeners.add(queue)

    def remove_listener(self, queue):
        with self.lock:
            self.listeners.remove(queue)

    def broadcast(self, message):
        with self.lock:
            for queue in self.listeners:
                queue.put(message)


leaderboard_broadcaster = SSEBroadcaster()


def broadcast_leaderboard_top5(difficulty):
    queryset = Game.objects.filter(status=Game.Status.WON, difficulty=difficulty)
    queryset = queryset.annotate(duration=F("updated_at") - F("created_at")).order_by(
        "duration"
    )[:5]

    data = ScoreSerializer(queryset, many=True).data
    leaderboard_broadcaster.broadcast(json.dumps(data))
