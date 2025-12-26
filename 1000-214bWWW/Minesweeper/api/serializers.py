import datetime

from rest_framework import serializers

from .models import Game, Tile


class TileSerializer(serializers.ModelSerializer):
    class Meta:
        model = Tile
        fields = [
            "row",
            "column",
            "is_revealed",
            "is_crossed",
            "is_flagged",
            "is_mine",
            "adjacent_mines",
        ]

    def to_representation(self, instance):
        data = super().to_representation(instance)

        if not instance.is_revealed:
            data["adjacent_mines"] = 0

        if instance.game.status == Game.Status.ACTIVE:
            data["is_mine"] = False

        return data


class GameSerializer(serializers.ModelSerializer):
    tiles = TileSerializer(source="tile_set", many=True, read_only=True)
    mines = serializers.SerializerMethodField()

    class Meta:
        model = Game
        fields = [
            "id",
            "player",
            "status",
            "difficulty",
            "rows",
            "columns",
            "created_at",
            "updated_at",
            "tiles",
            "mines",
        ]

    @staticmethod
    def get_mines(obj):
        return sum(1 for tile in obj.tile_set.all() if tile.is_mine)


class GameCreateSerializer(serializers.ModelSerializer):
    class Meta:
        model = Game
        fields = ["difficulty"]


class CoordinateSerializer(serializers.Serializer):
    row = serializers.IntegerField()
    column = serializers.IntegerField()


class RevealResponseSerializer(serializers.Serializer):
    status = serializers.ChoiceField(choices=Game.Status)
    tiles = TileSerializer(many=True)


class QuerySerializer(serializers.Serializer):
    difficulty = serializers.ChoiceField(choices=Game.Difficulty, required=False)
    limit = serializers.IntegerField(min_value=1, required=False, default=10)


class ScoreSerializer(serializers.Serializer):
    player = serializers.SerializerMethodField()
    difficulty = serializers.ChoiceField(choices=Game.Difficulty)
    updated_at = serializers.DateTimeField()
    duration = serializers.SerializerMethodField()

    @staticmethod
    def get_player(obj):
        if obj.player:
            return obj.player.username
        return "Anonymous"

    @staticmethod
    def get_duration(obj):
        delta = getattr(obj, "duration", None)

        if delta is None:
            delta = obj.updated_at - obj.created_at

        if isinstance(delta, datetime.timedelta):
            return int(delta.total_seconds())

        return delta
