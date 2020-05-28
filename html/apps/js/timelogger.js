$(function() {
    $.getJSON("/api/apps/timelogger/status?subid=" + getUrlParam("subid"), "", function(data) {
        let activities = data["activities"];
        for(let i = 0; i < activities.length; ++i) {
            let act = $("<div></div>").text(activities[i]["name"]);
            act.click(() => {
               select(act);
            });
            $(".spaceCenterWide").append(act);
        }
    });
})

function select(act) {
    $("#selected").attr("id", "");
    act.attr("id", "selected");
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