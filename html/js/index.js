let lowestBoxHeight = 200;
let lowestBoxId = -1;
let boxIdCounter = 0;

function notifyError(msg) {
    notify(msg, "notify-error");
}

function notify(msg, classname = "notify-info") {
    if(msg == null) {
        msg = "(null)";
    }
    let msgBox = $("<div></div>").text(msg).addClass(classname).addClass("notify")
    $("body").append(msgBox);
    let boxHeight = Math.min(lowestBoxHeight, window.outerHeight - 200);
    msgBox.css("top", boxHeight + "px");
    let id = ++boxIdCounter;
    lowestBoxId = id;

    let animationCounter = 0;
    let intervalID = setInterval(function() {
        animationCounter += 1;
        if(animationCounter > 70) {
            boxHeight -= 2.5;
            msgBox.css("top", boxHeight + "px");
        }
        if(lowestBoxId === id) {
            lowestBoxHeight = boxHeight + msgBox.height() + 20;
        }
        if(animationCounter >= 100) {
            msgBox.css("opacity", (150 - animationCounter) / 50.0);
        }

        if(animationCounter >= 150) {
            clearInterval(intervalID);
            msgBox.detach();
            if(lowestBoxId === id) {
                lowestBoxHeight = 200;
            }
        }
    }, 20);

}

$(function() {
    $("#header").load("/templates/header.html", "", () => {
        let pathname = window.location.pathname;
        if(pathname.startsWith("/hub")) {
            $("#nav_hub").addClass("selected");
        } else if(pathname.startsWith("/scripts")) {
            $("#nav_script").addClass("selected");
        } else if(pathname.startsWith("/db")) {
            $("#nav_db").addClass("selected");
        } else if(pathname.startsWith("/log")) {
            $("#nav_log").addClass("selected");
        } else if(pathname.startsWith("/apps")) {
            $("#nav_apps").addClass("selected");
        }
    });
})

class ReconnectingSocket {
    constructor(suburl, onopen = () => { }, onmsg = (msg) => { }, onclose = () => { }) {
        this.onopen = onopen;
        this.onmsg = onmsg;
        this.onclose = onclose;
        this.url = (location.protocol !== 'https:' ? "ws://" : "wss://") + location.host + suburl;
        this.connect();
    }

    connect() {
        this.socket = new WebSocket(this.url);
        console.log("Trying to connect...");
        this.socket.addEventListener("open", () => {
            console.log("Connected!");
            this.onopen();
        });
        this.socket.addEventListener("message", this.onmsg);
        let err = (e) => {
            if (this.socket.readyState === WebSocket.CLOSING || this.socket.readyState === WebSocket.CLOSED) {
                console.warn("Connection lost!");
                this.onclose()
                setTimeout(() => { this.connect() }, 1000)
            }
        };
        this.socket.addEventListener("close", err);
        this.socket.addEventListener("error", err);
    }

    send(msg) {
        console.debug("WS Sending: '" + msg + "'");
        this.socket.send(msg);
    }

    isOpen() {
        return this.socket.readyState === WebSocket.OPEN
    }
}

function getUrlParam(name) {
    const params = new URLSearchParams(window.location.search);
    return params.get(name);
}

function getUrlParamAsNumber(name) {
    return Number(getUrlParam(name));
}