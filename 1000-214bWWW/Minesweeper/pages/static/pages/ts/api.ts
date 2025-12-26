type Result<T> = { ok: true; data: T } | { ok: false; error: string; status?: number };

interface Tile {
    row: number;
    column: number;
    is_revealed: boolean;
    is_crossed: boolean;
    is_flagged: boolean;
    is_mine: boolean;
    adjacent_mines: number;
}

interface Game {
    id: number;
    player: number;
    status: "active" | "won" | "lost";
    difficulty: number;
    rows: number;
    columns: number;
    created_at: string;
    updated_at: string;
    tiles: Tile[];
    mines: number;
}

type Coordinates = Pick<Tile, "row" | "column">
type RevealResponse = Pick<Game, "status" | "tiles">;
type Score = Pick<Game, "difficulty" | "updated_at"> & { player: string, duration: number };

function getCsrfToken(): string {
    const match = document.cookie.match(/csrftoken=([^;]+)/);
    return match ? match[1] : "";
}

async function apiFetch<T>(method: string, path: string, body?: unknown): Promise<Result<T>> {
    try {
        const options: RequestInit = {
            method,
            credentials: "include",
            headers: {
                "Content-Type": "application/json",
                "X-CSRFToken": getCsrfToken(),
            }
        };

        if (body) {
            options.body = JSON.stringify(body);
        }

        const result = await fetch(path, options);

        if (!result.ok) {
            let message = `HTTP ${result.status}`;

            try {
                const error = await result.json();
                message = error.error ?? message;
            } catch {
                /* ignore */
            }

            return {ok: false, error: message, status: result.status};
        }

        if (result.status === 204) {
            return {ok: true, data: {} as T};
        }

        return {ok: true, data: await result.json()};

    } catch (ex) {
        return {ok: false, error: String(ex)};
    }
}

class MinesweeperApiClient {
    async createGame(difficulty: number): Promise<Result<Game>> {
        return apiFetch<Game>("POST", "/api/games/", {difficulty});
    }

    async retrieveGame(gameId: number): Promise<Result<Game>> {
        return apiFetch<Game>("GET", `/api/games/${gameId}/`);
    }

    async flagTile(gameId: number, row: number, column: number): Promise<Result<void>> {
        return apiFetch<void>("POST", `/api/games/${gameId}/flag/`, {row, column});
    }

    async revealTiles(gameId: number, coordinates: Coordinates[]): Promise<Result<RevealResponse>> {
        return apiFetch<RevealResponse>("POST", `/api/games/${gameId}/reveal/`, coordinates);
    }

    async listLeaderboard(difficulty: number, limit = 10): Promise<Result<Score[]>> {
        return apiFetch<Score[]>("GET", `/api/games/leaderboard/?difficulty=${difficulty}&limit=${limit}`);
    }
}

const api = new MinesweeperApiClient();

export {Result, Tile, Game, Score, getCsrfToken, api};
