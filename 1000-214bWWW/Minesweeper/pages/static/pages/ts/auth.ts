import {getCsrfToken} from "./api";

(window as any).handleToken = async function (response: any): Promise<void> {
    const result = await fetch("/api/auth/login/google/", {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
            "X-CSRFToken": getCsrfToken()
        },
        body: JSON.stringify({credential: response.credential})
    });

    if (result.ok) {
        location.replace("/");
    }
};

(window as any).logout = async function () {
    const result = await fetch("/api/auth/logout/", {
        method: "POST",
        headers: {"X-CSRFToken": getCsrfToken()}
    });

    if (result.ok) {
        location.replace("/");
    }
};
