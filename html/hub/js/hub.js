document.addEventListener("DOMContentLoaded", function() {
    $("#searchbar").keypress((key) => {
        if(key.which === 13) {
            window.location = ENGINES[active] + $("#searchbar").val();
        }
    })
})

let active = localStorage.getItem("engine");
if(!active) {
    active = "ddg";
}

document.addEventListener("DOMContentLoaded", function() {
    selectEngine(document.querySelector("[engine=" + active + "]"));
    setTimeout(() => {
        $(".logo").css("transition", "filter 0.5s");
    }, 500)

});

const ENGINES = {
    "google": "https://google.com/search?q=",
    "ddg": "https://duckduckgo.com/?q=",
    "qwant": "https://www.qwant.com/?q="
}

function selectEngine(engineElement) {
    $(".logo").addClass("inactive");
    $(engineElement).removeClass("inactive");
    active = engineElement.getAttribute("engine");
    localStorage.setItem("engine", active);
    document.getElementById("searchbar").select();
}