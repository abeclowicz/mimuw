import {api, Score} from "./api";

/* ------------------------------------------------------------------------------------------------------------------ */

const leaderboards = {
    1: {
        main: document.getElementById("leaderboard-easy"),
        small: document.getElementById("leaderboard-easy-small")
    },
    2: {
        main: document.getElementById("leaderboard-medium"),
        small: document.getElementById("leaderboard-medium-small")
    },
    3: {
        main: document.getElementById("leaderboard-hard"),
        small: document.getElementById("leaderboard-hard-small")
    },
    4: {
        main: document.getElementById("leaderboard-expert"),
        small: document.getElementById("leaderboard-expert-small")
    }
};

/* ------------------------------------------------------------------------------------------------------------------ */

function updateLeaderboard(leaderboard: HTMLElement, scores: Score[], small = false): void {
    leaderboard.innerHTML = "";

    scores.sort((a: Score, b: Score) => {
        if (a.duration !== b.duration) {
            return a.duration - b.duration;
        }

        return a.updated_at.localeCompare(b.updated_at);
    });

    if (small) {
        for (let i = 0; i < scores.length; ++i) {
            leaderboard.innerHTML += `${i + 1}. ${scores[i].duration}s `;
        }

        return;
    }

    for (let i = 0; i < scores.length; ++i) {
        const tr = document.createElement("tr");

        const date = scores[i].updated_at
            .replace("T", " ")
            .replace(/\.\d+Z$/, "");

        tr.innerHTML = `
            <td>${scores[i].duration}s</td>
            <td>${scores[i].player}</td>
            <td>${date}</td>
        `;

        if (i < 3) {
            tr.setAttribute("data-position", String(i + 1));
        }

        leaderboard.appendChild(tr);
    }
}

/* ------------------------------------------------------------------------------------------------------------------ */

function updateAll(limit = 10, small = false): void {
    Object.entries(leaderboards).forEach(([key, value]) => {
        const difficulty = Number(key);

        if (Number.isNaN(difficulty)) {
            return;
        }

        if (value.main) {
            api.listLeaderboard(difficulty, limit).then(result => {
                if (result.ok) {
                    updateLeaderboard(value.main as HTMLElement, result.data);
                }
            });
        }

        if (small && value.small) {
            api.listLeaderboard(difficulty, 5).then(result => {
                if (result.ok) {
                    updateLeaderboard(value.small as HTMLElement, result.data, true);
                }
            });
        }
    });
}

/* ------------------------------------------------------------------------------------------------------------------ */

const select = document.getElementById("leaderboard-limit") as HTMLSelectElement | null;
const updatedTimer = document.getElementById("leaderboard-updated-timer");

let limit = 10;

let seconds = 0;
let intervalId: ReturnType<typeof setInterval> | null = null;

function startReloadLoop(): void {
    function loop(): void {
        if (updatedTimer) {
            updatedTimer.innerText = `Updated ${seconds}s ago`;
        }

        seconds = (seconds + 1) % 10;

        if (seconds === 0) {
            updateAll(limit);
        }
    }

    loop();

    intervalId = setInterval(() => {
        loop();
    }, 1000);
}

function resetReloadLoop(): void {
    if (intervalId) {
        clearInterval(intervalId);
    }

    seconds = 0;
    startReloadLoop();
}

if (select) {
    limit = Number(select.value);

    select.addEventListener("change", (event: Event) => {
        limit = Number(select.value);

        updateAll(limit);
        resetReloadLoop();
    });
}

updateAll(limit, true);
startReloadLoop();

/* ------------------------------------------------------------------------------------------------------------------ */

const eventSource = new EventSource("/events/leaderboard/");

eventSource.onmessage = (event: MessageEvent) => {
    const data: Score[] = JSON.parse(event.data);

    if (data.length > 0) {
        // @ts-ignore
        updateLeaderboard(leaderboards[data[0].difficulty].small, data, true);
    }

    console.log(data);
};

window.addEventListener("beforeunload", () => {
    eventSource.close();
});
