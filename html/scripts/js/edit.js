let editor;
const params = new URLSearchParams(window.location.search);
const SCRIPTID = Number(params.get("scriptid"));
function saveScript() {
    $.ajax({
        type: "POST",
        url: "/api/scripts/update",
        data: {
            "scriptid": SCRIPTID,
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
function runScript() {
    $.ajax({
        type: "POST",
        url: "/api/scripts/run",
        data: {
            "scriptid": SCRIPTID
        },
        error: function(err, textStatus, errorThrown) {
            let result = JSON.parse(err.responseText)["return"]
            $("#output").append(result);
            let scroll=$('#output');
            scroll.animate({scrollTop: scroll.prop("scrollHeight")});
            notifyError(result);
        },
        success: function(data, status, jqXHR) {
            let result = JSON.parse(jqXHR.responseText)["return"]
            $("#output").append(result + "\n");
            let scroll=$('#output');
            scroll.animate({scrollTop: scroll.prop("scrollHeight")});
            notify(result);
        }
    })
}

$(function() {

    $.getJSON("/api/scripts/get", "scriptid=" + SCRIPTID, function(json) {
        let row = json[0];
        $("#editor").text(row["code"])
        editor = ace.edit("editor");
        editor.commands.addCommand({
            name: 'save',
            bindKey: {win: "Ctrl-S", "mac": "Cmd-S"},
            exec: function(editor) {
                saveScript();
            }
        });
    });
})