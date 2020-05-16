$(function() {
    let params = new URLSearchParams(window.location.search);
    $.getJSON("/api/scripts/get", "scriptid=" + params.get("scriptid"), function(json) {
        let row = json[0];
        $("#editor").text(row["code"])
        let editor = ace.edit("editor");
        editor.commands.addCommand({
            name: 'save',
            bindKey: {win: "Ctrl-S", "mac": "Cmd-S"},
            exec: function(editor) {
                $.ajax({
                    type: "POST",
                    url: "/api/scripts/update",
                    data: {
                        "scriptid": Number(params.get("scriptid")),
                        "code": editor.getValue()
                    },
                    dataType: "json",
                    error: function(err, textStatus, errorThrown) {
                        notifyError(err.responseText);
                    },
                    success: function(data, status, jqXHR) {
                        notify("Successfully compiled script!");
                    }
                })
            }
        });
    });
})