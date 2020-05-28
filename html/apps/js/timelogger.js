$(function() {
    $.getJSON("/api/apps/timelogger/status?subid=" + getUrlParam("subid"), "", function(data) {
        console.log(data);
    });
})