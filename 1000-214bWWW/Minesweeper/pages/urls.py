from django.urls import path

from . import views

urlpatterns = [
    path("", views.home, name="home"),
    path("play/<str:difficulty>", views.play, name="play"),
    path("games", views.games, name="games"),
    path("game/<int:id>", views.game, name="game"),
    path("leaderboard", views.leaderboard, name="leaderboard"),
]
