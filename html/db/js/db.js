function addTable(name, is_protected, prepend) {
    let div = $("<div></div>");
    div.append($("<span></span>").text(name).addClass("list-element"));

    let table = $("<span></span>").addClass("list-element-button").text("Table");
    table.click(function () {
        window.location = "/html/db/graph.html?tablename=" + name;
    });
    div.append(table);
    let graph = $("<span></span>").text("Graph");
    graph.click(function() {
        window.location = "/html/db/table.html?tablename=" + name;
    });
    div.append(graph);
    let del = $("<span></span>").text("Delete").addClass("deleteButton");
    if(is_protected) {
       del.addClass("inactive");
    } else {
        del.click(function() {
            if(!confirm("Do you really want to delete " + name + "?")) {
                return;
            }
            $.ajax({
                type: "POST",
                url: "/api/db/delete",
                data: {
                    "tablename": name,
                },
                error: function(err, textStatus, errorThrown) {
                    notifyError(err.responseText);
                },
                success: function(_data, status, jqXHR) {
                    div.detach();
                }
            })
        });
    }
    div.append(del);

    if(prepend) {
        $(".elementList").prepend(div)
    } else {
        $(".elementList").append(div);
    }
}

$(function() {
    $.getJSON("/api/db/all", "", function(data) {
        for(let i = 0; i < data.length; ++i) {
            addTable(data[i]["name"], data[i]["is_protected"], false)
        }
    })
})