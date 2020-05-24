const params = new URLSearchParams(window.location.search);
const TABLENAME = params.get("tablename");
g = null;
$(function() {
    g = new Dygraph(
        // containing div
        document.getElementById("graphdiv"),
        "/api/db/csv/" + TABLENAME + "?truncateTimestamps=true&skipId=true",
        {
            rollPeriod: 1,
            showRoller: true
        }
    );
    $("#graphdiv").resize(() => {g.resize();});
    $(window).resize(() => {g.resize();});
})
