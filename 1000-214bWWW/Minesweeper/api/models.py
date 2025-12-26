from django.contrib.auth.models import User
from django.core.validators import MaxValueValidator
from django.db import models


class Game(models.Model):
    class Difficulty(models.IntegerChoices):
        EASY = 1, "easy"
        MEDIUM = 2, "medium"
        HARD = 3, "hard"
        EXPERT = 4, "expert"

    class Status(models.TextChoices):
        ACTIVE = "active"
        WON = "won"
        LOST = "lost"

    player = models.ForeignKey(User, on_delete=models.CASCADE, null=True, blank=True)

    status = models.CharField(choices=Status, default=Status.ACTIVE, max_length=6)
    difficulty = models.IntegerField(choices=Difficulty)

    rows = models.PositiveIntegerField()
    columns = models.PositiveIntegerField()

    created_at = models.DateTimeField(auto_now_add=True)
    updated_at = models.DateTimeField(auto_now=True)

    def __str__(self):
        return f"Game #{self.id}"


class Tile(models.Model):
    game = models.ForeignKey(Game, on_delete=models.CASCADE)

    row = models.PositiveIntegerField()
    column = models.PositiveIntegerField()

    is_revealed = models.BooleanField(default=False)
    is_crossed = models.BooleanField(default=False)
    is_flagged = models.BooleanField(default=False)
    is_mine = models.BooleanField(default=False)

    adjacent_mines = models.PositiveIntegerField(
        default=0, validators=[MaxValueValidator(8)]
    )

    class Meta:
        unique_together = ("game", "row", "column")

    def __str__(self):
        return f"Tile #{self.id} at ({self.row}, {self.column})"
