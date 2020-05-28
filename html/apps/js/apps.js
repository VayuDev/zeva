$(function() {
    $.getJSON("/api/apps/all", "", function(data) {
        for(let i = 0; i < data.length; ++i) {
            let appRow = $("<div></div>")
            $(".elementList").append(appRow);
            appRow.append($("<span></span>").text(data[i]["name"]));

            if(data[i]["hasSub"]) {
                appRow.append($("<span></span>").text("+").click(() => {
                    let name = prompt("Name of the new element?");
                    if(name) {
                        $.ajax({
                            type: "POST",
                            url: "/api/apps/addSub",
                            data: {
                                "appname": data[i]["name"],
                                "subname": name
                            },
                            error: function(err, textStatus, errorThrown) {
                                notifyError(err.responseText);
                            },
                            success: function(data, status, jqXHR) {
                                window.location = window.location + "";
                            }
                        })
                    }
                }));
                $.ajax({
                    type: "GET",
                    url: "/api/apps/subsAll",
                    async: false,
                    data: {
                        "appname": data[i]["name"]
                    },
                    error: function(err, textStatus, errorThrown) {
                        notifyError(err.responseText);
                    },
                    success: function(_sub_data, _status, jqXHR) {
                        let subs = JSON.parse(jqXHR.responseText);
                        for(let j = 0; j < subs.length; ++j) {
                            console.log(subs[j]);
                            let subRow = $("<div></div>")
                                .addClass("subElement")
                                .text(subs[j]["name"])
                                .css("cursor", "pointer")
                                .click(() => {window.location = data[i]["path"] + "?subid=" + subs[j]["id"]});
                            $(".elementList").append(subRow);
                        }
                    }
                })
            } else {
                appRow.click(() => {window.location = data[i]["path"]}).css("cursor", "pointer");
            }
        }
    })
})