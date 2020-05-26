let first = null;

function logLevelToString(level) {
    if(level == 0)
        return "TRACE";
    else if(level == 1)
        return "DEBUG";
    else if(level == 2)
        return "INFO"
    else if(level == 3)
        return "WARN"
    else if(level == 4)
        return "ERROR"
    else if(level >= 5)
        return "FATAL";
}

function appendLog(msg) {
    let row = $("<div></div>").text(msg["msg"]).addClass("log_" + logLevelToString(msg["level"]));
    $("#logContainer").prepend(row);
}

let socket = new ReconnectingSocket("/api/log/ws_log",
    () => {
        socket.send("DEBUG");
        first = true;
    }, (msg) => {
        msg = JSON.parse(msg.data);
        if (first) {
            first = false;
            $("#logContainer").text("");
            for (let i = 0; i < msg.length; i++) {
                appendLog(msg[i]);
            }
        } else {
            appendLog(msg);
        }
    })