from django.contrib import admin

from api.models import Tile, Game


@admin.register(Game)
class GameAdmin(admin.ModelAdmin):
    list_display = [
        "id",
        "player",
        "status",
        "difficulty",
        "rows",
        "columns",
        "created_at",
        "updated_at",
    ]
    list_filter = ["status", "difficulty"]


@admin.register(Tile)
class TileAdmin(admin.ModelAdmin):
    list_display = [
        "game",
        "row",
        "column",
        "is_revealed",
        "is_crossed",
        "is_flagged",
        "is_mine",
        "adjacent_mines",
    ]
    list_filter = [
        "game",
        "is_revealed",
        "is_crossed",
        "is_flagged",
        "is_mine",
        "adjacent_mines",
    ]
