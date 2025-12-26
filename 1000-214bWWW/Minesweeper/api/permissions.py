from rest_framework import permissions


class IsGameOwner(permissions.BasePermission):
    def has_object_permission(self, request, view, obj):
        if request.user and request.user.is_staff:
            return True

        if obj.player:
            return obj.player == request.user

        anonymous_games = request.session.get("anonymous_games", [])
        return obj.id in anonymous_games
