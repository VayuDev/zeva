document.addEventListener("DOMContentLoaded", function() {
    $("#searchbar").keypress((key) => {
        if(key.which === 13)
            window.location = "https://google.com/search?q=" + $("#searchbar").val();
    })
})