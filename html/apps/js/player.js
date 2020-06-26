function navigateBack(path) {
    path = path.substr(0, path.lastIndexOf("/"));
    if(path === "")
        path = "."
    ls(path);
}
let currentPath = ".";
let first = true;
let reversePlaylist = false;
let queue = []
let queueCurrentPlayingIndex = null;
let highlightTimeoutId = null;
function ls(path, record = true) {
    $.ajax({
        dataType: "json",
        url: "/api/apps/player/ls?path=" + encodeURIComponent(path),
        success: function(data) {
            currentPath = path;
            if(record) {
                history.pushState({path: path}, "ZeVa", "?path=" + encodeURIComponent(path));
            }
            let songs = $("#songs");
            songs.text("");

            //delete .-Folder
            let index = data.findIndex((e) => e["name"].endsWith("/."));
            if(index >= 0)
                data.splice(index, 1);

            data = data.sort((a, b) => {
                return a["name"].toLowerCase().localeCompare(b["name"].toLowerCase())
            });

            for(let i = 0; i < data.length; ++i) {
                let row = $("<div></div>").addClass("song");
                row.append($("<img>").addClass("icon").attr("src", data[i].directory ? "/icons/directory.svg" : "/icons/file.svg"));
                row.attr("songname", data[i].name);
                row.append($("<span></span>").text(data[i].name.replace(/^.*[\\\/]/, '')));
                row.click(() => {
                    if(data[i].directory) {
                        if(data[i].name.endsWith("..")) {
                            navigateBack(path);
                        } else {
                            ls(data[i].name);
                        }
                    } else if(data[i].musicfile) {
                        queue = data.filter((e) => e.musicfile).map((e) => e.name);
                        if(reversePlaylist)
                            queue.reverse()
                        queueCurrentPlayingIndex = queue.findIndex((element) => element === data[i]["name"]);

                        $.ajax({
                            url: "/api/apps/player/setQueue",
                            type: "POST",
                            data: {
                                queue: JSON.stringify(queue),
                                startIndex: queueCurrentPlayingIndex
                            },
                            success: function(data) {
                                notify("Success!");
                                highlightPlayingSong();
                            },
                            error: function(err) {
                                notifyError(err.responseText);
                            }
                        })
                        console.log(queue);
                    }
                });
                songs.append(row);
            }
        },
        error: function(err) {
            notifyError(err.responseText);
        },
    });
}

function highlightPlayingSong() {
    if(highlightTimeoutId)
        clearTimeout(highlightTimeoutId);
    let row = $("[songname='" + queue[queueCurrentPlayingIndex] + "']");
    console.log(row);
    $(".playing").removeClass("playing");
    row.addClass("playing");
    $.ajax({
        dataType: "json",
        url: "/api/apps/player/duration",
        data: {
            songname: queue[queueCurrentPlayingIndex]
        },
        success: function(data) {
            //values in ns
            let duration = data["duration"];
            let position = data["position"];
            let remaining = (duration - position) / 1000.0 / 1000.0
            highlightTimeoutId = setTimeout(async function() {
                queueCurrentPlayingIndex += 1;
                highlightPlayingSong();
            }, remaining);
        },
        error: function(err) {
            if(queueCurrentPlayingIndex >= queue.length)
                return;
            notifyError(err.responseText);
            // try again
            setTimeout(() => highlightPlayingSong(), 100);
        },
    });
}

document.addEventListener("DOMContentLoaded", function() {
    let path = getUrlParam("path");
    if(path) {
        ls(path)
    } else {
        $.ajax({
            dataType: "json",
            url: "/api/apps/player/status",
            success: function(data) {
                let path = data["path"];
                ls(path);
            },
            error: function(err) {
                notifyError(err.responseText);
            },
        });
    }
    window.onpopstate = function(event) {
        ls(event.state["path"], false);
    }
});

function pause() {
    $.ajax({
        type: "POST",
        url: "/api/apps/player/pause",
        success: function(data) {
            notify("Paused!");
        },
        error: function(err) {
            notifyError(err.responseText);
        },
    });
}

function resume() {
    $.ajax({
        type: "POST",
        url: "/api/apps/player/resume",
        success: function(data) {
            notify("Resumed!");
        },
        error: function(err) {
            notifyError(err.responseText);
        },
    });
}

function next() {
    $.ajax({
        type: "POST",
        url: "/api/apps/player/next",
        success: function(data) {
            queueCurrentPlayingIndex += 1;
            highlightPlayingSong();
        },
        error: function(err) {
            notifyError(err.responseText);
        },
    });
}

function prev() {
    $.ajax({
        type: "POST",
        url: "/api/apps/player/prev",
        success: function(data) {
            queueCurrentPlayingIndex -= 1;
            highlightPlayingSong();
        },
        error: function(err) {
            notifyError(err.responseText);
        },
    });
}

function reverse() {
    reversePlaylist = !reversePlaylist;
    let songs = document.getElementById("songs");
    let goUp = songs.firstElementChild;
    songs.removeChild(goUp);
    songs.append(...Array.from(songs.childNodes).reverse());
    songs.prepend(goUp);
}