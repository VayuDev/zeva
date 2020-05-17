$(function() {
    $.getJSON("/api/scripts/all", "", function(data) {
        for(let i = 0; i < data.length; ++i) {
            let div = $("<div></div>");
            div.append($("<span></span>").text(data[i]["name"]).addClass("list-element"));
            let edit = $("<span></span>").addClass("list-element-button").text("Edit");
            edit.click(function () {
                window.location = "/html/scripts/edit.html?scriptid=" + data[i]["id"];
            });
            div.append(edit);
            let run = $("<span></span>").text("Run");
            run.click(function() {
                runScript(data[i]["id"]);
            });
            div.append(run);
            $(".elementList").append(div);
        }
    })
})