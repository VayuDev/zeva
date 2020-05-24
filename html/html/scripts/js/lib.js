function logToOutputAndMsg(result, isError) {
    let output = $("#output");
    if(output.length) {
        output.append(result + "\n");
        output.animate({scrollTop: output.prop("scrollHeight")});
    }
    if(isError) {
        notifyError(result);
    } else {
        notify(result);
    }
}

function logImage(img) {
    let output = $("#output");
    if(output.length) {
        output.append(img);
        output.append("\n");
        output.animate({scrollTop: output.prop("scrollHeight")});
    }
}

function runScript(scriptid, param = null) {
    $.ajax({
        type: "POST",
        url: "/api/scripts/run",
        data: {
            "scriptid": scriptid,
            "param": param
        },
        error: function(err, textStatus, errorThrown) {
            logToOutputAndMsg(err.responseText, true);
        },
        success: function(data, status, jqXHR) {
            logToOutputAndMsg(JSON.parse(jqXHR.responseText)["return"], false);
        }
    })
}