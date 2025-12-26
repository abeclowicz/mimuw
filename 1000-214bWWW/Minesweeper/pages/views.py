from django.contrib.auth.decorators import login_required
from django.shortcuts import render
from django.views.decorators.csrf import ensure_csrf_cookie

from api.models import Game


@ensure_csrf_cookie
def home(request):
    return render(request, "pages/home.html")


def play(request, difficulty: str):
    return render(
        request,
        "pages/game.html",
        {"difficulty": Game.Difficulty[difficulty.upper()].value},
    )


@login_required
def games(request):
    games = Game.objects.all().filter(player=request.user).order_by("-updated_at")
    return render(request, "pages/games.html", {"games": games})


def game(request, id: int):
    return render(request, "pages/game.html", {"id": id})


def leaderboard(request):
    return render(request, "pages/leaderboard.html")
