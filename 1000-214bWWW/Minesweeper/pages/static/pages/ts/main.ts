import "./auth";
import "./leaderboard";

import {api, Game, Result} from "./api";
import {runGame} from "./game";

/* ------------------------------------------------------------------------------------------------------------------ */

const containers = [...document.getElementsByClassName("game-container")] as HTMLElement[];

containers.forEach(container => {
    let readOnly = container.hasAttribute("data-readonly");

    let cachedGame: Game | null = null;
    let cleanupFn: (() => void) | void = undefined;

    async function getGame(): Promise<Game | void> {
        if (cachedGame) {
            return cachedGame;
        }

        let result: Result<Game> | undefined = undefined;
        let redirect = false;

        if (container.hasAttribute("data-id")) {
            const id = Number(container.getAttribute("data-id"));

            if (!Number.isNaN(id)) {
                result = await api.retrieveGame(id);
            }
        } else if ((window as any).APP_CONFIG) {
            if ((window as any).APP_CONFIG.id) {
                result = await api.retrieveGame((window as any).APP_CONFIG.id);
            }

            if ((window as any).APP_CONFIG.difficulty) {
                result = await api.createGame((window as any).APP_CONFIG.difficulty);
                redirect = true;
            }
        }

        if (result && result.ok) {
            if (redirect) {
                const [_, right] = document.title.split(" | ");

                document.title = `Game #${result.data.id} | ${right}`;
                window.history.replaceState({}, "", `/game/${result.data.id}`);
            }

            return cachedGame = result.data;
        }
    }

    function drawGame(): void {
        getGame().then(game => {
            if (game && !cleanupFn) {
                cleanupFn = runGame(container, game, readOnly);
            }
        });
    }

    function clearGame(): void {
        if (cleanupFn) {
            cleanupFn();
            cleanupFn = undefined;
        }
    }

    if (!readOnly) {
        drawGame();
        return;
    }

    const observer = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                drawGame();
            } else {
                clearGame();
            }
        });
    }, {
        root: null,
        rootMargin: "10px",
        threshold: 0
    });

    observer.observe(container);
});

/* ------------------------------------------------------------------------------------------------------------------ */

document.querySelectorAll("a.beveled").forEach(el => {
    el.addEventListener("mouseenter", () => {
        el.classList.remove("beveled");
        el.classList.add("sunken");
    });

    el.addEventListener("mouseleave", () => {
        el.classList.remove("sunken");
        el.classList.add("beveled");
    });
});
