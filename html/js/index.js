let lowestBoxHeight = 200;
let lowestBoxId = -1;
let boxIdCounter = 0;
let boxCount = 0;

function notifyError(msg) {
    notify(msg, "notify-error");
}

function notify(msg, classname = "notify-info") {
    let msgBox = $("<div></div>").text(msg).addClass(classname).addClass("notify")
    $("body").append(msgBox);
    let boxHeight = lowestBoxHeight;
    msgBox.css("top", boxHeight + "px");
    let id = ++boxIdCounter;
    lowestBoxId = id;
    ++boxCount;

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
            --boxCount;
            if(boxCount === 0) {
                lowestBoxHeight = 200;
            }
        }
    }, 20);

}

$(function() {
    $("#header").load("/html/templates/header.html", "", () => {
        let pathname = window.location.pathname;
        if(pathname.startsWith("/html/hub")) {
            $("#nav_hub").addClass("selected");
        } else if(pathname.startsWith("/html/scripts")) {
            $("#nav_script").addClass("selected");
        } else if(pathname.startsWith("/html/db")) {
            $("#nav_db").addClass("selected");
        }
    });
})