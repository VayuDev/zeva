let timerStart = null;

$(function() {
    $.getJSON("/api/apps/timelogger/status?subid=" + getUrlParam("subid"), "", function(data) {
        let activities = data["activities"];
        for(let i = 0; i < activities.length; ++i) {
            let act = $("<div></div>").text(activities[i]["name"]).attr("activityid", activities[i]["id"]);
            act.click(() => {
               select(act);
            });
            $(".spaceCenterWide").append(act);
        }
        let currentActivityObj = data["currentActivity"];
        if(currentActivityObj) {
            timerStart = new Date(currentActivityObj["created"] * 1000);
            startTimer();
            setActive($("div[activityid='" + currentActivityObj["id"] + "']"));
        } else {
            stopTimer();
        }
    });
})
function digitToString(digit) {
    if(digit === 0)
        return "00";
    else if(digit >= 10)
        return digit;
    else
        return "0" + digit;
}

function setTimerDisplay(duration) {
    let date = new Date(duration * 1000);
    console.log(date);
    $("#timer").text(digitToString(date.getHours() - 1)
        + ":" + digitToString(date.getMinutes())
        + ":" + digitToString(date.getSeconds()));
}

let timerId = null;
function startTimer() {
    if(timerId)
        clearInterval(timerId);
    let intervalFunc = () => {
        let now = new Date();
        let duration = Math.floor((now-timerStart) / 1000);
        console.log(duration)
        setTimerDisplay(duration);
    }
    intervalFunc();
    timerId = setInterval(intervalFunc, 1000);
    $(".stopButtons").show();
    $(".startButtons").hide();
}

function stopTimer() {
    if(timerId)
        clearInterval(timerId);
    setTimerDisplay(0);
    $(".startButtons").show();
    $(".stopButtons").hide();
}

function select(act) {
    $(".selectedActivity").removeClass("selectedActivity");
    act.addClass("selectedActivity");
}

function setActive(act) {
    $(".activeActivity").removeClass("activeActivity");
    if(act)
        act.addClass("activeActivity");
}

function addNewActivity() {
    let activityName = prompt("What is the new activity called?");
    if(activityName) {
        $.ajax({
            type: "POST",
            url: "/api/apps/timelogger/createActivity",
            data: {
                "activityname": activityName,
                "subid": getUrlParam("subid")
            },
            error: function(err, textStatus, errorThrown) {
                notifyError(err.responseText);
            },
            success: function(data, status, jqXHR) {
                window.location = window.location + "";
            }
        })
    }
}

function startActivity() {
    let activityid = $(".selectedActivity").attr("activityid")
    if(activityid) {
        $.ajax({
            type: "POST",
            url: "/api/apps/timelogger/startActivity",
            data: {
                "activityid": activityid,
                "subid": getUrlParam("subid")
            },
            error: function(err, textStatus, errorThrown) {
                notifyError(err.responseText);
            },
            success: function(data, status, jqXHR) {
                notify("Started activity");
                timerStart = new Date();
                startTimer();
                setActive($("div[activityid='" + activityid + "']"));
            }
        })
    } else {
        notify("Nothing selected");
    }
}

function stopActivity() {
    $.ajax({
        type: "POST",
        url: "/api/apps/timelogger/stopActivity",
        data: {
            "subid": getUrlParam("subid")
        },
        error: function(err, textStatus, errorThrown) {
            notifyError(err.responseText);
        },
        success: function(data, status, jqXHR) {
            notify("Stopped activity");
            stopTimer();
            setActive(null);
        }
    })
}

function deleteActivity() {
    const activityid = $(".selectedActivity").attr("activityid")
    if(activityid) {
        $.ajax({
            type: "POST",
            url: "/api/apps/timelogger/deleteActivity",
            data: {
                "activityid": activityid,
                "subid": getUrlParam("subid")
            },
            error: function(err, textStatus, errorThrown) {
                notifyError(err.responseText);
            },
            success: function(data, status, jqXHR) {
                window.location = window.location + "";
            }
        })
    } else {
        notify("Nothing selected");
    }
}

function abortActivity() {
    if(!confirm("Are you sure you want to abort?"))
        return;
    $.ajax({
        type: "POST",
        url: "/api/apps/timelogger/abortActivity",
        data: {
            "subid": getUrlParam("subid")
        },
        error: function(err, textStatus, errorThrown) {
            notifyError(err.responseText);
        },
        success: function(data, status, jqXHR) {
            stopTimer();
            $(".activeActivity").removeClass("activeActivity");
            notify("Activity aborted");
        }
    })
}