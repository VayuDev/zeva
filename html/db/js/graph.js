const params = new URLSearchParams(window.location.search);
const TABLENAME = params.get("tablename");
$(function() {
    let g = new Dygraph(
        // containing div
        document.getElementById("graphdiv"),
        "/api/db/csv/" + TABLENAME + "?truncateTimestamps=true",
        {
            rollPeriod: 1,
            showRoller: true
        }
    );
    $(".main-grid").resize(() => {g.resize();});
    $(window).resize(() => {g.resize();});
})
