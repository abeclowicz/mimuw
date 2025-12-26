from django.urls import path
from rest_framework.routers import DefaultRouter

from . import views

router = DefaultRouter()
router.register("games", views.GameViewSet)

urlpatterns = router.urls + [
    path("auth/login/google/", views.google_login, name="google-login"),
    path("auth/logout/", views.logout, name="logout"),
]
