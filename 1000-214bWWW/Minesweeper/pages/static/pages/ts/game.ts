import {api, Game, Tile} from "./api";

/* ------------------------------------------------------------------------------------------------------------------ */

interface State {
    game: Game;

    flagCounter: HTMLElement | null;
    timer: HTMLElement | null;
    board: HTMLElement;

    hasCrossedTile: boolean;
    numFlags: number;
    permanentlyRevealedTiles: HTMLElement[];
    temporarilyRevealedTiles: HTMLElement[];

    timeoutId: number | null;
    intervalId: number | null;
}

function createState(container: HTMLElement, game: Game): State | void {
    const board = container.querySelector(".board") as HTMLElement | null;

    if (!board) {
        return;
    }

    const state: State = {
        game: game,
        flagCounter: container.querySelector(".flag-counter") as HTMLElement | null,
        timer: container.querySelector(".timer") as HTMLElement | null,
        board: board,
        hasCrossedTile: false,
        numFlags: game.mines,
        permanentlyRevealedTiles: [],
        temporarilyRevealedTiles: [],
        timeoutId: null,
        intervalId: null
    };

    game.tiles.forEach(tile => {
        if (tile.is_crossed) {
            state.hasCrossedTile = true;
        }

        if (tile.is_flagged) {
            state.numFlags--;
        }
    });

    return state;
}

/* ------------------------------------------------------------------------------------------------------------------ */

function setFlagCounter(value: number, state: State): void {
    if (state.flagCounter) {
        state.flagCounter.innerText = String(value);
    }
}

function initializeTimer(state: State): void {
    if (!state.timer) {
        return;
    }

    const created_at = Date.parse(state.game.created_at);

    if (Number.isNaN(created_at)) {
        return;
    }

    if (state.game.status !== "active") {
        const updated_at = Date.parse(state.game.updated_at);

        if (!Number.isNaN(updated_at)) {
            const seconds = Math.floor((updated_at - created_at) / 1000);
            state.timer.textContent = String(seconds);
        }

        return;
    }

    const tick = () => {
        const seconds = Math.floor((Date.now() - created_at) / 1000);

        if (state.game.status === "active") {
            // @ts-ignore
            state.timer.textContent = String(
                Math.max(0, Math.min(999, seconds))
            );
        }
    };

    tick();

    state.timeoutId = setTimeout(() => {
        tick();
        state.intervalId = setInterval(tick, 1000);
    }, 1000 - ((Date.now() - created_at) % 1000));
}

/* ------------------------------------------------------------------------------------------------------------------ */

function createHTMLElementFromTile(tile: Tile): HTMLElement {
    const div = document.createElement("div");

    div.classList.add("tile");

    if (!tile.is_revealed) {
        div.classList.add("beveled");
    }

    div.setAttribute("data-row", String(tile.row));
    div.setAttribute("data-column", String(tile.column));

    if (tile.is_crossed) {
        div.setAttribute("data-value", "cross");
    } else if (tile.is_flagged) {
        div.setAttribute("data-value", "flag");
    } else if (tile.is_mine) {
        div.setAttribute("data-value", "mine");
    }

    if (tile.adjacent_mines > 0) {
        div.setAttribute("data-value", String(tile.adjacent_mines));
    }

    return div;
}

function initializeBoard(state: State): void {
    state.board.addEventListener("contextmenu", (event: MouseEvent) => {
        event.preventDefault();
    });

    for (let row = 0; row < state.game.rows; ++row) {
        const tr = document.createElement("tr");

        for (let column = 0; column < state.game.columns; ++column) {
            const td = document.createElement("td");

            const tile = state.game.tiles.find(tile => {
                return (row == tile.row) && (column == tile.column)
            }) ?? {
                row: row,
                column: column,
                is_revealed: true,
                is_crossed: false,
                is_flagged: false,
                is_mine: false,
                adjacent_mines: 0
            };

            td.appendChild(createHTMLElementFromTile(tile));
            tr.appendChild(td);
        }

        state.board.appendChild(tr);
    }
}

function updateTile(tile: Tile, state: State): void {
    const tiles = [...state.board.getElementsByClassName("tile")] as HTMLElement[];

    const oldTile = tiles.find(t => {
        const row = getRow(t);
        const column = getColumn(t);

        if (Number.isNaN(row) || Number.isNaN(column)) {
            return false;
        }

        return (tile.row == row) && (tile.column == column);
    })

    if (oldTile) {
        const newTile = createHTMLElementFromTile(tile);

        if (isCrossed(newTile)) {
            state.hasCrossedTile = true;
        }

        addMouseDownEvent(newTile, state);
        oldTile.replaceWith(newTile);
    }
}

/* ------------------------------------------------------------------------------------------------------------------ */

function isActive(game: Game): boolean {
    return game.status === "active";
}

function isHidden(tile: HTMLElement): boolean {
    return tile.classList.contains("beveled");
}

function isCrossed(tile: HTMLElement): boolean {
    return tile.getAttribute("data-value") === "cross";
}

function isFlagged(tile: HTMLElement): boolean {
    return tile.getAttribute("data-value") === "flag";
}

function isClickable(tile: HTMLElement): boolean {
    return isHidden(tile) && !isFlagged(tile);
}

/* ------------------------------------------------------------------------------------------------------------------ */

function getNumericDataAttribute(tile: HTMLElement, name: String): number {
    return Number.parseInt(tile.getAttribute(`data-${name}`) ?? "");
}

function getRow(tile: HTMLElement): number {
    return getNumericDataAttribute(tile, "row");
}

function getColumn(tile: HTMLElement): number {
    return getNumericDataAttribute(tile, "column");
}

function getNumericValue(tile: HTMLElement): number {
    return getNumericDataAttribute(tile, "value");
}

function getAdjacentTiles(tile: HTMLElement, state: State): HTMLElement[] {
    const row = getRow(tile);
    const column = getColumn(tile);

    if (Number.isNaN(row) || Number.isNaN(column)) {
        return [];
    }

    const tiles = [...state.board.getElementsByClassName("tile")] as HTMLElement[];

    return tiles.filter(t => {
        const r = getRow(t);
        const c = getColumn(t);

        if (Number.isNaN(r) || Number.isNaN(c)) {
            return false;
        }

        const dr = Math.abs(row - r);
        const dc = Math.abs(column - c);

        return (dr <= 1 && dc <= 1) && !(dr == 0 && dc == 0);
    });
}

/* ------------------------------------------------------------------------------------------------------------------ */

function reveal(tile: HTMLElement, state: State): void {
    if (!isHidden(tile)) {
        return;
    }

    if (isCrossed(tile)) {
        tile.removeAttribute("data-value");
        state.hasCrossedTile = false;
    }

    tile.classList.remove("beveled");
}

function hide(tile: HTMLElement): void {
    if (!isHidden(tile)) {
        tile.classList.add("beveled");
    }
}

function flag(tile: HTMLElement, state: State): void {
    if (isFlagged(tile)) {
        tile.removeAttribute("data-value");
        setFlagCounter(++state.numFlags, state);
    } else if (state.numFlags > 0) {
        tile.setAttribute("data-value", "flag");
        setFlagCounter(--state.numFlags, state);
    }
}

/* ------------------------------------------------------------------------------------------------------------------ */

function handleLeftClick(tile: HTMLElement, state: State): void {
    if ((state.hasCrossedTile && !isCrossed(tile)) || !isActive(state.game)) {
        return;
    }

    if (isHidden(tile)) {
        if (!isFlagged(tile)) {
            reveal(tile, state);
            state.permanentlyRevealedTiles.push(tile);
        }

        return;
    }

    const value = getNumericValue(tile);

    if (Number.isNaN(value)) {
        return;
    }

    const adjacentTiles = getAdjacentTiles(tile, state);

    const adjacentClickableTiles = adjacentTiles.filter(t => isClickable(t));
    const adjacentFlaggedTiles = adjacentTiles.filter(t => isFlagged(t));

    adjacentClickableTiles.forEach(t => {
        reveal(t, state);
    });

    if (value == adjacentFlaggedTiles.length) {
        state.permanentlyRevealedTiles.push(...adjacentClickableTiles);
    } else {
        state.temporarilyRevealedTiles.push(...adjacentClickableTiles);
    }
}

function handleRightClick(tile: HTMLElement, state: State): void {
    if (state.hasCrossedTile || !isHidden(tile) || !isActive(state.game)) {
        return;
    }

    flag(tile, state);

    const row = getRow(tile);
    const column = getColumn(tile);

    if (Number.isNaN(row) || Number.isNaN(column)) {
        flag(tile, state); // undo
        return;
    }

    api.flagTile(state.game.id, row, column).then(result => {
        if (!result.ok) {
            flag(tile, state); // undo
        }
    });
}

function addMouseDownEvent(tile: HTMLElement, state: State): void {
    tile.addEventListener("mousedown", (event: MouseEvent) => {
        if (event.button === 0) {
            handleLeftClick(tile, state);
        }

        if (event.button === 2) {
            handleRightClick(tile, state);
        }
    });
}

/* ------------------------------------------------------------------------------------------------------------------ */

function runGame(container: HTMLElement, game: Game, readOnly = false): (() => void) | void {
    const state = createState(container, game);

    if (!state) {
        return;
    }

    setFlagCounter(state.numFlags, state);
    initializeTimer(state);
    initializeBoard(state);

    if (readOnly) {
        return () => {
            state.board.innerHTML = "";

            if (state.timeoutId) {
                clearTimeout(state.timeoutId);
            }

            if (state.intervalId) {
                clearInterval(state.intervalId);
            }
        }
    }

    const tiles = [...state.board.getElementsByClassName("tile")] as HTMLElement[];

    tiles.forEach(tile => {
        addMouseDownEvent(tile, state);
    });

    document.addEventListener("mouseup", async (event: MouseEvent) => {
        if (event.button === 0) { /* LMB */
            const coordinates = state.permanentlyRevealedTiles.map(tile => ({
                row: getRow(tile),
                column: getColumn(tile)
            }));

            if (coordinates.length > 0) {
                const result = await api.revealTiles(game.id, coordinates);

                if (result.ok) {
                    game.status = result.data.status;

                    result.data.tiles.forEach(tile => {
                        updateTile(tile, state);
                    });
                } else {
                    state.permanentlyRevealedTiles.forEach(tile => {
                        hide(tile);
                    });
                }
            }

            state.temporarilyRevealedTiles.forEach(tile => {
                hide(tile);
            });

            state.permanentlyRevealedTiles = [];
            state.temporarilyRevealedTiles = [];
        }
    });
}

/* ------------------------------------------------------------------------------------------------------------------ */

export {runGame};
