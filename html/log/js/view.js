first = null;

function appendLog(msg) {
    $("#logContainer").prepend(msg["msg"] + "<br>");
}

let socket = new ReconnectingSocket("/api/log/ws_log",
    () => {
        socket.send(getUrlParam("logname"));
        first = true;
    }, (msg) => {
        msg = JSON.parse(msg.data);
        if(first) {
            first = false;
            $("#logContainer").text("");
            for (let i = 0; i < msg.length; i++) {
                appendLog(msg[i]);
            }
        } else {
            appendLog(msg);
        }
    })