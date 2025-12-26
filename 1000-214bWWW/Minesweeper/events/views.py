import queue

from django.http import StreamingHttpResponse

from api.views import leaderboard_broadcaster


def leaderboard(request):
    def event_stream():
        event_queue = queue.Queue()
        leaderboard_broadcaster.add_listener(event_queue)

        try:
            yield "event: ping\ndata: connected\n\n"

            while True:
                try:
                    message = event_queue.get(timeout=10)
                    yield f"data: {message}\n\n"
                except queue.Empty:
                    yield ": keepalive\n\n"
        finally:
            leaderboard_broadcaster.remove_listener(event_queue)

    response = StreamingHttpResponse(event_stream(), content_type="text/event-stream")

    response["Cache-Control"] = "no-cache"
    response["X-Accel-Buffering"] = "no"

    return response
