function createNewScript() {
    let name = $("#newScriptNameInput").val()
    $.ajax({
        type: "POST",
        url: "/api/scripts/create",
        data: {
            "scriptname": name,
        },
        error: function(err, textStatus, errorThrown) {
            notifyError(err.responseText);
        },
        success: function(_data, status, jqXHR) {
            let data = JSON.parse(jqXHR.responseText);
            addScript(data["id"], data["name"], true);
        }
    })
}

function deleteScript(id, name, onSuccess) {
    if(confirm("Delete " + name + "?")) {
        $.ajax({
            type: "POST",
            url: "/api/scripts/delete",
            data: {
                "scriptid": id,
            },
            error: function(err, textStatus, errorThrown) {
                notifyError(err.responseText);
            },
            success: function(_data, status, jqXHR) {
                onSuccess()
            }
        })
    }
}

function addScript(id, name, prepend) {
    let div = $("<div></div>");
    div.append($("<span></span>").text(name).addClass("list-element"));
    let edit = $("<span></span>").addClass("list-element-button").text("Edit");
    edit.click(function () {
        window.location = "/html/scripts/edit.html?scriptid=" + id;
    });
    div.append(edit);
    let run = $("<span></span>").text("Run");
    run.click(function() {
        runScript(id);
    });
    div.append(run);

    let del = $("<span></span>").text("Delete").addClass("deleteButton");
    del.click(function() {
        deleteScript(id, name, () => {
            div.detach();
        });
    });
    div.append(del);
    if(prepend) {
        $(".elementList").prepend(div)
    } else {
        $(".elementList").append(div);
    }

}

$(function() {
    $.getJSON("/api/scripts/all", "", function(data) {
        for(let i = 0; i < data.length; ++i) {
            addScript(data[i]["id"], data[i]["name"], false)
        }
    })
})