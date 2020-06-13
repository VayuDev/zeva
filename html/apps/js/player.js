function navigateBack(path) {
    ls(path.substr(0, path.lastIndexOf("/")));
}
let currentPath = ".";
let first = true;
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
            let index = -1;
            for(let i = 0; i < data.length; ++i) {
                if(data[i]["name"] === ".") {
                    index = i;
                    break;
                }
            }
            data.splice(index, 1);

            data = data.sort((a, b) => {
                return a["name"].toLowerCase().localeCompare(b["name"].toLowerCase())
            });

            for(let i = 0; i < data.length; ++i) {
                let row = $("<div></div>").addClass("song");
                row.append($("<img>").addClass("icon").attr("src", data[i].directory ? "/icons/directory.svg" : "/icons/file.svg"));
                row.append($("<span></span>").text(data[i].name.replace(/^.*[\\\/]/, '')));
                row.click(() => {
                    if(data[i].directory) {
                        if(data[i].name === "..") {
                            navigateBack(path);
                        } else {
                            ls(path + "/" + data[i].name);
                        }
                    } else if(data[i].musicfile) {
                        let queue = data.filter((e) => e.musicfile).map((e) => e.name);
                        let startIndex = queue.findIndex((element) => element === data[i]["name"]);
                        $.ajax({
                            url: "/api/apps/player/setQueue",
                            type: "POST",
                            data: {
                                queue: JSON.stringify(queue),
                                startIndex: startIndex
                            },
                            success: function(data) {
                                notify("Success!");
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